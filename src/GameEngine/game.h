/**
 * @file game.h
 *
 * LocalGen Module: GameEngine
 *
 * Games
 *
 * Basic game operations
 */

#ifndef LGEN_MODULE_GE_GAME_H
#define LGEN_MODULE_GE_GAME_H

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <vector>

#include "board.h"
#include "player.h"
#include "tile.h"

class Board;
class Move;
class Player;

namespace game {

/// The namespace for game configuration.
namespace config {

/// Vision modes.
enum class VisionMode : uint8_t { NEAR8, NEAR4 };

/// List of game configuration units. Edit the items here to add/remove/change
/// game configuration units.
#define GAME_CONFIG_UNIT_LIST(F)           \
    F(int, CityVisionRange, 1)             \
    F(int, SpawnVisionRange, 1)            \
    F(bool, RanklistShowLand, true)        \
    F(bool, RanklistShowArmy, true)        \
    F(bool, RanklistShowPlayerIndex, true) \
    F(bool, RanklistShowPlayerName, true)  \
    F(bool, RanklistShowTeamIndex, true)   \
    F(bool, RanklistShowColor, true)       \
    F(VisionMode, OverallVisionMode, VisionMode::NEAR8)

/// Hard-code Config Type.
struct Config {
#define DECL(type, name, def) type name = def;
    GAME_CONFIG_UNIT_LIST(DECL)
#undef DECL
};

/// Patch of Config. Contains only some values, used for editing config.
struct ConfigPatch {
#define DECL(type, name, ...) std::optional<type> name;
    GAME_CONFIG_UNIT_LIST(DECL)
#undef DECL
};

namespace unit {
/// Construct directly-used configuration units.
#define UNIT(type, name, ...)            \
    constexpr ConfigPatch name(type v) { \
        ConfigPatch p;                   \
        p.name = v;                      \
        return p;                        \
    }
GAME_CONFIG_UNIT_LIST(UNIT)
#undef UNIT
}  // namespace unit

#define IF_ASSIGN(type, name, ...)     \
    if (rhs.name) res.name = rhs.name; \
/// Merge operator. Merges two patches. If some conflict, use the latter.
/// @param lhs Former patch.
/// @param rhs Latter patch, determining values.
/// @return The merged patch.
constexpr ConfigPatch operator|(const ConfigPatch& lhs,
                                const ConfigPatch& rhs) {
    ConfigPatch res = lhs;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN)
    return res;
}
#undef IF_ASSIGN

#define IF_ASSIGN_VALUE(type, name, ...) \
    if (rhs.name) res.name = *rhs.name;  \
/// Apply operator. Applies a patch to a specific config. If some conflict,
/// follow the patch.
/// @param lhs The to-be-applied config.
/// @param rhs The patch.
/// @return The applied config.
constexpr Config operator|(const Config& lhs, const ConfigPatch& rhs) {
    Config res = lhs;
    GAME_CONFIG_UNIT_LIST(IF_ASSIGN_VALUE)
    return res;
}
/// Apply operator. Applies a patch to a specific config. If some conflict,
/// follow the patch.
/// @param lhs The patch.
/// @param rhs The to-be-applied config.
/// @return The applied config.
constexpr Config operator|(const ConfigPatch& lhs, const Config& rhs) {
    return rhs | lhs;
}
#undef IF_ASSIGN_VALUE

/// List of game modifiers. Edit the items here to add/remove/change game
/// modifiers / their unit sets.
#define GAME_CONFIG_MODIFIER_LIST(F) \
    F(Watchtower, unit::CityVisionRange(3) | unit::SpawnVisionRange(3))

namespace modifier {
/// Construct directly-used modifier sets.
#define MODIFIER(name, value) constexpr ConfigPatch type = value;
GAME_CONFIG_MODIFIER_LIST(MODIFIER)
#undef MODIFIER
}  // namespace modifier

constexpr Config defaultConf;

}  // namespace config

enum class GameMessageType : uint8_t { WIN, CAPTURE, SURRENDER, TEXT };

class BasicGame {
   public:
    // type aliases
    using turn_t = uint32_t;
    using speed_t = double;

   protected:
    /// The current turn number.
    turn_t curTurn;
    /// Normally, this should be in the range of [0.25x, 256x]. Actually, speeds
    /// above 16x are not recommended. Recommended speeds (from generals.io):
    /// 0.25x, 0.5x, 0.75x, 1x, 1.5x, 2x, 3x, 4x.
    /// [TODO] If set to 0, the game will run as fast as possible, but the
    /// robots will block the game from running until their finished
    /// calculating. In other words, the game runs as a simulator, with the
    /// purpose of generating a replay.
    speed_t speed;

    /// These integers should be in the range of (-INF, +INF).
    /// If zero, no effect.
    /// If positive, pass |the number| turns per troop increased.
    /// If negative, increase |the number| troops per turn.
    /// Number are specific for each tile type.
    /// It is noticeable that values 1 and -1 mean the same things.
    std::array<army_t, TILE_TYPE_COUNT> increment;

    /// These integers should be in the range of (-INF, +INF).
    /// If zero, no effect.
    /// If positive, pass |the number| turns per troop decreased.
    /// If negative, decrease |the number| troops per turn.
    /// Number are specific for each tile type.
    /// It is noticeable that values 1 and -1 mean the same things.
    std::array<army_t, TILE_TYPE_COUNT> decrement;

