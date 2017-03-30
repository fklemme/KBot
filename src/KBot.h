#pragma once

#include <BWAPI.h>
#include <BWEM/bwem.h>

#include "Manager.h"
#include "General.h"

namespace KBot {

    class KBot : public BWAPI::AIModule {
    public:
        KBot();

        // Forbit copy & move
        KBot(const KBot&) = delete;
        KBot(KBot&&) = delete;
        KBot &operator=(const KBot&) = delete;
        KBot &operator=(KBot&&) = delete;

        virtual void onStart();
        virtual void onEnd(bool isWinner);
        virtual void onFrame();
        virtual void onSendText(std::string text);
        virtual void onReceiveText(BWAPI::Player player, std::string text);
        virtual void onPlayerLeft(BWAPI::Player player);
        virtual void onNukeDetect(BWAPI::Position target);
        virtual void onUnitDiscover(BWAPI::Unit unit);
        virtual void onUnitEvade(BWAPI::Unit unit);
        virtual void onUnitShow(BWAPI::Unit unit);
        virtual void onUnitHide(BWAPI::Unit unit);
        virtual void onUnitCreate(BWAPI::Unit unit);
        virtual void onUnitDestroy(BWAPI::Unit unit);
        virtual void onUnitMorph(BWAPI::Unit unit);
        virtual void onUnitRenegade(BWAPI::Unit unit);
        virtual void onSaveGame(std::string gameName);
        virtual void onUnitComplete(BWAPI::Unit unit);

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
