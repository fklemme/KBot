#include "KBot.h"

#include <cassert>
#include <iostream>
#include <random>

namespace KBot {

    using namespace BWAPI;

    KBot::KBot() : m_manager(*this), m_map(BWEM::Map::Instance()) {}

    // Called only once at the beginning of a game.
    void KBot::onStart() {
        if (Broodwar->isReplay()) return;

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

        BWEM::utils::MapPrinter::Initialize(&m_map);
        //BWEM::utils::printMap(m_map);    // will print the map into the file <StarCraftFolder>bwapi-data/map.bmp
        //BWEM::utils::pathExample(m_map); // add to the printed map a path between two starting locations

        // Ready to go. Good luck, have fun!
        Broodwar->sendText("gl hf");

        // Initialize possible enemy positions.
        auto allLocations = Broodwar->getStartLocations();
        const auto myLocation = Broodwar->self()->getStartLocation();
        const auto myLocationIt = std::find(allLocations.begin(), allLocations.end(), myLocation);
        assert(myLocationIt != allLocations.end());
        allLocations.erase(myLocationIt);
        std::sort(allLocations.begin(), allLocations.end(), [&](TilePosition a, TilePosition b) {
            return myLocation.getDistance(a) < myLocation.getDistance(b);
        });
        m_enemyLocations = allLocations;
        assert(!m_enemyLocations.empty());
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
        const auto myLocation = Broodwar->self()->getStartLocation();
        Broodwar->drawTextScreen(2, 10, "My StartLocation: %d, %d", myLocation.x, myLocation.y);
        Broodwar->drawTextScreen(2, 20, "EnemyLocation count: %d", m_enemyLocations.size());
        if (!m_enemyLocations.empty()) // Might be empty for a short period of time.
            Broodwar->drawTextScreen(2, 30, "Next EnemyLocation: %d, %d", m_enemyLocations.front().x, m_enemyLocations.front().y);
        else
            Broodwar->drawTextScreen(2, 30, "No more enemies!? :O");

        // Draw map
        BWEM::utils::drawMap(m_map);

        // Draw path to enemy and some regions
        if (!m_enemyLocations.empty()) {
            const auto myLocation = Broodwar->self()->getStartLocation();
            const auto enemyLocation = m_enemyLocations.front();
            const auto path = m_map.GetPath(Position(Broodwar->self()->getStartLocation()), Position(enemyLocation));

            Broodwar->drawCircleMap(Position(myLocation), 400, Colors::Green);
            if (!path.empty()) {
                Broodwar->drawLineMap(Position(Broodwar->self()->getStartLocation()), Position(path.front()->Center()), Colors::Red);
                Broodwar->drawCircleMap(Position(path.front()->Center()), 200, Colors::Orange);
                for (std::size_t i = 1; i < path.size(); ++i)
                    Broodwar->drawLineMap(Position(path[i - 1]->Center()), Position(path[i]->Center()), Colors::Red);
                Broodwar->drawLineMap(Position(path.back()->Center()), Position(enemyLocation), Colors::Red);
            }
        }

        // Update manager
        m_manager.update();

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // Iterate through all the units that we own
        for (auto &u : Broodwar->self()->getUnits()) {
            // Ignore the unit if it no longer exists
            // Make sure to include this block when handling any Unit pointer!
            if (!u->exists())
                continue;

            // Ignore the unit if it has one of the following status ailments
            if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
                continue;

            // Ignore the unit if it is in one of the following states
            if (u->isLoaded() || !u->isPowered() || u->isStuck())
                continue;

            // Ignore the unit if it is incomplete or busy constructing
            if (!u->isCompleted() || u->isConstructing())
                continue;


            // Finally make the unit do some stuff!
            // TODO: Remove all that demo stuff...
            // For now and for fun, let's just build a simple marine rush bot using what we have. :)


            // If the unit is a worker unit
            if (u->getType().isWorker()) {
                // if our worker is idle
                if (u->isIdle()) {
                    // Order workers carrying a resource to return them to the center,
                    // otherwise find a mineral patch to harvest.
                    if (u->isCarryingGas() || u->isCarryingMinerals()) {
                        u->returnCargo();
                    }
                    // Harvest from the nearest mineral patch or gas refinery
                    else if (!u->gather(u->getClosestUnit(Filter::IsMineralField || Filter::IsRefinery))) {
                        // If the call fails, then print the last error message
                        Broodwar << Broodwar->getLastError() << std::endl;
                    }
                } // closure: if idle
            }
            else if (u->getType() == UnitTypes::Terran_Marine) {
                //Broodwar->registerEvent([u](Game*) { Broodwar->drawCircleMap(u->getPosition(), 500, Colors::Yellow); }, nullptr, Broodwar->getLatencyFrames()); // debug!
                if (u->isIdle()) {
                    // Update enemy locations
                    while (!m_enemyLocations.empty() && Broodwar->isVisible(m_enemyLocations.front()) && Broodwar->getUnitsOnTile(m_enemyLocations.front(), Filter::IsEnemy).empty())
                        m_enemyLocations.pop_front();

                    // Defend, if there are only few marines.
                    if (Broodwar->getUnitsInRadius(u->getPosition(), 500, Filter::GetType == UnitTypes::Terran_Marine && Filter::IsOwned).size() < 20) {
                        auto nearbyEnemies = Broodwar->getUnitsInRadius(u->getPosition(), 500, Filter::IsEnemy);
                        if (!nearbyEnemies.empty())
                            u->attack(nearbyEnemies.getClosestUnit());
                        else if (u->getPosition().getDistance(Position(Broodwar->self()->getStartLocation())) > 100)
                            u->move(Position(Broodwar->self()->getStartLocation()));
                    }

                    // Otherwise, attack! :P
                    else if (!m_enemyLocations.empty())
                        u->attack(Position(m_enemyLocations.front()));
                }
            }
            else if (u->getType() == UnitTypes::Terran_Barracks) {
                if (u->isIdle()) {
                    // Spam marines! :D
                    if (!u->train(UnitTypes::Terran_Marine)) {
                        Position pos = u->getPosition();
                        Error lastErr = Broodwar->getLastError();
                        Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },
                            nullptr, Broodwar->getLatencyFrames());
                    }
                }
            }
            else if (u->getType().isResourceDepot()) { // A resource depot is a Command Center, Nexus, or Hatchery
                //Broodwar->registerEvent([u](Game*) { Broodwar->drawCircleMap(u->getPosition(), 300, Colors::Green); }, nullptr, Broodwar->getLatencyFrames()); // debug!
                // Limit amount of workers to produce.
                if (Broodwar->getUnitsInRadius(u->getPosition(), 300, Filter::IsWorker && Filter::IsOwned).size() < 20) {
                    // Order the depot to construct more workers! But only when it is idle.
                    if (u->isIdle() && !u->train(u->getType().getRace().getWorker())) {
                        // If that fails, draw the error at the location so that you can visibly see what went wrong!
                        // However, drawing the error once will only appear for a single frame
                        // so create an event that keeps it on the screen for some frames
                        Position pos = u->getPosition();
                        Error lastErr = Broodwar->getLastError();
                        Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); }, // action
                            nullptr, // condition
                            Broodwar->getLatencyFrames()); // frames to run
                    } // closure: failed to train idle unit
                }
            }
        } // closure: unit iterator
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
    void KBot::onUnitDiscover(BWAPI::Unit unit) {}

    // Called when a Unit becomes inaccessible.
    void KBot::onUnitEvade(BWAPI::Unit unit) {}

    // Called when a previously invisible unit becomes visible.
    void KBot::onUnitShow(BWAPI::Unit unit) {
        // Update enemy locations
        if (Broodwar->self()->isEnemy(unit->getPlayer()) && unit->getType().isBuilding()) {
            const auto myLocation = Broodwar->self()->getStartLocation();
            TilePosition location{ unit->getPosition() };
            if (std::find(m_enemyLocations.begin(), m_enemyLocations.end(), location) == m_enemyLocations.end()) {
                const auto it = std::lower_bound(m_enemyLocations.begin(), m_enemyLocations.end(), location,
                    [myLocation](TilePosition a, TilePosition b) { return myLocation.getDistance(a) < myLocation.getDistance(b); });
                m_enemyLocations.insert(it, location);
            }
        }
    }

    // Called just as a visible unit is becoming invisible.
    void KBot::onUnitHide(BWAPI::Unit unit) {}

    // Called when any unit is created.
    void KBot::onUnitCreate(BWAPI::Unit unit) {}

    // Called when a unit is removed from the game either through death or other means.
    void KBot::onUnitDestroy(BWAPI::Unit unit) {
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
    void KBot::onUnitComplete(BWAPI::Unit unit) {}

} // namespace