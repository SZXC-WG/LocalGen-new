/**
 * @file bot.h
 *
 * LocalGen Module: GameEngine
 *
 * Basic bot infrastructure: abstract class + static factory/registry.
 */

#ifndef LGEN_GAMEENGINE_BOT_H
#define LGEN_GAMEENGINE_BOT_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "player.h"

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
        std::vector<std::string> names;
        for (auto& kv : creators) {
            names.push_back(kv.first);
        }
        return names;
    }

    BasicBot* create(const std::string& name) const {
        auto it = creators.find(name);
        return it == creators.end() ? nullptr : it->second();
    }

   private:
    std::unordered_map<std::string, Creator> creators;
};

template <typename T>
struct BotRegistrar {
    BotRegistrar(const std::string& name) {
        BotFactory::instance().registerBot(
            name, []() -> BasicBot* { return new T(); });
    }
};

#endif  // LGEN_GAMEENGINE_BOT_H
