// A collection of useful helper functions.

#pragma once

#include <BWAPI.h>
#include <BWEM/bwem.h>
#include <limits>

namespace KBot {

/// Returns distance between positions considering BWEM paths. Returns max. int if no path is
/// available.
template <typename PositionA, typename PositionB>
int distance(const PositionA &a, const PositionB &b, BWEM::Map &map = BWEM::Map::Instance()) {
    int length;
    map.GetPath(BWAPI::Position(a), BWAPI::Position(b), &length);
    return length != -1 ? length : std::numeric_limits<int>::max();
}

/// Checks if the given unit is ready to accept orders.
inline bool readyToAcceptOrders(const BWAPI::Unit &unit) {
    assert(unit->exists());

    // Ignore the unit if it has one of the following status ailments
    if (unit->isLockedDown() || unit->isMaelstrommed() || unit->isStasised())
        return false;

    // Ignore the unit if it is in one of the following states
    if (unit->isLoaded() || !unit->isPowered() || unit->isStuck())
        return false;

    return true;
}

/// Checks if the given vespene geyser or refinery is available to be mined from.
inline bool gasAvailable(const BWAPI::Unit &gas) {
    return gas->getType().isRefinery() && gas->getPlayer() == BWAPI::Broodwar->self() &&
           gas->isCompleted() && gas->getResources() > 0;
}

} // namespace
