#include "Manager.h"

#include <BWAPI.h>

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

        // Build depos and racks!
        static int delay = 0;
        if (Broodwar->getFrameCount() > delay + 250) {
            const auto &me = *Broodwar->self();
            if (me.supplyUsed() >= me.supplyTotal() - 4 && me.minerals() >= UnitTypes::Terran_Supply_Depot.mineralPrice()) {
                if (buildNearPosition(UnitTypes::Terran_Supply_Depot, me.getStartLocation()))
                    delay = Broodwar->getFrameCount();
                else
                    Broodwar << Broodwar->getLastError() << std::endl;
            }
            else if (me.minerals() >= UnitTypes::Terran_Barracks.mineralPrice()) {
                if (buildNearPosition(UnitTypes::Terran_Barracks, me.getStartLocation()))
                    delay = Broodwar->getFrameCount();
                else
                    Broodwar << Broodwar->getLastError() << std::endl;
            }
        }
    }

} // namespace
