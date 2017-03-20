#include "Base.h"

#include "KBot.h"

namespace KBot {

    using namespace BWAPI;

    namespace {
        // Helper for easy building placement.
        bool buildNearPosition(UnitType unit, TilePosition position) {
            auto builder = Broodwar->getClosestUnit(Position(position),
                Filter::GetType == UnitTypes::Terran_SCV &&
                (Filter::IsIdle || Filter::IsGatheringMinerals) &&
                Filter::IsOwned);

            if (builder) {
                auto location = Broodwar->getBuildLocation(unit, position);
                if (location) {
                    Broodwar->registerEvent([unit, location](Game*) {
                        Broodwar->drawBoxMap(Position(location), Position(location + unit.tileSize()), Colors::Blue);
                    }, nullptr, 250);

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
                    else if (!unit->gather(unit->getClosestUnit(Filter::IsMineralField || Filter::IsRefinery))) {
                        // No visible minerals.
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

        // Build depos and racks!
        static int delay = 0;
        if (Broodwar->getFrameCount() > delay + 250) {
            const auto &me = *Broodwar->self();
            const auto location = me.getStartLocation();
            UnitType toBeBuild = UnitTypes::None;

            if (me.supplyUsed() >= me.supplyTotal() - 4 && me.minerals() >= UnitTypes::Terran_Supply_Depot.mineralPrice())
                toBeBuild = UnitTypes::Terran_Supply_Depot;
            else if (me.minerals() >= UnitTypes::Terran_Barracks.mineralPrice())
                toBeBuild = UnitTypes::Terran_Barracks;

            if (toBeBuild != UnitTypes::None) {
                if (!buildNearPosition(toBeBuild, location)) {
                    // Location might not be explored yet. Send SCV.
                    auto builder = Broodwar->getClosestUnit(Position(location),
                        Filter::GetType == UnitTypes::Terran_SCV &&
                        (Filter::IsIdle || Filter::IsGatheringMinerals) &&
                        Filter::IsOwned);

                    if (builder) {
                        auto buildLocation = Broodwar->getBuildLocation(toBeBuild, location);
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
