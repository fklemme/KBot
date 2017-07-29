#include "KBot.h"

#include "Squad.h"
#include "utils.h"
#include <string>

// Some ugly macro magic to get BUILDNUMBER as a string.
#pragma warning(push)
#pragma warning(disable : 4003) // there is some silly warning in VS2017
#define MACROSTR(S) #S ""
#define STRINGER(X) MACROSTR(X)
static const std::string buildNumber = STRINGER(BUILDNUMBER);
#pragma warning(pop)

namespace KBot {

using namespace BWAPI;

KBot::KBot() : m_manager(*this), m_general(*this), m_enemy(*this) {}

// Called only once at the beginning of a game.
void KBot::onStart() {
    // Print build information
    Broodwar << "KBot build " << (buildNumber.empty() ? "local" : buildNumber) << std::endl;

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
    // http://wiki.teamliquid.net/starcraft/2_Rax_FE_(vs._Zerg)
    using namespace UnitTypes;
    const auto buildorder = {
        Terran_SCV,    Terran_SCV,          Terran_SCV,
        Terran_SCV,    Terran_SCV,          Terran_Supply_Depot, // @ 9/10 supply
        Terran_SCV,    Terran_SCV,          Terran_Barracks,     // @ 11/18 supply
        Terran_SCV,    Terran_SCV,          Terran_Barracks,     // @ 13/18 supply
        Terran_SCV,    Terran_Supply_Depot,                      // @ 14/18 supply
        Terran_SCV,    Terran_Marine,       Terran_SCV,
        Terran_Marine, Terran_Refinery, // @ 18/26 supply
        Terran_SCV,    Terran_Marine,       Terran_SCV,
        Terran_Marine, Terran_Academy,                          // @ 22/26 supply
        Terran_SCV,    Terran_Marine,       Terran_Supply_Depot // @ 24/26 supply
    };

    int priority = (int) BuildTask::Priority::buildorder;
    for (const auto &unit : buildorder)
        m_manager.addBuildTask({m_manager, unit, (BuildTask::Priority) priority--});

    // Ready to go. Good luck, have fun!
    Broodwar->sendText("gl hf");
}

// Called once at the end of a game.
void KBot::onEnd(bool /*isWinner*/) {}

// Called once for every execution of a logical frame in Broodwar.
void KBot::onFrame() {
    // Return if the game is paused
    if (Broodwar->isPaused())
        return;

    // Display some debug information
    Broodwar->drawTextScreen(2, 0, "FPS: %d, APM: %d", Broodwar->getFPS(), Broodwar->getAPM());
    Broodwar->drawTextScreen(2, 10, "Scouted enemy positions: %d", m_enemy.getPositionCount());

    if (m_enemy.getPositionCount() > 0) {
        const auto enemyPosition = m_enemy.getClosestPosition();
        Broodwar->drawTextScreen(2, 20, "Next enemy position: (%d, %d)", enemyPosition.x,
                                 enemyPosition.y);
    } else
        Broodwar->drawTextScreen(2, 20, "Next enemy position: Unknown");

    // Update manager
    m_manager.update();

    // Update general
    m_general.update();

    // Update enemy
    m_enemy.update();

    // ----- Prevent spamming -----------------------------------------------
    // Everything below is executed only occasionally and not on every frame.
    if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
        return;
}

// Called when the user attempts to send a text message.
void KBot::onSendText(std::string /*text*/) {}

// Called when the client receives a message from another Player.
void KBot::onReceiveText(BWAPI::Player /*player*/, std::string /*text*/) {}

// Called when a Player leaves the game.
void KBot::onPlayerLeft(BWAPI::Player /*player*/) {}

// Called when a Nuke has been launched somewhere on the map.
void KBot::onNukeDetect(BWAPI::Position /*target*/) {}

// Called when a Unit becomes accessible.
void KBot::onUnitDiscover(BWAPI::Unit unit) { assert(unit->exists()); }

// Called when a Unit becomes inaccessible.
void KBot::onUnitEvade(BWAPI::Unit unit) { assert(!unit->exists()); }

// Called when a previously invisible unit becomes visible.
void KBot::onUnitShow(BWAPI::Unit unit) {
    assert(unit->exists());

    // Update enemy positions
    if (Broodwar->self()->isEnemy(unit->getPlayer()) && unit->getType().isBuilding())
        m_enemy.addPosition(TilePosition(unit->getPosition()));
}

// Called just as a visible unit is becoming invisible.
void KBot::onUnitHide(BWAPI::Unit unit) {
    assert(!unit->exists()); // ???
}

// Called when any unit is created.
void KBot::onUnitCreate(BWAPI::Unit unit) {
    assert(unit->exists());

    // My unit
    if (unit->getPlayer() == Broodwar->self()) {
        // Notify build tasks
        m_manager.buildTaskOnUnitCreatedOrMorphed(unit);
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
        if (unit->getType().isBuilding() || unit->getType().isWorker() ||
            unit->getType().isMineralField())
            m_manager.takeOwnership(unit);
        else
            m_general.takeOwnership(unit);
    }

    // Update BWEM information
    if (unit->getType().isMineralField())
        m_map.OnMineralDestroyed(unit);
    else if (unit->getType().isSpecialBuilding())
        m_map.OnStaticBuildingDestroyed(unit);
}

// Called when a unit changes its UnitType.
void KBot::onUnitMorph(BWAPI::Unit unit) {
    // For example, when a Drone transforms into a Hatchery, a Siege Tank uses Siege Mode, or a
    // Vespene Geyser receives a Refinery.
    assert(unit->exists());

    // My unit
    if (unit->getPlayer() == Broodwar->self()) {
        // Notify build tasks
        m_manager.buildTaskOnUnitCreatedOrMorphed(unit);
    }
}

// Called when a unit changes ownership.
void KBot::onUnitRenegade(BWAPI::Unit /*unit*/) {
    // This occurs when the Protoss ability Mind Control is used, or if a unit changes ownership in
    // Use Map Settings.
    // TODO!
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
            m_manager.giveOwnership(unit);
        else
            m_general.giveOwnership(unit);
    }
}

} // namespace KBot
