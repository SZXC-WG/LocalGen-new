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

class BasicBot : public Player {
   public:
    const std::string botName;

   protected:
    explicit BasicBot(std::string name) : botName(std::move(name)) {}

   public:
    virtual void compute() = 0;
    virtual ~BasicBot() = default;
};

/// Factory / Registry
class BotFactory {
   public:
    using Creator = std::function<std::unique_ptr<BasicBot>()>;

    // Get global registry (lazy-init).
    static std::vector<Creator>& registry() {
        static std::vector<Creator> list;
        return list;
    }

    // Helper: create all registered bots.
    static std::vector<std::unique_ptr<BasicBot>> createAll() {
        std::vector<std::unique_ptr<BasicBot>> bots;
        for (auto& make : registry()) bots.emplace_back(make());
        return bots;
    }
};

/// Registration macro
#define REGISTER_BOT(ClassName, BotNameStr)                              \
    namespace {                                                          \
    struct ClassName##__registrar {                                      \
        ClassName##__registrar() {                                       \
            BotFactory::registry().emplace_back(                         \
                [] { return std::make_unique<ClassName>(BotNameStr); }); \
        }                                                                \
    };                                                                   \
    static ClassName##__registrar ClassName##__registrar_instance;       \
    }

/**
 * A simple guide of how to create a bot:
 *
 * 1. Add a new source file, e.g. MyBot.cpp.
 * 2. Define a class derived from BasicBot and implement compute().
 * 3. Register it with REGISTER_BOT(MyBot, "MyBot").
 * 4. Add the file to CMake (PROJECT_SOURCES).
 * 5. Build & run; the bot is now available.
 *
 * Follow the guide above and you can create your bot easily.
 */

#endif  // LGEN_MODULE_GE_BOT_H
