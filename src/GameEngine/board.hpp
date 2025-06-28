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

#include "tile.hpp"
#include "utils/coord.hpp"

class Player;

/// Game map board. Zoomable.
/// The %Board is a complex thing. Many systems are part of it (e.g. vision
/// system).
class Board {
   protected:
    pos_t row, col;
    std::vector<std::vector<Tile>> tiles;
    /// Get a tile using %Coord. This is a function for convenience.
    inline Tile& getTile(Coord coord) { return tiles[coord.x][coord.y]; }

   public:
    Board() : row(0), col(0), tiles(0, std::vector<Tile>(0)) {}
    Board(pos_t _row, pos_t _col) : row(_row), col(_col) {
        assert(row >= 0 && col >= 0);
        // board index starts at 1.
        // give 1 index space at borders for safety and convenience issues.
        tiles.resize(_row + 2, std::vector<Tile>(_col + 2, Tile()));
    }

   public:
    /// Check whether a %Coord indicating a tile position is valid.
    bool isValidCoord(Coord coord) const {
        return coord.x >= 1 && coord.x <= col && coord.y >= 1 && coord.y <= row;
    }
    /// Check whether a %Coord indicating a tile position is invalid.
    bool isInvalidCoord(Coord coord) const { return !isValidCoord(coord); }

   protected:
    /// Map coding system derived from v5.
    /// LocalGen v6 is compatible with v5, so we saved this system.

    inline intmax_t v5codingPmod(intmax_t& x) {
        intmax_t res = x & 63;  // 63 = 0b111111
        x >>= 6;
        return res;
    }

#define PMod v5codingPmod
#define CHAR_AD 48

    // Zip a map using v5 coding.
    inline std::string v5codingZip() {
        std::string strZip;
        int i, j;
        intmax_t k1 = row, k2 = col;
        strZip.push_back(PMod(k1) + CHAR_AD);
        strZip.push_back(PMod(k1) + CHAR_AD);
        strZip.push_back(PMod(k2) + CHAR_AD);
        strZip.push_back(PMod(k2) + CHAR_AD);

        for (i = 1; i <= row; i++)
            for (j = 1; j <= col; j++) {
                strZip.push_back(0 +
                                 CHAR_AD);  // The occupier has to be undefined.

                int type = 0;
                switch (tiles[i][j].type) {
                    case TILE_BLANK:       type = 0; break;
                    case TILE_SWAMP:       type = 1; break;
                    case TILE_MOUNTAIN:    type = 2; break;
                    case TILE_SPAWN:       type = 3; break;
                    case TILE_CITY:        type = 4; break;
                    case TILE_DESERT:      type = 5; break;
                    case TILE_LOOKOUT:     type = 6; break;
                    case TILE_OBSERVATORY: type = 7; break;
                    default:               break;
                }

                char ch = (type << 2) + (tiles[i][j].lit << 1);
                k1 = tiles[i][j].army;

                if (k1 < 0) {
                    k1 = -k1;
                    strZip.push_back(ch += CHAR_AD + 1);
                } else
                    strZip.push_back(ch += CHAR_AD);

                for (k2 = 1; k2 <= 8; k2++)
                    strZip.push_back(PMod(k1) + CHAR_AD);
            }
        // strZip[p] = '\0'; // not necessary
        return strZip;
    }
    /// Unzip a map with v5 coding.
    inline void v5codingUnzip(std::string strUnzip) {
        strUnzip.push_back('\0');

        int i, j, k = 4;
        int f, p = 0;

        for (; strUnzip[p] != '\0'; p++) strUnzip[p] -= CHAR_AD;

        row = (strUnzip[1] << 6) + strUnzip[0];
        col = (strUnzip[3] << 6) + strUnzip[2];
        tiles.resize(row + 2, std::vector<Tile>(col + 2, Tile()));

        for (i = 1; i <= row; i++)
            for (j = 1; j <= col; j++) {
                tiles[i][j].occupier = (strUnzip[k++], nullptr);
                bool f = strUnzip[k] & 1;
                strUnzip[k] >>= 1;
                tiles[i][j].lit = strUnzip[k] & 1;
                strUnzip[k] >>= 1;
                int type = strUnzip[k++];
                tiles[i][j].army = 0;

                switch (type) {
                    case 0:  tiles[i][j].type = TILE_BLANK; break;
                    case 1:  tiles[i][j].type = TILE_SWAMP; break;
                    case 2:  tiles[i][j].type = TILE_MOUNTAIN; break;
                    case 3:  tiles[i][j].type = TILE_SPAWN; break;
                    case 4:  tiles[i][j].type = TILE_CITY; break;
                    case 5:  tiles[i][j].type = TILE_DESERT; break;
                    case 6:  tiles[i][j].type = TILE_LOOKOUT; break;
                    case 7:  tiles[i][j].type = TILE_OBSERVATORY; break;
                    default: break;
                }

                for (p = 7; p >= 0; p--)
                    tiles[i][j].army =
                        (tiles[i][j].army << 6) + strUnzip[k + p];
                k += 8;
                tiles[i][j].army = f ? (-tiles[i][j].army) : tiles[i][j].army;
            }
    }

#undef PMod
#undef CHAR_AD

