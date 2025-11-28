/**
 * @file game.h
 *
 * LocalGen Module: GameEngine
 *
 * Games
 *
 * Core game operations
 */

#ifndef LGEN_MODULE_GE_GAME_H
#define LGEN_MODULE_GE_GAME_H

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>

#include "board.h"
#include "player.h"
#include "tile.h"

class Board;
class Move;
class Player;

namespace game {

/// Namespace that contains all game-related configuration.
namespace config {

/// Supported vision modes.
enum class VisionMode : uint8_t { NEAR8, NEAR4 };

/// List of configuration items.
/// To add / remove / modify items, edit this macro list.
/// Macro format: F(type, name, default_value)
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

/// Strongly-typed configuration structure with defaults.
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

/// A partial configuration patch.
/// Only the specified fields will be overridden.
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
/// Helper constructors for configuration units.
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
/// Merge operator.
/// Merge two patches.
/// When conflicts occur, values from the right-hand side prevail.
/// @param lhs Former patch
/// @param rhs Latter patch; its values take precedence
/// @return    Merged patch
constexpr inline ConfigPatch operator|(const ConfigPatch& lhs,
                                       const ConfigPatch& rhs) {
    ConfigPatch res = lhs;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN)
    return res;
}
#undef IF_ASSIGN

#define IF_ASSIGN_VALUE(type, name, ...) \
    if (rhs.name) res.name = *rhs.name;
/// Apply operator.
/// Apply a patch to an existing configuration.
/// Fields present in the patch override those in the base config.
/// @param lhs Base configuration
/// @param rhs Patch to apply
/// @return    Resulting configuration
constexpr inline Config operator|(const Config& lhs, const ConfigPatch& rhs) {
    Config res = lhs;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN_VALUE)
    return res;
}
/// Same as above, but with operands reversed for convenience.
constexpr inline Config operator|(const ConfigPatch& lhs, const Config& rhs) {
    return rhs | lhs;
}
#undef IF_ASSIGN_VALUE

#define IF_ASSIGN_OPTIONAL(type, name, ...) \
    if (rhs.name) res.name = lhs.name;
/// Intersection operator.
/// Creates a new ConfigPatch that keeps only the fields specified in `rhs`,
/// while taking the *current* values from the full `lhs` Config.
/// @param lhs Full configuration to sample values from.
/// @param rhs Mask patch that decides which keys survive.
/// @return    A ConfigPatch containing lhs values for keys present in rhs.
constexpr inline ConfigPatch operator&(const Config& lhs,
                                       const ConfigPatch& rhs) {
    ConfigPatch res;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN_OPTIONAL);
    return res;
}
/// Same as above, but with operands reversed for convenience.
constexpr inline ConfigPatch operator&(const ConfigPatch& lhs,
                                       const Config& rhs) {
    return rhs & lhs;
}
#undef IF_ASSIGN_OPTIONAL

/// List of predefined game modifiers.
/// Modify the list to add / remove / update modifiers.
/// Macro format: F(name, config_patch_value)
#define GAME_CONFIG_MODIFIER_LIST(F)                                    \
    F(Watchtower, unit::CityVisionRange(3) | unit::SpawnVisionRange(3)) \
    F(MistyVeil, unit::OverallVisionRange(0))

namespace modifier {
/// Concrete modifier instances.
#define MODIFIER(name, value) constexpr ConfigPatch name = value;
GAME_CONFIG_MODIFIER_LIST(MODIFIER)
#undef MODIFIER
}  // namespace modifier

/// Default configuration instance.
constexpr Config defaultConf;

/// Relationship between a ConfigPatch and a Config.
///
/// Legend per *edited* field (patch != defaultConf):
///   P – config == patch       (enabled)
///   D – config == defaultConf (disabled)
///   M – otherwise             (over-written elsewhere)
enum class PatchStatus : uint8_t {
    FULLY_ENABLED,      // All edited fields are P      (P>0, D=0, M=0)
    PARTIALLY_ENABLED,  // P>0 with at least one D or M (mixed)
    DISABLED,           // No P, only D                 (P=0, D>0, M=0)
    OVERRIDDEN          // No P, at least one M         (P=0, M>0)
};

/// Evaluate how a ConfigPatch is represented in a given Config.
/// For more, see @ref PatchStatus.
/// @param config Current configuration.
/// @param patch  Patch to evaluate (guaranteed to contain ≥1 edited field).
/// @return       PatchStatus describing the result.
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

/// Types of in-game broadcast messages.
enum class GameMessageType : uint8_t { WIN, CAPTURE, SURRENDER, TEXT };

/// Struct to hold game constants.
/// Used to initialize players.
struct GameConstantsPack {
    pos_t mapHeight, mapWidth;
    index_t playerCount;
    std::vector<index_t> teams;
    std::array<army_t, TILE_TYPE_COUNT> increment;
    std::array<army_t, TILE_TYPE_COUNT> decrement;
    config::Config config;
};

class BasicGame {
   public:
    // type aliases
    using turn_t = uint32_t;
    using speed_t = double;

   protected:
    std::mt19937 rng{std::random_device()()};

   protected:
    /// Current turn index.
    turn_t curTurn{};

