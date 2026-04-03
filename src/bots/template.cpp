// Copyright (C) 2026 %AUTHORNAME%
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file template.cpp
 *
 * This is a template file.
 *
 * @author %AUTHORNAME%
 */

#ifndef /* LGEN_BOTS_%BOTNAME% */
#define /* LGEN_BOTS_%BOTNAME% */

// Remember to include the bot header file.
// Without this, you can't define your bot class, and the registry won't work.
#include "core/bot.h"
#include "core/game.hpp"

// You can include additional headers you need.
#include <random>

class /*%BotClass%*/ : public BasicBot {
    // Do not define any constructors.
    // Destructors can be defined if necessary.
   protected:
   public:
    void init(index_t playerId, const GameConstantsPack& constants) override {}

    void requestMove(const BoardView& boardView,
                     const std::vector<RankItem>& rank) override {}
};

// Do not forget to register your bot.
static BotRegistrar</*%BotClass%*/> /*%BOTNAME%*/ _reg(/*%BOTNAME%*/);

#endif  // LGEN_BOTS_%BOTNAME%
