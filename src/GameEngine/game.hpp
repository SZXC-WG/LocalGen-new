/**
 * @file game.hpp
 *
 * LocalGen Module: GameEngine
 *
 * Games
 *
 * Core game operations
 */

#ifndef LGEN_GAMEENGINE_GAME_HPP
#define LGEN_GAMEENGINE_GAME_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <numeric>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "board.hpp"
#include "move.hpp"
#include "player.hpp"
#include "tile.hpp"
#include "utils.hpp"

namespace game {

namespace config {

enum class VisionMode : uint8_t { NEAR8, NEAR4 };

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
    F(int, SpawnVisionRange, 1)

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

enum class GameMessageType : uint8_t { WIN, CAPTURE, SURRENDER, TEXT };

struct GameConstantsPack {
    pos_t mapHeight, mapWidth;
    index_t playerCount;
    std::vector<index_t> teams;
    config::Config config;
};

class BasicGame {
   protected:
    std::mt19937 rng{std::random_device()()};
    turn_t curTurn{};

   public:
    inline turn_t getCurTurn() const { return curTurn; }

   protected:
    std::vector<Player*> players;
    std::unordered_map<Player*, index_t> indexMap;
    std::vector<std::string> names;
    std::vector<index_t> teams;
    std::vector<bool> alive;
    std::vector<Coord> spawnCoord;

    inline bool isValidPlayer(index_t player) const {
        return player >= 0 && player < static_cast<index_t>(players.size());
    }

   public:
    inline bool isAlive(index_t player) const {
        return isValidPlayer(player) && alive[player];
    }
    inline index_t getTeam(index_t player) const { return teams[player]; }
    inline std::vector<index_t> getTeams() const { return teams; }
    inline std::string getName(index_t player) const { return names[player]; }
    inline std::vector<std::string> getNames() const { return names; }

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
    void broadcast(turn_t turn, GameMessageType message,
                   std::vector<index_t> associatedList);

   public:
    class GameBoard : public Board {
       protected:
        BasicGame* game;

       public:
        GameBoard() = delete;
        explicit GameBoard(BasicGame* _game);
        GameBoard(BasicGame* _game, Board* _board);

        bool visible(pos_t row, pos_t col, index_t player) const override;
        void update(turn_t turn);

        friend class BasicGame;
    };

   protected:
    GameBoard board{this};
    std::deque<std::pair<index_t, Move>> movesInQueue;

    inline BoardView view(Player* player) {
        return board.view(indexMap[player]);
    }

    void enqueueMove(index_t player, const Move& move);
    void sortMoves();
    void capture(index_t p1, index_t p2);
    void executeMoves();

   public:
    BasicGame() = delete;
    BasicGame(bool remainIndex, std::vector<Player*> _players,
              std::vector<index_t> _teams, std::vector<std::string> name,
              InitBoard _board);
    ~BasicGame();

   protected:
    void update();
    void act(Player* player);
    void process();

   public:
    void performTurn();

   public:
    class RankInfo {
       private:
        index_t player;
        army_t army;
        pos_t land[TILE_TYPE_COUNT];

       public:
        friend class BasicGame;
    };

   protected:
    std::vector<RankInfo> rank;

   public:
    void ranklist();
    inline std::vector<RankInfo> getRanklist() { return rank; }

   public:
    int initSpawn();
    int init();

   protected:
    InitBoard initialBoard;

   public:
    inline void setInitialBoard(InitBoard initial) { initialBoard = initial; }
    inline InitBoard getInitialBoard() { return initialBoard; }

