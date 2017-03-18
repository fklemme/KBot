#pragma once

#include <BWAPI.h>

namespace KBot {

    class KBot;

    enum class SquadState {scout, attack, defend};

    class General {
    public:
        General(KBot &parent);

        // Is called every KBot::onFrame().
        void update();

        // Transfer ownership of a unit to general.
        void transferOwnership(BWAPI::Unit unit);

    private:
        KBot &m_kBot;

        SquadState m_squadState;
        BWAPI::Unitset m_squad;
    };

} // namespace
