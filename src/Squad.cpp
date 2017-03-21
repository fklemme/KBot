#include "Squad.h"

#include "KBot.h"
#include "utils.h"

namespace KBot {

    using namespace BWAPI;

    std::string to_string(SquadState state) {
        switch (state) {
        case SquadState::scout:
            return "Scout";
        case SquadState::attack:
            return "Attack";
        case SquadState::defend:
            return "Defend";
        default:
            throw std::logic_error("Unknown SquadState!");
        }
    }

    Squad::Squad(KBot &kBot) : m_kBot(&kBot), m_state(SquadState::scout) {}

    void Squad::update() {
        if (!empty()) {
            // Draw squad radius
            Broodwar->drawCircleMap(getPosition(), 400, Colors::Red);
            Broodwar->drawTextMap(getPosition(), "Squad: %s", to_string(m_state).c_str());

            // Show membership
            for (const auto &unit : *this)
                Broodwar->drawLineMap(getPosition(), unit->getPosition(), Colors::Grey);

            // Draw path to enemy
            if (m_kBot->getEnemyPositionCount() > 0) {
                const auto enemyPosition = Position(m_kBot->getNextEnemyPosition());
                const auto path = m_kBot->map().GetPath(getPosition(), enemyPosition);
                if (!path.empty()) {
                    Broodwar->drawLineMap(getPosition(), Position(path.front()->Center()), Colors::Red);
                    for (std::size_t i = 1; i < path.size(); ++i)
                        Broodwar->drawLineMap(Position(path[i - 1]->Center()), Position(path[i]->Center()), Colors::Red);
                    Broodwar->drawLineMap(Position(path.back()->Center()), enemyPosition, Colors::Red);
                }
                else
                    Broodwar->drawLineMap(getPosition(), enemyPosition, Colors::Red);
            }
        }

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // TODO: remove
        const auto enemiesNearBase = Broodwar->getUnitsInRadius(Position(Broodwar->self()->getStartLocation()), 1000, Filter::IsEnemy);

        // Update squad state
        const auto oldState = m_state;
        switch (m_state) {
        case SquadState::scout:
            if (!enemiesNearBase.empty())
                m_state = SquadState::defend;
            else if (m_kBot->getEnemyPositionCount() > 0) {
                if (size() >= 20)
                    m_state = SquadState::attack;
                else
                    m_state = SquadState::defend;
            }
            break;
        case SquadState::attack:
            if (!enemiesNearBase.empty())
                m_state = SquadState::defend;
            else if (m_kBot->getEnemyPositionCount() == 0)
                m_state = SquadState::scout;
            else if (size() < 10)
                m_state = SquadState::defend;
            break;
        case SquadState::defend:
            if (enemiesNearBase.empty()) {
                if (m_kBot->getEnemyPositionCount() == 0)
                    m_state = SquadState::scout;
                if (size() >= 20)
                    m_state = SquadState::attack;
            }
            break;
        default:
            throw std::logic_error("Unknown SquadState!");
        }
        // Reassign orders on state change
        if (m_state != oldState)
            this->stop();

        // TODO: Move unit logic
        for (const auto &unit : *this) {
            assert(unit->exists());

            // Ignore the unit if it has one of the following status ailments
            if (unit->isLockedDown() || unit->isMaelstrommed() || unit->isStasised())
                continue;

            // Ignore the unit if it is in one of the following states
            if (unit->isLoaded() || !unit->isPowered() || unit->isStuck())
                continue;

            if (unit->getType() == UnitTypes::Terran_Marine) {
                switch (m_state) {
                case SquadState::scout:
                    if (unit->isIdle())
                        // Scout!
                        unit->attack(Position(m_kBot->getNextEnemyPosition()));
                    break;
                case SquadState::attack:
                    if (distance(unit->getPosition(), getPosition()) > 400 && !unit->isUnderAttack()) {
                        // Regroup!
                        // Prevent spamming, check if order is already set.
                        if (unit->getOrder() != Orders::AttackMove || distance(unit->getOrderTargetPosition(), getPosition()) >= 400) {
                            const Point<double> vector = unit->getPosition() - getPosition();
                            // Move further to the middle to prevent edge-sticky behavior.
                            auto position = getPosition() + vector * 300 / vector.getLength();
                            if (!Broodwar->isWalkable(WalkPosition(position)))
                                // Fallback: Go to the unit nearest to the middle.
                                position = getClosestUnit(Filter::GetType == UnitTypes::Terran_Marine && Filter::IsOwned)->getPosition();
                            unit->attack(position);
                            // debug
                            Broodwar->registerEvent([unit, position](Game*) {
                                Broodwar->drawLineMap(unit->getPosition(), position, Colors::Purple);
                            }, [unit](Game*) { return unit->exists(); }, Broodwar->getLatencyFrames());
                        }
                    }
                    else if (unit->isIdle())
                        // Attack!
                        unit->attack(Position(m_kBot->getNextEnemyPosition()));
                    break;
                case SquadState::defend:
                    if (distance(unit->getPosition(), Broodwar->self()->getStartLocation()) > 1000) {
                        // Retreat!
                        // Prevent spamming, check if order is already set.
                        if (unit->getOrder() != Orders::AttackMove || distance(unit->getOrderTargetPosition(), Broodwar->self()->getStartLocation()) > 1000)
                            unit->attack(Position(Broodwar->self()->getStartLocation()));
                    }
                    else if (unit->isIdle() && !enemiesNearBase.empty())
                        // Defend!
                        unit->attack(unit->getClosestUnit(Filter::IsEnemy)->getPosition());
                    break;
                default:
                    throw std::logic_error("Unknown SquadState!");
                }
            }
        }
    }

} // namespace
