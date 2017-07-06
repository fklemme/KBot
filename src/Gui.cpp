#include "Gui.h"

#include <BWAPI.h>
#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <thread>

namespace KBot {

class MainForm : public nana::form {
public:
    MainForm()
        : m_frameCounterText{*this, "Frame counter:"}, m_frameCounterValue{*this, "-"},
          m_closeButton{*this, "Close"} {
        m_closeButton.events().click([this] { close(); });

        // Layout management
        // (http://qpcr4vir.github.io/nana-doxy/html/dc/de3/classnana_1_1place.html#details)
        auto &place = get_place();
        place.div("margin=10 vertical <frameCounter> <weight=25 closeButton>");
        place["frameCounter"] << m_frameCounterText << m_frameCounterValue;
        place["closeButton"] << m_closeButton;
        place.collocate();
    }

    void update(const KBot &kbot) {
        m_frameCounterValue.caption(std::to_string(BWAPI::Broodwar->getFrameCount()));
    }

private:
    nana::label  m_frameCounterText;
    nana::label  m_frameCounterValue;
    nana::button m_closeButton;
};

Gui::Gui(const KBot &kbot) : m_kbot(kbot) {
    // Open gui in a new thread.
    m_guiThread = std::make_unique<std::thread>([this]() {
        // All gui elements have to be created by the thread that runs nana::exec().
        MainForm form;
        m_mainForm = &form; // store reference in Gui

        // Show the form and block until it's closed.
        form.show();
        nana::exec();

        // Main form is about to be destroyed. Clear reference.
        m_mainForm = nullptr;
    });
}

Gui::~Gui() {
    if (m_mainForm)
        m_mainForm->close();
    if (m_guiThread)
        m_guiThread->join();
}

void Gui::update(const KBot &kbot) {
    if (m_mainForm)
        m_mainForm->update(kbot);
}

} // namespace KBot
