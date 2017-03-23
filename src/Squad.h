#pragma once

#include <BWAPI.h>
#include "KBot.h"

namespace KBot {

    enum class SquadState { scout, attack, defend };

    std::string to_string(SquadState state);

    class Squad : public BWAPI::Unitset {
    public:
        Squad(KBot &kBot);

        void update();
        SquadState getState() const { return m_state; }

    private:
        std::reference_wrapper<KBot> m_kBot;
        SquadState m_state;
    };

} // namespace
