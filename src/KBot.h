/** \mainpage
 *
 * Here will be some text. Maybe all the stuff from Readme.
 */

#pragma once

#include <BWAPI.h>
#include <BWEM/bwem.h>

#include "Enemy.h"
#include "General.h"
#include "Manager.h"

namespace KBot {

/// Implements the BWAPI interface.
class KBot : public BWAPI::AIModule {
    // Prohibit copy & move.
    KBot(const KBot &) = delete;
    KBot(KBot &&) = delete;
    KBot &operator=(const KBot &) = delete;
    KBot &operator=(KBot &&) = delete;

public:
    KBot();

    // Implement BWAPI interface.
    void onStart() override;
    void onEnd(bool isWinner) override;
    void onFrame() override;
    void onSendText(std::string text) override;
    void onReceiveText(BWAPI::Player player, std::string text) override;
    void onPlayerLeft(BWAPI::Player player) override;
    void onNukeDetect(BWAPI::Position target) override;
    void onUnitDiscover(BWAPI::Unit unit) override;
    void onUnitEvade(BWAPI::Unit unit) override;
    void onUnitShow(BWAPI::Unit unit) override;
    void onUnitHide(BWAPI::Unit unit) override;
    void onUnitCreate(BWAPI::Unit unit) override;
    void onUnitDestroy(BWAPI::Unit unit) override;
    void onUnitMorph(BWAPI::Unit unit) override;
    void onUnitRenegade(BWAPI::Unit unit) override;
    void onSaveGame(std::string gameName) override;
    void onUnitComplete(BWAPI::Unit unit) override;

    // Getter for members.
    Manager &        manager() { return m_manager; }
    const Manager &  manager() const { return m_manager; }
    General &        general() { return m_general; }
    const General &  general() const { return m_general; }
    Enemy &          enemy() { return m_enemy; }
    const Enemy &    enemy() const { return m_enemy; }
    BWEM::Map &      map() { return m_map; };
    const BWEM::Map &map() const { return m_map; };

private:
    Manager    m_manager;
    General    m_general;
    Enemy      m_enemy;
    BWEM::Map &m_map = BWEM::Map::Instance();
};

} // namespace KBot
