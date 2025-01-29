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

#include "lib/coord.hpp"

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
        row(0), col(0), tiles(0, std::vector<Tile>(0)) {}
    Board(pos_t _row, pos_t _col) :
        row(_row), col(_col) {
        assert(row >= 0 && col >= 0);
        // board index starts at 1.
        // give 1 space at borders for safety and convenience issues.
        tiles.resize(_row + 2, std::vector<Tile>(_col + 2, Tile()));
    }

   public:
    /// Check whether a %Coord indicating a tile position is valid.
    bool is_valid_coord(Coord coord) const {
        return coord.x >= 1 && coord.x <= col && coord.y >= 1 && coord.y <= row;
    }
    /// Check whether a %Coord indicating a tile position is invalid.
    bool is_invalid_coord(Coord coord) const { return !is_valid_coord(coord); }

   public:
    /// Check whether the %Tile at (x,y) is visible to a %Player.
    /// Declare this function as *virtual* for we'll declare a different function in-game.
    virtual bool visible(pos_t x, pos_t y, Player* player) const {
        // invalid check
        if(player == nullptr) return false;
        if(x < 1 || x > col || y < 1 || y > row) return false;

        // occupier visibility
        if(in_same_team(tiles[x][y].occupier, player)) return true;
        // adjacent visibility
        if(true &&  // overall check may not consider modifiers
           (in_same_team(tiles[x - 1][y].occupier, player) ||
            in_same_team(tiles[x + 1][y].occupier, player) ||
            in_same_team(tiles[x][y - 1].occupier, player) ||
            in_same_team(tiles[x][y + 1].occupier, player) ||
            in_same_team(tiles[x - 1][y - 1].occupier, player) ||
            in_same_team(tiles[x - 1][y + 1].occupier, player) ||
            in_same_team(tiles[x + 1][y - 1].occupier, player) ||
            in_same_team(tiles[x + 1][y + 1].occupier, player)))
            return true;

        // lookout check
        auto find_lookout_occupier = [&](pos_t x, pos_t y) {
            Player* occupier = nullptr;
            army_t maxArmy = 0;
            if(tiles[x - 1][y].army > maxArmy) occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
            if(tiles[x + 1][y].army > maxArmy) occupier = tiles[x + 1][y].occupier, maxArmy = tiles[x + 1][y].army;
            if(tiles[x][y - 1].army > maxArmy) occupier = tiles[x][y - 1].occupier, maxArmy = tiles[x][y - 1].army;
            if(tiles[x][y + 1].army > maxArmy) occupier = tiles[x][y + 1].occupier, maxArmy = tiles[x][y + 1].army;
            if(tiles[x - 1][y - 1].army > maxArmy) occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
            if(tiles[x - 1][y + 1].army > maxArmy) occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
            if(tiles[x + 1][y - 1].army > maxArmy) occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
            if(tiles[x + 1][y + 1].army > maxArmy) occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
            return occupier;
        };
        for(pos_t i = std::max(x - 3, 1); i <= std::min(x + 3, row); ++i) {
            for(pos_t j = std::max(y - 3, 1); j <= std::min(y + 3, col); ++j) {
                if(tiles[i][j].type == TILE_LOOKOUT)
                    if(find_lookout_occupier(i, j) == player) return true;
            }
        }

        // observatory check
        for(pos_t i = std::max(x - 8, 1); i < x; ++i) {
            if(tiles[i][y].type == TILE_OBSERVATORY)
                if(tiles[i - 1][y].occupier == player) return true;
        }
        for(pos_t i = x + 1; i <= std::min(x + 8, row); ++i) {
            if(tiles[i][y].type == TILE_OBSERVATORY)
                if(tiles[i + 1][y].occupier == player) return true;
        }
        for(pos_t j = std::max(y - 8, 1); j < y; ++j) {
            if(tiles[x][j].type == TILE_OBSERVATORY)
                if(tiles[x][j - 1].occupier == player) return true;
        }
        for(pos_t j = y + 1; j <= std::min(y + 8, col); ++j) {
            if(tiles[x][j].type == TILE_OBSERVATORY)
                if(tiles[x][j + 1].occupier == player) return true;
        }

        // none matches, not visible
        return false;
    }
    /// Same as above, but using %Coord.
    /// This is not declared as *virtual* for redefining it is unnecessary.
    bool visible(const Coord& coord, Player* player) const { return visible(SEPA(coord), player); }

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

    /// Give a player a view of a certain tile of the %Board.
    /// Returns a %TileView.
    /// Leaving this function public is safe, for a %Player cannot get another %Player's address.
    TileView view(Player* player, pos_t row, pos_t col) {
        return TileView(tiles[row][col], visible(row, col, player));
    }
    /// Give a player a view of a certain tile of the %Board.
    /// Returns a %TileView.
    /// Leaving this function public is safe, for a %Player cannot get another %Player's address.
    TileView view(Player* player, Coord pos) {
        return TileView(tiles[pos.x][pos.y], visible(pos, player));
    }
};

/// Declare alias for convenience.
using BoardView = Board::BoardView;
