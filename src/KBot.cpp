#include "KBot.h"

#include <random>
#include "Squad.h"
#include "utils.h"

// Some ugly macro magic to get BUILDNUMBER as a string.
#pragma warning(push)
#pragma warning(disable: 4003) // there is some silly warning in VS2017
#define MACROSTR(S) #S ""
#define STRINGER(X) MACROSTR(X)
static const std::string buildNumber = STRINGER(BUILDNUMBER);
#pragma warning(pop)

namespace KBot {

    using namespace BWAPI;

    KBot::KBot() : m_manager(*this), m_general(*this) {}

    // Called only once at the beginning of a game.
    void KBot::onStart() {
        if (Broodwar->isReplay() || !Broodwar->self())
            return;

        // Print build information
        Broodwar << "KBot build " << (buildNumber.size() ? buildNumber : "local") << std::endl;

        // This bot is written for Terran, so make sure we are indeed Terran!
        if (Broodwar->self()->getRace() != Races::Terran) {
            Broodwar << "Wrong race selected! KBot has to be Terran." << std::endl;
            Broodwar->leaveGame();
        }

        // This bot is written for one-on-one melee games.
        if (Broodwar->getGameType() != GameTypes::Melee) {
            Broodwar << "Unexpected game type. KBot might not work correctly." << std::endl;
            // Unfortunate, but no reason to leave.
        }

        // Set the command optimization level so that common commands can be grouped.
        Broodwar->setCommandOptimizationLevel(2);

        // BWEM map initialization
        m_map.Initialize();
        m_map.EnableAutomaticPathAnalysis();
        const bool r = m_map.FindBasesForStartingLocations();
        assert(r);

        // Test BuildTask, TODO: Replace hardcoded build order
        using namespace UnitTypes;
        const auto buildorder = {
            Terran_SCV, Terran_SCV, Terran_SCV, // build SCVs...
            Terran_SCV, Terran_SCV, Terran_Supply_Depot, // @ 9/10 supply
            Terran_SCV, Terran_SCV, Terran_Barracks, // @ 11/18 supply
            Terran_SCV, Terran_SCV, Terran_Barracks, // @ 13/18 supply
            Terran_SCV, Terran_Supply_Depot // @ 14/18 supply
        };

        int priority = (int) BuildTask::Priority::buildorder;
        for (const auto &unit : buildorder)
            m_manager.addBuildTask({m_manager, unit, (BuildTask::Priority) priority--});

        // Ready to go. Good luck, have fun!
        Broodwar->sendText("gl hf");
    }

    // Called once at the end of a game.
    void KBot::onEnd(bool isWinner) {}

    // Called once for every execution of a logical frame in Broodwar.
    void KBot::onFrame() {
        // Return if the game is a replay or is paused
        if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
            return;

        // Display some debug information
        Broodwar->drawTextScreen(2, 0, "FPS: %d, APM: %d", Broodwar->getFPS(), Broodwar->getAPM());
        Broodwar->drawTextScreen(2, 10, "Scouted enemy positions: %d", m_enemyPositions.size());

        if (!m_enemyPositions.empty()) {
            const auto enemyPosition = m_enemyPositions.front();
            Broodwar->drawTextScreen(2, 20, "Next enemy position: (%d, %d)", enemyPosition.x, enemyPosition.y);
        } else
            Broodwar->drawTextScreen(2, 20, "Next enemy position: Unknown");

        // Update enemy positions
        while (!m_enemyPositions.empty() && Broodwar->isVisible(m_enemyPositions.front()) && Broodwar->getUnitsOnTile(m_enemyPositions.front(), Filter::IsEnemy).empty())
            m_enemyPositions.erase(m_enemyPositions.begin());

        // Update manager
        m_manager.update();

        // Update general
        m_general.update();

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;
    }

    // Called when the user attempts to send a text message.
    void KBot::onSendText(std::string text) {}

    // Called when the client receives a message from another Player.
    void KBot::onReceiveText(BWAPI::Player player, std::string text) {}

    // Called when a Player leaves the game.
    void KBot::onPlayerLeft(BWAPI::Player player) {}

    // Called when a Nuke has been launched somewhere on the map.
    void KBot::onNukeDetect(BWAPI::Position target) {}

    // Called when a Unit becomes accessible.
    void KBot::onUnitDiscover(BWAPI::Unit unit) {
        assert(unit->exists());
    }

    // Called when a Unit becomes inaccessible.
    void KBot::onUnitEvade(BWAPI::Unit unit) {
        assert(!unit->exists());
    }

