#include "BuildTask.h"

#include "Manager.h"

namespace KBot {

    BuildTask::BuildTask(Manager &manager,
        const BWAPI::UnitType &toBuild, const BuildPriority priority,
        const BWAPI::TilePosition &position, const bool exactPosition)
        : m_manager(&manager), m_toBuild(toBuild), m_priority(priority),
        m_position(position), m_exactPosition(exactPosition) {}

    void BuildTask::update() {
        // TODO
    }

} // namespace
