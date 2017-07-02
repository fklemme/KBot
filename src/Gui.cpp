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
        nana::label lab{form, "Hello, <bold blue size=16>Nana C++ Library</>"};
        lab.format(true);

        // Define a button and answer the click event.
        nana::button btn{form, "Quit"};
        btn.events().click([&form] { form.close(); });

        // Layout management
        form.div("vert <><<><weight=80% text><>><><weight=24<><button><>><>");
        form["text"] << lab;
        form["button"] << btn;
        form.collocate();

        // Show the form
        form.show();
        std::cout << "Gui opened!" << std::endl; // DEBUG

        // Start to event loop process, it blocks until the form is closed.
        nana::exec();
        std::cout << "Gui closed!" << std::endl; // DEBUG

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
