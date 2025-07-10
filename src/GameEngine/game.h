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
#include "tile.h"

class Board;
class Move;
class Player;

namespace game_conf {

enum class VisionMode : uint8_t {
    VISION_TYPE_NEARSIGHTED_8,
    VISION_TYPE_NEARSIGHTED_4
};

#define GAME_CONFIG_FIELD_LIST(F)          \
    F(int, CityVisionRange, 1)             \
    F(int, SpawnVisionRange, 1)            \
    F(bool, RanklistShowLand, true)        \
    F(bool, RanklistShowArmy, true)        \
    F(bool, RanklistShowPlayerIndex, true) \
    F(bool, RanklistShowPlayerName, true)  \
    F(bool, RanklistShowTeamIndex, true)   \
    F(bool, RanklistShowColor, true)       \
    F(VisionMode, OverallVisionMode, VISION_TYPE_NEARSIGHTED_8)

struct Config {
#define DECL(type, name, def) type name = def;
    GAME_CONFIG_FIELD_LIST(DECL)
#undef DECL
};

struct ConfigPatch {
#define DECL(type, name, ...) std::optional<type> name;
    GAME_CONFIG_FIELD_LIST(DECL)
#undef DECL
};

#define BUILDER(type, name, ...)         \
    constexpr ConfigPatch name(type v) { \
        ConfigPatch p{.name = v};        \
        return p;                        \
    }
GAME_CONFIG_FIELD_LIST(BUILDER)
#undef BUILDER

#define IF_ASSIGN(type, name, ...) \
    if (latt.name) {               \
        form.name = latt.name;     \
    }
#define IF_ASSIGN_VALUE(type, name, ...) \
    if (latt.name) {                     \
        form.name = *latt.name;          \
    }
inline bool operator|(ConfigPatch form, ConfigPatch latt) {
    GAME_CONFIG_FIELD_LIST(IF_ASSIGN);
}
inline bool operator|(Config form, ConfigPatch latt) {
    GAME_CONFIG_FIELD_LIST(IF_ASSIGN_VALUE)
}
inline bool operator|(ConfigPatch form, Config latt) { return latt | form; }
#undef IF_ASSIGN
#undef IF_ASSIGN_VALUE

#define GAME_MODIFIER_FIELD_LIST(F) \
    F(Watchtower, CityVisionRange(3) | SpawnVisionRange(3))

const Config defaultConf{};

// #undef GAME_CONFIG_FIELD_LIST // Why #undef this? It's useful.

}  // namespace game_conf

enum class game_message_e {
    GAME_MESSAGE_WIN,
    GAME_MESSAGE_CAPTURE,
    GAME_MESSAGE_SURRENDER,
};

class BasicGame {
   public:
    // type aliases
    using turn_t = uint32_t;
    using speed_t = double;

   protected:
    /// The current turn number.
    turn_t curTurn;
    /// This should be in the range of [0.25x, 256x]. Actually, speeds above 16x
    /// is not recommended.
    /// Recommended speeds (from generals.io): 0.25x, 0.5x, 0.75x, 1x, 1.5x, 2x,
    /// 3x, 4x.
    speed_t speed;

    /// This integer should be in the range of (-INF, +INF).
    /// If zero, it has no effect.
    /// If positive, it indicates how many turns will pass for the tile troops
    /// to self-increase 1.
    /// If negative, it indicates how many troops of the tile will self-increase
    /// when it passes 1 turn. It is noticeable that values 1 and -1 mean the
    /// same things.
    std::array<army_t, TILE_TYPE_COUNT> increment;

    /// This integer should be in the range of (-INF, +INF).
    /// If zero, it has no effect.
    /// If positive, it indicates how many turns will pass for the tile troops
    /// to self-decrease 1.
    /// If negative, it indicates how many troops of the tile will self-decrease
    /// when it passes 1 turn. It is noticeable that values 1 and -1 mean the
    /// same things.
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
    /// The God of the game. Has full control of the game.
    Player* god;

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
    inline index_t getTeam(index_t player) const {
        return players[player]->teamId;
    };
    inline bool inSameTeam(index_t player1, index_t player2) const {
        return getTeam(player1) == getTeam(player2);
    };

   protected:
    /// Configuration variable. Default value 0.
    /// Placed in `protected` to avoid being modified illegally.
    config_t conf = defaultConf;

   public:
    /// Get game configuration value.
    /// @return The current configuration value.
    inline config_t getConfig() const { return conf; }

   public:
    /// [TODO]
    /// Broadcast a game message to the game.
    /// @param turn the turn number the message is dedicated to.
    /// @param message type of the message.
    /// @param associatedList the needed extra information of the message.
    void broadcast(turn_t turn, game_message_e message,
                   std::vector<index_t> associatedList);

   public:
    class InGameBoard : public Board {
       public:
        friend class BasicGame;

       public:
        BasicGame* game;

       public:
        /// The default constructor is deleted, for an %InGameBoard relies on a
        /// specific game object.
        InGameBoard() = delete;
        InGameBoard(BasicGame* _game);
        InGameBoard(BasicGame* _game, Board* _board);

        bool visible(Player* player, pos_t row, pos_t col);

        void update(turn_t turn);

       public:
        class MoveProcessor;
        friend class InGameBoard::MoveProcessor;
        /// Move Processor used by games.
        /// A %MoveProcessor is used to contain and
        /// process moves.
        class MoveProcessor {
           protected:
            std::deque<Move> movesInQueue;

           protected:
            BasicGame* game;
            InGameBoard* board;

            /// Constructors.
            /// The default constructor is deleted, for
            /// a %MoveProcessor must be linked to a
            /// %Board.
           public:
            MoveProcessor() = delete;
            MoveProcessor(BasicGame* _game, InGameBoard* _board);

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
    InGameBoard board{this};

    inline BoardView getBoard(Player* player) {
        return BoardView(&board, player);
    }

   public:
    BasicGame() = delete;
    BasicGame(Player* _god, std::vector<Player*> _players, InitBoard _board,
              speed_t _speed);

   protected:
    /// Update the map for the turn.
    /// What to update? For example, the self-increment of tile armies.
    /// This function recursively let the %InGameBoard do the job due to
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

#endif  // LGEN_MODULE_GE_GAME_H
