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
#include <concepts>
#include <cstdint>
#include <deque>
#include <functional>
#include <numeric>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
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
    std::vector<index_t> teams;
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
    std::unordered_map<Player*, index_t> indexMap;
    std::vector<std::string> names;
    std::vector<index_t> teams;
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
    index_t getTeam(index_t player) const noexcept { return teams[player]; }
    constexpr const std::vector<index_t>& getTeams() const noexcept {
        return teams;
    }
    const std::string& getName(index_t player) const noexcept {
        return names[player];
    }
    constexpr const std::vector<std::string>& getNames() const noexcept {
        return names;
    }

    /// How many soft-vision rings Fading Smog should have grown by now.
    /// This count starts from 0, and grows by 1 every `FadingSmogInterval`
    /// half-turns. When Fading Smog is off, this function always returns 0.
    int getSmogRings() const noexcept {
        if (conf.FadingSmogInterval <= 0) return 0;
        return (static_cast<int>(curTurn) * 2 + curHalfTurnPhase) /
               conf.FadingSmogInterval;
    }

    const BoardView& view(index_t player) const {
        BoardView& playerView = playerViews.at(player);
        board.view(player, playerView);
        return playerView;
    }

    BoardView fullView() const { return board.fullView(); }

    constexpr bool inSameTeam(index_t player1, index_t player2) const noexcept {
        if (!isValidPlayer(player1) || !isValidPlayer(player2)) return false;
        return getTeam(player1) == getTeam(player2);
    }

   private:
    const GameConfig conf;

   public:
    void setEventCallback(
        std::function<void(const GameEvent&)> callback) noexcept {
        eventCallback = std::move(callback);
    }

    void broadcast(turn_t turn, const GameMessageData& messageData);
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
    BasicGame(bool remainIndex, std::vector<Player*> _players,
              std::vector<index_t> _teams, std::vector<std::string> name,
              Board _board, GameConfig config = defaultConf);
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

   protected:
    Board initialBoard;

   public:
    void setInitialBoard(Board initial) noexcept {
        initialBoard = std::move(initial);
    }
    constexpr const Board& getInitialBoard() const noexcept {
        return initialBoard;
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
                if (!conf.LeapfrogEnabled) tile.type = TILE_CAPTURED_GENERAL;
            } else if (tile.army > 1) {
                tile.army = (tile.army + 1) >> 1;
            }
        } else if (conf.LeapfrogEnabled && tile.occupier == p1 &&
                   tile.type == TILE_GENERAL) {
            // The original general becomes a city,
            // but keep it as a blank tile in others' fog.
            tile.type = TILE_CAPTURED_GENERAL;
        }
    }

    broadcast(curTurn, GameMessageCapture{p1, p2});
}

