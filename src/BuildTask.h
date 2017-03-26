#pragma once

#include <BWAPI.h>

namespace KBot {

    class Manager;

    class BuildTask {
    public:
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

        enum class Priority : int {
            low = -100,
            normal = 0,
            high = 100,
            buildorder = 200
        };

    public:
        BuildTask(Manager &manager, const BWAPI::UnitType &toBuild, const Priority priority = Priority::normal,
            const BWAPI::TilePosition &position = BWAPI::Broodwar->self()->getStartLocation(), const bool exactPosition = false);

        // Is called every KBot::onFrame().
        void update();

        bool onUnitCreated(const BWAPI::Unit &unit);
        bool onUnitDestroyed(const BWAPI::Unit &unit);

        State getState() const { return m_state; }
        std::string toString() const;

    private:
        Manager *m_manager;
        BWAPI::UnitType m_toBuild;
        Priority m_priority;
        BWAPI::TilePosition m_position;
        bool m_exactPosition;

        State m_state = State::initialize;
        BWAPI::Unit m_worker = nullptr;
        bool m_allocatedBuildPosition = false;
        BWAPI::TilePosition m_buildPosition;
        BWAPI::Unit m_buildingUnit = nullptr;
    };

} // namespace
