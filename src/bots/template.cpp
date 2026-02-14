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

#ifndef /* LGEN_BOTS_%BOTNAME% */
#define /* LGEN_BOTS_%BOTNAME% */

// Remember to include the bot header file.
// Without this, you can't define your bot class, and the registry won't work.
#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

// You can include additional headers you need.
#include <random>

class /*%BotClass%*/ : public BasicBot {
    // Do not define any constructors.
    // Destructors can be defined if necessary.
   protected:
   public:
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override {}

    void requestMove(const BoardView& boardView,
                     const std::vector<game::RankItem>& rank) override {}
};

// Do not forget to register your bot.
static BotRegistrar</*%BotClass%*/> /*%BOTNAME%*/ _reg(/*%BOTNAME%*/);

#endif  // LGEN_BOTS_%BOTNAME%
