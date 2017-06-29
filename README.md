# KBot
A simple, built-from-scratch Starcraft bot using BWAPI and modern C++.

# Build status

| Branch      | Visual Studio                           | GCC & Clang                                     |
| ----------- | --------------------------------------- | ----------------------------------------------- |
| master      | [![Build status][vs master]][vs build]  | [![Build Status][travis master]][travis build]  |
| development | [![Build status][vs develop]][vs build] | [![Build Status][travis develop]][travis build] |

[vs build]: https://kruecke.visualstudio.com/KBot
[vs master]: https://kruecke.visualstudio.com/_apis/public/build/definitions/30f6aa6a-33ee-4633-a315-57f354033160/3/badge
[vs develop]: https://kruecke.visualstudio.com/_apis/public/build/definitions/30f6aa6a-33ee-4633-a315-57f354033160/2/badge
[travis build]: https://travis-ci.org/Kruecke/KBot
[travis master]: https://travis-ci.org/Kruecke/KBot.svg?branch=master
[travis develop]: https://travis-ci.org/Kruecke/KBot.svg?branch=development

KBot is developed with Visual Studio 2017 on Windows but build services ensure it can be compiled on Linux with GCC and clang as well.

# Quick build with Visual Studio
Run `build.sh` from VS 2017 Command Prompt to build BWAPI and KBot for debug and release.

# Concepts and Structure of KBot
> Incomplete but gotta start somewhere. ;-)

KBot is built in a hierarchical manner that is reflected in its classes.

## Structure
Looking at the following list, the higher level item usually holds the class instances of its subordinated items.

- **KBot**: Implements the BWAPI interface.
  - **Manager**: Handles all economic units and tasks.
    - **Base**: Abstraction of a base. (depot, minerals, workers...)
    - **BuildTask**: Abstraction of a unit to be created.
  - **General**: Handles all military units and tasks.
    - **Squad**: A group of military units sharing a task.
  - **Enemy**: Informational data base about the enemy.

## Ownership
> A few lines of unit ownership in KBot.

# SSCAIT
KBot is running on SSCAIT. [Vote](http://sscaitournament.com/index.php?action=voteForPlayers&botId=384) for it to see it play on [Stream](https://www.twitch.tv/sscait). :)
