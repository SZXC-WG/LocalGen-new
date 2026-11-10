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
 * Core game state machine, turn resolution, and player coordination.
 */

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <functional>
#include <limits>
#include <random>
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
#include "vision.hpp"

/// Immutable game parameters passed to players during initialization.
struct GameConstantsPack {
    pos_t mapHeight, mapWidth;
    index_t playerCount;
    GameConfig config;
};

/// Leaderboard entry for a single player.
struct RankItem {
    army_t army = 0;
    index_t player = -1;
    pos_t land = 0;
    int killCount = 0;
    turn_t surrLeft = static_cast<turn_t>(-1);
    bool alive = false;
};

/// Core game engine managing board state, turn execution, vision, and events.
class BasicGame {
   protected:
    // ---- Core Game State ----------------------------------------------------

    const GameConfig conf;
    Board board;
    Vision vision;
    std::mt19937 rng{std::random_device{}()};
    turn_t curTurn = 0;
    uint8_t curHalfTurnPhase = 0;

    // ---- Player State -------------------------------------------------------

    std::vector<Player*> players;
    std::vector<std::string> names;
    mutable std::vector<BoardView> playerViews;
    std::vector<bool> alive;
    std::vector<int> killCount;
    std::vector<turn_t> eliminatedTurn;

    // ---- Queues & Callbacks -------------------------------------------------

    std::deque<std::pair<index_t, turn_t>> surrenderQueue;
    std::function<void(const GameEvent&)> eventCallback;

    // ---- Turn & Player Helpers ----------------------------------------------

    constexpr turn_t curHalfTurn() const noexcept {
        return (curTurn << 1) | curHalfTurnPhase;
    }

    constexpr bool isValidPlayer(index_t player) const noexcept {
        return player >= 0 && player < static_cast<index_t>(players.size());
    }

    void eliminatePlayer(index_t player) noexcept {
        alive[player] = false;
        eliminatedTurn[player] = curHalfTurn();
    }

    // ---- Initialization & Spawn Helpers -------------------------------------

    /// Select and assign one general tile per player by priority:
    /// 1. One random spawn from each labeled team group
    /// 2. Unlabeled spawn tiles
    /// 3. Remaining tiles from labeled team groups
    /// 4. Empty plain tiles
    [[nodiscard]] bool initSpawn() {
        const std::size_t playerCount = players.size();
        std::unordered_map<unsigned, std::vector<Coord>> spawnGroups;
        std::vector<Coord> unlabelledSpawns, emptySpawns;

        board.forEachTile([&](Coord coord, const Tile& tile) {
            if (tile.type == TILE_SPAWN) {
                (tile.spawnTeam == 0 ? unlabelledSpawns
                                     : spawnGroups[tile.spawnTeam])
                    .push_back(coord);
            } else if (tile.type == TILE_PLAIN && tile.army == 0) {
                emptySpawns.push_back(coord);
            }
        });

        std::vector<Coord> primarySpawns, extraSpawns;
        for (auto& [_, group] : spawnGroups) {
            std::ranges::shuffle(group, rng);
            primarySpawns.push_back(group.front());
            extraSpawns.insert(extraSpawns.end(), group.begin() + 1,
                               group.end());
        }

        std::vector<Coord> spawns;
        spawns.reserve(playerCount);
        for (auto* pool :
             {&primarySpawns, &unlabelledSpawns, &extraSpawns, &emptySpawns}) {
            if (spawns.size() == playerCount) break;
            std::ranges::shuffle(*pool, rng);
            const std::size_t needed =
                std::min(playerCount - spawns.size(), pool->size());
            spawns.insert(spawns.end(), pool->begin(), pool->begin() + needed);
        }
        if (spawns.size() < playerCount) return false;

        std::ranges::shuffle(spawns, rng);
        for (Tile& tile : board.tiles()) {
            if (tile.type == TILE_SPAWN) {
                tile.type = TILE_BLANK;
                tile.army = 0;
            }
        }
        for (index_t i = 0; i < getPlayerCount(); ++i) {
            Tile& spawnTile = board.tileAt(spawns[i]);
            spawnTile.occupier = i;
            spawnTile.type = TILE_GENERAL;
            spawnTile.army = 1;
        }
        return true;
    }

