/**
 * @file GameEngine.h
 *
 * LocalGen Module: GameEngine (Draft)
 *
 * The GameEngine module is the core of the LocalGen game framework. It
 * declares and defines the core functions and data structures required
 * for LocalGen.
 */

#ifndef LGEN_MODULE_GAMEENGINE
#define LGEN_MODULE_GAMEENGINE 1

#include <cstdint>

#include "coord.h"

typedef int64_t army_t;

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
	tile_type_e type;
	army_t army;
	// Light is a tile attribute, not a tile type.
	bool light;
};

#include <cassert>
#include <vector>

class Player {};  // todo))

class Board {
   private:
	pos_t row, col;
	std::vector<std::vector<Tile>> tiles;
   public:
	Board() :
	    row(0), col(0) {}
	Board(pos_t _row, pos_t _col) :
	    row(_row), col(_col) {
		// board index starts at 1.
		// give 1 space at borders for safety and convenience issues.
		tiles.resize(row + 2, std::vector<Tile>(col + 2));
	}
	Tile getTile(pos_t x, pos_t y, Player* pplayer) {
		// test tile existance
		assert(x >= 1 && x <= row && y >= 1 && y <= col);
		// todo))
	}
};

#endif  // LGEN_MODULE_GAMEENGINE
