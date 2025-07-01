/**
 * @file game.hpp
 *
 * LocalGen Module: GameEngine
 *
 * Games
 *
 * Basic game operations
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

BasicGame::InGameBoard::InGameBoard(BasicGame* _game) : game(_game) {}
BasicGame::InGameBoard::InGameBoard(BasicGame* _game, Board _board)
    : game(_game), Board(_board) {}

inline void BasicGame::InGameBoard::update(turn_t turn) {
    for (auto& row : tiles) {
        for (auto& tile : row) {
            if (tile.occupier == -1) {
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
                if (tile.type == TILE_SWAMP) tile.occupier = -1;
            }
            if (tile.army < 0) {
                tile.occupier = -1;
                tile.army = 0;
            }
        }
    }
}

BasicGame::InGameBoard::MoveProcessor::MoveProcessor(BasicGame* _game,
                                                     InGameBoard* _board)
    : game(_game), board(_board) {}

inline void BasicGame::InGameBoard::MoveProcessor::add(Move move) {
    if (move.available(board)) movesInQueue.emplace_back(move);
}

void BasicGame::InGameBoard::MoveProcessor::sort() {}

void BasicGame::InGameBoard::MoveProcessor::capture(index_t p1, index_t p2) {
    for (auto& row : board->tiles) {
        for (auto& tile : row) {
            if (tile.occupier == p2) {
                tile.occupier = p1;
                if (tile.type == TILE_GENERAL) tile.type = TILE_CITY;
            }
        }
    }
    game->broadcast(game->curTurn, GAME_MESSAGE_CAPTURE, {p1, p2});
}
void BasicGame::InGameBoard::MoveProcessor::execute() {
    for (auto move : movesInQueue) {
        if (!move.available(board)) continue;
        Tile& fromTile = board->getTile(move.from);
        Tile& toTile = board->getTile(move.to);
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
    }
}

BasicGame::BasicGame() {}
BasicGame::BasicGame(std::vector<Player*> _players, Board _board)
    : players(_players), board(this, _board), alive(_players.size()) {
    for (decltype(_players)::size_type i = 0; i < _players.size(); ++i) {
        _players[i]->index = i + 1;
    }
}

inline void BasicGame::update() { board.update(curTurn); }

inline void BasicGame::act(Player* player) {
    while (!player->moveQueue.empty() &&
           !player->moveQueue.front().available(&board))
        player->moveQueue.pop_front();
    if (player->moveQueue.empty()) return;
    board.processor.add(player->moveQueue.front());
}

inline void BasicGame::process() {
    board.processor.sort();
    board.processor.execute();
}

inline void BasicGame::performTurn() {
    for (auto player : players) act(player);
    update();
    process();
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
    if (spawnCount + blankCount >= playerCount) return assign(), 0;
    return 1;
}

void BasicGame::init() {
    initSpawn();
    alive = std::vector(players.size(), true);
}

void BasicGame::run() {}

void BasicGame::broadcast(turn_t turn, game_message_e message,
                          std::vector<index_t> associatedList) {}

#endif  // LGEN_MODULE_GE_GAME_CPP
