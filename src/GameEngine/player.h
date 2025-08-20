/**
 * @file player.h
 *
 * LocalGen Module: GameEngine
 *
 * Players
 *
 * Players are basic participants of games.
 */

#ifndef LGEN_MODULE_GE_PLAYER_H
#define LGEN_MODULE_GE_PLAYER_H 1

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "move.h"

class Move;
class BoardView;

using index_t = std::uint32_t;

/// Base struct for players.
class Player {
   protected:
    std::deque<Move> moveQueue;

   public:
    Player() = default;
    ~Player() = default;

   protected:
    inline void surrender() {
        moveQueue.clear();
        moveQueue.emplace_back(MoveType::SURRENDER);
    }

   public:
    virtual void init(index_t playerId, std::vector<index_t> teamIds) = 0;
    virtual void requestMove(BoardView& boardView) = 0;
    inline Move step() {
        if (moveQueue.empty()) return Move();
        Move move = moveQueue.front();
        moveQueue.pop_front();
        return move;
    }
};

#endif  // LGEN_MODULE_GE_PLAYER_H
