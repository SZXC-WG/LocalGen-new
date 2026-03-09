/**
 * @file message.hpp
 *
 * LocalGen Module: core
 *
 * Game Event Messages
 *
 * Defines the GameEvent and GameMessage data structures.
 */

#ifndef LGEN_CORE_MESSAGE_HPP
#define LGEN_CORE_MESSAGE_HPP

#include <string>
#include <variant>

#include "utils.hpp"  // For index_t, turn_t

namespace game {

// Win Message
struct GameMessageWin {
    index_t winner;
};

// Capture Message
struct GameMessageCapture {
    index_t capturer;
    index_t captured;
};

// Surrender Message
struct GameMessageSurrender {
    index_t player;
};

// Chat Text Message
struct GameMessageText {
    index_t sender;
    std::string text;
};

// The data variant
using GameMessageData = std::variant<GameMessageWin, GameMessageCapture,
                                     GameMessageSurrender, GameMessageText>;

// The complete Event wrapper
struct GameEvent {
    turn_t turn;
    GameMessageData data;
};

}  // namespace game

#endif  // LGEN_CORE_MESSAGE_HPP
