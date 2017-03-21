#include "Manager.h"

#include "KBot.h"
#include "utils.h"

namespace KBot {

    using namespace BWAPI;

    Manager::Manager(KBot &kBot) : m_kBot(kBot) {}

    void Manager::update() {
        // Display debug information
        Broodwar->drawTextScreen(2, 50, "Manager: -");

        // Update bases
        for (auto &base : m_bases)
            base.update();

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // TODO: When and how to expand? Just a expand test so far...
        TilePosition max(Broodwar->mapWidth(), Broodwar->mapHeight());
        const auto scv_count = Broodwar->getUnitsInRectangle(Position(0, 0), Position(max), Filter::IsWorker).size();
        const auto natural = m_kBot.getNextBaseLocation();
        if (m_bases.size() == 1 && scv_count >= 20)
            createBase(natural);
    }

    void Manager::createBase(const TilePosition &location) {
        m_bases.emplace_back(m_kBot, location);
    }

    void Manager::transferOwnership(const Unit &unit) {
        // Assign unit to nearest base.
        const auto it = std::min_element(m_bases.begin(), m_bases.end(), [&unit](const Base &a, const Base &b) {
            return distance(unit->getPosition(), a.getPosition())
                < distance(unit->getPosition(), b.getPosition());
        });
        assert(it != m_bases.end());
        it->transferOwnership(unit);
    }

    void Manager::onUnitDestroy(const Unit &unit) {
        // Just let every base know...
        for (auto &base : m_bases)
            base.onUnitDestroy(unit);
    }

} // namespace
