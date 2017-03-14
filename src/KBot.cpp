#include "KBot.h"

#include <iostream>

namespace KBot {

    using namespace BWAPI;

    // Called only once at the beginning of a game.
    void KBot::onStart() {
        // Good luck, have fun!
        Broodwar->sendText("gl hf");

        // Set the command optimization level so that common commands can be grouped
        // and reduce the bot's APM (Actions Per Minute).
        Broodwar->setCommandOptimizationLevel(2);
    }

    // Called once at the end of a game.
    void KBot::onEnd(bool isWinner) {}

    // Called once for every execution of a logical frame in Broodwar.
    void KBot::onFrame() {
        // Display the game frame rate as text in the upper left area of the screen
        Broodwar->drawTextScreen(2, 0, "FPS: %d", Broodwar->getFPS());
        Broodwar->drawTextScreen(2, 10, "Average FPS: %f", Broodwar->getAverageFPS());

        // Return if the game is a replay or is paused
        if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
            return;

        // Prevent spamming by only running our onFrame once every number of latency frames.
        // Latency frames are the number of frames before commands are processed.
        if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
            return;

        // Iterate through all the units that we own
        for (auto &u : Broodwar->self()->getUnits()) {
            // Ignore the unit if it no longer exists
            // Make sure to include this block when handling any Unit pointer!
            if (!u->exists())
                continue;

            // Ignore the unit if it has one of the following status ailments
            if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
                continue;

            // Ignore the unit if it is in one of the following states
            if (u->isLoaded() || !u->isPowered() || u->isStuck())
                continue;

            // Ignore the unit if it is incomplete or busy constructing
            if (!u->isCompleted() || u->isConstructing())
                continue;


            // Finally make the unit do some stuff!
            // TODO: Remove all that demo stuff...


            // If the unit is a worker unit
            if (u->getType().isWorker()) {
                // if our worker is idle
                if (u->isIdle()) {
                    // Order workers carrying a resource to return them to the center,
                    // otherwise find a mineral patch to harvest.
                    if (u->isCarryingGas() || u->isCarryingMinerals()) {
                        u->returnCargo();
                    }
                    else if (!u->getPowerUp()) {  // The worker cannot harvest anything if it
                                                  // is carrying a powerup such as a flag
                        // Harvest from the nearest mineral patch or gas refinery
                        if (!u->gather(u->getClosestUnit(Filter::IsMineralField || Filter::IsRefinery))) {
                            // If the call fails, then print the last error message
                            Broodwar << Broodwar->getLastError() << std::endl;
                        }
                    } // closure: has no powerup
                } // closure: if idle
            }
            else if (u->getType().isResourceDepot()) // A resource depot is a Command Center, Nexus, or Hatchery
            {
                // Order the depot to construct more workers! But only when it is idle.
                if (u->isIdle() && !u->train(u->getType().getRace().getWorker())) {
                    // If that fails, draw the error at the location so that you can visibly see what went wrong!
                    // However, drawing the error once will only appear for a single frame
                    // so create an event that keeps it on the screen for some frames
                    Position pos = u->getPosition();
                    Error lastErr = Broodwar->getLastError();
                    Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },   // action
                        nullptr,    // condition
                        Broodwar->getLatencyFrames());  // frames to run

                    // Retrieve the supply provider type in the case that we have run out of supplies
                    UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
                    static int lastChecked = 0;

                    // If we are supply blocked and haven't tried constructing more recently
                    if (lastErr == Errors::Insufficient_Supply &&
                        lastChecked + 400 < Broodwar->getFrameCount() &&
                        Broodwar->self()->incompleteUnitCount(supplyProviderType) == 0)
                    {
                        lastChecked = Broodwar->getFrameCount();

                        // Retrieve a unit that is capable of constructing the supply needed
                        Unit supplyBuilder = u->getClosestUnit(Filter::GetType == supplyProviderType.whatBuilds().first &&
                            (Filter::IsIdle || Filter::IsGatheringMinerals) &&
                            Filter::IsOwned);
                        // If a unit was found
                        if (supplyBuilder) {
                            if (supplyProviderType.isBuilding()) {
                                TilePosition targetBuildLocation = Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
                                if (targetBuildLocation) {
                                    // Register an event that draws the target build location
                                    Broodwar->registerEvent([targetBuildLocation, supplyProviderType](Game*)
                                    {
                                        Broodwar->drawBoxMap(Position(targetBuildLocation),
                                            Position(targetBuildLocation + supplyProviderType.tileSize()),
                                            Colors::Blue);
                                    },
                                        nullptr,  // condition
                                        supplyProviderType.buildTime() + 100);  // frames to run

                                    // Order the builder to construct the supply structure
                                    supplyBuilder->build(supplyProviderType, targetBuildLocation);
                                }
                            }
                            else {
                                // Train the supply provider (Overlord) if the provider is not a structure
                                supplyBuilder->train(supplyProviderType);
                            }
                        } // closure: supplyBuilder is valid
                    } // closure: insufficient supply
                } // closure: failed to train idle unit

            }

        } // closure: unit iterator
    }

    // Called when the user attempts to send a text message.
    void KBot::onSendText(std::string text) {
        // If Flag::UserInput is disabled, then this function is not called.

        // Send the text to the game if it is not being processed.
        Broodwar->sendText("%s", text.c_str());

        // Make sure to use %s and pass the text as a parameter,
        // otherwise you may run into problems when you use the %(percent) character!
    }

    // Called when the client receives a message from another Player.
    void KBot::onReceiveText(BWAPI::Player player, std::string text) {
        // Parse the received text
        Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
    }

    // Called when a Player leaves the game.
    void KBot::onPlayerLeft(BWAPI::Player player) {
        // Interact verbally with the other players in the game by
        // announcing that the other player has left.
        Broodwar->sendText("Goodbye %s!", player->getName().c_str());
    }

    // Called when a Nuke has been launched somewhere on the map.
    void KBot::onNukeDetect(BWAPI::Position target) {
        // Check if the target is a valid position
        if (target) {
            // if so, print the location of the nuclear strike target
            Broodwar << "Nuclear Launch Detected at " << target << std::endl;
        }
        else {
            // Otherwise, ask other players where the nuke is!
            Broodwar->sendText("Where's the nuke?");
        }

        // You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
    }

    // Called when a Unit becomes accessible.
    void KBot::onUnitDiscover(BWAPI::Unit unit) {}

    // Called when a Unit becomes inaccessible.
    void KBot::onUnitEvade(BWAPI::Unit unit) {}

    // Called when a previously invisible unit becomes visible.
    void KBot::onUnitShow(BWAPI::Unit unit) {}

    // Called just as a visible unit is becoming invisible.
    void KBot::onUnitHide(BWAPI::Unit unit) {}

    // Called when any unit is created.
    void KBot::onUnitCreate(BWAPI::Unit unit) {}

    // Called when a unit is removed from the game either through death or other means.
    void KBot::onUnitDestroy(BWAPI::Unit unit) {}

    // Called when a unit changes its UnitType.
    void KBot::onUnitMorph(BWAPI::Unit unit) {
        // For example, when a Drone transforms into a Hatchery, a Siege Tank uses Siege Mode, or a Vespene Geyser receives a Refinery.
    }

    // Called when a unit changes ownership.
    void KBot::onUnitRenegade(BWAPI::Unit unit) {
        // This occurs when the Protoss ability Mind Control is used, or if a unit changes ownership in Use Map Settings.
    }

    // Called when the state of the Broodwar game is saved to file.
    void KBot::onSaveGame(std::string gameName) {
        Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
    }

    // Called when the state of a unit changes from incomplete to complete.
    void KBot::onUnitComplete(BWAPI::Unit unit) {}

} // namespace