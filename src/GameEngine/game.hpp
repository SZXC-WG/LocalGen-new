/**
 * @file game.hpp
 *
 * LocalGen Module: GameEngine
 *
 * Games
 *
 * Basic game operations
 */

#ifndef LGEN_MODULE_GE_GAME
#define LGEN_MODULE_GE_GAME

#include "board.hpp"
#include "player.hpp"

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
    turn_t curTurn;

   public:
    /// [TODO]
    /// Broadcast a game message to the game.
    /// @param turn The turn number the message is dedicated to.
    /// @param message type of the message.
    /// @param associatedList the needed extra information of the message.
    void broadcast(turn_t turn, game_message_e message,
                   std::initializer_list<Player*> associatedList);

    class InGameBoard : public Board {
       public:
        BasicGame* game;

       public:
        /// The default constructor is deleted, for an %InGameBoard relies on a
        /// specific game object.
        InGameBoard() = delete;
        InGameBoard(BasicGame* _game) : game(_game) {}
        InGameBoard(BasicGame* _game, Board _board)
            : game(_game), Board(_board) {}

       public:
        class MoveProcessor;
        friend class InGameBoard::MoveProcessor;
        /// Move Processor used by games.
        /// A %MoveProcessor is used to contain and process moves.
        class MoveProcessor {
           protected:
            std::deque<Move> movesInQueue;

           protected:
            BasicGame* game;
            InGameBoard* board;

            /// Constructors.
            /// The default constructor is deleted, for a %MoveProcessor must be
            /// linked to a %Board.
           public:
            MoveProcessor() = delete;
            MoveProcessor(BasicGame* _game, InGameBoard* _board)
                : game(_game), board(_board) {}

           public:
            /// Add a %Move to the waiting-to-be-processed queue.
            /// @param move added %Move.
            inline void add(Move move) {
                if (move.available(board)) movesInQueue.emplace_back(move);
            }

            /// Sort the in-queue moves according to their priority.
            /// May not be implemented.
            void sort() {}

            /// Perform an action where a player captures another player.
            /// @param p1 the player that captures.
            /// @param p2 the player that is captured.
            void capture(Player* p1, Player* p2) {
                for (auto row : board->tiles) {
                    for (auto tile : row) {
                        if (tile.occupier == p2) {
                            tile.occupier = p1;
                            if (tile.type == TILE_GENERAL)
                                tile.type = TILE_CITY;
                        }
                    }
                }
                game->broadcast(game->curTurn, GAME_MESSAGE_CAPTURE, {p1, p2});
            }
            /// Execute all the in-queue moves, one by one.
            void execute() {
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
                            capture(move.player, toTile.occupier);
                        toTile.occupier = move.player;
                    }
                }
            }
        };

       public:
        MoveProcessor processor{game, this};
    };

   protected:
    InGameBoard board{this};
    std::vector<Player*> players;

   public:
    BasicGame() {}
    BasicGame(std::vector<Player*> _players, Board _board)
        : players(_players), board(this, _board) {}

   public:
    /// Get action from a %Player. In other words, make this %Player act.
    /// @param player The acting player.
    inline void act(Player* player) {
        while (!player->moveQueue.empty() &&
               !player->moveQueue.front().available(&board))
            player->moveQueue.pop_front();
        if (player->moveQueue.empty()) return;
        board.processor.add(player->moveQueue.front());
    }

   protected:
    /// Process all the existing moves in the processor, make them work.
    inline void process() {
        board.processor.sort();
        board.processor.execute();
    }

   public:
    virtual void run() {}
};

#endif  // LGEN_MODULE_GE_GAME
