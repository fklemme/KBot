#include "Gui.h"

#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <thread>

namespace KBot {

Gui::Gui(const KBot &kbot) : m_kbot(kbot) {
    // Open gui in a new thread.
    m_guiThread = std::make_unique<std::thread>([this]() {
        // All gui elements have to be created by the thread that runs nana::exec().
        nana::form form;    // main form
        m_mainForm = &form; // store reference in Gui

        // Define a label and display a text.
        nana::label frameCounterText{form, "Frame counter:"};
        nana::label frameCounterValue(form, "123");

        // Define a button and answer the click event.
        nana::button closeButton{form, "Close"};
        closeButton.events().click([&form] { form.close(); });

        // Layout management
        // (http://qpcr4vir.github.io/nana-doxy/html/dc/de3/classnana_1_1place.html#details)
        form.div("margin=10 vertical <frameCounter> <weight=50 closeButton>");
        form["frameCounter"] << frameCounterText << frameCounterValue;
        form["closeButton"] << closeButton;
        form.collocate();

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

void Gui::update() {
    // TODO: magic!
}

} // namespace KBot