    // Called when a previously invisible unit becomes visible.
    void KBot::onUnitShow(BWAPI::Unit unit) {
        assert(unit->exists());

        // Update enemy positions
        if (Broodwar->self()->isEnemy(unit->getPlayer()) && unit->getType().isBuilding()) {
            const auto myPosition = Broodwar->self()->getStartLocation();
            const TilePosition position(unit->getPosition());
            if (std::find(m_enemyPositions.begin(), m_enemyPositions.end(), position) == m_enemyPositions.end()) {
                const auto it = std::lower_bound(m_enemyPositions.begin(), m_enemyPositions.end(), position,
                    [&](const TilePosition &a, const TilePosition &b) { return distance(myPosition, a) < distance(myPosition, b); });
                m_enemyPositions.insert(it, position);
            }
        }
    }

    // Called just as a visible unit is becoming invisible.
    void KBot::onUnitHide(BWAPI::Unit unit) {
        //assert(!unit->exists()); ???
    }

    // Called when any unit is created.
    void KBot::onUnitCreate(BWAPI::Unit unit) {
        assert(unit->exists());

        // My unit
        if (unit->getPlayer() == Broodwar->self()) {
            // Notify build tasks
            m_manager.buildTaskOnUnitCreated(unit);
        }

    }

    // Called when a unit is removed from the game either through death or other means.
    void KBot::onUnitDestroy(BWAPI::Unit unit) {
        assert(!unit->exists());

        // My unit
        if (unit->getPlayer() == Broodwar->self()) {
            // Notify build tasks
            m_manager.buildTaskOnUnitDestroyed(unit);

            // Dispatch
            if (unit->getType().isBuilding() || unit->getType().isWorker())
                m_manager.onUnitDestroy(unit);
            else
                m_general.onUnitDestroy(unit);
        }

        // Update BWEM information
        if (unit->getType().isMineralField())
            m_map.OnMineralDestroyed(unit);
        else if (unit->getType().isSpecialBuilding())
            m_map.OnStaticBuildingDestroyed(unit);
    }

    // Called when a unit changes its UnitType.
    void KBot::onUnitMorph(BWAPI::Unit unit) {
        // For example, when a Drone transforms into a Hatchery, a Siege Tank uses Siege Mode, or a Vespene Geyser receives a Refinery.
    }

    // Called when a unit changes ownership.
    void KBot::onUnitRenegade(BWAPI::Unit unit) {
        // This occurs when the Protoss ability Mind Control is used, or if a unit changes ownership in Use Map Settings.
    }

    // Called when the state of the Broodwar game is saved to file.
    void KBot::onSaveGame(std::string gameName) {
        Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
    }

    // Called when the state of a unit changes from incomplete to complete.
    void KBot::onUnitComplete(BWAPI::Unit unit) {
        assert(unit->exists());

        // My unit
        if (unit->getPlayer() == Broodwar->self()) {
            // Notify build tasks
            m_manager.buildTaskOnUnitCompleted(unit);

            // Dispatch ownership
            if (unit->getType().isBuilding() || unit->getType().isWorker())
                m_manager.transferOwnership(unit);
            else
                m_general.transferOwnership(unit);
        }
    }

    TilePosition KBot::getNextEnemyPosition() const {
        if (!m_enemyPositions.empty())
            return m_enemyPositions.front();
        else {
            // TODO: Update to multiple own bases concept?
            // Positions to scout
            auto positions = m_map.StartingLocations();
            if (std::all_of(positions.begin(), positions.end(), [](const TilePosition &p) { return Broodwar->isExplored(p); })) {
                positions.clear();
                for (const auto &area : m_map.Areas())
                    for (const auto &base : area.Bases())
                        positions.push_back(base.Location());
            }

            // Always exclude our own base.
            const auto myPosition = Broodwar->self()->getStartLocation();
            const auto it = std::find(positions.begin(), positions.end(), myPosition);
            assert(it != positions.end());
            positions.erase(it);

            // Order positions by isExplored and distance to own base.
            std::sort(positions.begin(), positions.end(), [&](TilePosition a, TilePosition b) {
                return distance(myPosition, a) < distance(myPosition, b);
            });
            std::stable_sort(positions.begin(), positions.end(), [](TilePosition a, TilePosition b) {
                return Broodwar->isExplored(a) < Broodwar->isExplored(b);
            });
            if (!Broodwar->isExplored(positions.front()))
                return positions.front();

            // If all positions are already explored, return a random position.
            static std::default_random_engine generator;
            std::uniform_int_distribution<int> dist(0, positions.size() - 1);
            return positions[dist(generator)];
        }
    }

} // namespace
