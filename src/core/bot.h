/**
 * @file bot.h
 *
 * LocalGen Module: core
 *
 * Basic bot infrastructure: abstract class + static factory/registry.
 */

#ifndef LGEN_CORE_BOT_H
#define LGEN_CORE_BOT_H

#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "player.hpp"

namespace bot_random {

struct SeedContext {
    bool active = false;
    std::uint64_t gameSeed = 0;
    std::uint64_t counter = 0;
};

inline SeedContext& threadSeedContext() {
    static thread_local SeedContext context;
    return context;
}

inline void beginDeterministicSeeds(std::uint64_t gameSeed) {
    threadSeedContext() = SeedContext{true, gameSeed, 0};
}

inline void endDeterministicSeeds() { threadSeedContext() = SeedContext{}; }

inline std::mt19937 makeBotRng(std::uint64_t salt = 0) {
    SeedContext& context = threadSeedContext();
    if (!context.active) return std::mt19937(std::random_device{}());

    const std::uint64_t serial = context.counter++;
    const std::uint64_t seed =
        context.gameSeed + salt +
        0x9E3779B97F4A7C15ULL * static_cast<std::uint64_t>(serial + 1);
    std::seed_seq seedSeq{
        static_cast<std::uint32_t>(seed),
        static_cast<std::uint32_t>(seed >> 32U),
        static_cast<std::uint32_t>(salt),
        static_cast<std::uint32_t>(serial)};
    return std::mt19937(seedSeq);
}

}  // namespace bot_random

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

#endif  // LGEN_CORE_BOT_H
