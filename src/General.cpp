#include "General.h"

#include "KBot.h"

namespace KBot {

    using namespace BWAPI;

    General::General(KBot &parent) : m_kBot(parent) {}

    void General::update() {
        // Draw squad radius
        if (!m_squad.empty())
            Broodwar->drawCircleMap(m_squad.getPosition(), 400, Colors::Red);

        // Draw individual radius
        //for (const auto unit : m_squad)
        //    Broodwar->drawCircleMap(unit->getPosition(), 200, Colors::Yellow);

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        for (const auto unit : m_squad) {
            // Remove dead units
            if (!unit->exists()) {
                m_squad.erase(unit);
                continue;
            }

            // Ignore the unit if it has one of the following status ailments
            if (unit->isLockedDown() || unit->isMaelstrommed() || unit->isStasised())
                continue;

            // Ignore the unit if it is in one of the following states
            if (unit->isLoaded() || !unit->isPowered() || unit->isStuck())
                continue;

            if (unit->getType() == UnitTypes::Terran_Marine) {
                if (!Broodwar->getUnitsInRadius(unit->getPosition(), 200, Filter::IsEnemy).empty())
                    // FIXME: Don't spam commands im enemy is near?
                    continue;

                // If we haven't found an enemy yet, scout!
                if (m_kBot.getEnemyLocationCount() == 0)
                    unit->attack(Position(m_kBot.getNextEnemyLocation()));
                // If far away from squad and alone, go there and join squad.
                else if (unit->getDistance(m_squad.getPosition()) > 400 && Broodwar->getUnitsInRadius(unit->getPosition(), 200, Filter::GetType == UnitTypes::Terran_Marine && Filter::IsOwned).size() < 10)
                    unit->attack(m_squad.getPosition());
                // Otherwise, if we're big enough, let's attack!
                else if (m_squad.size() >= 20)
                    unit->attack(Position(m_kBot.getNextEnemyLocation()));
            }
        }
    }

    void General::transferOwnership(BWAPI::Unit unit) {
        Broodwar->registerEvent([unit](Game*) {
            Broodwar->drawTextMap(Position(unit->getPosition()), "General: %s", unit->getType().c_str());
        }, [unit](Game*) { return unit->exists(); }, 250);

        m_squad.insert(unit);
    }

} // namespace
