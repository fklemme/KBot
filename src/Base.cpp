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

    Base::Base(Manager &manager, const TilePosition &position) : m_manager(&manager), m_position(position) {}

    void Base::update() {
        const auto mineralWorkerRatio = 2;
        const auto gasWorkerRatio = 3;
        const auto targetMineralWorkers = std::size_t(std::ceil(mineralWorkerRatio * m_mineralPatches.size()));
        const auto targetGasWorkers = std::size_t(std::ceil(gasWorkerRatio * m_gasGeysirs.size()));

        // Display debug information
        const auto center = Position(m_position) + Position(UnitTypes::Terran_Command_Center.tileSize()) / 2;
        Broodwar->drawCircleMap(center, 400, Colors::Green);
        Broodwar->drawBoxMap(Position(m_position), Position(m_position + UnitTypes::Terran_Command_Center.tileSize()), Colors::Green);

        // Show membership
        for (const auto &unit : m_units)
            Broodwar->drawLineMap(center, unit->getPosition(), Colors::Grey);

        // Print resource information
        Broodwar->drawTextMap(center, "Minerals: %d / %d", m_mineralWorkers.size(), targetMineralWorkers);
        Broodwar->drawTextMap(m_gasGeysirs.getPosition(), "Gas: %d / %d", m_gasWorkers.size(), targetGasWorkers);

        // ----- Prevent spamming -----------------------------------------------
        // Everything below is executed only occasionally and not on every frame.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // Update base resources
        m_mineralPatches = Broodwar->getUnitsInRadius(center, 400, Filter::IsMineralField); // TODO: Use BWEM instead?
        m_mineralWorkers = Broodwar->getUnitsInRadius(center, 400, Filter::IsWorker && Filter::IsGatheringMinerals && Filter::IsOwned);
        m_gasGeysirs = Broodwar->getUnitsInRadius(center, 400, Filter::GetType == UnitTypes::Resource_Vespene_Geyser || (Filter::IsRefinery && Filter::IsOwned));
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

    void Base::transferOwnership(const Unit &unit) {
        m_units.insert(unit);
    }

    void Base::onUnitDestroy(const Unit &unit) {
        m_units.erase(unit);
    }

} // namespace
