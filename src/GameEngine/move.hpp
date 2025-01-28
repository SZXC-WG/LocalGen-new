/**
 * @file move.hpp
 * 
 * LocalGen Module: GameEngine
 * 
 * Moves
 * 
 * Moves are basic operations of players in games.
 */

#ifndef LGEN_MODULE_GE_MOVE
#define LGEN_MODULE_GE_MOVE 1

#include "lib/coord.hpp"

#include "player.hpp"

/// Move Container.
/// A %Move is a basic operation of a player in a game.
/// Basically, it consists of two coordinates, representing the `from` and `to`.
/// Note that players shouldn't access other players' moves in game.
class Move {
   private:
    Player* player;  // whether we should use direct pointer or its index is still unknown.
    Coord from, to;
};

#endif  // LGEN_MODULE_GE_MOVE
