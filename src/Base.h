#pragma once

#include <BWAPI.h>
#include <vector>

namespace KBot {

class Manager;

class Base {
    static const auto catchmentRadius = 400;
    static const auto mineralWorkerRatio = 2;
    static const auto gasWorkerRatio = 3;

public:
    Base(Manager &manager, BWAPI::TilePosition position);

    // Called every KBot::onFrame().
    void update();

    const BWAPI::TilePosition &getPosition() const { return m_position; }

    // Transfer ownership of a unit to base.
    void giveOwnership(const BWAPI::Unit &unit);

    // Take ownership of a unit from base (forcibly).
    void takeOwnership(const BWAPI::Unit &unit);

    // Returns the closest matching unit in this base or nullptr, if none has been found. This
    // method does not transfer the unit's ownership so takeOwnership() has to be called explicitly.
    BWAPI::Unit findWorker(const BWAPI::UnitType &workerType,
                           const BWAPI::Position &nearPosition) const;

private:
    // Returns the total amount of workers this base should have to mine minerals.
    int targetMineralWorkers() const;

    // Returns the total amount of workers this base should have to gather vespine gas.
    int targetGasWorkers() const;

    // Returns the amount of workers this base should add to be saturated. TODO: Move this function?
    // DEBUG only?
    int workersLeftToBuild() const;

    Manager *           m_manager;
    BWAPI::TilePosition m_position;

    // Units owned by this base.
    BWAPI::Unitset m_mineralPatches;
    BWAPI::Unitset m_mineralWorkers;
    std::vector<std::pair<BWAPI::Unit, BWAPI::Unitset>>
                   m_gasesAndWorkers; // refineries and assigned workers
    BWAPI::Unitset m_otherUnits;      // buildings and unassigned workers
};

} // namespace
