/**
 * @file board.h
 * 
 * LocalGen Module: GameEngine
 * 
 * Map Boards
 * 
 * Container of maps.
 */

#include <cassert>
#include <vector>

#include "coord.h"

#include "player.h"
#include "tile.h"

using BoardView = std::vector<std::vector<Tile>>;

/// Game map board. Zoomable.
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
    BoardView view(Player* player) const {
        // TODO
    }
};
