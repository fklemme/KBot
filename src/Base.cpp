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
                auto location = Broodwar->canBuildHere(position, unit, builder) ? position : Broodwar->getBuildLocation(unit, position);
                if (location) {
                    Broodwar->registerEvent([unit, location](Game*) {
                        Broodwar->drawBoxMap(Position(location), Position(location + unit.tileSize()), Colors::Blue);
                    }, nullptr, 1000);

                    return builder->build(unit, location);
                }
            }
            return false;
        }
    }

    Base::Base(KBot &kBot, const TilePosition &location) : m_kBot(kBot), m_location(location) {}

    void Base::update() {
        // Display debug information
        const auto center = Position(m_location) + Position(UnitTypes::Terran_Command_Center.tileSize()) / 2;
        Broodwar->drawCircleMap(center, 400, Colors::Green);

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

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
                    // Order workers carrying a resource to return them to the center,
                    // otherwise find a mineral patch to harvest.
                    if (unit->isCarryingGas() || unit->isCarryingMinerals()) {
                        unit->returnCargo();
                    }
                    // Harvest from the nearest mineral patch or gas refinery
                    const auto resource = unit->getClosestUnit(Filter::IsMineralField || Filter::IsRefinery, 400);
                    if (resource) {
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
                } // closure: if idle
            }
            else if (unit->getType() == UnitTypes::Terran_Barracks) {
                if (unit->isIdle()) {
                    // Spam marines! :D
                    unit->train(UnitTypes::Terran_Marine);
                }
            }
            else if (unit->getType().isResourceDepot()) {
                // Limit amount of workers to produce.
                if (Broodwar->getUnitsInRadius(unit->getPosition(), 400, Filter::IsWorker && Filter::IsOwned).size() < 20) {
                    // Order the depot to construct more workers! But only when it is idle.
                    if (unit->isIdle())
                        unit->train(UnitTypes::Terran_SCV);
                }
            }
        } // closure: unit iterator

        // Build all the structures!
        static int delay = 0;
        if (Broodwar->getFrameCount() > delay + 250) {
            const auto &me = *Broodwar->self();
            UnitType toBeBuild = UnitTypes::None;

            if (Broodwar->getUnitsOnTile(m_location, Filter::IsResourceDepot).empty() && me.minerals() >= UnitTypes::Terran_Command_Center.mineralPrice())
                toBeBuild = UnitTypes::Terran_Command_Center;
            else if (me.supplyUsed() >= me.supplyTotal() - 4 && me.minerals() >= UnitTypes::Terran_Supply_Depot.mineralPrice())
                toBeBuild = UnitTypes::Terran_Supply_Depot;
            else if (me.minerals() >= (1 + 0.5
                * Broodwar->getUnitsInRadius(Position(m_location), 1000, Filter::GetType == UnitTypes::Terran_Barracks).size())
                * UnitTypes::Terran_Barracks.mineralPrice())
                toBeBuild = UnitTypes::Terran_Barracks;

            if (toBeBuild != UnitTypes::None) {
                if (!buildAtOrNearPosition(toBeBuild, m_location)) {
                    // Location might not be explored yet. Send SCV.
                    auto builder = Broodwar->getClosestUnit(Position(m_location),
                        Filter::GetType == UnitTypes::Terran_SCV &&
                        (Filter::IsIdle || Filter::IsGatheringMinerals) &&
                        Filter::IsOwned);

                    if (builder) {
                        auto buildLocation = Broodwar->getBuildLocation(toBeBuild, m_location);
                        if (buildLocation/* && Broodwar->isExplored(...)*/)
                            builder->move(Position(buildLocation));
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
