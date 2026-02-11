/**
 * @file player.hpp
 *
 * LocalGen Module: GameEngine
 *
 * Players
 *
 * Players are basic participants of games.
 */

#ifndef LGEN_GAMEENGINE_PLAYER_HPP
#define LGEN_GAMEENGINE_PLAYER_HPP

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "board.hpp"
#include "move.hpp"
#include "utils.hpp"

namespace game {
struct GameConstantsPack;
}  // namespace game

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
    virtual void init(index_t playerId,
                      const game::GameConstantsPack& constants) = 0;

    virtual void requestMove(BoardView& boardView) = 0;
    inline Move step() {
        if (moveQueue.empty()) return Move();
        Move move = moveQueue.front();
        moveQueue.pop_front();
        return move;
    }
};

#endif  // LGEN_GAMEENGINE_PLAYER_HPP
