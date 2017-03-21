#include "Base.h"

#include "KBot.h"
#include "utils.h"

namespace KBot {

    using namespace BWAPI;

    namespace {
        // Helper for easy building placement.
        bool buildAtOrNearPosition(UnitType unit, TilePosition position) {
            auto builder = Broodwar->getClosestUnit(Position(position),
                Filter::GetType == UnitTypes::Terran_SCV &&
                (Filter::IsIdle || Filter::IsGatheringMinerals) &&
                Filter::IsOwned);

            if (builder) {
                auto buildPosition = Broodwar->canBuildHere(position, unit, builder) ? position : Broodwar->getBuildLocation(unit, position);
                if (buildPosition) {
                    Broodwar->registerEvent([unit, buildPosition](Game*) {
                        Broodwar->drawBoxMap(Position(buildPosition), Position(buildPosition + unit.tileSize()), Colors::Blue);
                    }, nullptr, 1000);

                    return builder->build(unit, buildPosition);
                }
            }
            return false;
        }
    }

    Base::Base(KBot &kBot, const TilePosition &position) : m_kBot(kBot), m_position(position) {}

    void Base::update() {
        const auto mineralWorkerRatio = 2;
        const auto gasWorkerRatio = 3;
        const auto targetMineralWorkers = std::size_t(std::ceil(mineralWorkerRatio * m_minerals.size()));
        const auto targetGasWorkers = std::size_t(std::ceil(gasWorkerRatio * m_gas.size()));

        // Display debug information
        const auto center = Position(m_position) + Position(UnitTypes::Terran_Command_Center.tileSize()) / 2;
        Broodwar->drawCircleMap(center, 400, Colors::Green);
        Broodwar->drawBoxMap(Position(m_position), Position(m_position + UnitTypes::Terran_Command_Center.tileSize()), Colors::Green);

        // Show membership
        for (const auto &unit : m_units)
            Broodwar->drawLineMap(center, unit->getPosition(), Colors::Grey);

        // Print resource information
        Broodwar->drawTextMap(center, "Minerals: %d / %d", m_mineralWorkers.size(), targetMineralWorkers);
        Broodwar->drawTextMap(m_gas.getPosition(), "Gas: %d / %d", m_gasWorkers.size(), targetGasWorkers);

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // Update base resources
        m_minerals = Broodwar->getUnitsInRadius(center, 400, Filter::IsMineralField); // TODO: Use BWEM instead?
        m_mineralWorkers = Broodwar->getUnitsInRadius(center, 400, Filter::IsWorker && Filter::IsGatheringMinerals && Filter::IsOwned);
        m_gas = Broodwar->getUnitsInRadius(center, 400, Filter::GetType == UnitTypes::Resource_Vespene_Geyser || (Filter::IsRefinery && Filter::IsOwned));
        m_gasWorkers = Broodwar->getUnitsInRadius(center, 400, Filter::IsWorker && Filter::IsGatheringGas && Filter::IsOwned);

        for (const auto &unit : m_units) {
            assert(unit->exists());

            // Ignore the unit if it has one of the following status ailments
            if (unit->isLockedDown() || unit->isMaelstrommed() || unit->isStasised())
                continue;

            // Ignore the unit if it is in one of the following states
            if (unit->isLoaded() || !unit->isPowered() || unit->isStuck())
                continue;

            // If the unit is a worker unit
            if (unit->getType().isWorker()) {
                // if our worker is idle
                if (unit->isIdle()) {
                    const auto resource = Broodwar->getClosestUnit(center, Filter::IsMineralField /*|| Filter::IsRefinery*/, 400);
                    // Order workers carrying a resource to return them to the center
                    if (unit->isCarryingGas() || unit->isCarryingMinerals())
                        unit->returnCargo();
                    else if (resource) {
                        const bool r = unit->gather(resource);
                        assert(r);
                    }
                    else {
                        // Easy workaround for now: Use BWEM to find the next minerals.
                        const auto &minerals = m_kBot.map().Minerals();
                        using MineralsPtr = decltype(*minerals.begin());
                        const auto it = std::min_element(minerals.begin(), minerals.end(), [&unit](MineralsPtr a, MineralsPtr b) {
                            return distance(unit->getPosition(), a->Pos()) < distance(unit->getPosition(), b->Pos());
                        });
                        assert(it != minerals.end()); // FIXME: No more minerals! :O
                        // The unit might not be accessable, so just move there.
                        const auto &mineral = **it;
                        unit->move(mineral.Pos());

                        // debug
                        Broodwar->registerEvent([unit, &mineral](Game*) {
                            Broodwar->drawLineMap(unit->getPosition(), mineral.Pos(), Colors::Purple);
                        }, [unit, &mineral](Game*) {
                            return unit->exists() && unit->getOrder() == Orders::Move && unit->getOrderTargetPosition() == mineral.Pos();
                        }, 1000);
                    }
                }
            }
            else if (unit->getType() == UnitTypes::Terran_Barracks) {
                if (unit->isIdle()) {
                    // Spam marines! :D
                    unit->train(UnitTypes::Terran_Marine);
                }
            }
            else if (unit->getType().isResourceDepot()) {
                // Limit amount of workers to produce.
                if (m_mineralWorkers.size() < targetMineralWorkers /*|| m_gasWorkers.size() < targetGasWorkers*/) {
                    // Order the depot to construct more workers! But only when it is idle.
                    if (unit->isIdle())
                        unit->train(UnitTypes::Terran_SCV);
                }
            }
        }

        // Build all the structures!
        static int delay = 0;
        if (Broodwar->getFrameCount() > delay + 250) {
            const auto &me = *Broodwar->self();
            UnitType toBeBuild = UnitTypes::None;

            if (Broodwar->getUnitsOnTile(m_position, Filter::IsResourceDepot).empty()) {
                if (me.minerals() >= UnitTypes::Terran_Command_Center.mineralPrice())
                    toBeBuild = UnitTypes::Terran_Command_Center;
            }
            else if (me.supplyUsed() >= me.supplyTotal() - 4 && me.minerals() >= UnitTypes::Terran_Supply_Depot.mineralPrice())
                toBeBuild = UnitTypes::Terran_Supply_Depot;
            else if (me.minerals() >= (1 + 0.5
                * Broodwar->getUnitsInRadius(Position(m_position), 1000, Filter::GetType == UnitTypes::Terran_Barracks && Filter::IsOwned).size())
                * UnitTypes::Terran_Barracks.mineralPrice())
                toBeBuild = UnitTypes::Terran_Barracks;

            if (toBeBuild != UnitTypes::None) {
                if (!buildAtOrNearPosition(toBeBuild, m_position)) {
                    // Position might not be fully explored yet. Send SCV.
                    auto builder = Broodwar->getClosestUnit(Position(m_position),
                        Filter::GetType == UnitTypes::Terran_SCV &&
                        (Filter::IsIdle || Filter::IsGatheringMinerals) &&
                        Filter::IsOwned);

                    if (builder) {
                        auto buildPosition = Broodwar->getBuildLocation(toBeBuild, m_position);
                        if (buildPosition /*&& Broodwar->isExplored(...)*/)
                            builder->move(Position(buildPosition));
                    }
                }
                delay = Broodwar->getFrameCount();
            }
        }
    }

    void Base::transferOwnership(const Unit &unit) {
        m_units.insert(unit);
    }

    void Base::onUnitDestroy(const Unit &unit) {
        m_units.erase(unit);
    }

} // namespace
