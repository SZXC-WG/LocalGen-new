/**
 * @file game.hpp
 *
 * LocalGen Module: core
 *
 * Games
 *
 * Core game operations
 */

#ifndef LGEN_CORE_GAME_HPP
#define LGEN_CORE_GAME_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "board.hpp"
#include "message.hpp"
#include "move.hpp"
#include "player.hpp"
#include "tile.hpp"
#include "utils.hpp"

namespace game {

namespace config {

enum class VisionMode : uint8_t { NEAR8, NEAR4 };
enum class MoveProcessMode : uint8_t { FULL, PARITY };

#define GAME_CONFIG_UNIT_LIST(F)                        \
    F(bool, RanklistShowLand, true)                     \
    F(bool, RanklistShowArmy, true)                     \
    F(bool, RanklistShowPlayerIndex, true)              \
    F(bool, RanklistShowPlayerName, true)               \
    F(bool, RanklistShowTeamIndex, true)                \
    F(bool, RanklistShowColor, true)                    \
    F(VisionMode, OverallVisionMode, VisionMode::NEAR8) \
    F(int, OverallVisionRange, 1)                       \
    F(int, CityVisionRange, 1)                          \
    F(int, SpawnVisionRange, 1)                         \
    F(MoveProcessMode, MoveProcessMethod, MoveProcessMode::FULL)

struct Config {
#define DECL(type, name, def) type name = def;
    GAME_CONFIG_UNIT_LIST(DECL)
#undef DECL
};

#define IF_EQUAL(type, name, ...) \
    if (lhs.name != rhs.name) return false;
constexpr inline bool operator==(const Config& lhs, const Config& rhs) {
    GAME_CONFIG_UNIT_LIST(IF_EQUAL)
    return true;
}
#undef IF_EQUAL

struct ConfigPatch {
#define DECL(type, name, ...) std::optional<type> name;
    GAME_CONFIG_UNIT_LIST(DECL)
#undef DECL
};

#define IF_EQUAL(type, name, ...) \
    if (lhs.name != rhs.name) return false;
constexpr inline bool operator==(const ConfigPatch& lhs,
                                 const ConfigPatch& rhs) {
    GAME_CONFIG_UNIT_LIST(IF_EQUAL)
    return true;
}
#undef IF_EQUAL

namespace unit {
#define UNIT(type, name, ...)            \
    constexpr ConfigPatch name(type v) { \
        ConfigPatch p;                   \
        p.name = v;                      \
        return p;                        \
    }
GAME_CONFIG_UNIT_LIST(UNIT)
#undef UNIT
}  // namespace unit

#define IF_ASSIGN(type, name, ...) \
    if (rhs.name) res.name = rhs.name;
constexpr inline ConfigPatch operator|(const ConfigPatch& lhs,
                                       const ConfigPatch& rhs) {
    ConfigPatch res = lhs;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN)
    return res;
}
#undef IF_ASSIGN

#define IF_ASSIGN_VALUE(type, name, ...) \
    if (rhs.name) res.name = *rhs.name;
constexpr inline Config operator|(const Config& lhs, const ConfigPatch& rhs) {
    Config res = lhs;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN_VALUE)
    return res;
}
constexpr inline Config operator|(const ConfigPatch& lhs, const Config& rhs) {
    return rhs | lhs;
}
#undef IF_ASSIGN_VALUE

#define IF_ASSIGN_OPTIONAL(type, name, ...) \
    if (rhs.name) res.name = lhs.name;
constexpr inline ConfigPatch operator&(const Config& lhs,
                                       const ConfigPatch& rhs) {
    ConfigPatch res;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN_OPTIONAL);
    return res;
}
constexpr inline ConfigPatch operator&(const ConfigPatch& lhs,
                                       const Config& rhs) {
    return rhs & lhs;
}
#undef IF_ASSIGN_OPTIONAL

#define GAME_CONFIG_MODIFIER_LIST(F)                                    \
    F(Watchtower, unit::CityVisionRange(3) | unit::SpawnVisionRange(3)) \
    F(MistyVeil, unit::OverallVisionRange(0))

namespace modifier {
#define MODIFIER(name, value) constexpr ConfigPatch name = value;
GAME_CONFIG_MODIFIER_LIST(MODIFIER)
#undef MODIFIER
}  // namespace modifier

constexpr Config defaultConf;

enum class PatchStatus : uint8_t {
    FULLY_ENABLED,
    PARTIALLY_ENABLED,
    DISABLED,
    OVERRIDDEN
};