    /// This variable should not be in %Game, but saved its comments here as a
    /// reference for docs.
    /// Playback / simulation speed.
    /// Valid range: [0.25×, 256×] (values >16× are not recommended).
    /// Suggested presets (from generals.io):
    ///   0.25×, 0.5×, 0.75×, 1×, 1.5×, 2×, 3×, 4×
    /// [TODO] If set to 0, the game runs as fast as possible; however,
    /// robots still block until they finish computing, effectively
    /// becoming a headless replay generator.
    /* speed_t speed{1.0}; */

   public:
    inline turn_t getCurTurn() const { return curTurn; }

   protected:
    /// All players participating in the game.
    /// Their indices are their player IDs.
    std::vector<Player*> players;
    /// Map from Player* to index_t.
    /// This is used to quickly find the player ID from a Player* pointer.
    std::unordered_map<Player*, index_t> indexMap;
    /// The name of each player.
    /// Use player IDs as indices.
    std::vector<std::string> names;
    /// The team id for each player.
    /// Use player IDs as indices.
    std::vector<index_t> teams;
    /// Per-player alive status.
    std::vector<bool> alive;
    /// Each player’s spawn coordinate.
    std::vector<Coord> spawnCoord;

   public:
    inline bool isAlive(index_t player) const { return alive[player]; };
    inline index_t getTeam(index_t player) const { return teams[player]; };
    inline std::vector<index_t> getTeams() const { return teams; };
    inline std::string getName(index_t player) const { return names[player]; };
    inline std::vector<std::string> getNames() const { return names; };

    inline bool inSameTeam(index_t player1, index_t player2) const {
        return getTeam(player1) == getTeam(player2);
    };

   protected:
    /// Game configuration.
    config::Config conf = config::defaultConf;

   public:
    inline config::Config getConfig() const { return conf; }
    inline void setConfig(config::ConfigPatch patch) { conf = conf | patch; }

   public:
    /// [TODO] Broadcast a message to all players.
    /// @param turn           Turn when the message should appear
    /// @param message        Message type
    /// @param associatedList Extra information (e.g., involved player IDs)
    void broadcast(turn_t turn, GameMessageType message,
                   std::vector<index_t> associatedList);

   public:
    class GameBoard : public Board {
       public:
        friend class BasicGame;

       protected:
        BasicGame* game;
        /// 3-dimensional visibility map
        /// visibility[p][r][c] counts the number of sources
        /// [Is that what we need?]
        /// Functions unimplemented.
        std::vector<std::vector<std::vector<int>>> visibility;

       public:
        /// Deleted default constructor: a GameBoard must belong to a game.
        GameBoard() = delete;
        GameBoard(BasicGame* _game);
        GameBoard(BasicGame* _game, Board* _board);

        bool visible(pos_t row, pos_t col, index_t player) const override;

        /// Update board state for the specified turn.
        void update(turn_t turn);

       public:
        class MoveProcessor;
        friend class GameBoard::MoveProcessor;

        /// Container & executor for pending moves.
        class MoveProcessor {
           protected:
            std::deque<std::pair<index_t, Move>> movesInQueue;

           protected:
            BasicGame* game;
            GameBoard* board;

           public:
            /// Deleted default constructor.
            MoveProcessor() = delete;
            MoveProcessor(BasicGame* _game, GameBoard* _board);

           public:
            /// Enqueue a move for later execution.
            void add(index_t player, Move move);

            /// Sort queued moves by priority.
            void sort();

            /// Handle a capture event: p1 captures p2.
            void capture(index_t p1, index_t p2);
            /// Execute all queued moves in order.
            void execute();
        };

       public:
        MoveProcessor processor{game, this};
    };

   protected:
    GameBoard board{this};

    /// Obtain a player-specific view of the board.
    /// NOTE: This returns a “view”, not the raw board.
    inline BoardView getBoard(Player* player) {
        return BoardView(&board, indexMap[player]);
    }

   public:
    BasicGame() = delete;
    BasicGame(bool remainIndex, std::vector<Player*> _players,
              std::vector<index_t> _teams, std::vector<std::string> name,
              InitBoard _board);
    ~BasicGame();

   protected:
    /// Update map state at the start of each turn.
    /// (e.g., automatic troop growth)
    void update();

   protected:
    /// Request an action from a player.
    void act(Player* player);

    /// Process all queued moves.
    void process();

   public:
    /// Execute all per-turn operations in correct order.
    void performTurn();

   public:
    /// Run-time ranking information for each player.
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
    /// Recompute ranking information.
    void ranklist();

    /// Retrieve the current rank list.
    /// Make sure the ranklist is already computed using ranklist() before
    /// calling this function.
    inline std::vector<RankInfo> getRanklist() { return rank; };

   public:
    /// Initialize player spawn positions.
    /// @return 0 on success, non-zero on failure
    int initSpawn();

    /// Perform overall game initialization.
    int init();

   protected:
    InitBoard initialBoard;

   public:
    inline void setInitialBoard(InitBoard board) { initialBoard = board; };
    inline InitBoard getInitialBoard() { return initialBoard; };

   protected:
    struct ReplayUnit {};
    std::vector<ReplayUnit> replay;

   public:
};

}  // namespace game

#endif  // LGEN_MODULE_GE_GAME_H
