#include "KBot.h"
#include <BWAPI.h>
#include <BWAPI/Client.h>
#include <chrono>
#include <iostream>
#include <thread>

using namespace BWAPI;

static void busy_wait_connect() {
    while (!BWAPIClient.connect()) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1000});
    }
}

int main(int argc, const char **argv) {
    std::cout << "Connecting..." << std::endl;
    busy_wait_connect();
    while (true) {
        std::cout << "waiting to enter match" << std::endl;
        while (!Broodwar->isInGame()) {
            BWAPI::BWAPIClient.update();
            if (!BWAPI::BWAPIClient.isConnected()) {
                std::cout << "Reconnecting..." << std::endl;
                ;
                busy_wait_connect();
            }
        }

        // if (Broodwar->isReplay()) // TODO: Handle here?

        std::cout << "starting match!" << std::endl;
        KBot::KBot bot;

        while (Broodwar->isInGame()) {
            for (auto &e : Broodwar->getEvents()) {
                switch (e.getType()) {
                case EventType::MatchStart:
                    bot.onStart();
                    break;
                case EventType::MatchEnd:
                    bot.onEnd(e.isWinner());
                    break;
                case EventType::MatchFrame:
                    bot.onFrame();
                    break;
                case EventType::MenuFrame:
                    bot.onFrame(); // TODO: skip these?
                    break;
                case EventType::SendText:
                    bot.onSendText(e.getText());
                    break;
                case EventType::ReceiveText:
                    bot.onReceiveText(e.getPlayer(), e.getText());
                    break;
                case EventType::PlayerLeft:
                    bot.onPlayerLeft(e.getPlayer());
                    break;
                case EventType::NukeDetect:
                    bot.onNukeDetect(e.getPosition());
                    break;
                case EventType::UnitDiscover:
                    bot.onUnitDiscover(e.getUnit());
                    break;
                case EventType::UnitEvade:
                    bot.onUnitEvade(e.getUnit());
                    break;
                case EventType::UnitShow:
                    bot.onUnitShow(e.getUnit());
                    break;
                case EventType::UnitHide:
                    bot.onUnitHide(e.getUnit());
                    break;
                case EventType::UnitCreate:
                    bot.onUnitCreate(e.getUnit());
                    break;
                case EventType::UnitDestroy:
                    bot.onUnitDestroy(e.getUnit());
                    break;
                case EventType::UnitMorph:
                    bot.onUnitMorph(e.getUnit());
                    break;
                case EventType::UnitRenegade:
                    bot.onUnitRenegade(e.getUnit());
                    break;
                case EventType::SaveGame:
                    bot.onSaveGame(e.getText());
                    break;
                case EventType::UnitComplete:
                    bot.onUnitComplete(e.getUnit());
                    break;
                default:
                    // FIXME: This will never happen?
                    assert(false);
                }
            }

            BWAPI::BWAPIClient.update();
            if (!BWAPI::BWAPIClient.isConnected()) {
                std::cout << "Reconnecting..." << std::endl;
                busy_wait_connect();
            }
        }
        std::cout << "Game ended" << std::endl;
    }
    std::cout << "Press ENTER to continue..." << std::endl;
    std::cin.ignore();

    return 0;
}
