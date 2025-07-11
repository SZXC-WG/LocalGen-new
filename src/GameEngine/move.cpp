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
Move::Move(MoveType _type, Player* _player) : type(_type), player(_player) {}
Move::Move(MoveType _type, Player* _player, Coord _from, Coord _to,
           bool _takeHalf)
    : type(_type), player(_player), from(_from), to(_to), takeHalf(_takeHalf) {}

bool Move::available(Board* board) {
    if (type == MoveType::SURRENDER) return true;
    if (type == MoveType::MOVE_ARMY) {
        // coordinate validity check
        if (!board->isInvalidCoord(from) || !board->isInvalidCoord(to))
            return false;

        // visibility check
        if (!board->visible(from, player->index)) return false;

        // %from tile availability check
        auto fromTile = board->view(player->index, from);
        switch (fromTile.type) {
            case TILE_MOUNTAIN:
            case TILE_LOOKOUT:
            case TILE_OBSERVATORY: return false;
        }
        if (fromTile.occupier != player->index) return false;
        if (fromTile.army <= 1) return false;

        // %to tile availability check
        auto toTile = board->view(player->index, to);
        switch (toTile.type) {
            case TILE_MOUNTAIN:
            case TILE_LOOKOUT:
            case TILE_OBSERVATORY: return false;
        }

        // all passed, available move
        return true;
    }
    return false;
}

bool operator<(const Move& a, const Move& b) {
    return (a.from != b.from ? a.from < b.from : a.to < b.to);
}
bool operator>(const Move& a, const Move& b) { return b < a; }
bool operator<=(const Move& a, const Move& b) { return !(b < a); }
bool operator>=(const Move& a, const Move& b) { return !(a < b); }

#endif  // LGEN_MODULE_GE_MOVE_CPP
