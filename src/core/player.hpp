// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file player.hpp
 *
 * LocalGen Module: core
 *
 * Players
 *
 * Players are basic participants of games.
 */

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
    void surrender() {
        moveQueue.clear();
        moveQueue.emplace_back(MoveType::SURRENDER);
    }

   public:
    void setSendMessageCallback(std::function<void(const std::string&)> cb) {
        sendMessageCb = std::move(cb);
    }

    void sendMessage(const std::string& text) {
        if (sendMessageCb) sendMessageCb(text);
    }

    virtual void init(index_t playerId, const GameConstantsPack& constants) = 0;

    virtual void requestMove(const BoardView& boardView,
                             const std::vector<RankItem>& rank) = 0;

    virtual void onGameEvent(const GameEvent& event) {
        std::visit(
            overloaded{
                [&](const GameMessageWin& m) { onWin(event.turn, m); },
                [&](const GameMessageCapture& m) { onCapture(event.turn, m); },
                [&](const GameMessageSurrender& m) {
                    onSurrender(event.turn, m);
                },
                [&](const GameMessageText& m) { onText(event.turn, m); }},
            event.data);
    }

   protected:
    virtual void onWin(turn_t, const GameMessageWin&) {}
    virtual void onCapture(turn_t, const GameMessageCapture&) {}
    virtual void onSurrender(turn_t, const GameMessageSurrender&) {}
    virtual void onText(turn_t, const GameMessageText&) {}

   public:
    std::deque<Move>& getMoveQueue() { return moveQueue; }

    Move step() {
        if (moveQueue.empty()) return {};
        Move move = moveQueue.front();
        moveQueue.pop_front();
        return move;
    }
};
