/**
 * @file board.h
 *
 * LocalGen Module: GameEngine
 *
 * Map Boards
 *
 * Container of maps.
 */

#ifndef LGEN_MODULE_GE_BOARD_H
#define LGEN_MODULE_GE_BOARD_H 1

#include <string>
#include <vector>

#include "../utils/coord.h"
#include "tile.h"

class Tile;
class TileView;

class Player;

/// Game map board. Zoomable.
/// The %Board is a complex thing. Many systems are part of it (e.g. vision
/// system).
class Board {
   protected:
    pos_t row, col;
    std::vector<std::vector<Tile>> tiles;

    /// Get a tile at a certain position.
    /// @param coord The position of the tile.
    /// @return A reference to the tile.
    Tile& tileAt(Coord coord);
    /// Change the width (col) of the %Board.
    /// @param _col The new width.
    /// @return 0 if the operation is successful, 1 otherwise.
    int setWidth(pos_t _col);
    /// Change the height (row) of the %Board.
    /// @param _row The new height.
    /// @return 0 if the operation is successful, 1 otherwise.
    int setHeight(pos_t _row);

   public:
    /// Check whether a %Coord indicating a tile position is valid.
    bool isValidCoord(Coord coord) const;
    /// Check whether a %Coord indicating a tile position is invalid.
    bool isInvalidCoord(Coord coord) const;

   public:
    /// Get a tile using %Coord. This is a function for convenience.
    Tile getTile(Coord coord) const;
    inline int getWidth() const { return col; }
    inline int getHeight() const { return row; }

   public:
    Board();
    Board(pos_t _row, pos_t _col);

   public:
    /// Map coding system derived from v5.
    /// LocalGen v6 is compatible with v5, so we saved this system.

    intmax_t v5Pmod(intmax_t& x);

    // Zip a map using v5 coding.
    std::string v5Zip();
    /// Unzip a map with v5 coding.
    void v5Unzip(std::string strUnzip);

   public:
    /// Check whether the %Tile at (x,y) is visible to a %Player.
    /// Declare this function as *virtual* for we'll declare a different
    /// function in-game.
    virtual bool visible(pos_t x, pos_t y, Player* player) const;
    /// Same as above, but using %Coord.
    /// This is not declared as *virtual* for redefining it is unnecessary.
    bool visible(const Coord& coord, Player* player) const;

   public:
    friend class BoardView;

   public:
    /// Give a player a view of the %Board.
    /// Returns a %BoardView.
    /// Leaving this function public is safe, for a %Player cannot get another
    /// %Player's address.
    BoardView view(Player* player) const;

    /// Give a player a view of a certain tile of the %Board.
    /// Returns a %TileView.
    /// Leaving this function public is safe, for a %Player cannot get another
    /// %Player's address.
    TileView view(Player* player, pos_t row, pos_t col);
    /// Give a player a view of a certain tile of the %Board.
    /// Returns a %TileView.
    /// Leaving this function public is safe, for a %Player cannot get another
    /// %Player's address.
    TileView view(Player* player, Coord pos);
};

/// A view for a %Board, accessible to a player.
/// Declared as class for it may access direct information of the %Board.
class BoardView {
   public:
    /// Here these are declared as public as this is only a view and
    /// contains no content associated to the original %Board directly.
    pos_t row, col;
    std::vector<std::vector<TileView>> tiles;

    /// Get a tile at a certain position.
    /// @param coord The position of the tile.
    /// @return A reference to the tile.
    TileView& tileAt(Coord coord);

   public:
    /// Check whether a %Coord indicating a tile position is valid.
    bool isValidCoord(Coord coord) const;
    /// Check whether a %Coord indicating a tile position is invalid.
    bool isInvalidCoord(Coord coord) const;

   public:
    BoardView();
    /// Constructor for generating a %BoardView using a %Board and a
    /// %Player. Leaving this function public is safe, for a %Player cannot
    /// get another %Player's address.
    BoardView(const Board* const& board, Player* player);
};

/// Derived class for initial boards (pre-game boards).
class InitBoard : public Board {
   public:
    InitBoard();
    InitBoard(pos_t row, pos_t col);

   public:
    using Board::setHeight;
    using Board::setWidth;

   public:
    /// Tag all spawns and their team preferences.
    std::vector<std::pair<Coord, int>> spawns;

    /// Change a tile at a certain position.
    /// @param coord The position of the tile.
    /// @param tile The new tile.
    /// @return 0 if the operation is successful, 1 otherwise.
    int changeTile(Coord coord, Tile tile);

    /// Set attributes of a spawn.
    /// @param coord The position of the spawn.
    /// @param team The team preference of the spawn.
    /// @return 0 if the operation is successful, 1 otherwise.
    int setSpawn(Coord coord, unsigned team);

    /// Get the team preference of a spawn.
    /// @param coord The position of the spawn.
    /// @return The team preference of the spawn. -1 if failed.
    int getSpawnTeam(Coord coord);
};

#endif  // LGEN_MODULE_GE_BOARD_H
