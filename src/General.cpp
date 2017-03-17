#include "General.h"

#include <BWAPI.h>
#include "KBot.h"

namespace KBot {

    using namespace BWAPI;

    General::General(KBot &parent) : m_kBot(parent) {}

    void General::update() {
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
            if (u->getType() == UnitTypes::Terran_Marine) {
                if (u->isIdle()) {
                    const auto gatheringPoint = Broodwar->getRegionAt(Position(Broodwar->self()->getStartLocation()))->getCenter();
                    // Defend, if there are only few marines.
                    if (Broodwar->getUnitsInRadius(u->getPosition(), 400, Filter::GetType == UnitTypes::Terran_Marine && Filter::IsOwned).size() < 20) {
                        auto nearbyEnemies = Broodwar->getUnitsInRadius(u->getPosition(), 400, Filter::IsEnemy);
                        if (!nearbyEnemies.empty())
                            u->attack(nearbyEnemies.getClosestUnit()->getPosition());
                        else if (u->getPosition().getDistance(gatheringPoint) > 150)
                            u->attack(gatheringPoint);
                    }
                    // Otherwise, attack! :P
                    else
                        u->attack(Position(m_kBot.getNextEnemyLocation()));
                }
            }
        } // closure: unit iterator
    }

} // namespace
