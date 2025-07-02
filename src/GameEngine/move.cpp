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

Move::Move() : player(nullptr) {}
Move::Move(Player* _player, Coord _from, Coord _to, bool _takeHalf)
    : player(_player), from(_from), to(_to), takeHalf(_takeHalf) {}

bool Move::available(Board* board) {
    // coordinate validity check
    if (!board->isInvalidCoord(from) || !board->isInvalidCoord(to))
        return false;

    // visibility check
    if (!board->visible(from, player)) return false;

    // %from tile availability check
    auto fromTile = board->view(player, from);
    switch (fromTile.type) {
        case TILE_MOUNTAIN:
        case TILE_LOOKOUT:
        case TILE_OBSERVATORY: return false;
    }
    if (fromTile.occupier != player->index) return false;
    if (fromTile.army <= 1) return false;

    // %to tile availability check
    auto toTile = board->view(player, to);
    switch (toTile.type) {
        case TILE_MOUNTAIN:
        case TILE_LOOKOUT:
        case TILE_OBSERVATORY: return false;
    }

    // all passed, available move
    return true;
}

bool operator<(const Move& a, const Move& b) {
    return (a.from != b.from ? a.from < b.from : a.to < b.to);
}
bool operator>(const Move& a, const Move& b) { return b < a; }
bool operator<=(const Move& a, const Move& b) { return !(b < a); }
bool operator>=(const Move& a, const Move& b) { return !(a < b); }

#endif  // LGEN_MODULE_GE_MOVE_CPP
