#pragma once

#include "KBot.h"
#include <memory>
#include <nana/gui.hpp>
#include <thread>

namespace KBot {

class Gui {
public:
    Gui(const KBot &kbot);
    ~Gui();

    void update();

private:
    const KBot &                 m_kbot;
    nana::form *                 m_mainForm = nullptr; // FIXME: Protect with mutex!
    std::unique_ptr<std::thread> m_guiThread;
};

} // namespace KBot