   public:
    /// Check whether the %Tile at (x,y) is visible to a %Player.
    /// Declare this function as *virtual* for we'll declare a different
    /// function in-game.
    virtual bool visible(pos_t x, pos_t y, Player* player) const {
        // invalidity check
        if (player == nullptr) return false;
        if (x < 1 || x > col || y < 1 || y > row) return false;

        // occupier visibility
        if (inSameTeam(tiles[x][y].occupier, player)) return true;
        // adjacent visibility
        if (true &&  // overall check may not consider modifiers
            (inSameTeam(tiles[x - 1][y].occupier, player) ||
             inSameTeam(tiles[x + 1][y].occupier, player) ||
             inSameTeam(tiles[x][y - 1].occupier, player) ||
             inSameTeam(tiles[x][y + 1].occupier, player) ||
             inSameTeam(tiles[x - 1][y - 1].occupier, player) ||
             inSameTeam(tiles[x - 1][y + 1].occupier, player) ||
             inSameTeam(tiles[x + 1][y - 1].occupier, player) ||
             inSameTeam(tiles[x + 1][y + 1].occupier, player)))
            return true;

        // lookout check
        auto findLookoutOccupier = [&](pos_t x, pos_t y) {
            Player* occupier = nullptr;
            army_t maxArmy = 0;
            if (tiles[x - 1][y].army > maxArmy)
                occupier = tiles[x - 1][y].occupier,
                maxArmy = tiles[x - 1][y].army;
            if (tiles[x + 1][y].army > maxArmy)
                occupier = tiles[x + 1][y].occupier,
                maxArmy = tiles[x + 1][y].army;
            if (tiles[x][y - 1].army > maxArmy)
                occupier = tiles[x][y - 1].occupier,
                maxArmy = tiles[x][y - 1].army;
            if (tiles[x][y + 1].army > maxArmy)
                occupier = tiles[x][y + 1].occupier,
                maxArmy = tiles[x][y + 1].army;
            if (tiles[x - 1][y - 1].army > maxArmy)
                occupier = tiles[x - 1][y].occupier,
                maxArmy = tiles[x - 1][y].army;
            if (tiles[x - 1][y + 1].army > maxArmy)
                occupier = tiles[x - 1][y].occupier,
                maxArmy = tiles[x - 1][y].army;
            if (tiles[x + 1][y - 1].army > maxArmy)
                occupier = tiles[x - 1][y].occupier,
                maxArmy = tiles[x - 1][y].army;
            if (tiles[x + 1][y + 1].army > maxArmy)
                occupier = tiles[x - 1][y].occupier,
                maxArmy = tiles[x - 1][y].army;
            return occupier;
        };
        for (pos_t i = std::max(x - 2, 1); i <= std::min(x + 2, row); ++i) {
            for (pos_t j = std::max(y - 2, 1); j <= std::min(y + 2, col); ++j) {
                if (tiles[i][j].type == TILE_LOOKOUT)
                    if (findLookoutOccupier(i, j) == player) return true;
            }
        }

        // observatory check
        for (pos_t i = std::max(x - 8, 1); i < x; ++i) {
            if (tiles[i][y].type == TILE_OBSERVATORY)
                if (tiles[i - 1][y].occupier == player) return true;
        }
        for (pos_t i = x + 1; i <= std::min(x + 8, row); ++i) {
            if (tiles[i][y].type == TILE_OBSERVATORY)
                if (tiles[i + 1][y].occupier == player) return true;
        }
        for (pos_t j = std::max(y - 8, 1); j < y; ++j) {
            if (tiles[x][j].type == TILE_OBSERVATORY)
                if (tiles[x][j - 1].occupier == player) return true;
        }
        for (pos_t j = y + 1; j <= std::min(y + 8, col); ++j) {
            if (tiles[x][j].type == TILE_OBSERVATORY)
                if (tiles[x][j + 1].occupier == player) return true;
        }

        // none matches, not visible
        return false;
    }
    /// Same as above, but using %Coord.
    /// This is not declared as *virtual* for redefining it is unnecessary.
    bool visible(const Coord& coord, Player* player) const {
        return visible(SEPA(coord), player);
    }

