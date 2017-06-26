#include "Enemy.h"

#include "KBot.h"
#include "utils.h"
#include <random>

namespace KBot {

using namespace BWAPI;

Enemy::Enemy(KBot &kBot) : m_kBot(kBot) {}

void Enemy::update() {
    // ----- Prevent spamming -----------------------------------------------
    // Everything below is executed only occasionally and not on every frame.
    if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
        return;

    // Delete enemy positions if there is no enemy (any more).
    // TODO: Works for now, but can surely be improved?
    while (!m_positions.empty() && Broodwar->isVisible(m_positions.front()) &&
           Broodwar->getUnitsOnTile(m_positions.front(), Filter::IsEnemy).empty())
        m_positions.erase(m_positions.begin());
}

void Enemy::addPosition(const BWAPI::TilePosition &position) {
    const auto myPosition = Broodwar->self()->getStartLocation();
    auto distComp = [&](const TilePosition &a, const TilePosition &b) {
        return distance(myPosition, a) < distance(myPosition, b);
    };

    // Insert enemy position if not already in vector.
    const auto it = std::lower_bound(m_positions.begin(), m_positions.end(), position, distComp);
    if (it == m_positions.end() || *it != position)
        m_positions.insert(it, position);
}

TilePosition Enemy::getClosestPosition() const {
    if (!m_positions.empty())
        return m_positions.front();

    // TODO: Update to multiple own bases concept?
    // Positions to scout
    auto positions = m_kBot.map().StartingLocations();
    if (std::all_of(positions.begin(), positions.end(),
                    [](const TilePosition &p) { return Broodwar->isExplored(p); })) {
        positions.clear();
        for (const auto &area : m_kBot.map().Areas())
            for (const auto &base : area.Bases())
                positions.push_back(base.Location());
    }

    // Always exclude our own base.
    const auto myPosition = Broodwar->self()->getStartLocation();
    const auto it = std::find(positions.begin(), positions.end(), myPosition);
    assert(it != positions.end());
    positions.erase(it);

    // Order positions by isExplored and distance to own base.
    std::sort(positions.begin(), positions.end(), [&](TilePosition a, TilePosition b) {
        return distance(myPosition, a) < distance(myPosition, b);
    });
    std::stable_sort(positions.begin(), positions.end(), [](TilePosition a, TilePosition b) {
        return (int)Broodwar->isExplored(a) < (int)Broodwar->isExplored(b);
    });
    if (!Broodwar->isExplored(positions.front()))
        return positions.front();

    // If all positions are already explored, return a random position.
    static std::default_random_engine  generator;
    std::uniform_int_distribution<int> dist(0, positions.size() - 1);
    return positions[dist(generator)];
}

} // namespace
