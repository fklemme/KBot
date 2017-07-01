#include "KBot.h"
#include <BWAPI.h>
#include <BWAPI/Client.h>
#include <chrono>
#include <iostream>
#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <thread>

void nanaHelloWorld() {
    using namespace nana;

    // Define a form.
    form fm;

    // Define a label and display a text.
    label lab{fm, "Hello, <bold blue size=16>Nana C++ Library</>"};
    lab.format(true);

    // Define a button and answer the click event.
    button btn{fm, "Quit"};
    btn.events().click([&fm] { fm.close(); });

    // Layout management
    fm.div("vert <><<><weight=80% text><>><><weight=24<><button><>><>");
    fm["text"] << lab;
    fm["button"] << btn;
    fm.collocate();

    // Show the form
    fm.show();

    // Start to event loop process, it blocks until the form is closed.
    exec();
}

template <typename Bot>
void dispatchEvents(Bot &bot) {
    for (auto &e : BWAPI::Broodwar->getEvents())
        switch (e.getType()) {
        case BWAPI::EventType::MatchStart:
            bot.onStart();
            break;
        case BWAPI::EventType::MatchEnd:
            bot.onEnd(e.isWinner());
            break;
        case BWAPI::EventType::MatchFrame:
            bot.onFrame();
            break;
        case BWAPI::EventType::MenuFrame:
            // Looking at the server implementation, it seems that this event is never fired while
            // being in-game. It looks like events are fired every frame, even when we're not in a
            // game. And while we're outside a game, this will be the event fired each frame. So
            // this case has no relevance while being in-game. It will just never happen.
            assert(false);
            break;
        case BWAPI::EventType::SendText:
            bot.onSendText(e.getText());
            break;
        case BWAPI::EventType::ReceiveText:
            bot.onReceiveText(e.getPlayer(), e.getText());
            break;
        case BWAPI::EventType::PlayerLeft:
            bot.onPlayerLeft(e.getPlayer());
            break;
        case BWAPI::EventType::NukeDetect:
            bot.onNukeDetect(e.getPosition());
            break;
        case BWAPI::EventType::UnitDiscover:
            bot.onUnitDiscover(e.getUnit());
            break;
        case BWAPI::EventType::UnitEvade:
            bot.onUnitEvade(e.getUnit());
            break;
        case BWAPI::EventType::UnitShow:
            bot.onUnitShow(e.getUnit());
            break;
        case BWAPI::EventType::UnitHide:
            bot.onUnitHide(e.getUnit());
            break;
        case BWAPI::EventType::UnitCreate:
            bot.onUnitCreate(e.getUnit());
            break;
        case BWAPI::EventType::UnitDestroy:
            bot.onUnitDestroy(e.getUnit());
            break;
        case BWAPI::EventType::UnitMorph:
            bot.onUnitMorph(e.getUnit());
            break;
        case BWAPI::EventType::UnitRenegade:
            bot.onUnitRenegade(e.getUnit());
            break;
        case BWAPI::EventType::SaveGame:
            bot.onSaveGame(e.getText());
            break;
        case BWAPI::EventType::UnitComplete:
            bot.onUnitComplete(e.getUnit());
            break;
        default:
            // This will never happen.
            assert(false);
        }
}

int main(int argc, const char **argv) {
    if (argc == 2 && std::string(argv[1]) == "--gui") {
        // Run nana test in thread so that is doesn't block.
        std::thread gui(nanaHelloWorld);
        gui.detach(); // fine for now...
    }

    std::cout << "Connecting to server..." << std::endl;
    while (!BWAPI::BWAPIClient.connect()) {
        std::this_thread::sleep_for(std::chrono::milliseconds{300});
    }

    // Main loop
    for (int gameCounter = 1; BWAPI::BWAPIClient.isConnected(); ++gameCounter) {
        std::cout << "Waiting for a game to begin..." << std::endl;
        while (BWAPI::BWAPIClient.isConnected() && !BWAPI::Broodwar->isInGame())
            BWAPI::BWAPIClient.update(); // update shared memory

        if (!BWAPI::BWAPIClient.isConnected())
            break;

        // Handle non-playing cases right here so we can remove them from the bot entirly. (TODO!)
        if (BWAPI::Broodwar->self() == nullptr || BWAPI::Broodwar->isReplay())
            return EXIT_FAILURE;

        // Initialize game objects
        std::cout << gameCounter << ". game ready!" << std::endl;
        KBot::KBot kbot;

        // Dispatch events
        while (BWAPI::BWAPIClient.isConnected() && BWAPI::Broodwar->isInGame()) {
            dispatchEvents(kbot);

            // Trigger shared memory update. Blocks until next frame.
            BWAPI::BWAPIClient.update();
        }
        std::cout << "Game ended!" << std::endl;
    }
    std::cout << "Connection closed!" << std::endl;

    return EXIT_SUCCESS;
}
