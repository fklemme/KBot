#include "General.h"

#include <BWAPI.h>

namespace KBot {

    using namespace BWAPI;

    General::General(KBot &parent) : m_kBot(parent) {}

    void General::update() {
        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;
    }

} // namespace
