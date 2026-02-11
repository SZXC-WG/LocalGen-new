/**
 * @file move.cpp
 *
 * LocalGen Module: GameEngine
 *
 * Moves
 */

#ifndef LGEN_GAMEENGINE_MOVE_CPP
#define LGEN_GAMEENGINE_MOVE_CPP

#include "move.h"

#include "board.h"
#include "player.h"

bool operator<(const Move& a, const Move& b) {
    return (a.from != b.from ? a.from < b.from : a.to < b.to);
}
bool operator>(const Move& a, const Move& b) { return b < a; }
bool operator<=(const Move& a, const Move& b) { return !(b < a); }
bool operator>=(const Move& a, const Move& b) { return !(a < b); }

#endif  // LGEN_GAMEENGINE_MOVE_CPP