   public:
    inline turn_t getCurTurn() const { return curTurn; }
    inline speed_t getSpeed() const { return speed; }
    inline std::array<army_t, TILE_TYPE_COUNT> getIncrement() const {
        return increment;
    };
    inline std::array<army_t, TILE_TYPE_COUNT> getDecrement() const {
        return decrement;
    };

   protected:
    static constexpr index_t PLAYER_INDEX_START = 1;
    /// Players in the game.
    std::vector<Player*> players;
    /// Alive status of the players.
    std::vector<bool> alive;
    /// Spawn coordinates of the players.
    std::vector<Coord> spawnCoord;

   public:
    /// Check whether a player is alive.
    /// @param player the index of the player.
    /// @return Whether the player is alive.
    inline bool isAlive(index_t player) const { return alive[player]; };
    /// Get the team ID of a player.
    /// @param player the index of the player.
    /// @return The team ID of the player.
    inline index_t getTeam(index_t player) const {
        return players[player]->teamId;
    };
    /// Check whether two players are in the same team.
    /// @param player1 the index of the first player.
    /// @param player2 the index of the second player.
    /// @return Whether the two players are in the same team.
    inline bool inSameTeam(index_t player1, index_t player2) const {
        return getTeam(player1) == getTeam(player2);
    };

   protected:
    /// Configuration variable. Default value 0.
    /// Placed in `protected` to avoid being modified illegally.
    config::Config conf = config::defaultConf;

   public:
    /// Get game configuration value.
    /// @return The current configuration value.
    inline config::Config getConfig() const { return conf; }

   public:
    /// [TODO]
    /// Broadcast a game message to the game.
    /// @param turn the turn number the message is dedicated to.
    /// @param message type of the message.
    /// @param associatedList the needed extra information of the message.
    void broadcast(turn_t turn, GameMessageType message,
                   std::vector<index_t> associatedList);

   public:
    class GameBoard : public Board {
       public:
        friend class BasicGame;

       public:
        BasicGame* game;
        std::vector<std::vector<std::vector<int>>> visibility;

       public:
        /// The default constructor is deleted, for an %GameBoard relies on a
        /// specific game object.
        GameBoard() = delete;
        GameBoard(BasicGame* _game);
        GameBoard(BasicGame* _game, Board* _board);

        bool visible(Player* player, pos_t row, pos_t col);

        void update(turn_t turn);

       public:
        class MoveProcessor;
        friend class GameBoard::MoveProcessor;

        /// Move Processor used by games.
        /// A %MoveProcessor is used to contain and
        /// process moves.
        class MoveProcessor {
           protected:
            std::deque<Move> movesInQueue;

           protected:
            BasicGame* game;
            GameBoard* board;

            /// Constructors.
            /// The default constructor is deleted, for
            /// a %MoveProcessor must be linked to a
            /// %GameBoard.
           public:
            MoveProcessor() = delete;
            MoveProcessor(BasicGame* _game, GameBoard* _board);

           public:
            /// Add a %Move to the waiting-to-be-processed queue.
            /// @param move added %Move.
            void add(Move move);

            /// Sort the in-queue moves according to their priority.
            /// May not be implemented.
            void sort();

            /// Perform an action where a player captures another player.
            /// @param p1 the player that captures.
            /// @param p2 the player that is captured.
            void capture(index_t p1, index_t p2);
            /// Execute all the in-queue moves, one by one.
            void execute();
        };

       public:
        MoveProcessor processor{game, this};
    };

   protected:
    GameBoard board{this};

    /// Get a view of the %GameBoard, using privilege of a %Player.
    /// Remember: this function is used to get "views"!
    /// @param player The player that views the %GameBoard.
    /// @return The view of the %GameBoard.
    inline BoardView getBoard(Player* player) {
        return BoardView(&board, player->index);
    }

   public:
    BasicGame() = delete;
    BasicGame(std::vector<Player*> _players, InitBoard _board, speed_t _speed);

   protected:
    /// Update the map for the turn.
    /// What to update? For example, the self-increment of tile armies.
    /// This function recursively let the %GameBoard do the job due to
    /// accessment issues.
    void update();

   protected:
    /// Get action from a %Player. In other words, make this %Player act.
    /// @param player The acting player.
    void act(Player* player);

   protected:
    /// Process all the existing moves in the processor, make them work.
    void process();

    /// Perform the operations that a turn needs.
    void performTurn();

   public:
    /// Sub-class to store real-time ranklists in game.
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
    /// Generate the ranklist.
    void ranklist();

   public:
    /// Get the ranklist.
    /// @return The ranklist.
    std::vector<RankInfo> getRanklist();

   protected:
    /// Initialize the spawn points for the players.
    /// @return 0 if the initialization is successful, 1 otherwise.
    int initSpawn();

    /// Initialize the game.
    /// @return 0 if the initialization is successful, 1 otherwise.
    int init();

   public:
    /// Run the game.
    virtual void run();

   protected:
    InitBoard initialBoard;

   public:
    inline void setInitialBoard(InitBoard board) { initialBoard = board; };
    inline InitBoard getInitialBoard() { return initialBoard; };

   protected:
    struct ReplayUnit {};
    std::vector<ReplayUnit> replay;
};

}  // namespace game

#endif  // LGEN_MODULE_GE_GAME_H
