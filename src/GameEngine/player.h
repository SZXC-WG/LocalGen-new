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

class Move;
class BoardView;

/// Base struct for players.
class Player {
   public:
    using index_t = std::uint32_t;

   public:
    const std::string name;
    index_t index;
    index_t teamId;
    std::deque<Move> moveQueue;

    Player(const std::string& name);
    ~Player();

    virtual Move step(const BoardView& view) = 0;
};

#endif  // LGEN_MODULE_GE_PLAYER_H