    /// Broadcast a system notice listing any special tile types on the board.
    void announceSpecialTiles() {
        bool hasTile[TILE_TYPE_COUNT] = {};
        for (const Tile& tile : board.tiles()) {
            if (tile.type >= 0 && tile.type < TILE_TYPE_COUNT) {
                hasTile[tile.type] = true;
            }
        }

        static constexpr std::pair<tile_type_e, const char*> kSpecialTypes[] = {
            {TILE_SWAMP, "swamps"},
            {TILE_DESERT, "deserts"},
            {TILE_LOOKOUT, "lookouts"},
            {TILE_OBSERVATORY, "observatories"},
        };

        std::string tileList;
        std::size_t count = 0;
        for (auto [type, label] : kSpecialTypes) {
            if (!hasTile[type]) continue;
            if (count++ > 0) tileList.append(", ");
            tileList.append(label);
        }
        if (count == 1) {
            sendSystemMessage("1 special tile is enabled: " + tileList);
        } else if (count > 1) {
            sendSystemMessage(std::to_string(count) +
                              " special tiles are enabled: " + tileList);
        }
    }

    // ---- Board & Territory Updates ------------------------------------------

    /// Apply full-turn army growth and swamp attrition across all owned tiles.
    void updateBoard(bool increaseAllArmy = false) noexcept {
        for (Tile& tile : board.tiles()) {
            if (tile.occupier == -1) continue;
            switch (tile.type) {
                case TILE_DESERT:           continue;
                case TILE_CITY:
                case TILE_GENERAL:
                case TILE_CAPTURED_GENERAL: ++tile.army; break;
                case TILE_SWAMP:
                    if (tile.army > 0 && --tile.army == 0) {
                        tile.occupier = -1;
                        continue;
                    }
                    break;
                default: break;
            }
            if (increaseAllArmy) ++tile.army;
        }
    }

    /// Turn all tiles of `player` neutral and mark their general as captured.
    void neutralize(index_t player) noexcept {
        alive[player] = false;
        for (Tile& tile : board.tiles()) {
            if (tile.occupier == player) {
                tile.occupier = -1;
                if (tile.type == TILE_GENERAL) {
                    tile.type = TILE_CAPTURED_GENERAL;
                }
            }
        }
    }

    /// Handle `p1` capturing `p2`'s general: eliminate `p2`, halve `p2`'s
    /// remaining armies, transfer territory to `p1`, and broadcast the capture.
    void capture(index_t p1, index_t p2) {
        ++killCount[p1];
        eliminatePlayer(p2);

        for (Tile& tile : board.tiles()) {
            if (tile.occupier == p2) {
                tile.occupier = p1;
                if (tile.type == TILE_GENERAL) {
                    // Leapfrog: victor's general moves to the captured general,
                    // so its type remains TILE_GENERAL.
                    if (!conf.leapfrog) tile.type = TILE_CAPTURED_GENERAL;
                } else if (tile.army > 1) {
                    tile.army = (tile.army + 1) >> 1;
                }
            } else if (conf.leapfrog && tile.occupier == p1 &&
                       tile.type == TILE_GENERAL) {
                // The original general becomes a city,
                // but stays displayed as a blank tile in others' fog.
                tile.type = TILE_CAPTURED_GENERAL;
            }
        }

        broadcast(curTurn, GameMessageCapture{p1, p2});
    }

    /// Tick surrender countdowns and neutralize players whose timers expire.
    void processSurrenderQueue() noexcept {
        for (auto& [_, turnsLeft] : surrenderQueue) --turnsLeft;
        while (!surrenderQueue.empty() && surrenderQueue.front().second <= 0) {
            neutralize(surrenderQueue.front().first);
            surrenderQueue.pop_front();
        }
    }

    // ---- Move Validation, Ordering & Execution ------------------------------

    /// Check whether `move` is currently legal for `player`.
    bool isMoveAvailable(index_t player, Move move) const noexcept {
        if (move.type == MoveType::EMPTY) return false;
        if (move.type == MoveType::SURRENDER) return true;
        assert(move.type == MoveType::MOVE_ARMY);

        const Coord &from = move.from, &to = move.to;
        if (board.isInvalidPos(from) || board.isInvalidPos(to)) return false;
        if (std::abs(from.x - to.x) + std::abs(from.y - to.y) != 1)
            return false;

        const Tile& fromTile = board.tileAt(from);
        if (fromTile.occupier != player || fromTile.army <= 1) return false;
        return !isImpassableTile(board.tileAt(to).type);
    }

