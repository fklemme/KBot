#pragma once

#include "KBot.h"
#include <memory>
#include <thread>

namespace KBot {
namespace Gui {

class MainForm;

class Gui {
public:
    Gui(const KBot &kbot);
    ~Gui();

    void update(const KBot &kbot);

private:
    const KBot &m_kbot;
    MainForm *  m_mainForm = nullptr; // FIXME: Protect with mutex!

    std::unique_ptr<std::thread> m_guiThread; // Owns MainForm instance
};

} // namespace Gui
} // namespace KBot
