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
#include <string>

/// Base struct for players.
class Player {
   public:
    using index_t = std::uint32_t;

   public:
    const std::string name;
    index_t index;
    index_t teamId;

    Player(const std::string& name) :
        name(name) {}
    virtual ~Player() {}
    // virtual Move step(const BoardView& view) {}
};

inline bool in_same_team(Player* p1, Player* p2) {
    if(p1 == nullptr || p2 == nullptr) return false;
    return p1->teamId == p2->teamId;
}

#endif  // LGEN_MODULE_GE_PLAYER
