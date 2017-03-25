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

        // Update build tasks
        for (auto &buildTask : m_buildQueue)
            buildTask.update();

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;
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

    //TilePosition KBot::getNextBasePosition() const {
    //    std::vector<TilePosition> positions;
    //    for (const auto &area : m_map.Areas()) {
    //        for (const auto &base : area.Bases())
    //            if (std::all_of(m_manager.getBases().begin(), m_manager.getBases().end(),
    //                [&base](const Base &b) { return b.getPosition() != base.Location(); }))
    //                positions.push_back(base.Location());
    //    }

    //    // Order position by and distance to own starting base.
    //    std::sort(positions.begin(), positions.end(), [this](TilePosition a, TilePosition b) {
    //        const auto startPosition = manager().getBases().front().getPosition();
    //        return distance(startPosition, a) < distance(startPosition, b);
    //    });

    //    return positions.front();
    //}

    void Manager::addBuildTask(const BuildTask &buildTask) {
        m_buildQueue.push_back(buildTask);
    }

    void Manager::onBuildTaskCreated(const Unit &unit) {}

    void Manager::onBuildTaskDestroyed(const Unit &unit) {}

    void Manager::onBuildTaskCompleted(const Unit &unit) {}

} // namespace
