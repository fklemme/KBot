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

#include <iostream> // DEBUG

namespace KBot {
namespace Gui {

class updatable {
public:
    virtual void update(const KBot &kbot) = 0;
};

class Overview : public nana::panel<false>, public updatable {
public:
    Overview(nana::window parent) : nana::panel<false>(parent) {
        m_place.div("<framecounter>");
        m_place["framecounter"] << m_frameCounterText << m_frameCounterValue;
    }

    void update(const KBot &kbot) override {
        m_frameCounterValue.caption(std::to_string(BWAPI::Broodwar->getFrameCount()));
    }

private:
    nana::place m_place{*this};

    nana::label m_frameCounterText{*this, "Frame counter:"};
    nana::label m_frameCounterValue{*this, "0"};
};

class Manager : public nana::panel<false>, public updatable {
    // For C++17: Move to template lambda?
    template <typename Access>
    struct ValueCompare {
        ValueCompare(Access func) : access{func} {}

        bool operator()(const std::string &a, nana::any *lhs, const std::string &b, nana::any *rhs,
                        bool reverse) {
            if (lhs)
                std::cout << "lhs type: " << lhs->type().name() << std::endl;
            else
                std::cout << "lhs was nullptr" << std::endl;
            if (rhs)
                std::cout << "rhs type: " << lhs->type().name() << std::endl;
            else
                std::cout << "rhs was nullptr" << std::endl;
            // auto a = access(nana::any_cast<BuildTask>(*lhs));
            // auto b = access(nana::any_cast<BuildTask>(*rhs));
            return reverse ? a > b : a < b;
        }

        Access access;
    };

    template <typename Access>
    ValueCompare<Access> makeValueCompare(Access func) {
        return ValueCompare<Access>(func);
    }

public:
    Manager(nana::window parent) : nana::panel<false>(parent) {
        auto taskCol = m_buildQueueView.append_header("Build task");
        auto prioCol = m_buildQueueView.append_header("Priority", 50);
        auto stateCol = m_buildQueueView.append_header("State");

        // m_buildQueueView.set_sort_compare(
        //    prioCol, makeValueCompare([](const BuildTask &task) { return task.getPriority(); }));
        // m_buildQueueView.set_sort_compare(
        //    stateCol, makeValueCompare([](const BuildTask &task) { return task.getState(); }));

        m_place.div("vertical <weight=20 buildQueue> <buildQueueView>");
        m_place["buildQueue"] << m_buildQueueText << m_buildQueueValue;
        m_place["buildQueueView"] << m_buildQueueView;
    }

    void update(const KBot &kbot) override {
        const auto &buildQueue = kbot.manager().getBuildQueue();

        // Due to restrictions, we need a new model if queue size changes
        if (m_buildQueueSize != buildQueue.size()) {
            auto cellTranslator = [](const BuildTask &task) {
                std::vector<nana::listbox::cell> cells;
                cells.emplace_back(task.getTargetType().getName());
                cells.emplace_back(std::to_string((int) task.getPriority()));
                cells.emplace_back(task.toString(false)); // no build name prefix
                return cells;
            };

            m_buildQueueView.at(0).shared_model<std::mutex>(buildQueue, cellTranslator);

            m_buildQueueValue.caption(std::to_string(buildQueue.size()));
            m_buildQueueSize = buildQueue.size(); // remember last queue size
        }

        nana::API::refresh_window(m_buildQueueView); // redraw listbox
    }

private:
    nana::place m_place{*this};

    nana::listbox m_buildQueueView{*this};
    std::size_t   m_buildQueueSize = 0;
    nana::label   m_buildQueueText{*this, "Build tasks:"};
    nana::label   m_buildQueueValue{*this, "0"};
};

class MainForm : public nana::form, public updatable {
public:
    MainForm(const KBot &kbot) {
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

    Overview m_overview{*this};
    Manager  m_manager{*this};
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
