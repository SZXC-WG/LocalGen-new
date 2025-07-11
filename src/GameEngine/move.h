/**
 * @file move.h
 *
 * LocalGen Module: GameEngine
 *
 * Moves
 *
 * Moves are basic operations of players.
 */

#ifndef LGEN_MODULE_GE_MOVE_H
#define LGEN_MODULE_GE_MOVE_H 1

#include "../utils/coord.h"

class Player;
class Board;

enum class MoveType { MOVE_ARMY, SURRENDER };

/// Move Container.
/// A %Move is a basic operation of a player in a game.
/// Basically, it consists of two coordinates, representing the `from` and
/// `to`. Note that players shouldn't access other players' moves in game.
class Move {
   public:
    MoveType type;
    Player* player;  // whether we should use direct pointer or its index is
                     // still unknown.
    Coord from, to;
    bool takeHalf;

   public:
    Move();
    Move(MoveType _type, Player* _player);
    Move(MoveType _type, Player* _player, Coord _from, Coord _to,
         bool _takeHalf);

    /// Check whether a %Move is available on a certain Board.
    bool available(Board* board);
};

/// Comparisons of (%Move)s. Defined to make usage of comparison-based
/// containers like std::set and std::map more convenient.
bool operator<(const Move& a, const Move& b);
bool operator>(const Move& a, const Move& b);
bool operator<=(const Move& a, const Move& b);
bool operator>=(const Move& a, const Move& b);

#endif  // LGEN_MODULE_GE_MOVE_H
