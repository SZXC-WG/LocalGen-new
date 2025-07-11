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

using index_t = std::uint32_t;

/// Base struct for players.
class Player {
   public:
   public:
    const std::string name;
    index_t index;
    index_t teamId;
    std::deque<Move> moveQueue;

    Player(const std::string& name);
    ~Player();

    void surrender();

    virtual Move act() = 0;
};

#endif  // LGEN_MODULE_GE_PLAYER_H
