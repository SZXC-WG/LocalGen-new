// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file game.hpp
 *
 * LocalGen Module: core
 *
 * Games
 *
 * Core game operations
 */

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "board.hpp"
#include "game-config.hpp"
#include "message.hpp"
#include "move.hpp"
#include "player.hpp"
#include "tile.hpp"
#include "utils.hpp"

struct GameConstantsPack {
    pos_t mapHeight, mapWidth;
    index_t playerCount;
    GameConfig config;
};

struct RankItem {
    army_t army = 0;
    index_t player = -1;
    pos_t land = 0;
    int killCount = 0;
    turn_t surrLeft = static_cast<turn_t>(-1);
    bool alive = false;
};

class BasicGame {
   protected:
    std::mt19937 rng{std::random_device()()};
    turn_t curTurn{};
    uint8_t curHalfTurnPhase{};

   public:
    turn_t getCurTurn() const noexcept { return curTurn; }
    uint8_t getHalfTurnPhase() const noexcept { return curHalfTurnPhase; }

   protected:
    std::vector<Player*> players;
    mutable std::vector<BoardView> playerViews;
    std::vector<std::string> names;
    std::vector<bool> alive;
    std::vector<int> killCount;
    std::vector<turn_t> eliminatedTurn;
    std::vector<Coord> spawnCoord;

    constexpr bool isValidPlayer(index_t player) const noexcept {
        return player >= 0 && player < static_cast<index_t>(players.size());
    }

   public:
    constexpr bool isAlive(index_t player) const noexcept {
        return isValidPlayer(player) && alive[player];
    }
    index_t getPlayerCount() const noexcept {
        return static_cast<index_t>(players.size());
    }
    index_t getAlivePlayerCount() const noexcept {
        return static_cast<index_t>(std::ranges::count(alive, true));
    }
    const std::string& getName(index_t player) const noexcept {
        return names[player];
    }
    constexpr const std::vector<std::string>& getNames() const noexcept {
        return names;
    }

    /// How many soft-vision rings Fading Smog should have grown by now.
    /// This count starts from 0, and grows by 1 every 25 half-turns.
    /// When Fading Smog is off, this function always returns 0.
    int getSmogRings() const noexcept {
        if (!conf.fadingSmog) return 0;
        return (static_cast<int>(curTurn) * 2 + curHalfTurnPhase) / 25;
    }

    const BoardView& view(index_t player) const {
        BoardView& playerView = playerViews.at(player);
        board.view(player, playerView);
        return playerView;
    }

    BoardView fullView() const { return board.fullView(); }

   private:
    const GameConfig conf;

   public:
    void setEventCallback(
        std::function<void(const GameEvent&)> callback) noexcept {
        eventCallback = std::move(callback);
    }

    void broadcast(turn_t turn, GameMessageData messageData);
    void sendPlayerMessage(index_t sender, std::string text);
    void sendSystemMessage(std::string text) {
        broadcast(curTurn, GameMessageText{-1, std::move(text)});
    }

   protected:
    Board board;

    std::deque<std::pair<index_t, turn_t>> surrenderQueue;
    std::function<void(const GameEvent&)> eventCallback;

    void neutralize(index_t player) noexcept;
    void takeOver(index_t p1, index_t p2) noexcept;
    void capture(index_t p1, index_t p2);

   public:
    BasicGame() = delete;
    BasicGame(std::vector<Player*> _players, std::vector<std::string> name,
              Board _board, GameConfig config);
    ~BasicGame();

    void step();

    std::vector<RankItem> ranklist() const;

    [[nodiscard]] int initSpawn();
    [[nodiscard]] int init();

    std::vector<index_t> getAlivePlayers() const {
        std::vector<index_t> res;
        for (index_t i = 0; i < static_cast<index_t>(alive.size()); ++i) {
            if (alive[i]) res.push_back(i);
        }
        return res;
    }
};

inline void BasicGame::neutralize(index_t player) noexcept {
    takeOver(-1, player);
}
inline void BasicGame::takeOver(index_t p1, index_t p2) noexcept {
    alive[p2] = false;
    for (auto& tile : board.tiles) {
        if (tile.occupier == p2) {
            tile.occupier = p1;
            if (tile.type == TILE_GENERAL) {
                tile.type = TILE_CAPTURED_GENERAL;
            }
        }
    }
}

inline void BasicGame::capture(index_t p1, index_t p2) {
    ++killCount[p1];
    alive[p2] = false;
    eliminatedTurn[p2] = curTurn << 1 | curHalfTurnPhase;

    for (auto& tile : board.tiles) {
        if (tile.occupier == p2) {
            tile.occupier = p1;
            if (tile.type == TILE_GENERAL) {
                // Leapfrog: victor's general moves to the captured general
                // so type would still be TILE_GENERAL.
                if (!conf.leapfrog) tile.type = TILE_CAPTURED_GENERAL;
            } else if (tile.army > 1) {
                tile.army = (tile.army + 1) >> 1;
            }
        } else if (conf.leapfrog && tile.occupier == p1 &&
                   tile.type == TILE_GENERAL) {
            // The original general becomes a city,
            // but keep it as a blank tile in others' fog.
            tile.type = TILE_CAPTURED_GENERAL;
        }
    }

    broadcast(curTurn, GameMessageCapture{p1, p2});
}

