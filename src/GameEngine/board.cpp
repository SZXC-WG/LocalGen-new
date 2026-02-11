/**
 * @file board.cpp
 *
 * LocalGen Module: GameEngine
 *
 * Map Boards
 */

#ifndef LGEN_GAMEENGINE_BOARD_CPP
#define LGEN_GAMEENGINE_BOARD_CPP

#include "board.h"

#include <algorithm>
#include <cassert>

Tile& Board::tileAt(pos_t x, pos_t y) {
    assert(isValidPos(x, y));
    return tiles[x][y];
}
Tile& Board::tileAt(Coord pos) { return tileAt(pos.x, pos.y); }
int Board::setWidth(pos_t _col) {
    if (_col < 0) return 1;
    col = _col;
    for (int i = 0; i < row + 2; i++) tiles[i].resize(_col + 2);
    return 0;
}
int Board::setHeight(pos_t _row) {
    if (_row < 0) return 1;
    row = _row;
    tiles.resize(_row + 2, std::vector<Tile>(col + 2, Tile()));
    return 0;
}

Tile Board::getTile(pos_t x, pos_t y) const {
    assert(isValidPos(x, y));
    return tiles[x][y];
}
Tile Board::getTile(Coord pos) const { return getTile(pos.x, pos.y); }

Board::Board() : row(0), col(0), tiles(0, std::vector<Tile>(0)) {}
Board::Board(pos_t _row, pos_t _col) : row(_row), col(_col) {
    assert(row >= 0 && col >= 0);
    // board index starts at 1.
    // give 1 index space at borders for safety and convenience issues.
    tiles.resize(_row + 2, std::vector<Tile>(_col + 2, Tile()));
}
Board::Board(Board* _board)
    : row(_board->row), col(_board->col), tiles(_board->tiles) {}

intmax_t Board::v5Pmod(intmax_t& x) {
    intmax_t res = x & 63;  // 63 = 0b111111
    x >>= 6;
    return res;
}

#define PMod v5Pmod
#define CHAR_AD 48

std::string Board::v5Zip() {
    std::string strZip;
    int i, j;
    intmax_t k1 = row, k2 = col;
    strZip.push_back(PMod(k1) + CHAR_AD);
    strZip.push_back(PMod(k1) + CHAR_AD);
    strZip.push_back(PMod(k2) + CHAR_AD);
    strZip.push_back(PMod(k2) + CHAR_AD);

    for (i = 1; i <= row; i++)
        for (j = 1; j <= col; j++) {
            strZip.push_back(tiles[i][j].occupier + CHAR_AD);

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
void Board::v5Unzip(std::string strUnzip) {
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

bool Board::visible(pos_t x, pos_t y, index_t player) const {
    // invalidity check
    if (x < 1 || x > col || y < 1 || y > row) return false;

    // occupier visibility
    if (tiles[x][y].occupier == player) return true;
    // adjacent visibility
    if (true &&  // overall check may not consider modifiers
        (tiles[x - 1][y].occupier == player ||
         tiles[x + 1][y].occupier == player ||
         tiles[x][y - 1].occupier == player ||
         tiles[x][y + 1].occupier == player ||
         tiles[x - 1][y - 1].occupier == player ||
         tiles[x - 1][y + 1].occupier == player ||
         tiles[x + 1][y - 1].occupier == player ||
         tiles[x + 1][y + 1].occupier == player))
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
bool Board::visible(const Coord& pos, index_t player) const {
    return visible(pos.x, pos.y, player);
}

bool Board::available(index_t player, Move move) {
    MoveType& type = move.type;
    if (type == MoveType::EMPTY) return false;
    if (type == MoveType::SURRENDER) return true;
    Coord& from = move.from;
    Coord& to = move.to;
    if (type == MoveType::MOVE_ARMY) {
        // coordinate validity check
        if (isInvalidPos(from) || isInvalidPos(to)) return false;

        // %from tile availability check
        auto fromTile = view(player, from);
        switch (fromTile.type) {
            case TILE_MOUNTAIN:
            case TILE_LOOKOUT:
            case TILE_OBSERVATORY: return false;
        }
        if (fromTile.occupier != player) return false;
        if (fromTile.army <= 1) return false;

        // %to tile availability check
        auto toTile = view(player, to);
        switch (toTile.type) {
            case TILE_MOUNTAIN:
            case TILE_LOOKOUT:
            case TILE_OBSERVATORY: return false;
        }

        // all passed, available move
        return true;
    }
    return false;
}

BoardView Board::view(index_t player) const {
    // Simply use the constructor to generate it.
    return BoardView(this, player);
}

TileView Board::view(index_t player, pos_t row, pos_t col) {
    return TileView(tiles[row][col], visible(row, col, player));
}
TileView Board::view(index_t player, Coord pos) {
    return TileView(tileAt(pos), visible(pos, player));
}

TileView& BoardView::tileAt(pos_t x, pos_t y) {
    assert(isValidPos(x, y));
    return tiles[x][y];
}
TileView& BoardView::tileAt(Coord pos) { return tileAt(pos.x, pos.y); }

BoardView::BoardView() : row(0), col(0) {}
BoardView::BoardView(const Board* const& board, index_t player)
    : row(board->row), col(board->col) {
    tiles.resize(board->row + 2, std::vector<TileView>(board->col + 2));
    for (pos_t i = 1; i <= row; ++i) {
        for (pos_t j = 1; j <= col; ++j) {
            tiles[i][j].updateFrom(board->tiles[i][j],
                                   board->visible(i, j, player));
        }
    }
}

InitBoard::InitBoard() {}
InitBoard::InitBoard(pos_t row, pos_t col) : Board(row, col) {}

int InitBoard::changeTile(Coord pos, Tile tile) {
    if (isInvalidPos(pos)) return 1;
    if (tile.type == TILE_SPAWN) {
        return setSpawn(pos, 0);
    }
    tileAt(pos) = tile;
    return 0;
}

int InitBoard::setSpawn(Coord pos, unsigned team) {
    if (isInvalidPos(pos)) return 1;
    if (tileAt(pos).type == TILE_SPAWN) {
        spawns[std::lower_bound(begin(spawns), end(spawns), std::pair(pos, 0)) -
               begin(spawns)]
            .second = team;
        return 0;
    }
    tileAt(pos).type = TILE_SPAWN;
    spawns.emplace_back(pos, team);
    std::sort(begin(spawns), end(spawns));
    return 0;
}

int InitBoard::getSpawnTeam(Coord pos) {
    if (isInvalidPos(pos)) return -1;
    if (tileAt(pos).type != TILE_SPAWN) return -1;
    return std::lower_bound(begin(spawns), end(spawns), std::pair(pos, 0))
        ->second;
}

#endif  // LGEN_GAMEENGINE_BOARD_CPP
