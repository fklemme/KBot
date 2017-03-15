#include "KBot.h"

#include <cassert>
#include <iostream>
#include <random>

namespace KBot {

    using namespace BWAPI;

    // Called only once at the beginning of a game.
    void KBot::onStart() {
        if (Broodwar->isReplay()) return;

        // This bot is written for Terran, so make sure we are indeed Terran!
        if (Broodwar->self()->getRace() != Races::Terran) {
            Broodwar << "Wrong race selected! KBot has to be Terran." << std::endl;
            Broodwar->leaveGame();
        }

        // This bot is written for one-on-one melee games.
        if (Broodwar->getGameType() != GameTypes::Melee) {
            Broodwar << "Unexpected game type. KBot might not work correctly." << std::endl;
            // Unfortunate, but no reason to leave.
        }

        // Set the command optimization level so that common commands can be grouped.
        Broodwar->setCommandOptimizationLevel(2);

        // Ready to go. Good luck, have fun!
        Broodwar->sendText("gl hf");

        // Try to figure out enemy location.
        const auto enemyLocation = Broodwar->enemy()->getStartLocation();
        if (enemyLocation != TilePositions::Unknown)
            mEnemyLocations.push_back(enemyLocation);
        else {
            auto allLocations = Broodwar->getStartLocations();
            const auto myLocation = Broodwar->self()->getStartLocation();
            const auto myLocationIt = std::find(allLocations.begin(), allLocations.end(), Broodwar->self()->getStartLocation());
            assert(myLocationIt != allLocations.end());
            allLocations.erase(myLocationIt);
            std::sort(allLocations.begin(), allLocations.end(), [myLocation](TilePosition a, TilePosition b) {
                return myLocation.getDistance(a) < myLocation.getDistance(b);
            });
            std::copy(allLocations.begin(), allLocations.end(), std::back_inserter(mEnemyLocations));
        }
        assert(!mEnemyLocations.empty());
    }

    // Called once at the end of a game.
    void KBot::onEnd(bool isWinner) {}

