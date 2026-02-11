/**
 * @file move.hpp
 *
 * LocalGen Module: GameEngine
 *
 * Moves
 *
 * Moves are basic operations of players.
 */

#ifndef LGEN_GAMEENGINE_MOVE_HPP
#define LGEN_GAMEENGINE_MOVE_HPP

#include "utils.hpp"

enum class MoveType { EMPTY, MOVE_ARMY, SURRENDER };

/// Stores a single move: `from` -> `to`.
struct Move {
   public:
    MoveType type = MoveType::EMPTY;
    Coord from, to;
    bool takeHalf = false;

   public:
    constexpr Move() = default;
    constexpr Move(MoveType _type) : type(_type) {}
    constexpr Move(MoveType _type, Coord _from, Coord _to, bool _takeHalf)
        : type(_type), from(_from), to(_to), takeHalf(_takeHalf) {}
};

/// A pure empty move.
constexpr Move EMPTY_MOVE;

#endif  // LGEN_GAMEENGINE_MOVE_HPP
