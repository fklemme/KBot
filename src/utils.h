#pragma once

#include <BWAPI.h>
#include <BWEM/bwem.h>

namespace KBot {

    // Returns distance between positions considering BWEM paths. Returns max. int if no path is available.
    template <typename PositionTypeA, typename PositionTypeB>
    int distance(const PositionTypeA &a, const PositionTypeB &b, BWEM::Map &map = BWEM::Map::Instance()) {
        int length;
        map.GetPath(BWAPI::Position(a), BWAPI::Position(b), &length);
        return length != -1 ? length : std::numeric_limits<int>::max();
    }

} // namespace