   public:
    /// A view for a %Board, accessible to a player.
    /// Declared as class for it may access direct information of the %Board.
    class BoardView {
       public:
        /// Here these are declared as public as this is only a view and
        /// contains no content associated to the original %Board directly.
        pos_t row, col;
        std::vector<std::vector<TileView>> tiles;

       public:
        BoardView() : row(0), col(0) {}
        /// Constructor for generating a %BoardView using a %Board and a
        /// %Player. Leaving this function public is safe, for a %Player cannot
        /// get another %Player's address.
        BoardView(const Board* const& board, Player* player)
            : row(board->row), col(board->col) {
            tiles.resize(board->row + 2, std::vector<TileView>(board->col + 2));
            for (pos_t i = 1; i <= row; ++i) {
                for (pos_t j = 1; j <= col; ++j) {
                    tiles[i][j] = TileView(board->tiles[i][j],
                                           board->visible(i, j, player));
                }
            }
        }
    };

   public:
    /// Give a player a view of the %Board.
    /// Returns a %BoardView.
    /// Leaving this function public is safe, for a %Player cannot get another
    /// %Player's address.
    BoardView view(Player* player) const {
        // Simply use the constructor to generate.
        return BoardView(this, player);
    }

    /// Give a player a view of a certain tile of the %Board.
    /// Returns a %TileView.
    /// Leaving this function public is safe, for a %Player cannot get another
    /// %Player's address.
    TileView view(Player* player, pos_t row, pos_t col) {
        return TileView(tiles[row][col], visible(row, col, player));
    }
    /// Give a player a view of a certain tile of the %Board.
    /// Returns a %TileView.
    /// Leaving this function public is safe, for a %Player cannot get another
    /// %Player's address.
    TileView view(Player* player, Coord pos) {
        return TileView(tiles[pos.x][pos.y], visible(pos, player));
    }

   public:
    /// Move Container.
    /// A %Move is a basic operation of a player in a game.
    /// Basically, it consists of two coordinates, representing the `from` and
    /// `to`. Note that players shouldn't access other players' moves in game.
    class Move {
       public:
        Player* player;  // whether we should use direct pointer or its index is
                         // still unknown.
        Coord from, to;
        bool takeHalf;

       public:
        Move() : player(nullptr) {}
        Move(Player* _player, Coord _from, Coord _to, bool _takeHalf)
            : player(_player), from(_from), to(_to), takeHalf(_takeHalf) {}

        /// Check whether a %Move is available on a certain Board.
        bool available(Board* board) {
            // coordinate validity check
            if (!board->isInvalidCoord(from) || !board->isInvalidCoord(to))
                return false;

            // visibility check
            if (!board->visible(from, player)) return false;

            // %from tile availability check
            auto fromTile = board->view(player, from);
            switch (fromTile.type) {
                case TILE_MOUNTAIN:
                case TILE_LOOKOUT:
                case TILE_OBSERVATORY: return false;
            }
            if (fromTile.occupier != player->index) return false;
            if (fromTile.army <= 1) return false;

            // %to tile availability check
            auto toTile = board->view(player, to);
            switch (toTile.type) {
                case TILE_MOUNTAIN:
                case TILE_LOOKOUT:
                case TILE_OBSERVATORY: return false;
            }

            // all passed, available move
            return true;
        }
    };
};

/// Declare alias for convenience.
using BoardView = Board::BoardView;
using Move = Board::Move;

/// Comparisons of (%Move)s. Defined to make usage of comparison-based
/// containers like std::set and std::map more convenient.
bool operator<(const Board::Move& a, const Board::Move& b) {
    return (a.from != b.from ? a.from < b.from : a.to < b.to);
}
bool operator>(const Board::Move& a, const Board::Move& b) { return b < a; }
bool operator<=(const Board::Move& a, const Board::Move& b) { return !(b < a); }
bool operator>=(const Board::Move& a, const Board::Move& b) { return !(a < b); }

#endif  // LGEN_MODULE_GE_BOARD