   protected:
    struct ReplayUnit {};
    std::vector<ReplayUnit> replay;
};

inline BasicGame::GameBoard::GameBoard(BasicGame* _game) : game(_game) {}

inline BasicGame::GameBoard::GameBoard(BasicGame* _game, Board* _board)
    : Board(*_board), game(_game) {}

inline bool BasicGame::GameBoard::visible(pos_t row, pos_t col,
                                          index_t player) const {
    return Board::visible(row, col, player);
}

inline void BasicGame::GameBoard::update(turn_t turn) {
    for (auto& row : tiles) {
        for (auto& tile : row) {
            if (tile.occupier == -1) {
                continue;
            }
            switch (tile.type) {
                case TILE_CITY:
                case TILE_GENERAL:
                case TILE_CAPTURED_GENERAL: ++tile.army;
                case TILE_BLANK:
                    if (turn % 25 == 0) ++tile.army;
                    break;
                case TILE_SWAMP:
                    if (tile.army > 0) --tile.army;
                    break;
                default: break;
            }
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

inline void BasicGame::enqueueMove(index_t player, const Move& move) {
    if (board.available(player, move)) movesInQueue.emplace_back(player, move);
}

inline void BasicGame::sortMoves() {
    using value_t = decltype(movesInQueue)::value_type;
    auto&& ascending = [](value_t a, value_t b) -> bool {
        return a.first < b.first;
    };
    auto&& descending = [](value_t a, value_t b) -> bool {
        return a.first > b.first;
    };
    if (curTurn & 1) {
        std::sort(movesInQueue.begin(), movesInQueue.end(), ascending);
    } else {
        std::sort(movesInQueue.begin(), movesInQueue.end(), descending);
    }
}

inline void BasicGame::capture(index_t p1, index_t p2) {
    alive[p2] = false;
    for (auto& row : board.tiles) {
        for (auto& tile : row) {
            if (tile.occupier == p2) {
                tile.occupier = p1;
                if (tile.type == TILE_GENERAL) {
                    tile.type = TILE_CAPTURED_GENERAL;
                }
            }
        }
    }
    broadcast(curTurn, GameMessageType::CAPTURE, {p1, p2});
}

inline void BasicGame::executeMoves() {
    for (auto [player, move] : movesInQueue) {
        if (!board.available(player, move)) continue;
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
        } else if (move.type == MoveType::SURRENDER) {
            alive[player] = false;
            broadcast(curTurn, GameMessageType::SURRENDER, {player});
        }
    }
    movesInQueue.clear();
}

inline BasicGame::BasicGame(bool remainIndex, std::vector<Player*> _players,
                            std::vector<index_t> _teams,
                            std::vector<std::string> name, InitBoard _board)
    : initialBoard(_board),
      players(_players.size()),
      names(_players.size()),
      teams(_players.size()),
      board(this, &_board),
      alive(_players.size()),
      spawnCoord(_players.size()) {
    std::vector<index_t> randId(_players.size());
    std::iota(randId.begin(), randId.end(), 0);
    if (!remainIndex) std::shuffle(randId.begin(), randId.end(), rng);
    for (std::size_t i = 0; i < players.size(); ++i) {
        players[randId[i]] = _players[i];
        names[randId[i]] = name[i];
        teams[randId[i]] = _teams[i];
        indexMap[players[randId[i]]] = randId[i];
    }
}

inline BasicGame::~BasicGame() {
    for (auto player : players) delete player;
}

inline void BasicGame::update() { board.update(curTurn); }

inline void BasicGame::act(Player* player) {
    index_t playerIndex = indexMap[player];
    if (!isAlive(playerIndex)) return;

    BoardView boardView = view(player);
    player->requestMove(boardView);
    enqueueMove(playerIndex, player->step());
}

inline void BasicGame::process() {
    sortMoves();
    executeMoves();
}

inline void BasicGame::performTurn() {
    for (auto player : players) act(player);
    update();
    process();
    ++curTurn;
}

inline void BasicGame::ranklist() {
    rank.clear();
    rank.resize(players.size());
    for (auto& row : board.tiles) {
        for (auto& tile : row) {
            if (tile.occupier == -1) continue;
            rank[tile.occupier].army += tile.army;
        }
    }
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
    for (int i = 1; i < board.row; ++i) {
        for (int j = 1; j < board.col; ++j) {
            if (board.tiles[i][j].type == TILE_SPAWN) {
                spawnCandidates.emplace_back(i, j);
                ++spawnCount;
            }
        }
    }
    std::shuffle(spawnCandidates.begin(), spawnCandidates.end(), random);
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
    std::shuffle(spawnCandidates.begin() + spawnCount, spawnCandidates.end(),
                 random);
    if (spawnCount + blankCount >= playerCount) return assign(), 0;
    return 1;
}

inline int BasicGame::init() {
    int spawnReturn = initSpawn();
    if (spawnReturn != 0) return spawnReturn;

    alive = std::vector(players.size(), true);
    for (index_t i = 0; i < static_cast<index_t>(players.size()); ++i) {
        players[i]->init(
            i, GameConstantsPack{board.getHeight(), board.getWidth(),
                                 static_cast<index_t>(players.size()), teams,
                                 conf});
    }
    return 0;
}

inline void BasicGame::broadcast(turn_t turn, GameMessageType message,
                                 std::vector<index_t> associatedList) {}

}  // namespace game

#endif  // LGEN_GAMEENGINE_GAME_HPP
