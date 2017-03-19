#pragma once

#include <BWAPI.h>

namespace KBot {

    class KBot;

    enum class SquadState { scout, attack, defend };

    std::string to_string(SquadState state);

    class Squad : public BWAPI::Unitset {
    public:
        Squad(KBot &kBot);

        void update();
        SquadState getState() const { return m_state; }

    private:
        KBot *m_kBot; // reference would permit default operator=()
        SquadState m_state;
    };

} // namespace
