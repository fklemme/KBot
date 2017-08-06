#include "Gui.h"

#include <BWAPI.h>
#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <string>
#include <thread>
#include <vector>

namespace KBot {
namespace Gui {

class Overview : public nana::panel<false> {
public:
    Overview(nana::window parent) : nana::panel<false>(parent) {
        m_place.div("<framecounter>");
        m_place["framecounter"] << m_frameCounterText << m_frameCounterValue;
    }

    void update(const KBot &kbot) {
        m_frameCounterValue.caption(std::to_string(BWAPI::Broodwar->getFrameCount()));
    }

private:
    nana::place m_place{*this};
    nana::label m_frameCounterText{*this, "Frame counter:"};
    nana::label m_frameCounterValue{*this, "0"};
};

class Manager : public nana::panel<false> {
    // Generic value compare functor for nana::listbox
    template <typename T, typename Compare>
    class ValueCompare {
    public:
        ValueCompare(Compare comp) : m_comp{comp} {}

        bool operator()(const std::string &, nana::any *lhsAnyPtr, const std::string &,
                        nana::any *rhsAnyPtr, bool reverse) {
            if (lhsAnyPtr && rhsAnyPtr) {
                auto lhs = nana::any_cast<T>(*lhsAnyPtr);
                auto rhs = nana::any_cast<T>(*rhsAnyPtr);
                return reverse ? m_comp(rhs, lhs) : m_comp(lhs, rhs);
            }
            return false;
        }

    private:
        Compare m_comp;
    };

    template <typename T, typename Compare>
    ValueCompare<T, Compare> makeValueCompare(Compare comp) {
        return ValueCompare<T, Compare>(comp);
    }

public:
    Manager(nana::window parent) : panel<false>(parent) {
        const auto taskCol = m_buildQueueListbox.append_header("Build task");
        const auto prioCol = m_buildQueueListbox.append_header("Priority", 50);
        const auto stateCol = m_buildQueueListbox.append_header("State");

        using BuildTaskPtr = const BuildTask *; // value type stored in category
        m_buildQueueListbox.set_sort_compare(
            prioCol, makeValueCompare<BuildTaskPtr>([](BuildTaskPtr lhs, BuildTaskPtr rhs) {
                return lhs->getPriority() < rhs->getPriority();
            }));
        m_buildQueueListbox.set_sort_compare(
            stateCol, makeValueCompare<BuildTaskPtr>([](BuildTaskPtr lhs, BuildTaskPtr rhs) {
                return lhs->getState() < rhs->getState();
            }));

        m_place.div("vertical <weight=20 buildQueue> <buildQueueListbox>");
        m_place["buildQueue"] << m_buildQueueText << m_buildQueueValue;
        m_place["buildQueueListbox"] << m_buildQueueListbox;
    }

    void update(const KBot &kbot) {
        const auto &buildQueue = kbot.manager().getBuildQueue();
        m_buildQueueValue.caption(std::to_string(buildQueue.size()));

        // Due to restrictions, we need a new model if queue size changes
        if (m_buildQueueListbox.at(0).size() != buildQueue.size()) {
            auto cellTranslator = [](const BuildTask &task) {
                std::vector<nana::listbox::cell> cells;
                cells.emplace_back(task.getTargetType().getName());
                cells.emplace_back(std::to_string((int) task.getPriority()));
                cells.emplace_back(task.toString(false)); // no build name prefix
                return cells;
            };

            // FIXME: This triggers ess_->lister.sort() before back-references can be set.
            m_buildQueueListbox.at(0).shared_model<std::mutex>(buildQueue, cellTranslator);
        }

        // Set back-references for compare functions
        // It might happen that a value in buildQueue was just replaced
        // so update references every time to prevent dangling pointers.
        auto category = m_buildQueueListbox.at(0); // default category
        for (std::size_t i = 0; i < buildQueue.size(); ++i)
            category.at(i).value(&buildQueue[i]);

        nana::API::refresh_window(m_buildQueueListbox); // redraw listbox
    }

private:
    nana::place   m_place{*this};
    nana::listbox m_buildQueueListbox{*this};
    nana::label   m_buildQueueText{*this, "Build tasks:"};
    nana::label   m_buildQueueValue{*this, "0"};
};

class MainForm : public nana::form {
public:
    MainForm(const KBot &kbot) : nana::form(nana::API::make_center(400, 300)) {
        caption("KBot Viewer");

        m_tabbar.append("Overview", m_overview);
        m_tabbar.append("Manager", m_manager);
        m_tabbar.activated(0); // activate the overview tab

        // Layout management
        // (http://qpcr4vir.github.io/nana-doxy/html/dc/de3/classnana_1_1place.html#details)
        auto &place = get_place();
        place.div("margin=5 vertical"
                  "<weight=25 tabbar>"
                  "<margin=[5,0,0,0] tabpages>"); // [top, right, bottom, left]
        place["tabbar"] << m_tabbar;
        place["tabpages"].fasten(m_overview);
        place["tabpages"].fasten(m_manager);
        place.collocate();
    }

    void update(const KBot &kbot) {
        m_overview.update(kbot);
        m_manager.update(kbot);
    }

private:
    nana::tabbar<std::string> m_tabbar{*this};
    Overview                  m_overview{*this};
    Manager                   m_manager{*this};
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
