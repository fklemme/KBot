#include "Base.h"

#include "KBot.h"
#include "utils.h"

namespace KBot {

    using namespace BWAPI;

    Base::Base(Manager &manager, const TilePosition &position) : m_manager(&manager), m_position(position) {
        const auto center = Position(position) + Position(UnitTypes::Terran_Command_Center.tileSize()) / 2;
        m_mineralPatches = Broodwar->getUnitsInRadius(center, 400, Filter::IsMineralField); // TODO: Use BWEM instead?
        m_vespeneGeysers = Broodwar->getUnitsInRadius(center, 400, Filter::GetType == UnitTypes::Resource_Vespene_Geyser);
        // TODO: Handling of enemy refineries nearby.
    }

    void Base::update() {
        // Display debug information
        const auto center = Position(m_position) + Position(UnitTypes::Terran_Command_Center.tileSize()) / 2;
        Broodwar->drawCircleMap(center, 400, Colors::Green);
        Broodwar->drawBoxMap(Position(m_position), Position(m_position + UnitTypes::Terran_Command_Center.tileSize()), Colors::Green);

        // Show membership
        // TODO: Too much work! Remove/change this.
        auto unassignedUnits = m_units; // copy
        for (const auto &mineralWorker : m_mineralWorkers) {
            Broodwar->drawLineMap(center, mineralWorker->getPosition(), Colors::Cyan);
            unassignedUnits.erase(mineralWorker);
        }
        for (const auto &geyser : m_gasWorkers) {
            for (const auto &gasWorker : geyser.second) {
                Broodwar->drawLineMap(center, gasWorker->getPosition(), Colors::Green);
                unassignedUnits.erase(gasWorker);
            }
        }
        for (const auto &unit : unassignedUnits)
            Broodwar->drawLineMap(center, unit->getPosition(), Colors::Red); // buildings as well

        // Print resource information
        Broodwar->drawTextMap(center, "Minerals: %d / %d", m_mineralWorkers.size(), targetMineralWorkers());
        Broodwar->drawTextMap(center + Position(0, 10), "Worker neede: %d", workersLeftToBuild());
        for (const auto &gw : m_gasWorkers)
            Broodwar->drawTextMap(gw.first->getPosition(), "Gas: %d / %d", gw.second.size(), (int) gasWorkerRatio);
        for (const auto &geyser : m_vespeneGeysers)
            Broodwar->drawTextMap(geyser->getPosition(), "Unbuilt vespene geyser");

        // ----- Prevent spamming -----------------------------------------------
        // Everything below is executed only occasionally and not on every frame.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // Update base resources
        // FIXME TODO: This whole part has to be change. Go away from rebuilding everthing all the time and start to keep track of units...
        //m_mineralPatches = Broodwar->getUnitsInRadius(center, 400, Filter::IsMineralField); // TODO: Use BWEM instead?
        //m_mineralWorkers = Broodwar->getUnitsInRadius(center, 400, Filter::IsWorker && Filter::IsGatheringMinerals && Filter::IsOwned);
        //m_vespeneGeysers = Broodwar->getUnitsInRadius(center, 400, Filter::GetType == UnitTypes::Resource_Vespene_Geyser);
        //m_gasWorkers = Broodwar->getUnitsInRadius(center, 400, Filter::IsRefinery && Filter::IsOwned);
        //m_gasWorkers = Broodwar->getUnitsInRadius(center, 400, Filter::IsWorker && Filter::IsGatheringGas && Filter::IsOwned);

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
                    const auto resource = Broodwar->getClosestUnit(center, Filter::IsMineralField, 400);
                    // Order workers carrying a resource to return them to the center
                    if (unit->isCarryingGas() || unit->isCarryingMinerals())
                        unit->returnCargo();
                    else if (resource) {
                        const bool r = unit->gather(resource);
                        assert(r);
                    } else {
                        // Easy workaround for now: Use BWEM to find the next minerals.
                        const auto &minerals = BWEM::Map::Instance().Minerals();
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
        }
    }

    void Base::giveOwnership(const Unit &unit) {
        m_units.insert(unit);
    }

    void Base::takeOwnership(const Unit &unit) {
        m_units.erase(unit);
        // TODO: Other containers!
    }

} // namespace
