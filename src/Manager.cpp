#include "Manager.h"

#include "KBot.h"
#include "utils.h"

namespace KBot {

    using namespace BWAPI;

    Manager::Manager(KBot &kBot) : m_kBot(kBot) {
        // Create initial base
        m_bases.emplace_back(*this, Broodwar->self()->getStartLocation());
    }

    void Manager::update() {
        // Display debug information
        Broodwar->drawTextScreen(2, 50, "Manager: -");

        // Display build tasks
        Broodwar->drawTextScreen(200, 0, "Build queue:");
        for (std::size_t i = 0; i < m_buildQueue.size(); ++i)
            Broodwar->drawTextScreen(200, (i + 1) * 10, "%s", m_buildQueue[i].toString().c_str());

        // Display available resources
        Broodwar->drawTextScreen(450, 15, "(%d)", getAvailableMinerals());
        Broodwar->drawTextScreen(518, 15, "(%d)", getAvailableGas());

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

        // Cleanup finished build tasks.
        for (auto it = m_buildQueue.begin(); it != m_buildQueue.end();) {
            if (it->getState() == BuildTask::State::finalize)
                it = m_buildQueue.erase(it);
            else ++it;
        }
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


    void Manager::addBuildTask(const BuildTask &buildTask) {
        m_buildQueue.push_back(buildTask);
    }

    void Manager::buildTaskOnUnitCreated(const Unit &unit) {
        // Return as soon as the first build task can identify the created unit.
        for (auto &buildTask : m_buildQueue) {
            if (buildTask.onUnitCreated(unit))
                return;
        }
    }

    void Manager::buildTaskOnUnitDestroyed(const Unit &unit) {
        // Return as soon as the first build task can identify the destroyed unit.
        for (auto &buildTask : m_buildQueue) {
            if (buildTask.onUnitDestroyed(unit))
                return;
        }
    }

    void Manager::buildTaskOnUnitCompleted(const Unit &unit) {
        // TODO
    }

    bool Manager::acquireResources(const int minerals, const int gas, const BuildTask::Priority priority) {
        // All higher tasks have to have their resources allocated, otherwise fail!
        for (auto it = m_buildQueue.begin(); it != m_buildQueue.end() && it->getPriority() > priority; ++it) {
            if (it->getState() <= BuildTask::State::acquireResources)
                return false;
        }

        // Allocate resources if available.
        if (getAvailableMinerals() >= minerals && getAvailableGas() >= gas) {
            m_reservedMinerals += minerals;
            m_reservedGas += gas;
            return true;
        }
        return false;
    }

    void Manager::releaseResources(const int minerals, const int gas) {
        m_reservedMinerals -= minerals;
        m_reservedGas -= gas;
        assert(m_reservedMinerals >= 0);
        assert(m_reservedGas >= 0);
    }

    Unit Manager::acquireWorker(const UnitType &workerType, const Position &position) {
        Unit worker = Broodwar->getClosestUnit(position,
            Filter::GetType == workerType
            && Filter::IsOwned && Filter::IsCompleted
            && (Filter::IsIdle || Filter::IsGatheringMinerals));

        if (worker) {
            if (std::find(m_workers.begin(), m_workers.end(), worker) != m_workers.end())
                worker = nullptr; // worker already acquired
            else
                m_workers.push_back(worker);
        }

        return worker;
    }

    void Manager::releaseWorker(const Unit &worker) {
        m_workers.erase(std::remove(m_workers.begin(), m_workers.end(), worker), m_workers.end());
    }

} // namespace
