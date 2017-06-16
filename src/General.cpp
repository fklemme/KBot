#include "General.h"

#include "KBot.h"
#include "utils.h"

namespace KBot {

    using namespace BWAPI;

    General::General(KBot &kBot) : m_kBot(kBot) {}

    void General::update() {
        std::string squadSizes;
        for (auto &squad : m_squads)
            squadSizes += ", " + std::to_string(squad.size());
        squadSizes = squadSizes.empty() ? "No squads" : squadSizes.substr(2);

        // TODO: Update to multiple base concept.
        const auto enemiesNearBase = Broodwar->getUnitsInRadius(Position(Broodwar->self()->getStartLocation()), 1000, Filter::IsEnemy);

        // Display debug information
        Broodwar->drawTextScreen(2, 100, "General: -");
        Broodwar->drawTextScreen(2, 110, "Squad sizes: %s", squadSizes.c_str());
        Broodwar->drawTextScreen(2, 120, "Enemies near base: %d", enemiesNearBase.size());

        // Update squads
        for (auto &squad : m_squads)
            squad.update();

        // ----- Prevent spamming -----------------------------------------------
        // Everything below is executed only occasionally and not on every frame.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // Remove empty squads
        decltype(m_squads)::const_iterator it;
        while ((it = std::find_if(m_squads.begin(), m_squads.end(), [](const Squad &squad) { return squad.empty(); })) != m_squads.end())
            m_squads.erase(it);

        // Merge nearby and defensive squads
        bool merge = true;
        while (merge) {
            for (auto it = m_squads.begin(); it != m_squads.end(); ++it) {
                auto hit = std::find_if(it + 1, m_squads.end(), [it](const Squad &squad) {
                    return (distance(it->getPosition(), squad.getPosition()) < 200)
                        || (distance(it->getPosition(), squad.getPosition()) < 500
                            && it->getState() == Squad::State::defend && squad.getState() == Squad::State::defend);
                });

                if (hit != m_squads.end()) {
                    // Hold all units as some of them might have old, invalid orders.
                    it->stop();
                    hit->stop();
                    // Merge squad *hit into *it.
                    std::copy(hit->begin(), hit->end(), std::inserter(*it, it->end()));
                    // Remove empty squad *hit.
                    m_squads.erase(hit);
                    // This invalidates all container iterators, so we have to start all over again!
                    break;
                }
            }
            // No more squads to merge.
            merge = false;
        }
    }

    void General::transferOwnership(const Unit &unit) {
        Broodwar->registerEvent([unit](Game*) {
            Broodwar->drawTextMap(Position(unit->getPosition()), "General: %s", unit->getType().c_str());
        }, [unit](Game*) { return unit->exists(); }, 250);

        for (auto &squad : m_squads) {
            if (distance(unit->getPosition(), squad.getPosition()) < 400) {
                // Join squad
                squad.insert(unit);
                return;
            }
        }

        // Create new squad
        Squad squad(m_kBot);
        squad.insert(unit);
        m_squads.push_back(squad);
    }

    void General::onUnitDestroy(const Unit &unit) {
        for (auto &squad : m_squads)
            squad.erase(unit);
    }

} // namespace
