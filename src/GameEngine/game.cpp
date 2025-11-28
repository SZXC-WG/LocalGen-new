/**
 * @file game.cpp
 *
 * LocalGen Module: GameEngine
 *
 * Games
 */

#ifndef LGEN_MODULE_GE_GAME_CPP
#define LGEN_MODULE_GE_GAME_CPP

#include "game.h"

#include <algorithm>
#include <random>

#include "board.h"
#include "move.h"
#include "player.h"

namespace game {

BasicGame::GameBoard::GameBoard(BasicGame* _game) : game(_game) {}
BasicGame::GameBoard::GameBoard(BasicGame* _game, Board* _board)
    : game(_game),
      Board(_board),
      visibility(_game->players.size(),
                 std::vector(_board->getHeight() + 2,
                             std::vector(_board->getWidth() + 2, 0))) {}

bool BasicGame::GameBoard::visible(pos_t row, pos_t col, index_t player) const {
    // Use the parent class's visible function temporarily.
    return Board::visible(row, col, player);
}

void BasicGame::GameBoard::update(turn_t turn) {
    for (auto& row : tiles) {
        for (auto& tile : row) {
            if (tile.occupier == 0) {
                continue;
            }
            // increments
            if (game->increment[tile.type] > 0)
                tile.army += (turn % game->increment[tile.type] == 0);
            else if (game->increment[tile.type] < 0)
                tile.army += game->increment[tile.type];
            // decrements
            if (game->decrement[tile.type] > 0)
                tile.army -= (turn % game->decrement[tile.type] == 0);
            else if (game->decrement[tile.type] < 0)
                tile.army -= game->decrement[tile.type];
            // lost tiles
            if (tile.army == 0) {
                if (tile.type == TILE_SWAMP) tile.occupier = 0;
            }
            if (tile.army < 0) {
                tile.occupier = 0;
                tile.army = 0;
            }
        }
    }
}

BasicGame::GameBoard::MoveProcessor::MoveProcessor(BasicGame* _game,
                                                   GameBoard* _board)
    : game(_game), board(_board) {}

void BasicGame::GameBoard::MoveProcessor::add(index_t player, Move move) {
    if (board->available(player, move)) movesInQueue.emplace_back(player, move);
}

void BasicGame::GameBoard::MoveProcessor::sort() {}

void BasicGame::GameBoard::MoveProcessor::capture(index_t p1, index_t p2) {
    game->alive[p2] = false;
    for (auto& row : board->tiles) {
        for (auto& tile : row) {
            if (tile.occupier == p2) {
                tile.occupier = p1;
                if (tile.type == TILE_GENERAL) tile.type = TILE_CITY;
            }
        }
    }
    game->broadcast(game->curTurn, GameMessageType::CAPTURE, {p1, p2});
}
void BasicGame::GameBoard::MoveProcessor::execute() {
    for (auto [player, move] : movesInQueue) {
        if (!board->available(player, move)) continue;
        if (move.type == MoveType::MOVE_ARMY) {
            Tile& fromTile = board->tileAt(move.from);
            Tile& toTile = board->tileAt(move.to);

            army_t takenArmy =
                move.takeHalf ? (fromTile.army >> 1) : (fromTile.army - 1);

            fromTile.army -= takenArmy;
            if (game->inSameTeam(toTile.occupier, player)) {
                toTile.occupier = player;
                toTile.army += takenArmy;
            } else {
                toTile.army -= takenArmy;
                if (toTile.army < 0) {
                    toTile.army = -toTile.army;
                    if (toTile.type == TILE_GENERAL)
                        capture(player, toTile.occupier);
                    toTile.occupier = player;
                }
            }
        } else if (move.type == MoveType::SURRENDER) {
            game->alive[player] = false;
            game->broadcast(game->curTurn, GameMessageType::SURRENDER,
                            {player});
        }
    }
}
BasicGame::BasicGame(bool remainIndex, std::vector<Player*> _players,
                     std::vector<index_t> _teams, std::vector<std::string> name,
                     InitBoard _board)
    : initialBoard(_board),
      players(_players.size()),
      names(_players.size()),
      teams(_players.size()),
      board(this, &_board),
      alive(_players.size()),
      spawnCoord(_players.size()) {
    std::vector<index_t> randId(_players.size());
    iota(begin(randId), end(randId), 0);
    if (!remainIndex) std::shuffle(begin(randId), end(randId), rng);
    for (std::size_t i = 0; i < players.size(); ++i) {
        players[randId[i]] = _players[i];
        names[randId[i]] = name[i];
        teams[randId[i]] = _teams[i];
        indexMap[players[randId[i]]] = randId[i];
    }
}
BasicGame::~BasicGame() {
    for (auto player : players) delete player;
}

void BasicGame::update() { board.update(curTurn); }

void BasicGame::act(Player* player) {
    board.processor.add(indexMap[player], player->step());
}

void BasicGame::process() {
    board.processor.sort();
    board.processor.execute();
}

void BasicGame::performTurn() {
    for (auto player : players) act(player);
    update();
    process();
}

void BasicGame::ranklist() {
    rank.clear();
    rank.resize(players.size());
    for (auto& row : board.tiles) {
        for (auto& tile : row) {
            if (tile.occupier == 0) continue;
            rank[tile.occupier].army += tile.army;
        }
    }
}

int BasicGame::initSpawn() {
    std::mt19937 random{std::random_device()()};
    int playerCount = players.size();
    std::vector<index_t> playerSequence(players.size());
    iota(begin(playerSequence), end(playerSequence), 0);
    std::shuffle(begin(playerSequence), end(playerSequence), random);
    std::vector<Coord> spawnCandidates;
    auto assign = [&]() -> void {
        for (int i = 0; i < playerCount; ++i)
            spawnCoord[playerSequence[i]] = spawnCandidates[i];
    };
    int spawnCount = 0;
    for (int i = 1; i < board.row; ++i) {
        for (int j = 1; j < board.col; ++j) {
            if (board.tiles[i][j].type == TILE_SPAWN) {
                spawnCandidates.emplace_back(i, j);
                ++spawnCount;
            }
        }
    }
    std::shuffle(begin(spawnCandidates), end(spawnCandidates), random);
    if (spawnCount >= playerCount) return assign(), 0;
    int blankCount = 0;
    for (int i = 1; i < board.row; ++i) {
        for (int j = 1; j < board.col; ++j) {
            if (board.tiles[i][j].type == TILE_PLAIN &&
                board.tiles[i][j].army == 0) {
                spawnCandidates.emplace_back(i, j);
                ++blankCount;
            }
        }
    }
    std::shuffle(begin(spawnCandidates) + spawnCount, end(spawnCandidates),
                 random);
    if (spawnCount + blankCount >= playerCount) return assign(), 0;
    return 1;
}

int BasicGame::init() {
    int spawnReturn = initSpawn();
    if (spawnReturn != 0) return spawnReturn;
    alive = std::vector(players.size(), true);
    return 0;
}

void BasicGame::broadcast(turn_t turn, GameMessageType message,
                          std::vector<index_t> associatedList) {}

}  // namespace game

#endif  // LGEN_MODULE_GE_GAME_CPP
