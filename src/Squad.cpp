#include "Squad.h"

#include "KBot.h"

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
        // Draw squad radius and path to enemy
        if (!empty()) {
            Broodwar->drawCircleMap(getPosition(), 400, Colors::Red);
            Broodwar->drawTextMap(getPosition(), "Squad: %s", to_string(m_state).c_str());

            if (m_kBot->getEnemyLocationCount() > 0) {
                const auto enemyLocation = m_kBot->getNextEnemyLocation();
                const auto path = m_kBot->map().GetPath(getPosition(), Position(enemyLocation));
                if (!path.empty()) {
                    // Draw path
                    Broodwar->drawLineMap(getPosition(), Position(path.front()->Center()), Colors::Red);
                    for (std::size_t i = 1; i < path.size(); ++i)
                        Broodwar->drawLineMap(Position(path[i - 1]->Center()), Position(path[i]->Center()), Colors::Red);
                    Broodwar->drawLineMap(Position(path.back()->Center()), Position(enemyLocation), Colors::Red);
                }
                else
                    Broodwar->drawLineMap(getPosition(), Position(enemyLocation), Colors::Red);
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
            if (m_kBot->getEnemyLocationCount() > 0) {
                if (!enemiesNearBase.empty() || size() < 10)
                    m_state = SquadState::attack;
                else
                    m_state = SquadState::defend;
            }
            break;
        case SquadState::attack:
            if (!enemiesNearBase.empty())
                m_state = SquadState::defend;
            else if (m_kBot->getEnemyLocationCount() == 0)
                m_state = SquadState::scout;
            else if (size() < 10)
                m_state = SquadState::defend;
            break;
        case SquadState::defend:
            if (enemiesNearBase.empty() && size() >= 20) {
                m_state = SquadState::attack;
            }
            break;
        default:
            throw std::logic_error("Unknown SquadState!");
        }
        if (m_state != oldState)
            this->stop(); // reassign orders

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
                        unit->attack(Position(m_kBot->getNextEnemyLocation()));
                    break;
                case SquadState::attack:
                    if (unit->getPosition().getApproxDistance(getPosition()) > 400 && !unit->isUnderAttack()) {
                        // Regroup!
                        const Point<double> vector = unit->getPosition() - getPosition();
                        // Move further to the middle to prevent sticky behavior.
                        auto position = getPosition() + vector * 300 / vector.getLength();
                        if (!Broodwar->isWalkable(WalkPosition(position)))
                            position = Position(m_kBot->getNextEnemyLocation());
                        unit->attack(position); // FIXME: still pretty spammy
                        // debug
                        Broodwar->registerEvent([unit, position](Game*) {
                            Broodwar->drawLineMap(unit->getPosition(), position, Colors::Purple);
                        }, [unit](Game*) { return unit->exists(); }, Broodwar->getLatencyFrames());
                    }
                    else if (unit->isIdle())
                        // Attack!
                        unit->attack(Position(m_kBot->getNextEnemyLocation()));
                    break;
                case SquadState::defend:
                    if (unit->getPosition().getApproxDistance(Position(Broodwar->self()->getStartLocation())) > 1000)
                        // Retreat!
                        unit->attack(Position(Broodwar->self()->getStartLocation()));
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
