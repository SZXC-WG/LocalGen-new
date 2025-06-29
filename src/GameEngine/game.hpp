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

   public:
    /// [TODO]
    /// Broadcast a game message to the game.
    /// @param turn the turn number the message is dedicated to.
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

        inline void update(turn_t turn) {
            for (auto& row : tiles) {
                for (auto& tile : row) {
                    if (tile.occupier == nullptr) {
                        continue;
                    }
                    // increments
                    if (game->increment[tile.type] > 0)
                        tile.army += (turn % game->increment[tile.type] == 0);
                    else if (game->increment[tile.type] < 0)
                        tile.army += game->increment[tile.type];
                    // decrements
                    if (game->decrement[tile.type] > 0)
                        tile.army -= (turn % game->decrement[tile.type] == 0);
                    else if (game->decrement[tile.type] < 0)
                        tile.army -= game->decrement[tile.type];
                    // lost tiles
                    if (tile.army == 0) {
                        if (tile.type == TILE_SWAMP) tile.occupier = nullptr;
                    }
                    if (tile.army < 0) {
                        tile.occupier = nullptr;
                        tile.army = 0;
                    }
                }
            }
        }

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
                for (auto& row : board->tiles) {
                    for (auto& tile : row) {
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

   protected:
    /// Update the map for the turn.
    /// What to update? For example, the self-increment of tile armies. This
    /// function recursively let the %InGameBoard do the job due to accessment
    /// issues.
    inline void update() { board.update(curTurn); }

   protected:
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

    inline void performTurn() {
        for (auto player : players) act(player);
        update();
        process();
    }

   public:
    virtual void run() {}
};

#endif  // LGEN_MODULE_GE_GAME