constexpr inline PatchStatus patchStatus(const Config& config,
                                         const ConfigPatch& patch) {
    ConfigPatch confPatch = config & patch;
    ConfigPatch defPatch = defaultConf & patch;
    if (confPatch == patch) return PatchStatus::FULLY_ENABLED;
    if (confPatch == defPatch) return PatchStatus::DISABLED;
#define IF_MIXED(type, name, ...) \
    if (confPatch.name == patch.name) return PatchStatus::PARTIALLY_ENABLED;
    GAME_CONFIG_UNIT_LIST(IF_MIXED)
#undef IF_MIXED
    return PatchStatus::OVERRIDDEN;
}

}  // namespace config

struct GameConstantsPack {
    pos_t mapHeight, mapWidth;
    index_t playerCount;
    std::vector<index_t> teams;
    config::Config config;
};

struct RankItem {
    index_t player = -1;
    army_t army = 0;
    pos_t land = 0;
    bool alive = false;
};

/// Move priority categories (based on generals.io priority system).
/// Higher values = higher priority.
enum class MovePriority : uint8_t {
    ATTACK_GENERAL = 0,  // Attacks on enemy generals (lowest priority)
    NORMAL = 1,          // Normal attack moves
    DEFENSIVE = 2,       // Friendly-to-friendly moves
    CHASE = 3            // Chasing a fleeing enemy (highest priority)
};

class BasicGame {
   protected:
    std::mt19937 rng{std::random_device()()};
    turn_t curTurn{};
    uint8_t curHalfTurnPhase{};

   public:
    inline turn_t getCurTurn() const { return curTurn; }
    inline uint8_t getHalfTurnPhase() const { return curHalfTurnPhase; }

   protected:
    std::vector<Player*> players;
    mutable std::vector<BoardView> playerViews;
    std::unordered_map<Player*, index_t> indexMap;
    std::vector<std::string> names;
    std::vector<index_t> teams;
    std::vector<bool> alive;
    // Surrender uses a three-stage lifecycle:
    // request during the current turn, take effect after that turn finishes,
    // then dissolve any remaining territory 50 full turns later.
    std::vector<bool> pendingSurrender;
    std::vector<bool> surrendered;
    std::vector<bool> surrenderResolved;
    std::vector<turn_t> surrenderEffectiveTurn;
    std::vector<Coord> spawnCoord;

    inline bool isValidPlayer(index_t player) const {
        return player >= 0 && player < static_cast<index_t>(players.size());
    }

   public:
    inline bool isAlive(index_t player) const {
        return isValidPlayer(player) && alive[player];
    }
    inline bool isSurrenderPending(index_t player) const {
        return isValidPlayer(player) && pendingSurrender[player];
    }
    inline bool isSurrendered(index_t player) const {
        return isValidPlayer(player) && surrendered[player];
    }
    inline index_t getPlayerCount() const {
        return static_cast<index_t>(players.size());
    }
    inline index_t getTeam(index_t player) const { return teams[player]; }
    inline std::vector<index_t> getTeams() const { return teams; }
    inline std::string getName(index_t player) const { return names[player]; }
    inline std::vector<std::string> getNames() const { return names; }

    inline const BoardView& view(index_t player) const {
        BoardView& playerView = playerViews.at(player);
        board.view(player, playerView);
        return playerView;
    }

    inline BoardView fullView() const { return board.fullView(); }

    inline bool inSameTeam(index_t player1, index_t player2) const {
        if (!isValidPlayer(player1) || !isValidPlayer(player2)) return false;
        return getTeam(player1) == getTeam(player2);
    }

   protected:
    config::Config conf = config::defaultConf;

   public:
    inline config::Config getConfig() const { return conf; }
    inline void setConfig(config::ConfigPatch patch) { conf = conf | patch; }

   public:
    void broadcast(turn_t turn, const GameMessageData& messageData);

   protected:
    Board board;

    void capture(index_t p1, index_t p2);
    void resolveSurrenderedTerritory(index_t player);
    void clearSurrenderState(index_t player);
    inline bool acceptsOrders(index_t player) const {
        return isAlive(player) && !pendingSurrender[player];
    }
    inline bool shouldResolveSurrender(index_t player, turn_t turn) const {
        return surrendered[player] && !surrenderResolved[player] &&
               turn >= surrenderEffectiveTurn[player] + 51;
    }

