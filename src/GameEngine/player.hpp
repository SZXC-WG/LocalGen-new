/**
 * @file player.hpp
 *
 * LocalGen Module: GameEngine
 *
 * Players
 *
 * Players are basic participants of games.
 */

#ifndef LGEN_MODULE_GE_PLAYER
#define LGEN_MODULE_GE_PLAYER 1

#include <cstdint>
#include <deque>
#include <string>

#include "board.hpp"

/// Base struct for players.
class Player {
   public:
    using index_t = std::uint32_t;

   public:
    const std::string name;
    index_t           index;
    index_t           teamId;
    std::deque<Move>  moveQueue;

    Player(const std::string& name) : name(name) {}
    ~Player() {}
    virtual Move step(const BoardView& view) = 0;
};

inline bool inSameTeam(Player* p1, Player* p2) {
    if (p1 == nullptr || p2 == nullptr) return false;
    return p1->teamId == p2->teamId;
}

#endif  // LGEN_MODULE_GE_PLAYER
