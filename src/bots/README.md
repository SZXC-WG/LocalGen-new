## Local Generals.io Built-in Bots

This directory hosts the source code of the bots that are **compiled directly into the Local Generals.io executable**.

We welcome every kind of contribution, including but not limited to:

- Adding a new bot
- Improving an existing bot
- Fixing bugs in existing bots

If you believe a bot doesn’t belong here, please open an issue so we can discuss it.

---

### Adding a New Bot

Before submitting a bot to this folder, make sure it satisfies **all** of the following requirements:

1. The bot is written in C++ (or another language that can be compiled as C++).
2. It uses C++17 features only; the code must compile with a C++17-compliant compiler.
3. The entire implementation lives in a single source file (`*.cpp`).
4. That source file includes the header `src/GameEngine/bot.h`.
5. Your bot class
   - Has a unique name (i.e., does not clash with existing bots).
   - Inherits from `BasicBot`.
   - Overrides the `compute` method.
6. The bot is registered via the `REGISTER_BOT` macro.

See `DummyBot.cpp` for a reference implementation.

---

### Submission Checklist

If your bot meets the above requirements:

1. Place the source file in `src/bots/`.
2. Give the file a clear, unique name (e.g., `MyUniqueBot.cpp`).
3. Add that file to the `PROJECT_SOURCES` list in the top-level `CMakeLists.txt`.
4. Commit your changes and open a pull request. 🎉

If your bot does **not** meet all of these requirements, please read `docs/bot contributions.md` for alternative contribution paths.
