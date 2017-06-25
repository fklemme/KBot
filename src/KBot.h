#pragma once

#include <BWAPI.h>
#include <BWEM/bwem.h>

#include "Manager.h"
#include "General.h"
#include "Enemy.h"

namespace KBot {

    class KBot : public BWAPI::AIModule {
    public:
        KBot();

        // Prohibit copy & move.
        KBot(const KBot&) = delete;
        KBot(KBot&&) = delete;
        KBot &operator=(const KBot&) = delete;
        KBot &operator=(KBot&&) = delete;

        // Implement BWAPI interface.
        virtual void onStart() override;
        virtual void onEnd(bool isWinner) override;
        virtual void onFrame() override;
        virtual void onSendText(std::string text) override;
        virtual void onReceiveText(BWAPI::Player player, std::string text) override;
        virtual void onPlayerLeft(BWAPI::Player player) override;
        virtual void onNukeDetect(BWAPI::Position target) override;
        virtual void onUnitDiscover(BWAPI::Unit unit) override;
        virtual void onUnitEvade(BWAPI::Unit unit) override;
        virtual void onUnitShow(BWAPI::Unit unit) override;
        virtual void onUnitHide(BWAPI::Unit unit) override;
        virtual void onUnitCreate(BWAPI::Unit unit) override;
        virtual void onUnitDestroy(BWAPI::Unit unit) override;
        virtual void onUnitMorph(BWAPI::Unit unit) override;
        virtual void onUnitRenegade(BWAPI::Unit unit) override;
        virtual void onSaveGame(std::string gameName) override;
        virtual void onUnitComplete(BWAPI::Unit unit) override;

        // Getter for members.
        Manager &manager() { return m_manager; }
        const Manager &manager() const { return m_manager; }

        General &general() { return m_general; }
        const General &general() const { return m_general; }

        Enemy &enemy() { return m_enemy; }
        const Enemy &enemy() const { return m_enemy; }

        BWEM::Map &map() { return m_map; };
        const BWEM::Map &map() const { return m_map; };

    private:
        Manager    m_manager;
        General    m_general;
        Enemy      m_enemy;
        BWEM::Map &m_map = BWEM::Map::Instance();
    };

} // namespace
