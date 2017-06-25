#pragma once

#include <algorithm>
#include <BWAPI.h>
#include <numeric>

namespace KBot {

    class Manager;

    class Base {
        static const auto catchmentArea = 400;
        static const auto mineralWorkerRatio = 2;
        static const auto gasWorkerRatio = 3;

    public:
        Base(Manager &manager, const BWAPI::TilePosition &position);

        // Called every KBot::onFrame().
        void update();

        const BWAPI::TilePosition &getPosition() const { return m_position; }

        // Transfer ownership of a unit to base.
        void giveOwnership(const BWAPI::Unit &unit);
        void takeOwnership(const BWAPI::Unit &unit);

    private:
        // Returns the total amount of workers this base should have to mine minerals.
        int targetMineralWorkers() const;

        // Returns the total amount of workers this base should have to gather vespine gas.
        int targetGasWorkers() const;

        // Returns the amount of workers this base should add to be saturated. TODO: Move this function? DEBUG only?
        int workersLeftToBuild() const;

        Manager            *m_manager;
        BWAPI::TilePosition m_position;
        BWAPI::Unitset      m_units; // all units associated with this base

        // Units around this base
        BWAPI::Unitset m_mineralPatches;
        BWAPI::Unitset m_mineralWorkers;
        std::map<BWAPI::Unit, BWAPI::Unitset> m_gasWorkers; // refineries and assigned workers
    };

} // namespace
