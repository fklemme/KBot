# KBot
Starcraft Bot (using BWAPI)

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

# Build with Visual Studio
- Change the path in `StarcraftPath.props` to match your installation of Starcraft Broodwar.
- Run `build.sh` from VS 2017 Command Prompt to build BWAPI and KBot for debug and release.

# SSCAIT
KBot is running on SSCAIT as well. [Vote](http://sscaitournament.com/index.php?action=voteForPlayers&botId=384) for it to see it play on [Stream](https://www.twitch.tv/sscait). :)
