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
    public:
        BuildTask(Manager &manager, const BWAPI::UnitType &toBuild, const BuildPriority priority = BuildPriority::normal,
            const BWAPI::TilePosition &position = BWAPI::Broodwar->self()->getStartLocation(), const bool exactPosition = false);

        // Is called every KBot::onFrame().
        void update();

    private:
        Manager *m_manager;
        BWAPI::UnitType m_toBuild;
        BuildPriority m_priority;
        BWAPI::TilePosition m_position;
        bool m_exactPosition;
    };

} // namespace
