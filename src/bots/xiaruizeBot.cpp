/**
 * @file xiaruizeBot.cpp
 *
 * Legacy xiaruizeBot from LocalGen v5.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_XIARUIZEBOT
#define LGEN_BOTS_XIARUIZEBOT

#include <random>
#include <vector>

#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

class XiaruizeBot : public BasicBot {
   private:
    constexpr static Coord delta[] = {{0, 0}, {-1, 0}, {0, -1}, {1, 0}, {0, 1}};

    std::mt19937 mtrd{std::random_device{}()};

    pos_t height, width;
    index_t playerCnt;
    index_t id, team;
    std::vector<index_t> teamIds;
    game::config::Config config;

    BoardView board;
    std::vector<game::RankItem> rank;

    Coord spawnCoord;
    std::vector<int> operation;
    std::vector<std::vector<bool>> vis;
    int sendArmyProcess;
    int otherRobotProtection;
    Coord previousPos;

    inline int changeDirection(int x) {
        switch (x) {
            case 1: return 3;
            case 2: return 4;
            case 4: return 2;
            case 3: return 1;
        }
        return 0;
    }

    int dfs(Coord coord) {
        int checkOrder[5] = {0, 1, 2, 3, 4};
        std::shuffle(checkOrder + 1, checkOrder + 5, mtrd);

        for (int i = 1; i <= 4; ++i) {
            int x = checkOrder[i];
            Coord next = coord + delta[x];
            if (next.x < 1 || next.x > height || next.y < 1 || next.y > width)
                continue;
            if (isImpassableTile(board.tileAt(next).type)) continue;
            if (board.tileAt(next).type == TILE_GENERAL &&
                board.tileAt(next).occupier != id &&
                board.tileAt(next).army < board.tileAt(coord).army) {
                operation.push_back(x);
                previousPos = coord;
                return x;
            }
        }

        std::shuffle(checkOrder + 1, checkOrder + 5, mtrd);
        for (int i = 1; i <= 4; ++i) {
            int x = checkOrder[i];
            Coord next = coord + delta[x];
            if (next.x < 1 || next.x > height || next.y < 1 || next.y > width)
                continue;
            if (isImpassableTile(board.tileAt(next).type)) continue;
            if (board.tileAt(next).type == TILE_CITY &&
                board.tileAt(next).army <= board.tileAt(coord).army &&
                board.tileAt(next).occupier != id) {
                operation.push_back(x);
                previousPos = coord;
                return x;
            }
        }

        std::shuffle(checkOrder + 1, checkOrder + 5, mtrd);
        for (int i = 1; i <= 4; ++i) {
            int x = checkOrder[i];
            Coord next = coord + delta[x];
            if (next.x < 1 || next.x > height || next.y < 1 || next.y > width)
                continue;
            if (isImpassableTile(board.tileAt(next).type)) continue;
            if (vis[next.x][next.y]) continue;
            operation.push_back(x);
            previousPos = coord;
            return x;
        }

        std::shuffle(checkOrder + 1, checkOrder + 5, mtrd);
        for (int i = 1; i <= 4; ++i) {
            int x = checkOrder[i];
            Coord next = coord + delta[x];
            if (next.x < 1 || next.x > height || next.y < 1 || next.y > width)
                continue;
            if (isImpassableTile(board.tileAt(next).type)) continue;
            if (previousPos.x == next.x && previousPos.y == next.y) continue;
            operation.push_back(x);
            previousPos = coord;
            return x;
        }

        return -1;
    }

    Coord findGeneralPos() {
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).visible &&
                    board.tileAt(i, j).type == TILE_GENERAL &&
                    board.tileAt(i, j).occupier == id) {
                    return Coord(i, j);
                }
            }
        }
        return spawnCoord;
    }

   public:
    void init(index_t playerId,
              const game::GameConstantsPack& constants) override {
        id = playerId;
        height = constants.mapHeight;
        width = constants.mapWidth;
        playerCnt = constants.playerCount;
        teamIds = constants.teams;
        team = constants.teams.at(playerId);
        config = constants.config;

        spawnCoord = Coord(0, 0);
        operation.clear();
        vis.assign(height + 2, std::vector<bool>(width + 2, false));
        sendArmyProcess = 0;
        otherRobotProtection = 0;
        previousPos = Coord(-1, -1);
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<game::RankItem>& _rank) override {
        board = boardView;
        rank = _rank;

        moveQueue.clear();

        Coord coord = findGeneralPos();
        if (spawnCoord == Coord(0, 0)) {
            spawnCoord = coord;
        }

        if (!board.tileAt(coord).visible || board.tileAt(coord).army == 0 ||
            board.tileAt(coord).occupier != id) {
            vis.assign(height + 2, std::vector<bool>(width + 2, false));
            otherRobotProtection =
                std::max(0, std::min(static_cast<int>(operation.size()) - 10,
                                     static_cast<int>(mtrd() % 10)));
            sendArmyProcess = 1;
            return;
        }

        if (sendArmyProcess > 0) {
            if (sendArmyProcess > static_cast<int>(operation.size())) {
                sendArmyProcess = 0;
                return;
            }
            ++sendArmyProcess;
            if (otherRobotProtection > 0) {
                --otherRobotProtection;
                Coord next = coord + delta[operation[sendArmyProcess - 2]];
                moveQueue.emplace_back(MoveType::MOVE_ARMY, coord, next, false);
            } else {
                Coord next = coord + delta[operation[sendArmyProcess - 2]];
                moveQueue.emplace_back(MoveType::MOVE_ARMY, coord, next, true);
            }
            return;
        }

        vis[coord.x][coord.y] = true;
        int returnValue = dfs(coord);

        if (returnValue != -1) {
            Coord next = coord + delta[returnValue];
            moveQueue.emplace_back(MoveType::MOVE_ARMY, coord, next, true);
        } else {
            if (!operation.empty()) {
                int res = changeDirection(operation.back());
                operation.pop_back();
                Coord next = coord + delta[res];
                moveQueue.emplace_back(MoveType::MOVE_ARMY, coord, next, true);
            }
        }
    }
};

static BotRegistrar<XiaruizeBot> xiaruizeBot_reg("XiaruizeBot");

#endif
