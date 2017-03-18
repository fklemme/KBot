#include "Manager.h"

#include "KBot.h"

namespace KBot {

    using namespace BWAPI;

    // Helper for easy building placement.
    static bool buildNearPosition(UnitType unit, TilePosition position) {
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

    Manager::Manager(KBot &parent) : m_kBot(parent) {}

    void Manager::update() {
        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // Iterate through all the units that we own
        for (auto &u : Broodwar->self()->getUnits()) {
            // Ignore the unit if it no longer exists
            // Make sure to include this block when handling any Unit pointer!
            if (!u->exists())
                continue;

            // Ignore the unit if it has one of the following status ailments
            if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
                continue;

            // Ignore the unit if it is in one of the following states
            if (u->isLoaded() || !u->isPowered() || u->isStuck())
                continue;

            // Ignore the unit if it is incomplete or busy constructing
            if (!u->isCompleted() || u->isConstructing())
                continue;


            // Finally make the unit do some stuff!
            // TODO: Remove all that demo stuff...
            // For now and for fun, let's just build a simple marine rush bot using what we have. :)


            // If the unit is a worker unit
            if (u->getType().isWorker()) {
                // if our worker is idle
                if (u->isIdle()) {
                    // Order workers carrying a resource to return them to the center,
                    // otherwise find a mineral patch to harvest.
                    if (u->isCarryingGas() || u->isCarryingMinerals()) {
                        u->returnCargo();
                    }
                    // Harvest from the nearest mineral patch or gas refinery
                    else if (!u->gather(u->getClosestUnit(Filter::IsMineralField || Filter::IsRefinery))) {
                        // No visible minerals.
                    }
                } // closure: if idle
            }
            else if (u->getType() == UnitTypes::Terran_Barracks) {
                if (u->isIdle()) {
                    // Spam marines! :D
                    u->train(UnitTypes::Terran_Marine);
                }
            }
            else if (u->getType().isResourceDepot()) {
                // Limit amount of workers to produce.
                if (Broodwar->getUnitsInRadius(u->getPosition(), 400, Filter::IsWorker && Filter::IsOwned).size() < 20) {
                    // Order the depot to construct more workers! But only when it is idle.
                    if (u->isIdle())
                        u->train(UnitTypes::Terran_SCV);
                }
            }
        } // closure: unit iterator

        // Build depos and racks!
        static int delay = 0;
        if (Broodwar->getFrameCount() > delay + 250) {
            const auto &me = *Broodwar->self();
            if (me.supplyUsed() >= me.supplyTotal() - 4 && me.minerals() >= UnitTypes::Terran_Supply_Depot.mineralPrice()) {
                if (buildNearPosition(UnitTypes::Terran_Supply_Depot, me.getStartLocation()))
                    delay = Broodwar->getFrameCount();
            }
            else if (me.minerals() >= UnitTypes::Terran_Barracks.mineralPrice()) {
                if (buildNearPosition(UnitTypes::Terran_Barracks, me.getStartLocation()))
                    delay = Broodwar->getFrameCount();
            }
        }
    }

    void Manager::transferOwnership(BWAPI::Unit unit) {
        Broodwar->registerEvent([unit](Game*) {
            Broodwar->drawTextMap(Position(unit->getPosition()), "Manager: %s", unit->getType().c_str());
        }, [unit](Game*) { return unit->exists(); }, 250);
    }

} // namespace
