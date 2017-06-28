#pragma once

#include <BWAPI.h>
#include <string>

namespace KBot {

class KBot;

class Squad : public BWAPI::Unitset {
public:
    enum class State { scout, attack, defend };

public:
    Squad(KBot &kBot);

    // Called every KBot::onFrame().
    void  update();
    State getState() const { return m_state; }

private:
    KBot *m_kBot;
    State m_state;
};

std::string to_string(Squad::State state);

} // namespace
