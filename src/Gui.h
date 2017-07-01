#pragma once

#include "KBot.h"

namespace KBot {

class Gui {
public:
    Gui(const KBot &kbot);

    void update();

private:
    const KBot &m_kbot;
};

} // namespace KBot