inline BasicGame::BasicGame(std::vector<Player*> _players,
                            std::vector<std::string> name, Board _board,
                            GameConfig config)
    : players(std::move(_players)),
      playerViews(players.size()),
      names(std::move(name)),
      alive(players.size()),
      killCount(players.size()),
      conf(config),
      board(std::move(_board)) {
    if (players.empty()) {
        throw std::invalid_argument("BasicGame requires at least one player");
    }
    if (players.size() != names.size()) {
        throw std::invalid_argument("BasicGame players/names size mismatch");
    }
    if (std::ranges::find(players, nullptr) != players.end()) {
        throw std::invalid_argument("BasicGame received null player pointer");
    }

    eliminatedTurn.assign(players.size(), std::numeric_limits<turn_t>::max());
}

inline BasicGame::~BasicGame() {
    for (auto player : players) delete player;
}

inline void BasicGame::step() {
    // send initial notice
    if (curTurn == 0 && curHalfTurnPhase == 0) {
        bool hasTile[TILE_TYPE_COUNT] = {false};
        for (Tile& tile : board.tiles) hasTile[tile.type] = true;
        std::vector<std::string> specialTiles;
        if (hasTile[TILE_SWAMP]) specialTiles.push_back("swamps");
        if (hasTile[TILE_DESERT]) specialTiles.push_back("deserts");
        if (hasTile[TILE_LOOKOUT]) specialTiles.push_back("lookouts");
        if (hasTile[TILE_OBSERVATORY]) specialTiles.push_back("observatories");
        if (!specialTiles.empty()) {
            std::string tileList = specialTiles[0];
            for (std::size_t i = 1; i < specialTiles.size(); ++i) {
                tileList.append(", ").append(specialTiles[i]);
            }
            if (specialTiles.size() == 1) {
                sendSystemMessage("1 special tile is enabled: " + tileList);
            } else {
                sendSystemMessage(std::to_string(specialTiles.size()) +
                                  " special tiles are enabled: " + tileList);
            }
        }
    }

    // collect moves
    std::vector<Move> moves(players.size());
    std::vector<index_t> moveOrder;
    for (index_t i : getAlivePlayers()) {
        Move& move = moves[i];
        while ((move = players[i]->step()).type != MoveType::EMPTY &&
               !board.available(i, move));
        if (move.type == MoveType::MOVE_ARMY) {
            moveOrder.push_back(i);
        } else if (move.type == MoveType::SURRENDER) {
            surrenderQueue.emplace_back(i, 50);
            alive[i] = false;
            eliminatedTurn[i] = curTurn << 1 | curHalfTurnPhase;
            broadcast(curTurn, GameMessageSurrender{i});
        }
    }

    std::vector<std::tuple<int, army_t, index_t>> priority(players.size());
    for (index_t i : moveOrder) {
        const Move& move = moves[i];
        const Tile& target = board.tileAt(move.to);
        // General < normal < defensive < chase; chase takes precedence
        // even when the fleeing enemy is on a general.
        int rank = 1;
        if (isValidPlayer(target.occupier)) {
            if (target.occupier == i)
                rank = 2;
            else if (moves[target.occupier].type == MoveType::MOVE_ARMY &&
                     moves[target.occupier].from == move.to)
                rank = 3;
            else if (target.type == TILE_GENERAL)
                rank = 0;
        }
        // Slippery: general < chase < normal < defensive.
        if (conf.slippery && rank > 0) rank = rank % 3 + 1;
        // Break ties by source army, then alternating player index.
        priority[i] = {rank, board.tileAt(move.from).army,
                       curHalfTurnPhase ? i : -i};
    }
    std::ranges::sort(moveOrder, std::ranges::greater{},
                      [&](index_t i) noexcept { return priority[i]; });

    // execute moves
    for (index_t player : moveOrder) {
        const Move& move = moves[player];
        if (!alive[player] || !board.available(player, move))
            continue;  // skip just-captured players or invalid moves

        Tile& fromTile = board.tileAt(move.from);
        Tile& toTile = board.tileAt(move.to);

        army_t takenArmy =
            move.takeHalf ? (fromTile.army >> 1) : (fromTile.army - 1);

        fromTile.army -= takenArmy;
        if (toTile.occupier == player) {
            toTile.army += takenArmy;
        } else {
            toTile.army -= takenArmy;
            if (toTile.army < 0) {
                toTile.army = -toTile.army;
                if (toTile.type == TILE_GENERAL) {
                    capture(player, toTile.occupier);
                }
                toTile.occupier = player;
            }
        }
    }

    for (auto& entry : surrenderQueue) --entry.second;

    // handle afk players
    while (!surrenderQueue.empty() && surrenderQueue.front().second <= 0) {
        index_t player = surrenderQueue.front().first;
        surrenderQueue.pop_front();
        neutralize(player);
    }

    // check win condition
    std::vector<index_t> alivePlayers = getAlivePlayers();
    if (alivePlayers.size() == 1) {
        broadcast(curTurn, GameMessageWin{alivePlayers.front()});
    }

    // update board
    if (curHalfTurnPhase == 0) {
        board.update(curTurn > 0 && curTurn % 25 == 0);
    }
    curTurn += curHalfTurnPhase;
    curHalfTurnPhase ^= 1;
    board.updateVisionCache(conf, getPlayerCount(), getSmogRings());

    // request moves (for next turn)
    std::vector<RankItem> rank = ranklist();
    for (index_t i : alivePlayers) {
        players[i]->requestMove(view(i), rank);
    }
}

