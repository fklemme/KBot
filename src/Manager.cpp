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

    Manager::Manager(KBot &kBot) : m_kBot(kBot) {}

    void Manager::update() {
        // Display debug information
        Broodwar->drawTextScreen(2, 50, "Manager: -");

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // TODO: switch to onDestroy(). This is not save before C++14.
        // Remove dead units
        for (auto it = m_units.begin(); it != m_units.end();) {
            if (!(*it)->exists())
                it = m_units.erase(it);
            else ++it;
        }

        for (const auto unit : m_units) {
            // Ignore the unit if it has one of the following status ailments
            if (unit->isLockedDown() || unit->isMaelstrommed() || unit->isStasised())
                continue;

            // Ignore the unit if it is in one of the following states
            if (unit->isLoaded() || !unit->isPowered() || unit->isStuck())
                continue;

            // Ignore the unit if it is incomplete or busy constructing
            //if (!unit->isCompleted() || u->isConstructing())
            //    continue;

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

        m_units.insert(unit);
    }

} // namespace