    // Called once for every execution of a logical frame in Broodwar.
    void KBot::onFrame() {
        // Return if the game is a replay or is paused
        if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
            return;

        // Display the game frame rate as text in the upper left area of the screen
        Broodwar->drawTextScreen(2, 0, "FPS: %d, APM: %d", Broodwar->getFPS(), Broodwar->getAPM());

        // Some more debug infos...
        const auto myLocation = Broodwar->self()->getStartLocation();
        Broodwar->drawTextScreen(2, 10, "My StartLocation: %d, %d", myLocation.x, myLocation.y);
        Broodwar->drawTextScreen(2, 20, "EnemyLocation count: %d", mEnemyLocations.size());
        if (!mEnemyLocations.empty()) // Might be empty for a short period of time.
            Broodwar->drawTextScreen(2, 30, "Next EnemyLocation: %d, %d", mEnemyLocations.front().x, mEnemyLocations.front().y);
        else
            Broodwar->drawTextScreen(2, 30, "No more enemies!? :O");

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
            // For now and for fun, let's just build a simple marine rush bot using what we have. :)


            // If the unit is a worker unit
            if (u->getType().isWorker()) {
                // if our worker is idle
                if (u->isIdle()) {
                    // Order workers carrying a resource to return them to the center,
                    // otherwise find a mineral patch to harvest.
                    if (u->isCarryingGas() || u->isCarryingMinerals()) {
                        u->returnCargo();
                    }
                    // Harvest from the nearest mineral patch or gas refinery
                    else if (!u->gather(u->getClosestUnit(Filter::IsMineralField || Filter::IsRefinery))) {
                        // If the call fails, then print the last error message
                        Broodwar << Broodwar->getLastError() << std::endl;
                    }
                } // closure: if idle
            }
            else if (u->getType() == UnitTypes::Terran_Marine) {
                Broodwar->registerEvent([u](Game*) { Broodwar->drawCircleMap(u->getPosition(), 500, Colors::Yellow); }, nullptr, Broodwar->getLatencyFrames()); // debug!
                if (u->isIdle()) {
                    // Update enemy locations
                    if (Broodwar->isVisible(mEnemyLocations.front()) && Broodwar->getUnitsOnTile(mEnemyLocations.front(), Filter::IsEnemy).empty())
                        mEnemyLocations.pop_front();

                    // Defend, if there are only few marines.
                    if (Broodwar->getUnitsInRadius(u->getPosition(), 500, Filter::GetType == UnitTypes::Terran_Marine && Filter::IsOwned).size() < 20) {
                        auto nearbyEnemies = Broodwar->getUnitsInRadius(u->getPosition(), 500, Filter::IsEnemy);
                        if (!nearbyEnemies.empty())
                            u->attack(nearbyEnemies.getClosestUnit());
                        else
                            u->move(Position(Broodwar->self()->getStartLocation()));
                    }

                    // Otherwise, attack! :P
                    else if (!mEnemyLocations.empty())
                        u->attack(Position(mEnemyLocations.front()));
                }
            }
            else if (u->getType() == UnitTypes::Terran_Barracks) {
                if (u->isIdle()) {
                    // Spam marines! :D
                    if (!u->train(UnitTypes::Terran_Marine)) {
                        Position pos = u->getPosition();
                        Error lastErr = Broodwar->getLastError();
                        Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); },
                            nullptr, Broodwar->getLatencyFrames());
                    }
                }
            }
            else if (u->getType().isResourceDepot()) { // A resource depot is a Command Center, Nexus, or Hatchery
                Broodwar->registerEvent([u](Game*) { Broodwar->drawCircleMap(u->getPosition(), 300, Colors::Green); }, nullptr, Broodwar->getLatencyFrames()); // debug!
                // Limit amount of workers to produce.
                if (Broodwar->getUnitsInRadius(u->getPosition(), 300, Filter::IsWorker && Filter::IsOwned).size() < 20) {
                    // Order the depot to construct more workers! But only when it is idle.
                    if (u->isIdle() && !u->train(u->getType().getRace().getWorker())) {
                        // If that fails, draw the error at the location so that you can visibly see what went wrong!
                        // However, drawing the error once will only appear for a single frame
                        // so create an event that keeps it on the screen for some frames
                        Position pos = u->getPosition();
                        Error lastErr = Broodwar->getLastError();
                        Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); }, // action
                            nullptr, // condition
                            Broodwar->getLatencyFrames()); // frames to run
                    } // closure: failed to train idle unit
                }
            }
        } // closure: unit iterator

        // Build depos and racks!
        static int delay = 0;
        if (Broodwar->getFrameCount() > delay + 150) {
            const auto &me = *Broodwar->self();
            if (me.supplyUsed() >= me.supplyTotal() - 4 && me.minerals() >= UnitTypes::Terran_Supply_Depot.mineralPrice()) {
                auto builder = Broodwar->getClosestUnit(Position(me.getStartLocation()),
                    Filter::GetType == UnitTypes::Terran_SCV &&
                    (Filter::IsIdle || Filter::IsGatheringMinerals) &&
                    Filter::IsOwned);
                auto building = UnitTypes::Terran_Supply_Depot;

                if (builder) {
                    auto location = Broodwar->getBuildLocation(building, builder->getTilePosition());
                    if (location) {
                        Broodwar->registerEvent([location, building](Game*) {
                            Broodwar->drawBoxMap(Position(location), Position(location + building.tileSize()), Colors::Blue);
                        }, nullptr, 100);

                        builder->build(building, location); // FIXME: Print error on failure.
                        delay = Broodwar->getFrameCount();
                    }
                }
            }
            else if (me.minerals() >= UnitTypes::Terran_Barracks.mineralPrice()) {
                auto builder = Broodwar->getClosestUnit(Position(me.getStartLocation()),
                    Filter::GetType == UnitTypes::Terran_SCV &&
                    (Filter::IsIdle || Filter::IsGatheringMinerals) &&
                    Filter::IsOwned);
                auto building = UnitTypes::Terran_Barracks;

                if (builder) {
                    auto location = Broodwar->getBuildLocation(building, builder->getTilePosition());
                    if (location) {
                        Broodwar->registerEvent([location, building](Game*) {
                            Broodwar->drawBoxMap(Position(location), Position(location + building.tileSize()), Colors::Blue);
                        }, nullptr, 100);

                        builder->build(building, location); // FIXME: Print error on failure.
                        delay = Broodwar->getFrameCount();
                    }
                }
            }
        }
    }

    // Called when the user attempts to send a text message.
    void KBot::onSendText(std::string text) {}

    // Called when the client receives a message from another Player.
    void KBot::onReceiveText(BWAPI::Player player, std::string text) {}

    // Called when a Player leaves the game.
    void KBot::onPlayerLeft(BWAPI::Player player) {}

    // Called when a Nuke has been launched somewhere on the map.
    void KBot::onNukeDetect(BWAPI::Position target) {}

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