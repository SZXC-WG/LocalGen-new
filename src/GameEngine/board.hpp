/**
 * @file board.hpp
 * 
 * LocalGen Module: GameEngine
 * 
 * Map Boards
 * 
 * Container of maps.
 */

#ifndef LGEN_MODULE_GE_BOARD
#define LGEN_MODULE_GE_BOARD 1

#include <cassert>
#include <deque>
#include <map>
#include <unordered_map>
#include <vector>

#include "utils/coord.hpp"

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
        // invalidity check
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

   public:
    /// Move Container.
    /// A %Move is a basic operation of a player in a game.
    /// Basically, it consists of two coordinates, representing the `from` and `to`.
    /// Note that players shouldn't access other players' moves in game.
    class Move {
       public:
        Player* player;  // whether we should use direct pointer or its index is still unknown.
        Coord from, to;
        army_t taken_army;

       public:
        Move() :
            player(nullptr) {}
        Move(Player* _player, Coord _from, Coord _to, army_t _taken_army) :
            player(_player), from(_from), to(_to), taken_army(_taken_army) {}

        /// Check whether a %Move is available on a certain Board.
        bool available(Board* board) {
            if(!board->is_invalid_coord(from) || !board->is_invalid_coord(to)) return false;

            if(!board->visible(from, player) || !board->visible(to, player)) return false;

            auto fromTile = board->view(player, from);
            switch(fromTile.type) {
                case TILE_MOUNTAIN:
                case TILE_LOOKOUT:
                case TILE_OBSERVATORY:
                    return false;
            }
            if(fromTile.occupier != player->index) return false;
            if(fromTile.army <= taken_army) return false;

            auto toTile = board->view(player, to);
            switch(toTile.type) {
                case TILE_MOUNTAIN:
                case TILE_LOOKOUT:
                case TILE_OBSERVATORY:
                    return false;
            }

            return true;
        }
    };

   public:
    class MoveProcessor;
    friend class Board::MoveProcessor;
    /// Move Processor used by games.
    /// A %MoveProcessor is used to contain and process moves.
    class MoveProcessor {
       protected:
        std::deque<Move> in_queue_raw_moves;
        std::unordered_map<pos_t, Move> edge_map;

       protected:
        Board* board;

        /// Constructors.
        /// The default constructor is deleted, for a %MoveProcessor must be linked to a %Board.
       public:
        MoveProcessor(Board* _board) :
            board(_board) {}

       public:
        /// Add a %Move to the waiting-to-be-processed queue.
        inline void add_move(Move move) {
            if(move.available(board))
                in_queue_raw_moves.emplace_back(move);
        }
        /// Convert all %Moves in the move buffer to edges and move them into the edge buffer.
        inline void convert_to_edge() {
            while(!in_queue_raw_moves.empty()) {
                Move cur_move = in_queue_raw_moves.front();
                in_queue_raw_moves.pop_front();
                pos_t edge_index = biindex(cur_move.from, cur_move.to);
                pos_t edge_rev_index = biindex(cur_move.to, cur_move.from);
                if(edge_map.find(edge_rev_index) != edge_map.end()) {
                    Move rev_move = edge_map[edge_rev_index];
                    edge_map.erase(edge_rev_index);
                    if(rev_move.taken_army == cur_move.taken_army) continue;
                    if(rev_move.taken_army > cur_move.taken_army) std::swap(cur_move, rev_move), std::swap(edge_index, edge_rev_index);
                    cur_move.taken_army -= rev_move.taken_army;
                }
                edge_map[edge_index] = cur_move;
            }
        }
    };

   protected:
    MoveProcessor move_processor{ this };
};

/// Declare alias for convenience.
using BoardView = Board::BoardView;

/// Comparisons of (%Move)s. Defined to make usage of comparison-based containers like std::set and std::map more convenient.
bool operator<(const Board::Move& a, const Board::Move& b) { return (a.from != b.from ? a.from < b.from : a.to < b.to); }
bool operator>(const Board::Move& a, const Board::Move& b) { return b < a; }
bool operator<=(const Board::Move& a, const Board::Move& b) { return !(b < a); }
bool operator>=(const Board::Move& a, const Board::Move& b) { return !(a < b); }

#endif  // LGEN_MODULE_GE_BOARD
