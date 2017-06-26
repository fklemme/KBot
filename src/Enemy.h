#pragma once

#include <BWAPI.h>
#include <vector>

namespace KBot {

class KBot;

class Enemy {
public:
    Enemy(KBot &kBot);

    // Prohibit copy & move. (For now?)
    Enemy(const Enemy &) = delete;
    Enemy(Enemy &&) = delete;
    Enemy &operator=(const Enemy &) = delete;
    Enemy &operator=(Enemy &&) = delete;

    // Called every KBot::onFrame().
    void update();

    void addPosition(const BWAPI::TilePosition &position);
    BWAPI::TilePosition getClosestPosition() const;
    std::size_t         getPositionCount() const { return m_positions.size(); }

private:
    KBot &                           m_kBot;
    std::vector<BWAPI::TilePosition> m_positions; // ordered!
};

} // namespace
