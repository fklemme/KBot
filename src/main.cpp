#include "Gui.h"
#include "KBot.h"
#include <BWAPI/Client.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <set>
#include <thread>

template <typename Bot>
static void dispatchEvents(Bot &bot) {
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
    // Read commandline options (very simple approach, good enough for now)
    const std::set<std::string> options(std::next(argv), std::next(argv, argc));

    // Wait for server connection
    std::cout << "Connecting to server..." << std::endl;
    while (!BWAPI::BWAPIClient.connect())
        std::this_thread::sleep_for(std::chrono::milliseconds{100});

    // Main loop, playing games
    for (int gameCounter = 1; BWAPI::BWAPIClient.isConnected(); ++gameCounter) {
        std::cout << "Waiting for a game to begin..." << std::endl;
        while (BWAPI::BWAPIClient.isConnected() && !BWAPI::Broodwar->isInGame())
            BWAPI::BWAPIClient.update(); // update shared memory

        if (!BWAPI::BWAPIClient.isConnected())
            break; // disconnected

        // Handle non-playing cases right here so we can remove them from the bot entirly. (TODO!)
        if (BWAPI::Broodwar->self() == nullptr || BWAPI::Broodwar->isReplay()) {
            std::cerr << "Watching replays is not supported!" << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << gameCounter << ". game started!" << std::endl;

        // Initialize bot
        KBot::KBot kbot;

        // Create GUI, if enabled
        auto gui = [&]() -> std::unique_ptr<KBot::Gui> {
            if (options.count("--gui") == 1)
                return std::make_unique<KBot::Gui>(kbot);
            return nullptr;
        }();

        while (BWAPI::BWAPIClient.isConnected() && BWAPI::Broodwar->isInGame()) {
            // Dispatch new events from server
            dispatchEvents(kbot);

            // Update GUI, if enabled
            if (gui)
                gui->update(kbot);

            // Update shared memory. Blocks until next frame.
            BWAPI::BWAPIClient.update();
        }
        std::cout << "Game ended!" << std::endl;
    }
    std::cout << "Connection closed!" << std::endl;

    return EXIT_SUCCESS;
}