    /// Compute move execution priority: `(categoryRank, sourceArmy, tieBreak)`.
    std::tuple<int, army_t, index_t> movePriority(
        index_t player, const std::vector<Move>& moves) const noexcept {
        const Move& move = moves[player];
        const Tile& target = board.tileAt(move.to);

        // Default: general (0) < normal (1) < defensive (2) < chase (3).
        // Chase takes precedence even when the fleeing enemy is on a general.
        int rank = 1;
        if (target.occupier == player) {
            rank = 2;
        } else if (isValidPlayer(target.occupier)) {
            if (moves[target.occupier].type == MoveType::MOVE_ARMY &&
                moves[target.occupier].from == move.to) {
                rank = 3;
            } else if (target.type == TILE_GENERAL) {
                rank = 0;
            }
        }
        // Slippery: general (0) < chase (1) < normal (2) < defensive (3).
        if (conf.slippery && rank > 0) rank = rank % 3 + 1;

        return {rank, board.tileAt(move.from).army,
                curHalfTurnPhase ? player : -player};
    }

    /// Execute a single army move for `player` if still valid.
    void executeMove(index_t player, const Move& move) {
        if (!alive[player] || !isMoveAvailable(player, move)) {
            return;  // Skip just-captured players or invalid moves.
        }

        Tile& fromTile = board.tileAt(move.from);
        Tile& toTile = board.tileAt(move.to);

        const army_t takenArmy =
            move.takeHalf ? (fromTile.army >> 1) : (fromTile.army - 1);
        fromTile.army -= takenArmy;

        if (toTile.occupier == player) {
            toTile.army += takenArmy;
            return;
        }

        toTile.army -= takenArmy;
        if (toTile.army < 0 ||
            (conf.defenseless && toTile.type == TILE_GENERAL)) {
            toTile.army = std::max<army_t>(0, -toTile.army);
            if (toTile.type == TILE_GENERAL) {
                capture(player, toTile.occupier);
            }
            toTile.occupier = player;
        }
    }

    /// Poll each alive player's next valid move, process surrenders, sort army
    /// moves by priority, and execute them in order.
    void collectAndExecuteMoves() {
        std::vector<Move> moves(players.size());
        std::vector<index_t> moveOrder;

        for (index_t i = 0; i < getPlayerCount(); ++i) {
            if (!alive[i]) continue;
            Move& move = moves[i];
            while ((move = players[i]->step()).type != MoveType::EMPTY &&
                   !isMoveAvailable(i, move));
            if (move.type == MoveType::MOVE_ARMY) {
                moveOrder.push_back(i);
            } else if (move.type == MoveType::SURRENDER) {
                surrenderQueue.emplace_back(i, 50);
                eliminatePlayer(i);
                broadcast(curTurn, GameMessageSurrender{i});
            }
        }

        std::ranges::sort(
            moveOrder, std::ranges::greater{},
            [&](index_t i) noexcept { return movePriority(i, moves); });

        for (index_t player : moveOrder) {
            executeMove(player, moves[player]);
        }
    }

   public:
    // ---- Construction & Initialization --------------------------------------

    BasicGame() = delete;

    BasicGame(std::vector<Player*> _players, std::vector<std::string> name,
              Board _board, GameConfig config)
        : conf(config),
          board(std::move(_board)),
          players(std::move(_players)),
          names(std::move(name)),
          playerViews(players.size(),
                      BoardView(board.getHeight(), board.getWidth())),
          alive(players.size()),
          killCount(players.size()),
          eliminatedTurn(players.size(), std::numeric_limits<turn_t>::max()) {
        if (players.empty()) {
            throw std::invalid_argument(
                "BasicGame requires at least one player");
        }
        if (players.size() != names.size()) {
            throw std::invalid_argument(
                "BasicGame players/names size mismatch");
        }
        if (std::ranges::find(players, nullptr) != players.end()) {
            throw std::invalid_argument(
                "BasicGame received null player pointer");
        }
    }

    ~BasicGame() {
        for (Player* player : players) delete player;
    }

    /// Initialize spawns, board tiles, players, and initial vision.
    /// Returns 0 on success, 1 if spawns are insufficient, 2 if config is
    /// invalid.
    [[nodiscard]] int init() {
        if (!isValidConfig(conf)) return 2;
        if (!initSpawn()) return 1;

        alive.assign(players.size(), true);
        surrenderQueue.clear();

        const GameConstantsPack constants{board.getHeight(), board.getWidth(),
                                          getPlayerCount(), conf};
        for (index_t i = 0; i < getPlayerCount(); ++i) {
            players[i]->setSendMessageCallback(
                [this, i](const std::string& text) {
                    sendPlayerMessage(i, text);
                });
            players[i]->init(i, constants);
        }

        vision.update(board, conf, getPlayerCount());
        return 0;
    }

