/**
 * @file board.h
 *
 * LocalGen Module: GameEngine
 *
 * Map Boards
 *
 * Container of maps.
 */

#ifndef LGEN_GAMEENGINE_BOARD_H
#define LGEN_GAMEENGINE_BOARD_H

#include <string>
#include <vector>

#include "move.h"
#include "tile.h"
#include "utils.hpp"

class Tile;
class TileView;

class Player;

class BoardView;

/// Game map board. Zoomable.
/// The %Board is a complex thing. Many systems are part of it (e.g. vision
/// system).
class Board {
   protected:
    pos_t row, col;
    std::vector<std::vector<Tile>> tiles;

    Tile& tileAt(pos_t x, pos_t y);
    Tile& tileAt(Coord pos);
    /// Change the width (col) of the %Board.
    /// @return 0 if the operation is successful, 1 otherwise.
    int setWidth(pos_t _col);
    /// Change the height (row) of the %Board.
    /// @return 0 if the operation is successful, 1 otherwise.
    int setHeight(pos_t _row);

   public:
    inline bool isValidPos(pos_t x, pos_t y) const {
        return x >= 1 && x <= row && y >= 1 && y <= col;
    }
    inline bool isValidPos(Coord pos) const { return isValidPos(pos.x, pos.y); }
    inline bool isInvalidPos(pos_t x, pos_t y) const {
        return !isValidPos(x, y);
    }
    inline bool isInvalidPos(Coord pos) const { return !isValidPos(pos); }

   public:
    Tile getTile(pos_t x, pos_t y) const;
    Tile getTile(Coord pos) const;
    inline int getWidth() const { return col; }
    inline int getHeight() const { return row; }

   public:
    Board();
    Board(pos_t _row, pos_t _col);
    Board(Board* _board);

   public:
    /// Map coding system derived from v5.
    /// LocalGen v6 is compatible with v5, so we saved this system.

    intmax_t v5Pmod(intmax_t& x);

    std::string v5Zip();
    void v5Unzip(std::string strUnzip);

   public:
    /// Check whether the %Tile at (x,y) is visible to a %Player.
    virtual bool visible(pos_t x, pos_t y, index_t player) const;
    /// Same as above, but using %Coord.
    bool visible(const Coord& pos, index_t player) const;

   public:
    bool available(index_t player, Move move);

   public:
    friend class BoardView;

   public:
    BoardView view(index_t player) const;

    TileView view(index_t player, pos_t row, pos_t col);
    TileView view(index_t player, Coord pos);
};

/// A view for a %Board, accessible to a player.
/// Declared as class for it may access direct information of the %Board.
class BoardView {
   public:
    pos_t row, col;
    std::vector<std::vector<TileView>> tiles;

    TileView& tileAt(pos_t x, pos_t y);
    TileView& tileAt(Coord pos);

   public:
    inline bool isValidPos(pos_t x, pos_t y) const {
        return x >= 1 && x <= row && y >= 1 && y <= col;
    }
    inline bool isValidPos(Coord pos) const { return isValidPos(pos.x, pos.y); }
    inline bool isInvalidPos(pos_t x, pos_t y) const {
        return !isValidPos(x, y);
    }
    inline bool isInvalidPos(Coord pos) { return !isValidPos(pos); }

   public:
    BoardView();
    BoardView(const Board* const& board, index_t player);
};

/// Derived class for initial boards (pre-game boards).
class InitBoard : public Board {
   public:
    InitBoard();
    InitBoard(pos_t row, pos_t col);

   public:
    using Board::getTile;
    using Board::setHeight;
    using Board::setWidth;

   public:
    /// Use this to tag all spawns and their team preferences.
    std::vector<std::pair<Coord, int>> spawns;

    /// @return 0 if the operation is successful, 1 otherwise.
    int changeTile(Coord pos, Tile tile);

    /// Set attributes of a spawn.
    /// @param team The team preference of the spawn.
    /// @return 0 if the operation is successful, 1 otherwise.
    int setSpawn(Coord pos, unsigned team);

    /// Get the team preference of a spawn.
    /// @return The team preference of the spawn. -1 if failed.
    int getSpawnTeam(Coord pos);
};

#endif  // LGEN_GAMEENGINE_BOARD_H
