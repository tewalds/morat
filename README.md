# Morat

Morat is a game playing framework, along with implementations of several games. It includes some general purpose libraries (alarm, time, thread, random), and some game specific libraries (compacting tree, gtp, time controls).

It specializes in 2-player, perfect information, zero sum, deterministic games, especially placement games (where RAVE works).

So far it supports 3 games:
* [Havannah](https://en.wikipedia.org/wiki/Havannah)
* [Hex](https://en.wikipedia.org/wiki/Hex_%28board_game%29)
* Rex - Reverse Hex (ie try to force the opponent to connect their edges).
* [Y](https://en.wikipedia.org/wiki/Y_%28game%29)
* [Pentago](https://en.wikipedia.org/wiki/Pentago)

So far it supports 3 algorithms:
* [MCTS: Monte-Carlo Tree Search](https://en.wikipedia.org/wiki/Monte-Carlo_tree_search)
* [PNS: Proof Number Search](https://en.wikipedia.org/wiki/Proof-number_search)
* [AB: Alpha-Beta](https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning)

The goal is to make the algorithms game independent, and make it easier to implement new games with strong players. There is quite a bit of work left to make this a reality, so the current work is just to make the game code more similar and then move the code into common libraries.

The primary interface is [GTP (Go Text Protocol)](https://en.wikipedia.org/wiki/Go_Text_Protocol), which can be used from:
* the command line
* a UI like [HavannahGui](http://mgame99.mg.funpic.de/downloads.php)
* a [webserver](https://github.com/tewalds/pentagod/blob/master/web.py) and [interface](https://github.com/tewalds/pentagod/tree/master/pentagoo)


## Requirements

* Linux (probably works on any unix)
* C++ tool chain (g++ or clang)

## Usage

* Check out the code from github
* Run ```make``` to compile the code
* Run:
  * ```./castro``` for Havannah
  * ```./moy``` for Y
  * ```./pentagod``` for pentago

If you make any changes to the code and want to update the dependencies, just ```make clean```, or ```rm .Makefile```.

## License

This projected is licensed under the terms of the MIT license.
