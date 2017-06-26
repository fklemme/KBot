#include "Base.h"

#include <algorithm>
#include "KBot.h"
#include <numeric>
#include <random>
#include "utils.h"

namespace KBot {

    using namespace BWAPI;

    Base::Base(Manager &manager, const TilePosition &position) : m_manager(&manager), m_position(position) {
        const auto center = Position(position) + Position(UnitTypes::Terran_Command_Center.tileSize()) / 2;
        m_mineralPatches = Broodwar->getUnitsInRadius(center, catchmentArea, Filter::IsMineralField); // TODO: Use BWEM instead?

        const auto gases = Broodwar->getUnitsInRadius(center, catchmentArea, Filter::GetType == UnitTypes::Resource_Vespene_Geyser || Filter::IsRefinery);
        for (const auto &gas : gases)
            m_gasesAndWorkers.emplace_back(gas, Unitset());
        // TODO: Handling of enemy refineries?
    }

    void Base::update() {
        // Display debug information
        const auto center = Position(m_position) + Position(UnitTypes::Terran_Command_Center.tileSize()) / 2;
        Broodwar->drawCircleMap(center, catchmentArea, Colors::Green);
        Broodwar->drawBoxMap(Position(m_position), Position(m_position + UnitTypes::Terran_Command_Center.tileSize()), Colors::Green);

        // Show membership
        for (const auto &worker : m_mineralWorkers) {
            Broodwar->drawLineMap(center, worker->getPosition(), Colors::Cyan);
        }
        for (const auto &gasAndWorkers : m_gasesAndWorkers) {
            for (const auto &worker : gasAndWorkers.second) {
                Broodwar->drawLineMap(center, worker->getPosition(), Colors::Green);
            }
        }
        for (const auto &unit : m_otherUnits)// buildings and unassigned workers
            Broodwar->drawLineMap(center, unit->getPosition(), Colors::Grey);

        // Print resource information
        Broodwar->drawTextMap(center, "Minerals: %d / %d", m_mineralWorkers.size(), targetMineralWorkers());
        Broodwar->drawTextMap(center + Position(0, 10), "Workers needed: %d", workersLeftToBuild());
        for (const auto &gasAndWorkers : m_gasesAndWorkers) {
            if (gasAvailable(gasAndWorkers.first))
                Broodwar->drawTextMap(gasAndWorkers.first->getPosition(), "Gas: %d / %d",
                    gasAndWorkers.second.size(), (int) gasWorkerRatio);
            else
                Broodwar->drawTextMap(gasAndWorkers.first->getPosition(), "Unavailable Gas");
        }

        // ----- Prevent spamming -----------------------------------------------
        // Everything below is executed only occasionally and not on every frame.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // Move workers between groups. One at a time should be fine.
        {
            const int minWorkers = m_mineralWorkers.size();
            const int targetMinW = targetMineralWorkers();
            const int gasWorkers = std::accumulate(m_gasesAndWorkers.begin(), m_gasesAndWorkers.end(), 0,
                [](int sum, const auto &p) { return sum + p.second.size(); });
            const int targetGasW = targetGasWorkers();

            const auto gasWithFewestWorkers = std::min_element(m_gasesAndWorkers.begin(), m_gasesAndWorkers.end(),
                [](const auto &p1, const auto &p2) { return p1.second.size() < p2.second.size(); });
            const auto gasWithMostWorkers = std::max_element(m_gasesAndWorkers.begin(), m_gasesAndWorkers.end(),
                [](const auto &p1, const auto &p2) { return p1.second.size() < p2.second.size(); });

            // If minerals or gas are over-saturated, pull workers.
            // Pull only workers that are ready to accept orders. Just to make sure the "stop" command is considered.
            if (minWorkers > targetMinW) {
                assert(!m_mineralWorkers.empty());
                const auto pulled = std::find_if(m_mineralWorkers.begin(), m_mineralWorkers.end(), readyToAcceptOrders);
                if (pulled != m_mineralWorkers.end()) {
                    m_otherUnits.insert(*pulled);
                    (*pulled)->stop();
                    m_mineralWorkers.erase(pulled);
                }
            }
            if (gasWorkers > targetGasW) {
                assert(gasWithMostWorkers != m_gasesAndWorkers.end());
                auto &group = gasWithMostWorkers->second;
                const auto pulled = std::find_if(group.begin(), group.end(), readyToAcceptOrders);
                if (pulled != group.end()) {
                    m_otherUnits.insert(*pulled);
                    (*pulled)->stop();
                    group.erase(pulled);
                }
            }

            // If there is an unassigned worker, saturate minerals and gas.
            const auto unassignedWorker = std::find_if(m_otherUnits.begin(), m_otherUnits.end(),
                [](const Unit &unit) { return unit->getType().isWorker() && unit->isIdle(); });

            if (unassignedWorker != m_otherUnits.end()) {
                // Fill gas first
                if (gasWorkers < targetGasW) {
                    assert(gasWithFewestWorkers != m_gasesAndWorkers.end());
                    gasWithFewestWorkers->second.insert(*unassignedWorker);
                    m_otherUnits.erase(unassignedWorker);
                } else if (minWorkers < targetMinW) {
                    m_mineralWorkers.insert(*unassignedWorker);
                    m_otherUnits.erase(unassignedWorker);
                } else {
                    // TODO!
                }
            }
        }

        // Make sure that mineral and gas workers are not idle.
        static std::default_random_engine generator;
        if (!m_mineralPatches.empty()) {
            std::uniform_int_distribution<int> dist(0, m_mineralPatches.size() - 1);
            for (const auto &worker : m_mineralWorkers) {
                if (worker->isIdle()) {
                    const bool r = worker->gather(*std::next(m_mineralPatches.begin(), dist(generator)));
                    assert(r);
                }
            }
        }

        for (const auto &gasAndWorkers : m_gasesAndWorkers) {
            if (gasAvailable(gasAndWorkers.first)) {
                for (const auto &worker : gasAndWorkers.second) {
                    if (worker->isIdle()) {
                        const bool r = worker->gather(gasAndWorkers.first);
                        assert(r);
                    }
                }
            }
        }
    }

