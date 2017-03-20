#pragma once

#include <BWAPI.h>
#include "Base.h"

namespace KBot {

    class KBot;

    class Manager {
    public:
        Manager(KBot &kBot);

        // Is called every KBot::onFrame().
        void update();

        void createBase(const BWAPI::TilePosition &location);

        // Transfer ownership of a unit to manager.
        void transferOwnership(const BWAPI::Unit &unit);
        void onUnitDestroy(const BWAPI::Unit &unit);

    private:
        KBot &m_kBot;
        std::vector<Base> m_bases;
    };

} // namespace