    // ---- Simulation & Ranking -----------------------------------------------

    /// Advance the game by one half-turn.
    void step() {
        if (curTurn == 0 && curHalfTurnPhase == 0) {
            announceSpecialTiles();
        }

        collectAndExecuteMoves();
        processSurrenderQueue();

        if (getAlivePlayerCount() == 1) {
            const index_t winner = static_cast<index_t>(
                std::ranges::find(alive, true) - alive.begin());
            broadcast(curTurn, GameMessageWin{winner});
        }

        curTurn += curHalfTurnPhase;
        curHalfTurnPhase ^= 1;
        if (curHalfTurnPhase == 0) {
            updateBoard(curTurn % 25 == 0);
        }
        vision.update(board, conf, getPlayerCount(), curHalfTurn());

        const std::vector<RankItem> rank = ranklist();
        for (index_t i = 0; i < getPlayerCount(); ++i) {
            if (alive[i]) players[i]->requestMove(view(i), rank);
        }
    }

    /// Compute the current player leaderboard sorted by descending rank.
    std::vector<RankItem> ranklist() const {
        std::vector<RankItem> rank(players.size());
        for (index_t i = 0; i < getPlayerCount(); ++i) {
            rank[i].player = i;
            rank[i].killCount = killCount[i];
            rank[i].alive = alive[i];
        }

        for (auto [player, left] : surrenderQueue) {
            rank[player].surrLeft = left;
        }

        for (const Tile& tile : board.tiles()) {
            if (isValidPlayer(tile.occupier)) {
                RankItem& item = rank[tile.occupier];
                item.army += tile.army;
                ++item.land;
            }
        }

        std::ranges::sort(
            rank, std::ranges::greater{}, [&](const RankItem& r) noexcept {
                return std::tuple(r.killCount > 0, r.army,
                                  eliminatedTurn[r.player], -r.player);
            });
        return rank;
    }

    // ---- State & View Queries -----------------------------------------------

    constexpr turn_t getCurTurn() const noexcept { return curTurn; }
    constexpr uint8_t getHalfTurnPhase() const noexcept {
        return curHalfTurnPhase;
    }

    index_t getPlayerCount() const noexcept {
        return static_cast<index_t>(players.size());
    }
    index_t getAlivePlayerCount() const noexcept {
        return static_cast<index_t>(std::ranges::count(alive, true));
    }
    constexpr bool isAlive(index_t player) const noexcept {
        return isValidPlayer(player) && alive[player];
    }
    std::vector<index_t> getAlivePlayers() const {
        std::vector<index_t> res;
        for (index_t i = 0; i < getPlayerCount(); ++i) {
            if (alive[i]) res.push_back(i);
        }
        return res;
    }

    const std::string& getName(index_t player) const noexcept {
        return names[player];
    }
    constexpr const std::vector<std::string>& getNames() const noexcept {
        return names;
    }

    const BoardView& view(index_t player) const {
        BoardView& playerView = playerViews.at(player);
        vision.view(board, player, playerView);
        return playerView;
    }

    BoardView fullView() const { return Vision::fullView(board); }

    // ---- Events & Messaging -------------------------------------------------

    void setEventCallback(
        std::function<void(const GameEvent&)> callback) noexcept {
        eventCallback = std::move(callback);
    }

    void broadcast(turn_t turn, GameMessageData messageData) {
        GameEvent event{turn, curHalfTurnPhase, std::move(messageData)};
        for (Player* player : players) {
            player->onGameEvent(event);
        }
        if (eventCallback) eventCallback(event);
    }

    void sendPlayerMessage(index_t sender, std::string text) {
        auto notspace = [](unsigned char ch) noexcept {
            return std::isspace(ch) == 0;
        };
        if (isValidPlayer(sender) && std::ranges::any_of(text, notspace))
            broadcast(curTurn, GameMessageText{sender, std::move(text)});
    }

    void sendSystemMessage(std::string text) {
        broadcast(curTurn, GameMessageText{-1, std::move(text)});
    }
};
