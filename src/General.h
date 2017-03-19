#pragma once

#include <BWAPI.h>
#include "Squad.h"

namespace KBot {

    class KBot;

    class General {
    public:
        General(KBot &kBot);

        // Is called every KBot::onFrame().
        void update();

        // Transfer ownership of a unit to general.
        void transferOwnership(BWAPI::Unit unit);

    private:
        KBot &m_kBot;
        std::deque<Squad> m_squads;
    };

} // namespace