    // Move priority helper functions
    /// Check if a move is defensive (friendly-to-friendly, including
    /// teammates).
    inline bool isDefensiveMove(index_t player, const Move& move) const {
        if (move.type != MoveType::MOVE_ARMY) return false;
        const Tile& toTile = board.tileAt(move.to);
        return isValidPlayer(toTile.occupier) &&
               inSameTeam(toTile.occupier, player);
    }

    /// Check if a move is an attack on an enemy general.
    inline bool isAttackGeneral(index_t player, const Move& move) const {
        if (move.type != MoveType::MOVE_ARMY) return false;
        const Tile& toTile = board.tileAt(move.to);
        return toTile.type == TILE_GENERAL && isValidPlayer(toTile.occupier) &&
               !inSameTeam(toTile.occupier, player);
    }

    /// Check if a move is a chase (target tile's enemy occupier is moving out).
    inline bool isChaseMove(
        index_t player, const Move& move,
        const std::unordered_map<Coord, index_t>& moveOutMap) const {
        if (move.type != MoveType::MOVE_ARMY) return false;
        const Tile& toTile = board.tileAt(move.to);

        // Target tile must have an enemy occupier
        if (!isValidPlayer(toTile.occupier)) return false;
        if (inSameTeam(toTile.occupier, player)) return false;

        // That enemy must be moving out of the target tile
        auto it = moveOutMap.find(move.to);
        if (it == moveOutMap.end()) return false;

        return it->second == toTile.occupier;
    }

    /// Get the priority category of a move.
    inline MovePriority getMovePriority(
        index_t player, const Move& move,
        const std::unordered_map<Coord, index_t>& moveOutMap) const {
        if (isChaseMove(player, move, moveOutMap)) return MovePriority::CHASE;
        if (isDefensiveMove(player, move)) return MovePriority::DEFENSIVE;
        if (isAttackGeneral(player, move)) return MovePriority::ATTACK_GENERAL;
        return MovePriority::NORMAL;
    }

    /// Compare two moves by priority.
    /// Returns true if `a` should execute before `b`.
    inline bool compareMovePriority(
        const std::pair<index_t, Move>& a, const std::pair<index_t, Move>& b,
        const std::unordered_map<Coord, index_t>& moveOutMap) const {
        const MoveType& tA = a.second.type;
        const MoveType& tB = b.second.type;
        if (tA != tB)
            return static_cast<uint8_t>(tA) < static_cast<uint8_t>(tB);

        if (conf.MoveProcessMethod == config::MoveProcessMode::FULL) {
            // Priority category (higher enum value = higher priority)
            MovePriority pA = getMovePriority(a.first, a.second, moveOutMap);
            MovePriority pB = getMovePriority(b.first, b.second, moveOutMap);
            if (pA != pB)
                return static_cast<uint8_t>(pA) > static_cast<uint8_t>(pB);

            // Army size tiebreaker (larger army = higher priority)
            army_t armyA = board.tileAt(a.second.from).army;
            army_t armyB = board.tileAt(b.second.from).army;
            if (armyA != armyB) return armyA > armyB;
        }

        // Old priority (player index) as final tiebreaker
        // phase 0: ascending, phase 1: descending
        if (curHalfTurnPhase == 0) {
            return a.first < b.first;
        } else {
            return a.first > b.first;
        }
    }

   public:
    BasicGame() = delete;
    BasicGame(bool remainIndex, std::vector<Player*> _players,
              std::vector<index_t> _teams, std::vector<std::string> name,
              Board _board);
    ~BasicGame();

   public:
    void step();

   public:
    std::vector<RankItem> ranklist();

   public:
    int initSpawn();
    int init();

   public:
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
    inline void setInitialBoard(Board initial) { initialBoard = initial; }
    inline Board getInitialBoard() { return initialBoard; }

   protected:
    struct ReplayUnit {};
    std::vector<ReplayUnit> replay;
};

inline void BasicGame::capture(index_t p1, index_t p2) {
    alive[p2] = false;
    clearSurrenderState(p2);
    for (auto& tile : board.tiles) {
        if (tile.occupier == p2) {
            tile.occupier = p1;
            if (tile.type == TILE_GENERAL) {
                tile.type = TILE_CAPTURED_GENERAL;
            } else if (tile.army > 1) {
                tile.army >>= 1;
            }
        }
    }
    broadcast(curTurn, GameMessageCapture{p1, p2});
}

