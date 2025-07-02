/**
 * @file board.cpp
 *
 * LocalGen Module: GameEngine
 *
 * Map Boards
 */

#ifndef LGEN_MODULE_GE_BOARD_CPP
#define LGEN_MODULE_GE_BOARD_CPP 1

#include "board.h"

Tile& Board::getTile(Coord coord) { return tiles[coord.x][coord.y]; }

Board::Board() : row(0), col(0), tiles(0, std::vector<Tile>(0)) {}
Board::Board(pos_t _row, pos_t _col) : row(_row), col(_col) {
    assert(row >= 0 && col >= 0);
    // board index starts at 1.
    // give 1 index space at borders for safety and convenience issues.
    tiles.resize(_row + 2, std::vector<Tile>(_col + 2, Tile()));
}

bool Board::isValidCoord(Coord coord) const {
    return coord.x >= 1 && coord.x <= col && coord.y >= 1 && coord.y <= row;
}
bool Board::isInvalidCoord(Coord coord) const { return !isValidCoord(coord); }

inline intmax_t Board::v5codingPmod(intmax_t& x) {
    intmax_t res = x & 63;  // 63 = 0b111111
    x >>= 6;
    return res;
}

#define PMod v5codingPmod
#define CHAR_AD 48

inline std::string Board::v5codingZip() {
    std::string strZip;
    int i, j;
    intmax_t k1 = row, k2 = col;
    strZip.push_back(PMod(k1) + CHAR_AD);
    strZip.push_back(PMod(k1) + CHAR_AD);
    strZip.push_back(PMod(k2) + CHAR_AD);
    strZip.push_back(PMod(k2) + CHAR_AD);

    for (i = 1; i <= row; i++)
        for (j = 1; j <= col; j++) {
            strZip.push_back(0 + CHAR_AD);  // The occupier has to be undefined.

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

            for (k2 = 1; k2 <= 8; k2++) strZip.push_back(PMod(k1) + CHAR_AD);
        }
    // strZip[p] = '\0'; // not necessary
    return strZip;
}
inline void Board::v5codingUnzip(std::string strUnzip) {
    strUnzip.push_back('\0');

    int i, j, k = 4;
    int f, p = 0;

    for (; strUnzip[p] != '\0'; p++) strUnzip[p] -= CHAR_AD;

    row = (strUnzip[1] << 6) + strUnzip[0];
    col = (strUnzip[3] << 6) + strUnzip[2];
    tiles.resize(row + 2, std::vector<Tile>(col + 2, Tile()));

    for (i = 1; i <= row; i++)
        for (j = 1; j <= col; j++) {
            tiles[i][j].occupier = strUnzip[k++];
            if (tiles[i][j].occupier == 0) tiles[i][j].occupier = -1;
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
                tiles[i][j].army = (tiles[i][j].army << 6) + strUnzip[k + p];
            k += 8;
            tiles[i][j].army = f ? (-tiles[i][j].army) : tiles[i][j].army;
        }
}

#undef PMod
#undef CHAR_AD

bool Board::visible(pos_t x, pos_t y, Player* player) const {
    // invalidity check
    if (player == nullptr) return false;
    if (x < 1 || x > col || y < 1 || y > row) return false;

    // occupier visibility
    if (tiles[x][y].occupier == player->index) return true;
    // adjacent visibility
    if (true &&  // overall check may not consider modifiers
        (tiles[x - 1][y].occupier == player->index ||
         tiles[x + 1][y].occupier == player->index ||
         tiles[x][y - 1].occupier == player->index ||
         tiles[x][y + 1].occupier == player->index ||
         tiles[x - 1][y - 1].occupier == player->index ||
         tiles[x - 1][y + 1].occupier == player->index ||
         tiles[x + 1][y - 1].occupier == player->index ||
         tiles[x + 1][y + 1].occupier == player->index))
        return true;

    // lookout check
    auto findLookoutOccupier = [&](pos_t x, pos_t y) {
        index_t occupier = -1;
        army_t maxArmy = 0;
        if (tiles[x - 1][y].army > maxArmy)
            occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
        if (tiles[x + 1][y].army > maxArmy)
            occupier = tiles[x + 1][y].occupier, maxArmy = tiles[x + 1][y].army;
        if (tiles[x][y - 1].army > maxArmy)
            occupier = tiles[x][y - 1].occupier, maxArmy = tiles[x][y - 1].army;
        if (tiles[x][y + 1].army > maxArmy)
            occupier = tiles[x][y + 1].occupier, maxArmy = tiles[x][y + 1].army;
        if (tiles[x - 1][y - 1].army > maxArmy)
            occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
        if (tiles[x - 1][y + 1].army > maxArmy)
            occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
        if (tiles[x + 1][y - 1].army > maxArmy)
            occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
        if (tiles[x + 1][y + 1].army > maxArmy)
            occupier = tiles[x - 1][y].occupier, maxArmy = tiles[x - 1][y].army;
        return occupier;
    };
    for (pos_t i = std::max(x - 2, 1); i <= std::min(x + 2, row); ++i) {
        for (pos_t j = std::max(y - 2, 1); j <= std::min(y + 2, col); ++j) {
            if (tiles[i][j].type == TILE_LOOKOUT)
                if (findLookoutOccupier(i, j) == player->index) return true;
        }
    }

    // observatory check
    for (pos_t i = std::max(x - 8, 1); i < x; ++i) {
        if (tiles[i][y].type == TILE_OBSERVATORY)
            if (tiles[i - 1][y].occupier == player->index) return true;
    }
    for (pos_t i = x + 1; i <= std::min(x + 8, row); ++i) {
        if (tiles[i][y].type == TILE_OBSERVATORY)
            if (tiles[i + 1][y].occupier == player->index) return true;
    }
    for (pos_t j = std::max(y - 8, 1); j < y; ++j) {
        if (tiles[x][j].type == TILE_OBSERVATORY)
            if (tiles[x][j - 1].occupier == player->index) return true;
    }
    for (pos_t j = y + 1; j <= std::min(y + 8, col); ++j) {
        if (tiles[x][j].type == TILE_OBSERVATORY)
            if (tiles[x][j + 1].occupier == player->index) return true;
    }

    // none matches, not visible
    return false;
}
bool Board::visible(const Coord& coord, Player* player) const {
    return visible(SEPA(coord), player);
}

BoardView Board::view(Player* player) const {
    // Simply use the constructor to generate.
    return BoardView(this, player);
}

TileView Board::view(Player* player, pos_t row, pos_t col) {
    return TileView(tiles[row][col], visible(row, col, player));
}
TileView Board::view(Player* player, Coord pos) {
    return TileView(tiles[pos.x][pos.y], visible(pos, player));
}

BoardView::BoardView() : row(0), col(0) {}
BoardView::BoardView(const Board* const& board, Player* player)
    : row(board->row), col(board->col) {
    tiles.resize(board->row + 2, std::vector<TileView>(board->col + 2));
    for (pos_t i = 1; i <= row; ++i) {
        for (pos_t j = 1; j <= col; ++j) {
            tiles[i][j] =
                TileView(board->tiles[i][j], board->visible(i, j, player));
        }
    }
}

#endif  // LGEN_MODULE_GE_BOARD_CPP
