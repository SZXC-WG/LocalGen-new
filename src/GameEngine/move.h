/**
 * @file move.h
 *
 * LocalGen Module: GameEngine
 *
 * Moves
 *
 * Moves are basic operations of players.
 */

#ifndef LGEN_GAMEENGINE_MOVE_H
#define LGEN_GAMEENGINE_MOVE_H

#include "../utils/coord.h"

class Board;

enum class MoveType { EMPTY, MOVE_ARMY, SURRENDER };

/// Move Container.
/// A %Move is a basic operation of a player in a game.
/// Basically, it consists of two coordinates, representing the `from` and
/// `to`. Note that players shouldn't access other players' moves in game.
class Move {
   public:
    MoveType type;
    Coord from, to;
    bool takeHalf;

   public:
    constexpr Move()
        : type(MoveType::EMPTY), from(0, 0), to(0, 0), takeHalf(false) {}

    constexpr Move(MoveType _type)
        : type(_type), from(0, 0), to(0, 0), takeHalf(false) {}
    constexpr Move(MoveType _type, Coord _from, Coord _to, bool _takeHalf)
        : type(_type), from(_from), to(_to), takeHalf(_takeHalf) {}
};

/// A pure empty move.
#define EMPTY_MOVE (Move(MoveType::EMPTY))

/// Comparisons of (%Move)s. Defined to make usage of comparison-based
/// containers like std::set and std::map more convenient.
bool operator<(const Move& a, const Move& b);
bool operator>(const Move& a, const Move& b);
bool operator<=(const Move& a, const Move& b);
bool operator>=(const Move& a, const Move& b);

#endif  // LGEN_GAMEENGINE_MOVE_H
