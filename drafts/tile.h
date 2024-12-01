/**
 * @file tile.h
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

#include "player.h"

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

// Some tiles have some properties. Some tiles have aliases. Make them macros.
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
};

#endif  // LGEN_MODULE_GE_TILE
