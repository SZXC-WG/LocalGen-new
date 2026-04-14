// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file player.hpp
 *
 * LocalGen Module: core
 *
 * Players
 *
 * Players are basic participants of games.
 */

#ifndef LGEN_CORE_PLAYER_HPP
#define LGEN_CORE_PLAYER_HPP

#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <vector>

#include "board.hpp"
#include "message.hpp"
#include "move.hpp"
#include "utils.hpp"

struct GameConstantsPack;
struct RankItem;

/// Base struct for players.
class Player {
   protected:
    std::deque<Move> moveQueue;
    std::function<void(const std::string&)> sendMessageCb;

   public:
    Player() = default;
    virtual ~Player() = default;

   protected:
    inline void surrender() {
        moveQueue.clear();
        moveQueue.emplace_back(MoveType::SURRENDER);
    }

   public:
    inline void setSendMessageCallback(
        std::function<void(const std::string&)> cb) {
        sendMessageCb = std::move(cb);
    }

    inline void sendMessage(const std::string& text) {
        if (sendMessageCb) sendMessageCb(text);
    }

    virtual void init(index_t playerId, const GameConstantsPack& constants) = 0;

    virtual void requestMove(const BoardView& boardView,
                             const std::vector<RankItem>& rank) = 0;

    virtual void onGameEvent(const GameEvent& event) {
        std::visit(
            overloaded{[this, t = event.turn](const GameMessageWin& m) {
                           this->onWin(t, m);
                       },
                       [this, t = event.turn](const GameMessageCapture& m) {
                           this->onCapture(t, m);
                       },
                       [this, t = event.turn](const GameMessageSurrender& m) {
                           this->onSurrender(t, m);
                       },
                       [this, t = event.turn](const GameMessageText& m) {
                           this->onText(t, m);
                       }},
            event.data);
    }

   protected:
    virtual void onWin(turn_t turn, const GameMessageWin& msg) {}
    virtual void onCapture(turn_t turn, const GameMessageCapture& msg) {}
    virtual void onSurrender(turn_t turn, const GameMessageSurrender& msg) {}
    virtual void onText(turn_t turn, const GameMessageText& msg) {}

   public:
    std::deque<Move>& getMoveQueue() { return moveQueue; }

   public:
    inline Move step() {
        if (moveQueue.empty()) return Move();
        Move move = moveQueue.front();
        moveQueue.pop_front();
        return move;
    }
};

#endif  // LGEN_CORE_PLAYER_HPP
