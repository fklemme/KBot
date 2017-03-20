#pragma once

#include <BWAPI.h>

namespace KBot {

    class KBot;

    class Base {
    public:
        Base(KBot &kBot, const BWAPI::TilePosition &location);

    private:
        KBot &m_kBot;
        BWAPI::TilePosition m_location;
    };

} // namespace
