#include "General.h"

#include "KBot.h"

namespace KBot {

    using namespace BWAPI;

    static std::string to_string(SquadState state) {
        switch (state) {
        case SquadState::scout:
            return "Scouting";
        case SquadState::attack:
            return "Attacking";
        case SquadState::defend:
            return "Defending";
        default:
            throw std::logic_error("Unknown SquadState!");
        }
    }

    General::General(KBot &parent) : m_kBot(parent), m_squadState(SquadState::scout) {}

    void General::update() {
        // Draw squad radius and path to enemy
        if (!m_squad.empty()) {
            Broodwar->drawCircleMap(m_squad.getPosition(), 400, Colors::Red);
            if (m_kBot.getEnemyLocationCount() > 0) {
                const auto enemyLocation = m_kBot.getNextEnemyLocation();
                const auto path = m_kBot.map().GetPath(m_squad.getPosition(), Position(enemyLocation));
                if (!path.empty()) {
                    // Draw path
                    Broodwar->drawLineMap(m_squad.getPosition(), Position(path.front()->Center()), Colors::Red);
                    for (std::size_t i = 1; i < path.size(); ++i)
                        Broodwar->drawLineMap(Position(path[i - 1]->Center()), Position(path[i]->Center()), Colors::Red);
                    Broodwar->drawLineMap(Position(path.back()->Center()), Position(enemyLocation), Colors::Red);
                }
                else
                    Broodwar->drawLineMap(m_squad.getPosition(), Position(enemyLocation), Colors::Red);
            }
        }

        // Draw individual radius
        //for (const auto unit : m_squad)
        //    Broodwar->drawCircleMap(unit->getPosition(), 200, Colors::Yellow);

        const auto enemiesNearBase = Broodwar->getUnitsInRadius(Position(Broodwar->self()->getStartLocation()), 1000, Filter::IsEnemy);

        // Display debug information
        Broodwar->drawTextScreen(2, 100, "General: %s", to_string(m_squadState).c_str());
        Broodwar->drawTextScreen(2, 110, "Squad size: %d", m_squad.size());
        Broodwar->drawTextScreen(2, 120, "Enemies near base: %d", enemiesNearBase.size());

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // Update squad state
        switch (m_squadState) {
        case SquadState::scout:
            if (m_kBot.getEnemyLocationCount() > 0)
                m_squadState = SquadState::defend;
            break;
        case SquadState::attack:
            if (m_kBot.getEnemyLocationCount() == 0)
                m_squadState = SquadState::scout;
            else if (!enemiesNearBase.empty() || m_squad.size() < 10)
                m_squadState = SquadState::defend;
            break;
        case SquadState::defend:
            if (enemiesNearBase.empty() && m_squad.size() >= 20) {
                m_squadState = SquadState::attack;
            }
            break;
        default:
            throw std::logic_error("Unknown SquadState!");
        }

        // Remove dead units
        for (auto it = m_squad.begin(); it != m_squad.end();) {
            if (!(*it)->exists())
                it = m_squad.erase(it);
            else ++it;
        }

        for (const auto unit : m_squad) {
            // Ignore the unit if it has one of the following status ailments
            if (unit->isLockedDown() || unit->isMaelstrommed() || unit->isStasised())
                continue;

            // Ignore the unit if it is in one of the following states
            if (unit->isLoaded() || !unit->isPowered() || unit->isStuck())
                continue;

            if (unit->getType() == UnitTypes::Terran_Marine) {
                switch (m_squadState) {
                case SquadState::scout:
                    if (unit->isIdle())
                        // Scout!
                        unit->attack(Position(m_kBot.getNextEnemyLocation()));
                    break;
                case SquadState::attack:
                    if (unit->getDistance(m_squad.getPosition()) > 400) {
                        // Regroup!
                        const Point<double> vector = unit->getPosition() - m_squad.getPosition();
                        const auto position = m_squad.getPosition() + vector * 400 / vector.getLength();
                        unit->attack(position);
                        // debug
                        Broodwar->registerEvent([unit, position](Game*) {
                            Broodwar->drawLineMap(unit->getPosition(), position, Colors::Purple);
                            Broodwar->drawCircleMap(position, 20, Colors::Grey);
                        }, [unit](Game*) { return unit->exists(); }, Broodwar->getLatencyFrames());
                    }
                    else if (unit->isIdle())
                        // Attack!
                        unit->attack(Position(m_kBot.getNextEnemyLocation()));
                    break;
                case SquadState::defend:
                    if (unit->getDistance(Position(Broodwar->self()->getStartLocation())) > 1000)
                        // Retreat!
                        unit->attack(Position(Broodwar->self()->getStartLocation()));
                    else if (unit->isIdle() && !enemiesNearBase.empty())
                        // Defend!
                        unit->attack(unit->getClosestUnit(Filter::IsEnemy));
                    break;
                default:
                    throw std::logic_error("Unknown SquadState!");
                }
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
