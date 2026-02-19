/**
 * @file xrzBot.cpp
 *
 * Legacy xrzBot from LocalGen v5.
 *
 * @copyright Copyright (c) SZXC Work Group.
 */

#ifndef LGEN_BOTS_XRZBOT
#define LGEN_BOTS_XRZBOT

#include <random>
#include <vector>

#include "../GameEngine/bot.h"
#include "../GameEngine/game.hpp"

class XrzBot : public BasicBot {
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

    Coord previousPos;
    Coord currentPos;
    std::vector<std::vector<int>> visitTime;
    int turnCount;
    army_t armyNow;

    Coord findMaxArmyPos() {
        army_t maxArmy = 0;
        Coord maxCoo = Coord(1, 1);
        for (pos_t i = 1; i <= height; ++i) {
            for (pos_t j = 1; j <= width; ++j) {
                if (board.tileAt(i, j).visible &&
                    board.tileAt(i, j).occupier == id) {
                    if (board.tileAt(i, j).army > maxArmy) {
                        maxArmy = board.tileAt(i, j).army;
                        maxCoo = Coord(i, j);
                    }
                }
            }
        }
        return maxCoo;
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

        previousPos = Coord(-1, -1);
        currentPos = Coord(-1, -1);
        visitTime.assign(height + 2, std::vector<int>(width + 2, 0));
        turnCount = 0;
        armyNow = 0;
    }

    void requestMove(const BoardView& boardView,
                     const std::vector<game::RankItem>& _rank) override {
        board = boardView;
        rank = _rank;

        moveQueue.clear();

        if (currentPos == Coord(-1, -1) || board.tileAt(currentPos).army == 0 ||
            board.tileAt(currentPos).occupier != id) {
            for (auto& row : visitTime)
                for (auto& cell : row) cell = 0;
            currentPos = findMaxArmyPos();
        }

        armyNow = board.tileAt(currentPos).army;
        turnCount++;
        visitTime[currentPos.x][currentPos.y]++;

        int checkOrder[5] = {0, 1, 2, 3, 4};
        std::shuffle(checkOrder + 1, checkOrder + 5, mtrd);

        int okDir = 0;
        for (int j = 1; j <= 4; ++j) {
            int i = checkOrder[j];
            Coord next = currentPos + delta[i];
            if (next.x < 1 || next.x > height || next.y < 1 || next.y > width)
                continue;
            if (isImpassableTile(board.tileAt(next).type)) continue;
            okDir = i;

            if (board.tileAt(next).occupier != id &&
                board.tileAt(next).type == TILE_GENERAL) {
                previousPos = currentPos;
                currentPos = next;
                moveQueue.emplace_back(MoveType::MOVE_ARMY, previousPos, next,
                                       false);
                return;
            }
            if (board.tileAt(next).type == TILE_CITY &&
                board.tileAt(next).army <= board.tileAt(currentPos).army &&
                board.tileAt(next).occupier != id) {
                previousPos = currentPos;
                currentPos = next;
                moveQueue.emplace_back(MoveType::MOVE_ARMY, previousPos, next,
                                       false);
                return;
            }
        }

        int timeToTry = 1000;
        while (timeToTry--) {
            int i = (mtrd() % 4 + mtrd() % 4 + mtrd() % 4) % 4 + 1;
            Coord next = currentPos + delta[i];
            if (next.x < 1 || next.x > height || next.y < 1 || next.y > width)
                continue;
            if (isImpassableTile(board.tileAt(next).type)) continue;

            if (board.tileAt(next).occupier != id &&
                board.tileAt(next).type == TILE_GENERAL) {
                previousPos = currentPos;
                currentPos = next;
                moveQueue.emplace_back(MoveType::MOVE_ARMY, previousPos, next,
                                       false);
                return;
            }
            if (board.tileAt(next).type == TILE_CITY &&
                board.tileAt(next).army <= board.tileAt(currentPos).army &&
                board.tileAt(next).occupier == -1) {
                previousPos = currentPos;
                currentPos = next;
                moveQueue.emplace_back(MoveType::MOVE_ARMY, previousPos, next,
                                       false);
                return;
            }

            int cnt = 4;
            if (next.x == previousPos.x && next.y == previousPos.y)
                cnt += turnCount * 10;
            if (board.tileAt(next).occupier != id &&
                board.tileAt(next).occupier != -1)
                cnt--;
            if (board.tileAt(next).type == TILE_BLANK) cnt--;
            if (board.tileAt(next).type == TILE_SWAMP) cnt += 2;
            if (board.tileAt(next).occupier == -1) cnt--;
            if (board.tileAt(next).occupier == id &&
                board.tileAt(next).army >= 2000)
                cnt--;
            cnt += std::max(0, visitTime[next.x][next.y] * 10);

            if (mtrd() % cnt == 0) {
                previousPos = currentPos;
                currentPos = next;
                moveQueue.emplace_back(MoveType::MOVE_ARMY, previousPos, next,
                                       false);
                return;
            }
        }

        if (okDir != 0) {
            Coord next = currentPos + delta[okDir];
            previousPos = currentPos;
            currentPos = next;
            moveQueue.emplace_back(MoveType::MOVE_ARMY, previousPos, next,
                                   false);
        }
    }
};

static BotRegistrar<XrzBot> xrzBot_reg("XrzBot");

#endif
