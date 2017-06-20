#pragma once

#include <algorithm>
#include <BWAPI.h>
#include <numeric>

namespace KBot {

    class Manager;

    class Base {
        static const auto mineralWorkerRatio = 2;
        static const auto gasWorkerRatio = 3;

    public:
        Base(Manager &manager, const BWAPI::TilePosition &position);

        // Called every KBot::onFrame().
        void update();

        const BWAPI::TilePosition &getPosition() const { return m_position; }

        // Transfer ownership of a unit to base.
        void transferOwnership(const BWAPI::Unit &unit);
        void onUnitDestroy(const BWAPI::Unit &unit);

    private:
        Manager            *m_manager;
        BWAPI::TilePosition m_position;
        BWAPI::Unitset      m_units; // all units associated with this base

        // Units around this base
        BWAPI::Unitset m_mineralPatches, m_vespeneGeysers; // (unbuilt)
        BWAPI::Unitset m_mineralWorkers;
        std::map<BWAPI::Unit, BWAPI::Unitset> m_gasWorkers; // refineries and corresponding workers

        // Returns the total amount of workers this base should have to mine minerals.
        int targetMineralWorkers() const {
            return (int) std::ceil(mineralWorkerRatio * m_mineralPatches.size());
        }

        // Returns the total amount of workers this base should have to gather vespine gas.
        int targetGasWorkers() const {
            auto avaiable = [](auto refinery) { return refinery->isCompleted() && refinery->getResources() > 0; };
            const auto refineries = std::count_if(m_gasWorkers.begin(), m_gasWorkers.end(),
                [&](const auto &refAndWorkers) { return avaiable(refAndWorkers.first); });
            return (int) std::ceil(gasWorkerRatio * refineries);
        }

        // TODO: Move this function?
        int workersLeftToBuild() const {
            const auto gasWorkers = std::accumulate(m_gasWorkers.begin(), m_gasWorkers.end(), 0,
                [](int sum, const auto &p) { return sum + p.second.size(); });
            return targetMineralWorkers() - m_mineralWorkers.size() + targetGasWorkers() - gasWorkers;
        }
    };

} // namespace
