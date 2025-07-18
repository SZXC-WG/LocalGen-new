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

#include "../utils/tools.h"
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

bool BasicGame::GameBoard::visible(pos_t row, pos_t col, index_t player) {
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

void BasicGame::GameBoard::MoveProcessor::add(Move move) {
    if (move.available(board)) movesInQueue.emplace_back(move);
}

void BasicGame::GameBoard::MoveProcessor::sort() {}

void BasicGame::GameBoard::MoveProcessor::capture(index_t p1, index_t p2) {
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
    for (auto move : movesInQueue) {
        if (!move.available(board)) continue;
        if (move.type == MoveType::MOVE_ARMY) {
            Tile& fromTile = board->tileAt(move.from);
            Tile& toTile = board->tileAt(move.to);
            army_t takenArmy = fromTile.army;
            if (move.takeHalf) takenArmy >>= 1;
            fromTile.army -= takenArmy;
            toTile.army -= takenArmy;
            if (toTile.army < 0) {
                toTile.army = -toTile.army;
                if (toTile.type == TILE_GENERAL)
                    capture(move.player->index, toTile.occupier);
                toTile.occupier = move.player->index;
            }
        } else if (move.type == MoveType::SURRENDER) {
            game->alive[move.player->index] = false;
            game->broadcast(game->curTurn, GameMessageType::SURRENDER,
                            {move.player->index});
        }
    }
}
BasicGame::BasicGame(std::vector<Player*> _players, InitBoard _board,
                     speed_t _speed)
    : players(_players),
      initialBoard(_board),
      board(this, &_board),
      alive(_players.size()),
      speed(_speed) {
    for (decltype(_players)::size_type i = 0; i < _players.size(); ++i) {
        _players[i]->index = i + 1;
    }
}

void BasicGame::update() { board.update(curTurn); }

void BasicGame::act(Player* player) {
    while (!player->moveQueue.empty() &&
           !player->moveQueue.front().available(&board))
        player->moveQueue.pop_front();
    if (player->moveQueue.empty()) return;
    board.processor.add(player->moveQueue.front());
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
    iota(begin(playerSequence), end(playerSequence), PLAYER_INDEX_START);
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