inline BasicGame::BasicGame(bool remainIndex, std::vector<Player*> _players,
                            std::vector<index_t> _teams,
                            std::vector<std::string> name, Board _board)
    : initialBoard(_board),
      players(_players.size()),
      playerViews(_players.size()),
      names(_players.size()),
      teams(_players.size()),
      board(_board),
      alive(_players.size()),
      pendingSurrender(_players.size(), false),
      surrendered(_players.size(), false),
      surrenderResolved(_players.size(), false),
      surrenderEffectiveTurn(_players.size(), 0),
      spawnCoord(_players.size()) {
    if (_players.empty()) {
        throw std::invalid_argument("BasicGame requires at least one player");
    }
    if (_players.size() != _teams.size() || _players.size() != name.size()) {
        throw std::invalid_argument(
            "BasicGame players/teams/names size mismatch");
    }
    for (auto* player : _players) {
        if (player == nullptr) {
            throw std::invalid_argument(
                "BasicGame received null player pointer");
        }
    }

    std::vector<index_t> randId(_players.size());
    std::iota(randId.begin(), randId.end(), 0);
    if (!remainIndex) std::shuffle(randId.begin(), randId.end(), rng);
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

inline void BasicGame::resolveSurrenderedTerritory(index_t player) {
    if (!isValidPlayer(player) || surrenderResolved[player] ||
        !surrendered[player]) {
        return;
    }

    for (auto& tile : board.tiles) {
        if (tile.occupier != player) continue;

        tile.occupier = -1;
        // Original cities stay cities, surrendered generals turn into cities,
        // and every other tile becomes neutral land while keeping its army.
        if (tile.type == TILE_GENERAL) {
            tile.type = TILE_CITY;
        } else if (tile.type != TILE_CITY) {
            tile.type = TILE_BLANK;
        }
    }

    surrenderResolved[player] = true;
}

inline void BasicGame::clearSurrenderState(index_t player) {
    if (!isValidPlayer(player)) return;

    pendingSurrender[player] = false;
    surrendered[player] = false;
    surrenderResolved[player] = false;
    surrenderEffectiveTurn[player] = 0;
}

inline void BasicGame::step() {
    const uint8_t halfTurnPhase = curHalfTurnPhase;
    const turn_t turn = curTurn;

    if (halfTurnPhase == 0) {
        // Territory cleanup happens before a new full turn starts resolving.
        for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
            if (!shouldResolveSurrender(i, turn)) continue;
            resolveSurrenderedTerritory(i);
        }
    }

    // collect moves
    std::vector<std::pair<index_t, Move>> moves;
    for (index_t i : getAlivePlayers()) {
        if (!acceptsOrders(i)) continue;
        Player* player = players[i];
        Move move;
        while ((move = player->step()).type != MoveType::EMPTY &&
               !board.available(i, move));
        if (move.type == MoveType::SURRENDER) {
            pendingSurrender[i] = true;
            continue;
        }
        if (move.type != MoveType::EMPTY) moves.emplace_back(i, move);
    }

    // build move-out map for chase detection
    // moveOutMap[coord] = player_index means that player is moving out of coord
    std::unordered_map<Coord, index_t> moveOutMap;
    for (const auto& [player, move] : moves) {
        if (move.type == MoveType::MOVE_ARMY) {
            moveOutMap[move.from] = player;
        }
    }

    // sort moves by priority system
    // Priority order (high to low):
    // 1. Chase moves (catching fleeing enemies)
    // 2. Defensive moves (friendly-to-friendly)
    // 3. Normal attack moves
    // 4. Attacks on enemy generals (lowest)
    // Tiebreakers: army size, then old priority (player index)
    std::sort(moves.begin(), moves.end(),
              [this, &moveOutMap](const auto& a, const auto& b) {
                  return compareMovePriority(a, b, moveOutMap);
              });

    // execute moves
    for (auto [player, move] : moves) {
        if (!alive[player] || !board.available(player, move))
            continue;  // skip just-captured players or invalid moves
        if (move.type == MoveType::MOVE_ARMY) {
            Tile& fromTile = board.tileAt(move.from);
            Tile& toTile = board.tileAt(move.to);

            army_t takenArmy =
                move.takeHalf ? (fromTile.army >> 1) : (fromTile.army - 1);

            fromTile.army -= takenArmy;
            if (isValidPlayer(toTile.occupier) &&
                inSameTeam(toTile.occupier, player)) {
                toTile.occupier = player;
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
    }

    // update board
    if (halfTurnPhase == 0) {
        board.update(turn > 0 && turn % 25 == 0);
    }
    curTurn += halfTurnPhase;
    curHalfTurnPhase ^= 1;

    if (halfTurnPhase == 1) {
        // A surrender only becomes official after the current full turn has
        // finished all combat, capture, and growth checks.
        for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
            if (!pendingSurrender[i]) continue;

            pendingSurrender[i] = false;
            surrendered[i] = true;
            surrenderResolved[i] = false;
            surrenderEffectiveTurn[i] = turn;
            alive[i] = false;
            broadcast(turn, GameMessageSurrender{i});
        }
    }

    board.updateVisionCache();

    // request moves (for next turn)
    std::vector<RankItem> rank = ranklist();
    for (index_t i : getAlivePlayers()) {
        if (!acceptsOrders(i)) continue;
        players[i]->requestMove(view(i), rank);
    }
}

inline std::vector<RankItem> BasicGame::ranklist() {
    std::vector<RankItem> rank(players.size());
    for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
        rank[i].player = i;
        rank[i].alive = alive[i];
    }

    for (auto& tile : board.tiles) {
        if (isValidPlayer(tile.occupier)) {
            RankItem& item = rank[tile.occupier];
            item.army += tile.army, ++item.land;
        }
    }

    std::sort(rank.begin(), rank.end(),
              [](const RankItem& lhs, const RankItem& rhs) {
                  if (lhs.army != rhs.army) return lhs.army > rhs.army;
                  return lhs.player < rhs.player;
              });
    return rank;
}

