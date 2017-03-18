#pragma once

#include <BWAPI.h>

namespace KBot {

    class KBot;

    class Manager {
    public:
        Manager(KBot &parent);

        // Is called every KBot::onFrame().
        void update();

        // Transfer ownership of a unit to manager.
        void transferOwnership(BWAPI::Unit unit);

    private:
        KBot &m_kBot;
    };

} // namespace
