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
        void transferOwnership(const BWAPI::Unit &unit);
        void onUnitDestroy(const BWAPI::Unit &unit);

    private:
        KBot &m_kBot;
        std::vector<Squad> m_squads;
    };

} // namespace
