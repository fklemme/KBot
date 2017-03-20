#include "Base.h"

namespace KBot {

    using namespace BWAPI;

    Base::Base(KBot &kBot, const TilePosition &location) : m_kBot(kBot), m_location(location) {}

} // namespace
