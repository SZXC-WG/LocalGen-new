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

// You can include additional headers you need.
#include <random>

// DO NOT PUT YOUR BOT CODE IN A NAMESPACE. THAT WILL MAKE THE REGISTRY NOT
// WORK. AND SO, CHOOSE YOUR BOT NAME CAREFULLY, IT SHOULD BE UNIQUE.

class DummyBot : public BasicBot {
   public:
    explicit DummyBot() = default;

   private:
    // Store anything you need here. Be aware of threading issues.
    index_t id;

   public:
    void init(index_t playerId, std::vector<index_t> teamIds) override {
        // Bot initializer - typically useful to remember these data
        id = playerId;
    }
    void requestMove(BoardView& boardView) override {
        // Implement your bot's move logic here.
    }
};

// Do not forget to register your bot.
static BotRegistrar<DummyBot> dummyBot_reg("DummyBot");

#endif  // LGEN_BOTS_DUMMYBOT
