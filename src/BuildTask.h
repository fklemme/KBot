#pragma once

#include <BWAPI.h>

namespace KBot {

    class Manager;

    enum class BuildPriority : int {
        low = -100,
        normal = 0,
        high = 100,
        buildorder = 200
    };

    class BuildTask {
        enum class State {
            initialize,
            acquireResources,
            acquireWorker,
            moveToPosition,
            startBuild,
            waitForUnit,
            building,
            finalize
        };

    public:
        BuildTask(Manager &manager, const BWAPI::UnitType &toBuild, const BuildPriority priority = BuildPriority::normal,
            const BWAPI::TilePosition &position = BWAPI::Broodwar->self()->getStartLocation(), const bool exactPosition = false);

        // Is called every KBot::onFrame().
        void update();

        bool onUnitCreated(const BWAPI::Unit &unit);
        void onUnitDestroyed(const BWAPI::Unit &unit);

        std::string toString() const;

    private:
        Manager *m_manager;
        BWAPI::UnitType m_toBuild;
        BuildPriority m_priority;
        BWAPI::TilePosition m_position;
        bool m_exactPosition;

        State m_state = State::initialize;
        BWAPI::Unit m_worker = nullptr;
        bool m_allocatedBuildPosition = false;
        BWAPI::TilePosition m_buildPosition;
        BWAPI::Unit m_buildingUnit = nullptr;
    };

} // namespace
