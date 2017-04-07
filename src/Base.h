#pragma once

#include <BWAPI.h>

namespace KBot {

    class Manager;

    class Base {
    public:
        Base(Manager &manager, const BWAPI::TilePosition &position);

        // Is called every KBot::onFrame().
        void update();

        const BWAPI::TilePosition &getPosition() const { return m_position; }

        // Transfer ownership of a unit to base.
        void transferOwnership(const BWAPI::Unit &unit);
        void onUnitDestroy(const BWAPI::Unit &unit);

    private:
        Manager            *m_manager;
        BWAPI::TilePosition m_position;
        BWAPI::Unitset      m_units;

        BWAPI::Unitset m_mineralPatches, m_gasGeysirs;
        BWAPI::Unitset m_mineralWorkers, m_gasWorkers;
    };

} // namespace
