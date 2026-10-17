// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file bot.h
 *
 * LocalGen Module: core
 *
 * Basic bot infrastructure: abstract class + static factory/registry.
 */

#include <concepts>
#include <functional>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#include "player.hpp"

class BasicBot : public Player {};

/// Factory / Registry
class BotFactory {
   public:
    using Creator = std::function<BasicBot*()>;

    static BotFactory& instance() {
        static BotFactory f;
        return f;
    }

    void registerBot(const std::string& name, Creator c) {
        creators[name] = std::move(c);
    }

    std::vector<std::string> list() const {
        auto keys = std::views::keys(creators);
        return {keys.begin(), keys.end()};
    }

    BasicBot* create(const std::string& name) const {
        auto it = creators.find(name);
        return it == creators.end() ? nullptr : it->second();
    }

   private:
    std::unordered_map<std::string, Creator> creators;
};

template <std::derived_from<BasicBot> T>
struct BotRegistrar {
    BotRegistrar(const std::string& name) {
        BotFactory::instance().registerBot(name, [] { return new T(); });
    }
};