    void Base::giveOwnership(const Unit &unit) {
        m_otherUnits.insert(unit);
    }

    void Base::takeOwnership(const Unit &unit) {
        m_mineralWorkers.erase(unit);

        for (auto &gasAndWorkers : m_gasesAndWorkers)
            gasAndWorkers.second.erase(unit);

        m_otherUnits.erase(unit);
    }

    // A merged version of std::find_if and std::min_element.
    template<typename ForwardIt, typename UnaryPredicate, typename Compare>
    ForwardIt min_element_if(ForwardIt first, ForwardIt last, UnaryPredicate p, Compare comp) {
        while (first != last && !p(*first)) ++first;
        if (first == last) return last;

        ForwardIt smallest = first;
        ++first;
        for (; first != last; ++first) {
            if (p(*first) && comp(*first, *smallest)) {
                smallest = first;
            }
        }
        return smallest;
    }

    Unit Base::findWorker(const UnitType &workerType, const Position &nearPosition) const {
        auto pred = [&workerType](const Unit &u) { return u->getType() == workerType; };
        auto comp = [&nearPosition](const Unit &a, const Unit &b) {
            return distance(nearPosition, a->getPosition())
                < distance(nearPosition, b->getPosition());
        };

        // Prefer "other units" over mineral workers over gas workers.
        std::vector<const Unitset*> unitsets = {&m_otherUnits, &m_mineralWorkers};
        for (const auto &gasAndWorkers : m_gasesAndWorkers)
            unitsets.push_back(&gasAndWorkers.second);

        for (const auto &set : unitsets) {
            const auto worker = min_element_if(set->begin(), set->end(), pred, comp);
            if (worker != set->end())
                return *worker;
        }

        // No worker found.
        return nullptr;
    }

    int Base::targetMineralWorkers() const {
        return (int) std::ceil(mineralWorkerRatio * m_mineralPatches.size());
    }

    int Base::targetGasWorkers() const {
        const auto refineries = std::count_if(m_gasesAndWorkers.begin(), m_gasesAndWorkers.end(),
            [&](const auto &gas) { return gasAvailable(gas.first); });
        return (int) std::ceil(gasWorkerRatio * refineries);
    }

    int Base::workersLeftToBuild() const {
        const auto gasWorkers = std::accumulate(m_gasesAndWorkers.begin(), m_gasesAndWorkers.end(), 0,
            [](int sum, const auto &p) { return sum + p.second.size(); });
        return targetMineralWorkers() - m_mineralWorkers.size() + targetGasWorkers() - gasWorkers;
    }

} // namespace
