#pragma once

#include <BWAPI.h>

namespace KBot {

    class KBot;

    class Base {
    public:
        Base(KBot &kBot, const BWAPI::TilePosition &location);

        // Is called every KBot::onFrame().
        void update();

        const BWAPI::TilePosition &getPosition() const { return m_location; }

        // Transfer ownership of a unit to base.
        void transferOwnership(const BWAPI::Unit &unit);
        void onUnitDestroy(const BWAPI::Unit &unit);

    private:
        KBot &m_kBot;
        BWAPI::TilePosition m_location;
        BWAPI::Unitset m_units;
    };

} // namespace
