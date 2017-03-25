#pragma once

#include <BWAPI.h>
#include "Base.h"
#include "BuildTask.h"

namespace KBot {

    class KBot;

    class Manager {
    public:
        Manager(KBot &kBot);

        // Forbit copy & move
        Manager(const Manager&) = delete;
        Manager(Manager&&) = delete;
        Manager &operator=(const Manager&) = delete;
        Manager &operator=(Manager&&) = delete;

        // Is called every KBot::onFrame().
        void update();

        // Transfer ownership of a unit to manager.
        void transferOwnership(const BWAPI::Unit &unit);
        void onUnitDestroy(const BWAPI::Unit &unit);

        void addBuildTask(const BuildTask &buildTask);
        void onBuildTaskCreated(const BWAPI::Unit &unit);
        void onBuildTaskDestroyed(const BWAPI::Unit &unit);
        void onBuildTaskCompleted(const BWAPI::Unit &unit);

        int getAvailableMinerals() const { return BWAPI::Broodwar->self()->minerals() - m_reservedMinerals; }
        int getAvailableGas() const { return BWAPI::Broodwar->self()->gas() - m_reservedGas; }
        void aquireResources(const int minerals, const int gas);
        void releaseResources(const int minerals, const int gas);

    private:
        KBot &m_kBot;
        std::vector<Base> m_bases;
        std::vector<BuildTask> m_buildQueue;
        int m_reservedMinerals = 0;
        int m_reservedGas = 0;
    };

} // namespace
