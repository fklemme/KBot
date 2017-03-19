#pragma once

#include <BWAPI.h>

namespace KBot {

    class KBot;

    class Manager {
    public:
        Manager(KBot &kBot);

        // Is called every KBot::onFrame().
        void update();

        // Transfer ownership of a unit to manager.
        void transferOwnership(BWAPI::Unit unit);
        void onUnitDestroy(BWAPI::Unit unit);

    private:
        KBot &m_kBot;
        BWAPI::Unitset m_units;
    };

} // namespace
