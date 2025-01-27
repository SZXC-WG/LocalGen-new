/**
 * @file tile.hpp
 * 
 * LocalGen Module: GameEngine
 * 
 * Tiles
 * 
 * Tiles are basic elements of map boards.
 */

#ifndef LGEN_MODULE_GE_TILE
#define LGEN_MODULE_GE_TILE 1

#include <cstdint>

#include "player.hpp"

using army_t = int64_t;

/// Tile types.
/// Defined as enum for they're all constants.
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

    /// Mountains, cities, lookouts, observatories will appear as 'obstacles' if not visible.
    /// Give it a specific type number. It may appear as tile type return values.
    TILE_OBSTACLE = -1,
};

// Some tiles types have same properties. Some tiles have aliases. Make them macros.
#define TILE_GENERAL TILE_SPAWN
#define TILE_PLAIN   TILE_BLANK
#define TILE_NEUTRAL TILE_BLANK

/// Information of a single tile.
struct Tile {
    Player* occupier;
    tile_type_e type;
    army_t army;
    // Light is a tile attribute, not a tile type.
    bool light;

    Tile() :
        occupier(nullptr), type(TILE_BLANK), army(0), light(false) {}
};

/// View of a %Tile.
/// Why do we need this? In game, players need more information of a tile than just the tile itself.
struct TileView {
    /// For a player in game, whether a tile is visible or not is very important.
    bool visible;
    /// We cannot provide direct %Player pointers in a %TileView for a pointer gives access to some sacred things.
    Player::index_t occupier;
    tile_type_e type;
    army_t army;
    /// Light has no importance in game, and will not be given.
    // bool light;

    TileView() {}
    /// Constructor using a %Tile and its visibility.
    TileView(const Tile& tile, const bool& vis) :
        visible(vis) {
        if(vis) {
            occupier = tile.occupier->index;
            type = tile.type;
            army = tile.army;
        } else {
            occupier = 0;
            switch(tile.type) {
                case TILE_SPAWN:
                    type = TILE_BLANK;
                    break;
                case TILE_MOUNTAIN:
                case TILE_CITY:
                case TILE_LOOKOUT:
                case TILE_OBSERVATORY:
                    type = TILE_OBSTACLE;
                    break;
                default:
                    type = tile.type;
            }
            army = 0;
        }
    }
};

#endif  // LGEN_MODULE_GE_TILE
