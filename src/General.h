#pragma once

#include <BWAPI.h>

namespace KBot {

    class KBot;

    class General {
    public:
        General(KBot &parent);

        // Is called every KBot::onFrame().
        void update();

        // Transfer ownership of a unit to general.
        void transferOwnership(BWAPI::Unit unit);

    private:
        KBot &m_kBot;

        BWAPI::Unitset m_squad;
    };

} // namespace