inline BasicGame::BasicGame(bool remainIndex, std::vector<Player*> _players,
                            std::vector<index_t> _teams,
                            std::vector<std::string> name, Board _board,
                            GameConfig config)
    : players(_players.size()),
      playerViews(_players.size()),
      names(_players.size()),
      teams(_players.size()),
      alive(_players.size()),
      killCount(_players.size()),
      spawnCoord(_players.size()),
      conf(config),
      board(_board),
      initialBoard(_board) {
    if (_players.empty()) {
        throw std::invalid_argument("BasicGame requires at least one player");
    }
    if (_players.size() != _teams.size() || _players.size() != name.size()) {
        throw std::invalid_argument(
            "BasicGame players/teams/names size mismatch");
    }
    if (std::ranges::find(_players, nullptr) != _players.end()) {
        throw std::invalid_argument("BasicGame received null player pointer");
    }

    std::vector<index_t> randId(_players.size());
    std::iota(randId.begin(), randId.end(), 0);
    if (!remainIndex) std::ranges::shuffle(randId, rng);
    eliminatedTurn.assign(players.size(), std::numeric_limits<turn_t>::max());
    for (std::size_t i = 0; i < players.size(); ++i) {
        players[randId[i]] = _players[i];
        names[randId[i]] = name[i];
        teams[randId[i]] = _teams[i];
        indexMap[_players[i]] = randId[i];
    }
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

    if (conf.MoveOrder == MoveOrderMode::ALTERNATING_INDEX) {
        if (curHalfTurnPhase) std::ranges::reverse(moveOrder);
    } else {  // PRIORITY
        std::vector<std::tuple<int, army_t, index_t>> priority(players.size());
        for (index_t i : moveOrder) {
            const Move& move = moves[i];
            const Tile& target = board.tileAt(move.to);
            // General < normal < defensive < chase; chase takes precedence
            // even when the fleeing enemy is on a general.
            int rank = 1;
            if (isValidPlayer(target.occupier)) {
                if (inSameTeam(i, target.occupier))
                    rank = 2;
                else if (moves[target.occupier].type == MoveType::MOVE_ARMY &&
                         moves[target.occupier].from == move.to)
                    rank = 3;
                else if (target.type == TILE_GENERAL)
                    rank = 0;
            }
            // Slippery: general < chase < normal < defensive.
            if (conf.SlipperyEnabled && rank > 0) rank = rank % 3 + 1;
            // Break ties by source army, then alternating player index.
            priority[i] = {rank, board.tileAt(move.from).army,
                           curHalfTurnPhase ? i : -i};
        }
        std::ranges::sort(moveOrder, std::ranges::greater{},
                          [&](index_t i) noexcept { return priority[i]; });
    }

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
        if (isValidPlayer(toTile.occupier) &&
            inSameTeam(toTile.occupier, player)) {
            if (toTile.type != TILE_GENERAL) {
                toTile.occupier = player;
            }
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

        index_t tMate = player;
        // TODO: implementation for teams
        // If there are players left in the team, the player's land and army
        // will be taken over by the team mate whose general is closest to
        // theirs. Here, "closest" means to have the least distance, where
        // "distance" is the length of the shortest path not passing any
        // impassable tiles defined by `isImpassableTile(type)`.
        if (tMate == player) {
            neutralize(player);
        } else {
            takeOver(tMate, player);
        }
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
    board.updateVisionCache(conf, teams, getSmogRings());

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
    spawnCoord.assign(players.size(), Coord{});

    const int playerCount = static_cast<int>(players.size());
    if (playerCount == 0) return 0;

    std::vector<index_t> unassigned(playerCount);
    std::iota(unassigned.begin(), unassigned.end(), 0);

    auto collectTiles = [&](std::predicate<const Tile&> auto predicate)
        -> std::vector<std::pair<Coord, const Tile*>> {
        std::vector<std::pair<Coord, const Tile*>> result;
        for (int i = 1; i <= board.row; ++i)
            for (int j = 1; j <= board.col; ++j)
                if (predicate(board.tileAt(i, j)))
                    result.emplace_back(Coord(i, j), &board.tileAt(i, j));
        return result;
    };

    // Phase 1: Collect spawns by label
    std::unordered_map<unsigned, std::vector<Coord>> fixedSpawns;
    std::vector<Coord> flexibleSpawns;

    auto allSpawns = collectTiles(
        [](const Tile& t) noexcept { return t.type == TILE_SPAWN; });
    for (const auto& [coord, tile] : allSpawns) {
        if (tile->spawnTeam == 0)
            flexibleSpawns.push_back(coord);
        else
            fixedSpawns[tile->spawnTeam].push_back(coord);
    }

    // Phase 2: Build random label -> team mapping
    std::unordered_set<index_t> teamSet(teams.begin(), teams.end());
    std::vector<index_t> teamList(teamSet.begin(), teamSet.end());
    auto labels = std::views::keys(fixedSpawns);
    std::vector<unsigned> labelList(labels.begin(), labels.end());

    std::ranges::shuffle(teamList, rng);
    std::ranges::shuffle(labelList, rng);

    std::unordered_map<unsigned, index_t> labelToTeam;
    const size_t mappingCount = std::min(labelList.size(), teamList.size());
    for (size_t i = 0; i < mappingCount; ++i) {
        labelToTeam[labelList[i]] = teamList[i];
    }

    // Phase 3: Group players and spawns by team
    std::unordered_map<index_t, std::vector<index_t>> teamPlayers;
    for (index_t p = 0; p < playerCount; ++p) {
        teamPlayers[teams[p]].push_back(p);
    }

    std::unordered_map<index_t, std::vector<Coord>> teamSpawns;
    for (auto& [label, spawns] : fixedSpawns) {
        auto it = labelToTeam.find(label);
        if (it == labelToTeam.end()) {
            // Label not mapped to any team, convert to flexible
            flexibleSpawns.insert(flexibleSpawns.end(),
                                  std::make_move_iterator(spawns.begin()),
                                  std::make_move_iterator(spawns.end()));
        } else {
            auto& teamSpawn = teamSpawns[it->second];
            teamSpawn.insert(teamSpawn.end(),
                             std::make_move_iterator(spawns.begin()),
                             std::make_move_iterator(spawns.end()));
        }
    }

    std::vector<Coord> excessFixedSpawns;

    // Phase 4: Assign fixed spawns per team
    for (auto& [team, spawns] : teamSpawns) {
        auto& playersInTeam = teamPlayers[team];
        std::ranges::shuffle(playersInTeam, rng);
        std::ranges::shuffle(spawns, rng);

        const size_t count = std::min(playersInTeam.size(), spawns.size());
        for (size_t i = 0; i < count; ++i) {
            index_t player = playersInTeam[i];
            spawnCoord[player] = spawns[i];
            auto it = std::ranges::find(unassigned, player);
            if (it != unassigned.end()) {
                *it = unassigned.back();
                unassigned.pop_back();
            }
        }
        // Excess spawns held separately, released only if spawnTeam==0 runs out
        for (size_t i = count; i < spawns.size(); ++i) {
            excessFixedSpawns.push_back(std::move(spawns[i]));
        }
    }

    // Phase 5a: Assign spawnTeam==0 flexible spawns first
    std::ranges::shuffle(unassigned, rng);
    std::ranges::shuffle(flexibleSpawns, rng);

    const size_t flexAssigned =
        std::min(unassigned.size(), flexibleSpawns.size());
    for (size_t i = 0; i < flexAssigned; ++i)
        spawnCoord[unassigned[i]] = flexibleSpawns[i];

    unassigned.erase(unassigned.begin(), unassigned.begin() + flexAssigned);

    // Phase 5b: If still unassigned, release excess fixed spawns as flexible
    if (!unassigned.empty() && !excessFixedSpawns.empty()) {
        std::ranges::shuffle(unassigned, rng);
        std::ranges::shuffle(excessFixedSpawns, rng);

        const size_t excessAssigned =
            std::min(unassigned.size(), excessFixedSpawns.size());
        for (size_t i = 0; i < excessAssigned; ++i)
            spawnCoord[unassigned[i]] = excessFixedSpawns[i];

        unassigned.erase(unassigned.begin(),
                         unassigned.begin() + excessAssigned);
    }

    // Phase 6: Use blank tiles if still not enough spawns
    if (!unassigned.empty()) {
        auto blankTiles = collectTiles([](const Tile& t) noexcept {
            return t.type == TILE_PLAIN && t.army == 0;
        });
        std::ranges::shuffle(blankTiles, rng);

        if (blankTiles.size() < unassigned.size()) return 1;

        for (size_t i = 0; i < unassigned.size(); ++i)
            spawnCoord[unassigned[i]] = blankTiles[i].first;
    }

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

    for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
        players[i]->setSendMessageCallback([this, i](const std::string& text) {
            this->sendPlayerMessage(i, text);
        });
        players[i]->init(
            i, GameConstantsPack{board.getHeight(), board.getWidth(),
                                 static_cast<index_t>(players.size()), teams,
                                 conf});
    }

    surrenderQueue = decltype(surrenderQueue)();

    board.updateVisionCache(conf, teams, getSmogRings());
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

inline void BasicGame::broadcast(turn_t turn,
                                 const GameMessageData& messageData) {
    GameEvent event{turn, curHalfTurnPhase, messageData};
    for (Player* player : players) {
        player->onGameEvent(event);
    }
    if (eventCallback) eventCallback(event);
}
