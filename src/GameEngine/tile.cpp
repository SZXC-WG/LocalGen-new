/**
 * @file tile.cpp
 *
 * LocalGen Module: GameEngine
 *
 * Tiles
 *
 * Tiles are basic elements of map boards.
 */

#ifndef LGEN_MODULE_GE_TILE_CPP
#define LGEN_MODULE_GE_TILE_CPP 1

#include "tile.h"

#include <cstdint>

Tile::Tile() : occupier(-1), type(TILE_BLANK), army(0), lit(false) {}
Tile::Tile(Player* _occupier, tile_type_e _type, army_t _army, bool _lit)
    : occupier(_occupier->index), type(_type), army(_army), lit(_lit) {}
Tile::Tile(Player::index_t _occupier, tile_type_e _type, army_t _army,
           bool _lit)
    : occupier(_occupier), type(_type), army(_army), lit(_lit) {}

TileView::TileView() {}
/// Constructor using a %Tile and its visibility.
TileView::TileView(const Tile& tile, const bool& vis) : visible(vis) {
    if (vis) {
        occupier = tile.occupier;
        type = tile.type;
        army = tile.army;
    } else {
        occupier = 0;
        switch (tile.type) {
            case TILE_SPAWN:       type = TILE_BLANK; break;
            case TILE_MOUNTAIN:
            case TILE_CITY:
            case TILE_LOOKOUT:
            case TILE_OBSERVATORY: type = TILE_OBSTACLE; break;
            default:               type = tile.type;
        }
        army = 0;
    }
}

#endif  // LGEN_MODULE_GE_TILE_CPP
