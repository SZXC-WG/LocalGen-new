/**
 * @file bot.h
 *
 * LocalGen Module: GameEngine
 *
 * Basic bot infrastructure: abstract class + static factory/registry.
 */

#ifndef LGEN_MODULE_GE_BOT_H
#define LGEN_MODULE_GE_BOT_H 1

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "player.h"

class BasicBot : public Player {};

/// Factory / Registry
class BotFactory {
   public:
    using Creator = std::function<std::unique_ptr<BasicBot>()>;

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

    std::unique_ptr<BasicBot> create(const std::string& name) const {
        auto it = creators.find(name);
        return it == creators.end() ? nullptr : it->second();
    }

   private:
    std::unordered_map<std::string, Creator> creators;
};

template <typename T>
struct BotRegistrar {
    BotRegistrar(const std::string& name) {
        BotFactory::instance().registerBot(name, std::make_unique<T>);
    }
};

#endif  // LGEN_MODULE_GE_BOT_H
