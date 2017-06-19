#pragma once

#include <BWAPI.h>
#include <BWEM/bwem.h>

#include "Manager.h"
#include "General.h"

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

        // Getter for map, manager and general.
        BWEM::Map &map() { return m_map; };
        const BWEM::Map &map() const { return m_map; };
        Manager &manager() { return m_manager; }
        const Manager &manager() const { return m_manager; }
        General &general() { return m_general; }
        const General &general() const { return m_general; }

        BWAPI::TilePosition getNextEnemyPosition() const;
        std::size_t getEnemyPositionCount() const { return m_enemyPositions.size(); }

    private:
        Manager    m_manager;
        General    m_general;
        BWEM::Map &m_map = BWEM::Map::Instance();

        std::vector<BWAPI::TilePosition> m_enemyPositions;
    };

} // namespace
