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
namespace {

struct labelPair {
    nana::label text;
    nana::label value;
};

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

class Overview : public nana::panel<false> {
public:
    Overview(nana::window parent) : nana::panel<false>(parent) {
        m_place.div("vertical"
                    "<weight=20 frameCounter>"
                    "<weight=20 framesPerSecond>"
                    "<weight=20 actionsPerMinute>");

        m_place["frameCounter"] << m_frameCounter.text << m_frameCounter.value;
        m_place["framesPerSecond"] << m_framesPerSecond.text << m_framesPerSecond.value;
        m_place["actionsPerMinute"] << m_actionsPerMinute.text << m_actionsPerMinute.value;
    }

    void update(const KBot &kbot) {
        m_frameCounter.value.caption(std::to_string(BWAPI::Broodwar->getFrameCount()));
        m_framesPerSecond.value.caption(std::to_string(BWAPI::Broodwar->getFPS()) + " (Avg: " +
                                        std::to_string(BWAPI::Broodwar->getAverageFPS()) + ")");
        m_actionsPerMinute.value.caption(std::to_string(BWAPI::Broodwar->getAPM()));
    }

private:
    nana::place m_place{*this};
    labelPair   m_frameCounter{{*this, "Frame counter:"}, {*this, "0"}};
    labelPair   m_framesPerSecond{{*this, "Frames per second:"}, {*this, "0"}};
    labelPair   m_actionsPerMinute{{*this, "Actions per minute:"}, {*this, "0"}};
};

class Manager : public nana::panel<false> {
public:
    Manager(nana::window parent) : panel<false>(parent) {
        const auto taskCol = m_buildQueue.append_header("Build task", 150);
        const auto prioCol = m_buildQueue.append_header("Priority", 80);
        const auto stateCol = m_buildQueue.append_header("State", 150);

        using BuildTaskPtr = const BuildTask *; // value type stored in category
        m_buildQueue.set_sort_compare(
            prioCol, makeValueCompare<BuildTaskPtr>([](BuildTaskPtr lhs, BuildTaskPtr rhs) {
                return lhs->getPriority() < rhs->getPriority();
            }));
        m_buildQueue.set_sort_compare(
            stateCol, makeValueCompare<BuildTaskPtr>([](BuildTaskPtr lhs, BuildTaskPtr rhs) {
                return lhs->getState() < rhs->getState();
            }));

        m_place.div("vertical"
                    "<weight=20 minerals>"
                    "<weight=20 gas>"
                    "<buildQueue>");

        m_place["minerals"] << m_minerals.text << m_minerals.value;
        m_place["gas"] << m_gas.text << m_gas.value;
        m_place["buildQueue"] << m_buildQueue;
    }

    void update(const KBot &kbot) {
        m_minerals.value.caption(std::to_string(BWAPI::Broodwar->self()->minerals()) + " (" +
                                 std::to_string(kbot.manager().getAvailableMinerals()) + " / " +
                                 std::to_string(kbot.manager().getReservedMinerals()) + ")");
        m_gas.value.caption(std::to_string(BWAPI::Broodwar->self()->gas()) + " (" +
                            std::to_string(kbot.manager().getAvailableGas()) + " / " +
                            std::to_string(kbot.manager().getReservedGas()) + ")");

        // Due to restrictions, we need a new model if queue size changes
        const auto &buildQueue = kbot.manager().getBuildQueue();
        if (m_buildQueue.at(0).size() != buildQueue.size()) {
            auto cellTranslator = [](const BuildTask &task) {
                std::vector<nana::listbox::cell> cells;
                cells.emplace_back(task.getTargetType().getName());
                cells.emplace_back(std::to_string((int) task.getPriority()));
                cells.emplace_back(task.toString(false)); // no build name prefix
                return cells;
            };

            // FIXME: This triggers ess_->lister.sort() before back-references can be set.
            m_buildQueue.at(0).shared_model<std::mutex>(buildQueue, cellTranslator);
        }

        // Set back-references for compare functions
        // It might happen that a value in buildQueue was just replaced
        // so update references every time to prevent dangling pointers.
        auto category = m_buildQueue.at(0); // default category
        for (std::size_t i = 0; i < buildQueue.size(); ++i)
            category.at(i).value(&buildQueue[i]);

        nana::API::refresh_window(m_buildQueue); // redraw listbox
    }

private:
    nana::place   m_place{*this};
    nana::listbox m_buildQueue{*this};
    labelPair     m_minerals{{*this, "Minerals (available / reserved):"}, {*this, "0"}};
    labelPair     m_gas{{*this, "Gas (available / reserved):"}, {*this, "0"}};
};

class General : public nana::panel<false> {
public:
    General(nana::window parent) : nana::panel<false>(parent) {
        m_squads.append_header("Squad size");
        m_squads.append_header("Position");
        m_squads.append_header("State");

        m_place.div("<squads>");
        m_place["squads"] << m_squads;
    }

    void update(const KBot &kbot) {
        // Due to restrictions, we need a new model if queue size changes
        const auto &squads = kbot.general().getSquads();
        if (m_squads.at(0).size() != squads.size()) {
            auto cellTranslator = [](const Squad &squad) {
                std::vector<nana::listbox::cell> cells;
                cells.emplace_back(std::to_string(squad.size()));
                const auto pos = squad.getPosition();
                cells.emplace_back(std::to_string(pos.x) + ", " + std::to_string(pos.y));
                cells.emplace_back(to_string(squad.getState()));
                return cells;
            };

            m_squads.at(0).shared_model<std::mutex>(squads, cellTranslator);
        }

        nana::API::refresh_window(m_squads); // redraw listbox
    }

private:
    nana::place   m_place{*this};
    nana::listbox m_squads{*this};
};

class Enemy : public nana::panel<false> {
public:
    Enemy(nana::window parent) : nana::panel<false>(parent) {
        // TODO
    }

    void update(const KBot &kbot) {}

private:
    nana::place m_place{*this};
};

} // namespace

class MainForm : public nana::form {
public:
    MainForm(const KBot &kbot) : nana::form(nana::API::make_center(450, 300)) {
        caption("KBot Viewer");

        m_tabbar.append("Overview", m_overview);
        m_tabbar.append("Manager", m_manager);
        m_tabbar.append("General", m_general);
        m_tabbar.append("Enemy", m_enemy);
        m_tabbar.activated(0); // activate the overview tab

        // Layout management
        // https://github.com/qPCR4vir/nana-docs/wiki/Using-place-for-layouts
        auto &place = get_place();
        place.div("margin=5 vertical"
                  "<weight=25 tabbar>"
                  "<margin=[5] tabpages>");
        place["tabbar"] << m_tabbar;
        place["tabpages"].fasten(m_overview);
        place["tabpages"].fasten(m_manager);
        place["tabpages"].fasten(m_general);
        place["tabpages"].fasten(m_enemy);
        place.collocate();
    }

    void update(const KBot &kbot) {
        m_overview.update(kbot);
        m_manager.update(kbot);
        m_general.update(kbot);
        m_enemy.update(kbot);
    }

private:
    nana::tabbar<std::string> m_tabbar{*this};

    Overview m_overview{*this};
    Manager  m_manager{*this};
    General  m_general{*this};
    Enemy    m_enemy{*this};
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
