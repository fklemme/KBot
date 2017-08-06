#pragma once

#include "Squad.h"
#include <BWAPI.h>
#include <vector>

namespace KBot {

class KBot;

/// Handles all military units and tasks.
class General {
    // Prohibit copy & move. There is only one general.
    General(const General &) = delete;
    General(General &&) = delete;
    General &operator=(const General &) = delete;
    General &operator=(General &&) = delete;

public:
    General(KBot &kBot);

    /// Called every KBot::onFrame().
    void update();

    /// Transfer ownership of a unit to general.
    void giveOwnership(const BWAPI::Unit &unit);

    /// Take ownership of a unit from general (forcibly).
    void takeOwnership(const BWAPI::Unit &unit);

    const std::vector<Squad> &getSquads() const { return m_squads; }

private:
    KBot &             m_kBot;
    std::vector<Squad> m_squads;
};

} // namespace KBot
