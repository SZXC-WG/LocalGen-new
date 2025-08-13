/**
 * @file move.cpp
 *
 * LocalGen Module: GameEngine
 *
 * Moves
 */

#ifndef LGEN_MODULE_GE_MOVE_CPP
#define LGEN_MODULE_GE_MOVE_CPP 1

#include "move.h"

#include "board.h"
#include "player.h"

bool operator<(const Move& a, const Move& b) {
    return (a.from != b.from ? a.from < b.from : a.to < b.to);
}
bool operator>(const Move& a, const Move& b) { return b < a; }
bool operator<=(const Move& a, const Move& b) { return !(b < a); }
bool operator>=(const Move& a, const Move& b) { return !(a < b); }

#endif  // LGEN_MODULE_GE_MOVE_CPP
