#include "Manager.h"

#include "KBot.h"
#include "utils.h"
#include <algorithm>

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
    const std::size_t maxBuildDisplay = 10;
    Broodwar->drawTextScreen(200, 0, "Build queue:");
    for (std::size_t i = 0; i < m_buildQueue.size() && i < maxBuildDisplay; ++i)
        Broodwar->drawTextScreen(200, (i + 1) * 10, "%s", m_buildQueue[i].toString().c_str());
    if (m_buildQueue.size() > maxBuildDisplay)
        Broodwar->drawTextScreen(200, (maxBuildDisplay + 1) * 10, "and %d more...",
                                 m_buildQueue.size() - maxBuildDisplay);

    // Display available resources
    Broodwar->drawTextScreen(450, 15, "(%d)", getAvailableMinerals());
    Broodwar->drawTextScreen(518, 15, "(%d)", getAvailableGas());

    // Update bases
    for (auto &base : m_bases)
        base.update();

    // Update build tasks
    for (auto &buildTask : m_buildQueue)
        buildTask.update();

    // ----- Prevent spamming -----------------------------------------------
    // Everything below is executed only occasionally and not on every frame.
    if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
        return;

    // Cleanup finished build tasks.
    for (auto it = m_buildQueue.begin(); it != m_buildQueue.end();) {
        if (it->getState() == BuildTask::State::finalize)
            it = m_buildQueue.erase(it);
        else
            ++it;
    }
}

void Manager::giveOwnership(const Unit &unit) {
    // Assign unit to nearest base.
    const auto it =
        std::min_element(m_bases.begin(), m_bases.end(), [&unit](const Base &a, const Base &b) {
            return distance(unit->getPosition(), a.getPosition()) <
                   distance(unit->getPosition(), b.getPosition());
        });
    assert(it != m_bases.end());
    it->giveOwnership(unit);
}

void Manager::takeOwnership(const Unit &unit) {
    // Remove unit from workers
    m_workers.erase(std::remove(m_workers.begin(), m_workers.end(), unit), m_workers.end());

    // And pass on to bases...
    for (auto &base : m_bases)
        base.takeOwnership(unit);
}

void Manager::addBuildTask(const BuildTask &buildTask) { m_buildQueue.push_back(buildTask); }

void Manager::buildTaskOnUnitCreatedOrMorphed(const Unit &unit) {
    // Return as soon as the first build task can identify the created unit.
    for (auto &buildTask : m_buildQueue) {
        if (buildTask.onUnitCreatedOrMorphed(unit))
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
    // Since BuildTasks are busy waiting for completion right now, this is unused.
}

int Manager::getAvailableMinerals() const {
    return BWAPI::Broodwar->self()->minerals() - m_reservedMinerals;
}

int Manager::getAvailableGas() const { return BWAPI::Broodwar->self()->gas() - m_reservedGas; }

bool Manager::acquireResources(int minerals, int gas, BuildTask::Priority priority) {
    // All higher tasks have to have their resources allocated, otherwise fail!
    for (auto it = m_buildQueue.begin(); it != m_buildQueue.end() && it->getPriority() > priority;
         ++it) {
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

void Manager::releaseResources(int minerals, int gas) {
    m_reservedMinerals -= minerals;
    m_reservedGas -= gas;
    assert(m_reservedMinerals >= 0);
    assert(m_reservedGas >= 0);
}

Unit Manager::acquireWorker(const UnitType &workerType, const Position &nearPosition) {
    // Search for matching workers and remember their bases.
    std::vector<std::pair<Unit, Base *>> workers;
    for (auto &base : m_bases) {
        const auto worker = base.findWorker(workerType, nearPosition);
        if (worker != nullptr)
            workers.emplace_back(worker, &base);
    }

    if (workers.empty()) {
        // No worker found.
        return nullptr;
    }

    // Pick best candidate...
    const auto closestWorker = std::min_element(
        workers.begin(), workers.end(), [&nearPosition](const auto &a, const auto &b) {
            return distance(nearPosition, a.first->getPosition()) <
                   distance(nearPosition, b.first->getPosition());
        });

    // ...and take ownership explicitly!
    closestWorker->second->takeOwnership(closestWorker->first);
    m_workers.push_back(closestWorker->first);
    return closestWorker->first;
}

void Manager::releaseWorker(const Unit &worker) {
    m_workers.erase(std::remove(m_workers.begin(), m_workers.end(), worker), m_workers.end());

    // Pass ownership back to bases
    giveOwnership(worker);
}

} // namespace
