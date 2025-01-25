/**
 * @file board.hpp
 * 
 * LocalGen Module: GameEngine
 * 
 * Map Boards
 * 
 * Container of maps.
 */

#include <cassert>
#include <vector>

#include "coord.hpp"

#include "player.hpp"
#include "tile.hpp"

/// Game map board. Zoomable.
/// The %Board is a complex thing. Many systems are part of it (e.g. vision system).
class Board {
   protected:
    pos_t row, col;
    std::vector<std::vector<Tile>> tiles;

   public:
    Board() :
        row(0), col(0) {}
    Board(pos_t _row, pos_t _col) :
        row(_row), col(_col) {
        // board index starts at 1.
        // give 1 space at borders for safety and convenience issues.
        tiles.resize(_row + 2, std::vector<Tile>(_col + 2));
    }

   public:
    // Here should lie the vision system. Someone come and write it for me!

    /// Check whether the %Tile at (x,y) is visible to a %Player.
    /// Declare this function as *virtual* for we'll declare a different function in-game.
    virtual bool visible(pos_t x, pos_t y, Player* player) const {
        // TODO))
        return false;
    }
    /// Same as above, but using %Coord.
    /// This is not declared as *virtual* for redefining it is unnecessary.
    bool visible(Coord coo, Player* player) { return visible(SEPA(coo), player); }

   public:
    /// A view for a %Board, accessible to a player.
    /// Declared as class for it may access direct information of the %Board.
    class BoardView {
       public:
        /// Here these are declared as public as this is only a view and contains no content associated to the original %Board directly.
        pos_t row, col;
        std::vector<std::vector<TileView>> tiles;

       public:
        BoardView() :
            row(0), col(0) {}
        /// Constructor for generating a %BoardView using a %Board and a %Player.
        /// Leaving this function public is safe, for a %Player cannot get another %Player's address.
        BoardView(const Board& board, Player* player) :
            row(board.row), col(board.col) {
            tiles.resize(board.row + 2, std::vector<TileView>(board.col + 2));
            for(pos_t i = 1; i <= row; ++i) {
                for(pos_t j = 1; j <= col; ++j) {
                    tiles[i][j] = TileView(board.tiles[i][j], board.visible(i, j, player));
                }
            }
        }
    };

   public:
    /// Give a player a view of the %Board.
    /// Returns a %BoardView.
    /// Leaving this function public is safe, for a %Player cannot get another %Player's address.
    BoardView view(Player* player) const {
        // Simply use the constructor to generate.
        return BoardView(*this, player);
    }
};

/// Declare alias for convenience.
using BoardView = Board::BoardView;
