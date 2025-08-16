## Local Generals.io Built-in Bots

This directory contains the source code for Local Generals.io bots that are **compiled directly into the executable**.

We welcome all kinds of contributions, including but not limited to:

- Adding a new bot
- Improving an existing bot
- Fixing bugs in existing bots

If you feel a bot doesn’t belong in this folder, please open an issue so we can discuss it.

---

### Adding a New Bot

Before submitting a bot to this folder, make sure it meets all of the following requirements:

1. The bot is written in C++ (or another language that can be compiled as C++).
2. Only C++17 features are used; your code must compile with a C++17-compliant compiler.
3. The entire bot resides in a single source file (`*.cpp`).
4. The source file includes the header `src/GameEngine/bot.h`.
5. Your bot class:

   - Has a unique name (from existing bots).
   - Inherits from `BasicBot`.
   - Overrides the `compute` method.

   See `DummyBot.cpp` for a reference implementation.

6. The bot is registered via the `REGISTER_BOT` macro.

If your bot satisfies all of the above, feel free to open a pull request!

If it does not, please refer to `docs/bot contributions.md` for alternative contribution methods.
