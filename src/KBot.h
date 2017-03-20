#pragma once

#include <BWAPI.h>
#include <BWEM/bwem.h>

#include "Manager.h"
#include "General.h"

namespace KBot {

    class KBot : public BWAPI::AIModule {
    public:
        KBot();

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
        Manager &manager() { return m_manager; }
        General &general() { return m_general; }

        BWAPI::TilePosition getNextEnemyLocation();
        std::size_t getEnemyLocationCount() const { return m_enemyLocations.size(); }

    private:
        BWEM::Map &m_map;
        Manager m_manager;
        General m_general;

        std::vector<BWAPI::TilePosition> m_enemyLocations;
        std::vector<BWAPI::Unit> m_underConstruction;
    };

} // namespace