inline int BasicGame::initSpawn() {
    std::mt19937 random{std::random_device()()};
    int playerCount = static_cast<int>(players.size());
    std::vector<index_t> playerSequence(players.size());
    std::iota(playerSequence.begin(), playerSequence.end(), 0);
    std::shuffle(playerSequence.begin(), playerSequence.end(), random);
    std::vector<Coord> spawnCandidates;
    auto assign = [&]() -> void {
        for (int i = 0; i < playerCount; ++i)
            spawnCoord[playerSequence[i]] = spawnCandidates[i];
    };
    int spawnCount = 0;
    for (int i = 1; i <= board.row; ++i) {
        for (int j = 1; j <= board.col; ++j) {
            if (board.tileAt(i, j).type == TILE_SPAWN) {
                spawnCandidates.emplace_back(i, j);
                ++spawnCount;
            }
        }
    }
    std::shuffle(spawnCandidates.begin(), spawnCandidates.end(), random);
    if (spawnCount >= playerCount) return assign(), 0;

    int blankCount = 0;
    for (int i = 1; i <= board.row; ++i) {
        for (int j = 1; j <= board.col; ++j) {
            const Tile& tile = board.tileAt(i, j);
            if (tile.type == TILE_PLAIN && tile.army == 0) {
                spawnCandidates.emplace_back(i, j);
                ++blankCount;
            }
        }
    }
    std::shuffle(spawnCandidates.begin() + spawnCount, spawnCandidates.end(),
                 random);
    if (spawnCount + blankCount >= playerCount) return assign(), 0;
    return 1;
}

inline int BasicGame::init() {
    int spawnReturn = initSpawn();
    if (spawnReturn != 0) return spawnReturn;

    for (Tile& tile : board.tiles) {
        if (tile.type == TILE_SPAWN) {
            tile.type = TILE_BLANK;
            tile.army = 0;
        }
    }

    alive = std::vector(players.size(), true);
    pendingSurrender.assign(players.size(), false);
    surrendered.assign(players.size(), false);
    surrenderResolved.assign(players.size(), false);
    surrenderEffectiveTurn.assign(players.size(), 0);
    for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
        Tile& spawnTile = board.tileAt(spawnCoord[i]);
        spawnTile.occupier = i;
        spawnTile.type = TILE_GENERAL;
        spawnTile.army = 0;  // will be updated in the first turn
    }

    for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
        players[i]->init(
            i, GameConstantsPack{board.getHeight(), board.getWidth(),
                                 static_cast<index_t>(players.size()), teams,
                                 conf});
    }

    board.updateVisionCache();
    return 0;
}

inline void BasicGame::broadcast(turn_t turn,
                                 const GameMessageData& messageData) {
    GameEvent event{turn, messageData};
    for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
        players[i]->onGameEvent(event);
    }
}

}  // namespace game

#endif  // LGEN_CORE_GAME_HPP
