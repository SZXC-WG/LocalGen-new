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
    
    Player(const std::string& name) :
        name(name) {}
    virtual ~Player() {}
    // virtual Move step(const BoardView& view) {}
};

#endif  // LGEN_MODULE_GE_PLAYER
