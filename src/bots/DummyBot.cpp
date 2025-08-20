/**
 * @file dummyBot.cpp
 *
 * PLEASE READ THIS COMMENT AND THIS FILE CAREFULLY BEFORE YOU START TO
 * IMPLEMENT YOUR OWN BOT.
 *
 * This "DummyBot" is a simple example of how to create a bot.
 * Developers can refer to this file when implementing their own bots.
 * The bot implemented a pretty basic algorithm, just being an example.
 *
 * Like this file, bot codes should be put into the `src/bots` folder.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_DUMMYBOT
#define LGEN_BOTS_DUMMYBOT

// Remember to include the bot header file.
// Without this, you can't define your bot class, and the registry won't work.
#include "../GameEngine/bot.h"
#include "../GameEngine/game.h"

// You can include additional headers you need.
#include <random>

// DO NOT PUT YOUR BOT CODE IN ANY NAMESPACE. THAT WILL MAKE THE REGISTRY NOT
// WORK. AND SO, CHOOSE YOUR BOT NAME CAREFULLY, IT SHOULD BE UNIQUE.

class DummyBot : public BasicBot {
    // Do not define any constructors.
    // Destructors can be defined if necessary.

   protected:
    // You can put whatever you want, whatever you need here.
    // Don't define non-const static variables, they may cause thread problems.
    constexpr static pos_t deltaX[5] = {0, -1, 0, 1, 0};
    constexpr static pos_t deltaY[5] = {0, 0, -1, 0, 1};
    std::mt19937 rng{std::random_device{}()};

    // In most cases, you have to save the game constants in your bot.
    // For example, the map size, the player ID, etc.
    // Those things are sent to your bot via the `init` method, before the game
    // starts.
    // You can choose to ignore them, though. It is your choice.
    index_t cnt;                   // Player count
    index_t id;                    // My index
    index_t team;                  // My team index
    std::vector<index_t> teamIds;  // Team IDs
    pos_t height, width;           // Map height and width

    // Although the game class has given lots of useful methods to quickly tell
    // something, you may still need to implement them yourself for you can't
    // actually access the game object.
    inline bool inSameTeam(index_t anotherPlayer) const {
        return teamIds[anotherPlayer] == team;
    }
    inline bool inSameTeam(index_t player1, index_t player2) const {
        return teamIds[player1] == teamIds[player2];
    }

   public:
    // Remember to override the `init` method to process game constants.
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override {
        // This method is called when the game starts.
        // Game constants are sent via this method.
        // You can use this method to initialize your bot's state.
        // For example, you can store the player ID and team IDs.
        cnt = constants.playerCount;
        id = playerId;
        team = constants.teams[playerId];
        teamIds = constants.teams;
        height = constants.mapHeight;
        width = constants.mapWidth;
    }

    // Edit the `compute` method to implement your bot's main logic.
    // This method will be called **every turn**. Make that into your bot's
    // logic.
    void requestMove(BoardView& boardView) override {
        // This DummyBot uses a derivative of the smartRandomBot from LG v5.
        // As LG v6 removed the outdated "focus" feature, this bot will choose a
        // maximum tile every time.
        Coord start = Coord(1, 1);
        for (std::size_t r = 1; r <= height; ++r) {
            for (std::size_t c = 1; c <= width; ++c) {
                auto& tile = boardView.tileAt(r, c);
                if (tile.occupier != id) continue;
                if (tile.army > boardView.tileAt(start).army) {
                    start = Coord(r, c);
                }
            }
        }
    }
};

// Do not forget to register your bot.
static BotRegistrar<DummyBot> dummyBot_reg("DummyBot");

#endif  // LGEN_BOTS_DUMMYBOT
