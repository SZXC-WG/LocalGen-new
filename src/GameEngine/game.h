/**
 * @file game.hpp
 *
 * LocalGen Module: GameEngine
 *
 * Games
 *
 * Basic game operations
 */

#ifndef LGEN_MODULE_GE_GAME_H
#define LGEN_MODULE_GE_GAME_H

#include <cstdint>
#include <vector>

#include "board.h"
#include "tile.h"

class Board;
class Move;
class Player;

enum game_message_e {
    GAME_MESSAGE_WIN,
    GAME_MESSAGE_CAPTURE,
    GAME_MESSAGE_SURRENDER,
};

class BasicGame {
   public:
    // type aliases
    using turn_t = uint32_t;

   public:
    /// The current turn number.
    turn_t curTurn;
    /// This should be in the range of [0.25x, 256x]. Actually, speeds above 16x
    /// is not recommended.
    /// Recommended speeds (from generals.io): 0.25x, 0.5x, 0.75x, 1x, 1.5x, 2x,
    /// 3x, 4x.
    double speed;

    /// This integer should be in the range of (-INF, +INF).
    /// If zero, it has no effect.
    /// If positive, it indicates how many turns will pass for the tile troops
    /// to self-increase 1.
    /// If negative, it indicates how many troops of the tile will self-increase
    /// when it passes 1 turn. It is noticeable that values 1 and -1 mean the
    /// same things.
    army_t increment[TILE_TYPE_COUNT];

    /// This integer should be in the range of (-INF, +INF).
    /// If zero, it has no effect.
    /// If positive, it indicates how many turns will pass for the tile troops
    /// to self-decrease 1.
    /// If negative, it indicates how many troops of the tile will self-decrease
    /// when it passes 1 turn. It is noticeable that values 1 and -1 mean the
    /// same things.
    army_t decrement[TILE_TYPE_COUNT];

   protected:
    std::vector<Player*> players;
    std::vector<bool> alive;

   public:
    /// [TODO]
    /// Broadcast a game message to the game.
    /// @param turn the turn number the message is dedicated to.
    /// @param message type of the message.
    /// @param associatedList the needed extra information of the message.
    void broadcast(turn_t turn, game_message_e message,
                   std::vector<Player::index_t> associatedList);

    class InGameBoard : public Board {
       public:
        BasicGame* game;

       public:
        /// The default constructor is deleted, for an %InGameBoard relies on a
        /// specific game object.
        InGameBoard() = delete;
        InGameBoard(BasicGame* _game);
        InGameBoard(BasicGame* _game, Board _board);

        inline void update(turn_t turn);

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
            inline void add(Move move);

            /// Sort the in-queue moves according to their priority.
            /// May not be implemented.
            void sort();

            /// Perform an action where a player captures another player.
            /// @param p1 the player that captures.
            /// @param p2 the player that is captured.
            void capture(Player::index_t p1, Player::index_t p2);
            /// Execute all the in-queue moves, one by one.
            void execute();
        };

       public:
        MoveProcessor processor{game, this};
    };

   protected:
    InGameBoard board{this};

   public:
    BasicGame();
    BasicGame(std::vector<Player*> _players, Board _board);

   protected:
    /// Update the map for the turn.
    /// What to update? For example, the self-increment of tile armies.
    /// This function recursively let the %InGameBoard do the job due to
    /// accessment issues.
    inline void update();

   protected:
    /// Get action from a %Player. In other words, make this %Player act.
    /// @param player The acting player.
    inline void act(Player* player);

   protected:
    /// Process all the existing moves in the processor, make them work.
    inline void process();

    inline void performTurn();

   protected:
    void initSpawn();

    void init();

   public:
    virtual void run();
};

#endif  // LGEN_MODULE_GE_GAME_H
