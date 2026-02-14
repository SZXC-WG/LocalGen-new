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
#include "../GameEngine/game.hpp"

// You can include additional headers you need.
#include <random>

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

    bool unpassable(tile_type_e type) {
        return type == TILE_MOUNTAIN || type == TILE_LOOKOUT ||
               type == TILE_OBSERVATORY;
    }

    // Edit the `requestMove` method to implement your bot's main logic.
    // This method will be called **every turn**. Make that into your bot's
    // logic.
    void requestMove(const BoardView& boardView) override {
        // This DummyBot uses a derivative of the smartRandomBot from LG v5.
        // As LG v6 removed the outdated "focus" feature, this bot will choose a
        // maximum tile every time.
        static std::deque<Coord> lastCoord[20];
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
        struct node {
            int type;
            index_t team;
            long long army;
            int dir;
            std::ptrdiff_t lastCount;
        };
        node p[5];
        int pl = 0;
        for (int i = 1; i <= 4; ++i) {
            int nx = start.x + deltaX[i], ny = start.y + deltaY[i];
            if (nx < 1 || nx > height || ny < 1 || ny > width ||
                unpassable(boardView.tileAt(nx, ny).type))
                continue;
            p[++pl] = {boardView.tileAt(nx, ny).type,
                       boardView.tileAt(nx, ny).occupier,
                       boardView.tileAt(nx, ny).army, i,
                       std::find(lastCoord[id].rbegin(), lastCoord[id].rend(),
                                 Coord(nx, ny)) -
                           lastCoord[id].rbegin()};
        }
        bool rdret = rng() % 2;
        auto cmp = [&](node a, node b) -> bool {
            if (a.lastCount != b.lastCount) return a.lastCount > b.lastCount;
            if (a.type == 3 && a.team != id) return true;
            if (b.type == 3 && b.team != id) return false;
            if (a.team == 0) return rdret;
            if (b.team == 0) return !rdret;
            if (a.team == id && b.team != id) return false;
            if (a.team != id && b.team == id) return true;
            if (a.team == id && b.team == id) return a.army > b.army;
            return a.army < b.army;
        };
        std::sort(p + 1, p + pl + 1, cmp);
        moveQueue.emplace_back(
            MoveType::MOVE_ARMY, start,
            Coord(start.x + deltaX[p[1].dir], start.y + deltaY[p[1].dir]),
            true);
    }
};

// Do not forget to register your bot.
// And, append it to the CMakeLists.txt (not for this one).
static BotRegistrar<DummyBot> dummyBot_reg("DummyBot");

#endif  // LGEN_BOTS_DUMMYBOT