inline std::vector<RankItem> BasicGame::ranklist() const {
    std::vector<RankItem> rank(players.size());
    for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
        rank[i].player = i;
        rank[i].killCount = killCount[i];
        rank[i].alive = alive[i];
    }

    for (auto [player, left] : surrenderQueue) {
        rank[player].surrLeft = left;
    }

    for (auto& tile : board.tiles) {
        if (isValidPlayer(tile.occupier)) {
            RankItem& item = rank[tile.occupier];
            item.army += tile.army, ++item.land;
        }
    }

    std::ranges::sort(
        rank, std::ranges::greater{}, [&](const RankItem& r) noexcept {
            return std::tuple(r.killCount > 0, r.army, eliminatedTurn[r.player],
                              -r.player);
        });
    return rank;
}

inline int BasicGame::initSpawn() {
    const std::size_t playerCount = players.size();
    spawnCoord.clear();
    spawnCoord.reserve(playerCount);

    // 1. Collect spawn candidates
    std::unordered_map<unsigned, std::vector<Coord>> spawnGroups;
    std::vector<Coord> unlabelledSpawns;
    std::vector<Coord> emptySpawns;

    for (pos_t x = 1; x <= board.row; ++x) {
        for (pos_t y = 1; y <= board.col; ++y) {
            const Tile& tile = board.tileAt(x, y);
            const Coord coord{x, y};
            if (tile.type == TILE_SPAWN) {
                if (tile.spawnTeam == 0)
                    unlabelledSpawns.push_back(coord);
                else
                    spawnGroups[tile.spawnTeam].push_back(coord);
            } else if (tile.type == TILE_PLAIN && tile.army == 0) {
                emptySpawns.push_back(coord);
            }
        }
    }

    // 2. Sample one spawn from every group as primary, others as extra
    std::vector<Coord> primaryGroupSpawns;
    std::vector<Coord> extraGroupSpawns;
    for (auto& [_, group] : spawnGroups) {
        std::ranges::shuffle(group, rng);
        primaryGroupSpawns.push_back(group.front());
        extraGroupSpawns.insert(extraGroupSpawns.end(), group.begin() + 1,
                                group.end());
    }

    // 3. Sample N spawns by priority
    auto addSpawns = [&](std::vector<Coord>& pool) {
        if (spawnCoord.size() == playerCount) return;
        std::ranges::shuffle(pool, rng);
        const std::size_t needed =
            std::min(playerCount - spawnCoord.size(), pool.size());
        spawnCoord.insert(spawnCoord.end(), pool.begin(),
                          pool.begin() + needed);
    };

    addSpawns(primaryGroupSpawns);
    addSpawns(unlabelledSpawns);
    addSpawns(extraGroupSpawns);
    addSpawns(emptySpawns);

    if (spawnCoord.size() < playerCount) return 1;

    // 4. Assign spawns to N players
    std::ranges::shuffle(spawnCoord, rng);
    return 0;
}

inline int BasicGame::init() {
    if (const int result = initSpawn()) return result;

    for (Tile& tile : board.tiles) {
        if (tile.type == TILE_SPAWN) {
            tile.type = TILE_BLANK;
            tile.army = 0;
        }
    }

    alive.assign(players.size(), true);
    for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
        Tile& spawnTile = board.tileAt(spawnCoord[i]);
        spawnTile.occupier = i;
        spawnTile.type = TILE_GENERAL;
        spawnTile.army = 0;  // will be updated in the first turn
    }

    const GameConstantsPack constants{board.getHeight(), board.getWidth(),
                                      getPlayerCount(), conf};
    for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
        players[i]->setSendMessageCallback([this, i](const std::string& text) {
            this->sendPlayerMessage(i, text);
        });
        players[i]->init(i, constants);
    }

    surrenderQueue = decltype(surrenderQueue)();

    board.updateVisionCache(conf, getPlayerCount(), getSmogRings());
    return 0;
}

inline void BasicGame::sendPlayerMessage(index_t sender, std::string text) {
    if (!isValidPlayer(sender) || text.empty()) return;
    const auto isSpace = [](unsigned char ch) noexcept {
        return std::isspace(ch) != 0;
    };
    if (std::ranges::all_of(text, isSpace)) return;
    broadcast(curTurn, GameMessageText{sender, std::move(text)});
}

inline void BasicGame::broadcast(turn_t turn, GameMessageData messageData) {
    GameEvent event{turn, curHalfTurnPhase, std::move(messageData)};
    for (Player* player : players) {
        player->onGameEvent(event);
    }
    if (eventCallback) eventCallback(event);
}
