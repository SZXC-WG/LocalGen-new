/**
 * @file move.hpp
 * 
 * LocalGen Module: GameEngine
 * 
 * Moves
 * 
 * Moves are basic operations of players in games.
 */

#ifndef LGEN_MODULE_GE_MOVE
#define LGEN_MODULE_GE_MOVE 1

#include "utils/coord.hpp"
#include "player.hpp"
#include "board.hpp"

#include <unordered_map>

/// Move Container.
/// A %Move is a basic operation of a player in a game.
/// Basically, it consists of two coordinates, representing the `from` and `to`.
/// Note that players shouldn't access other players' moves in game.
class Move {
   private:
    Player* player;  // whether we should use direct pointer or its index is still unknown.
    Coord from, to;

   public:
    Move() :
        player(nullptr) {}
    Move(Player* _player, Coord _from, Coord _to) :
        player(_player), from(_from), to(_to) {}

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

/// Move Processor used by games.
/// A %MoveProcessor is used to contain and process moves.
class MoveProcessor {
   protected:
    std::vector<Move> in_queue_raw_moves;
    std::unordered_map<pos_t, Tile> normal_ether;  // normal two-tile ether

    // std::unordered_map<pos_t, Tile> four_tile_ether; // TODO; random-ether modifier

   public:
    Board* board;

    MoveProcessor() {}
};

#endif  // LGEN_MODULE_GE_MOVE
