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
};

/// Factory / Registry
class BotFactory {
   public:
    using Creator = std::function<std::unique_ptr<BasicBot>()>;

    static std::vector<Creator> list;
    static std::vector<std::string> nameList;

    // Get global registry (lazy-init).
    static inline std::vector<Creator>& registry() { return list; }
    static inline std::vector<std::string>& names() { return nameList; }

    // Helper: create a bot by its indice.
    static inline std::unique_ptr<BasicBot> create(std::size_t indice) {
        return registry()[indice]();
    }
    // Helper: get bot name by its indice.
    static inline std::string nameOf(std::size_t indice) {
        return names()[indice];
    }
};

/// Registration macro.
#define REGISTER_BOT(ClassName, BotNameStr)                              \
    namespace {                                                          \
    struct ClassName##__registrar {                                      \
        ClassName##__registrar() {                                       \
            BotFactory::registry().emplace_back(                         \
                [] { return std::make_unique<ClassName>(BotNameStr); }); \
            BotFactory::names().emplace_back(BotNameStr);                \
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
