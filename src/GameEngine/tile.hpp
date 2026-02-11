/**
 * @file tile.hpp
 *
 * LocalGen Module: GameEngine
 *
 * Tiles
 *
 * Tiles are basic elements of map boards.
 */

#ifndef LGEN_GAMEENGINE_TILE_HPP
#define LGEN_GAMEENGINE_TILE_HPP

#include "utils.hpp"

static constexpr int TILE_TYPE_COUNT = 8;

/// Tile types.
enum tile_type_e {
    /// Normal tile types.
    TILE_SPAWN = 0,
    TILE_BLANK = 1,
    TILE_MOUNTAIN = 2,
    TILE_CITY = 3,
    TILE_SWAMP = 4,
    TILE_DESERT = 5,
    TILE_LOOKOUT = 6,
    TILE_OBSERVATORY = 7,

    /// Mountains, cities, lookouts, observatories appear as 'obstacles' if
    /// not visible.
    /// Give it a specific number. It may appear as tile type return values.
    TILE_OBSTACLE = -1,

    /// Tile aliases
    TILE_GENERAL = TILE_SPAWN,
    TILE_PLAIN = TILE_BLANK,
    TILE_NEUTRAL = TILE_BLANK
};

/// Information of a single tile.
struct Tile {
    index_t occupier = -1;  // -1 stands for unoccupied
    tile_type_e type = TILE_BLANK;
    army_t army = 0;
    bool lit = false;

    Tile() = default;
    Tile(index_t _occupier, tile_type_e _type, army_t _army, bool _lit = false)
        : occupier(_occupier), type(_type), army(_army), lit(_lit) {};
};

/// Tile information visible to a specific player.
struct TileView {
    bool visible = false;
    index_t occupier = -1;
    tile_type_e type = TILE_BLANK;
    army_t army = 0;

    TileView() = default;
    TileView(const Tile& tile, bool vis) { fromTile(tile, vis); }

    /// Tile information + visibility -> TileView
    inline void fromTile(const Tile& tile, bool vis) {
        visible = vis;
        if (vis) {
            occupier = tile.occupier;
            type = tile.type;
            army = tile.army;
        } else {
            occupier = -1;
            switch (tile.type) {
                case TILE_SPAWN:
                case TILE_DESERT:      type = TILE_BLANK; break;
                case TILE_MOUNTAIN:
                case TILE_CITY:
                case TILE_LOOKOUT:
                case TILE_OBSERVATORY: type = TILE_OBSTACLE; break;
                default:               type = tile.type;
            }
            army = 0;
        }
    };
};

#endif  // LGEN_GAMEENGINE_TILE_HPP
