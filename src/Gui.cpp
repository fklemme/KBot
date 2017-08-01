#include "Gui.h"

#include <BWAPI.h>
#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace KBot {
namespace Gui {

class updatable {
public:
    virtual void update(const KBot &kbot) = 0;
};

class Overview : public nana::panel<false>, public updatable {
public:
    Overview(nana::window parent, const KBot &kbot) : nana::panel<false>(parent) {
        m_place.div("<framecounter>");
        m_place["framecounter"] << m_frameCounterText << m_frameCounterValue;
    }

    void update(const KBot &kbot) override {
        m_frameCounterValue.caption(std::to_string(BWAPI::Broodwar->getFrameCount()));
    }

private:
    nana::place m_place{*this};

    nana::label m_frameCounterText{*this, "Frame counter:"};
    nana::label m_frameCounterValue{*this, "-"};
};

class Manager : public nana::panel<false>, public updatable {
public:
    Manager(nana::window parent, const KBot &kbot) : nana::panel<false>(parent) {
        m_buildQueueView.append_header("Build task");
        m_buildQueueView.append_header("Priority");
        m_buildQueueView.append_header("State");

        auto cellTranslator = [](const BuildTask &task) {
            std::vector<nana::listbox::cell> cells;
            cells.emplace_back(task.getTargetType().getName());
            cells.emplace_back(std::to_string((int) task.getPriority()));
            cells.emplace_back(task.toString(false)); // no build name prefix
            return cells;
        };

        // TODO: Do we really need a *recursive* mutex here?
        m_buildQueueView.at(0).shared_model<std::recursive_mutex>(kbot.manager().getBuildQueue(),
                                                                  cellTranslator);

        m_place.div("vertical <weight=20 buildQueue> <buildQueueView>");
        m_place["buildQueue"] << m_buildQueueText << m_buildQueueValue;
        m_place["buildQueueView"] << m_buildQueueView;
    }

    void update(const KBot &kbot) override {
        // m_buildQueueValue.caption(std::to_string(kbot.manager().getBuildQueue().size()));

        auto modelGuard = m_buildQueueView.at(0).model(); // acquire lock
        nana::API::refresh_window(m_buildQueueView);

        using ContainerType = std::decay<decltype(kbot.manager().getBuildQueue())>::type;
        const auto &container =
            modelGuard.container<ContainerType>(); // FIXME: throws std::runtime_error!
        m_buildQueueValue.caption(std::to_string(container.size()));
    }

private:
    nana::place m_place{*this};

    nana::listbox m_buildQueueView{*this};

    nana::label m_buildQueueText{*this, "Build tasks:"};
    nana::label m_buildQueueValue{*this, "-"};
};

class MainForm : public nana::form, public updatable {
public:
    MainForm(const KBot &kbot) : m_overview{*this, kbot}, m_manager{*this, kbot} {
        m_tabbar.append("Overview", m_overview);
        m_tabbar.append("Manager", m_manager);
        m_tabbar.activated(0);

        // Layout management
        // (http://qpcr4vir.github.io/nana-doxy/html/dc/de3/classnana_1_1place.html#details)
        auto &place = get_place();
        place.div("margin=5 vertical"
                  "<weight=25 tabbar>"
                  "<margin=[5,0] tabpages>"); // TODO: now double margin at the bottom!
        place["tabbar"] << m_tabbar;
        place["tabpages"].fasten(m_overview);
        place["tabpages"].fasten(m_manager);
        place.collocate();
    }

    void update(const KBot &kbot) override {
        m_overview.update(kbot);
        m_manager.update(kbot);
    }

private:
    nana::tabbar<std::string> m_tabbar{*this};

    Overview m_overview;
    Manager  m_manager;
};

Gui::Gui(const KBot &kbot) : m_kbot(kbot) {
    // Open gui in a new thread.
    m_guiThread = std::make_unique<std::thread>([this]() {
        // All gui elements have to be created by the thread that runs nana::exec().
        MainForm form(m_kbot);
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

} // namespace Gui
} // namespace KBot
