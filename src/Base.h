#pragma once

#include <BWAPI.h>

namespace KBot {

    class KBot;

    class Base {
        friend class Manager;

    public:
        Base(KBot &kBot, const BWAPI::TilePosition &position);

        // Is called every KBot::onFrame().
        void update();

        const BWAPI::TilePosition &getPosition() const { return m_position; }

        // Transfer ownership of a unit to base.
        void transferOwnership(const BWAPI::Unit &unit);
        void onUnitDestroy(const BWAPI::Unit &unit);

    private:
        KBot &m_kBot;
        BWAPI::TilePosition m_position;
        BWAPI::Unitset m_units;

        BWAPI::Unitset m_minerals, m_mineralWorkers;
        BWAPI::Unitset m_gas, m_gasWorkers;
    };

} // namespace
