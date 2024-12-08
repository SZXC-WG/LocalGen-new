/**
 * @file player.h
 * 
 * LocalGen Module: GameEngine
 * 
 * Players
 * 
 * Players are basic participants of games.
 */

#ifndef LGEN_MODULE_GE_PLAYER
#define LGEN_MODULE_GE_PLAYER 1

#include <string>
#include "board.h"

/// Base struct for players.
class Player {
   public:
    const std::string name;
    Player(const std::string& name) :
        name(name) {}
    virtual ~Player() {}
    // virtual Move step(const BoardView& view) {}
};

#endif  // LGEN_MODULE_GE_PLAYER
