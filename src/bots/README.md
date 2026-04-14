## Local Generals.io Built-in Bots

This directory hosts the source code of the bots that are **compiled directly into the Local Generals.io executable**.

We welcome every kind of contribution, including but not limited to:

- Adding a new bot
- Improving an existing bot
- Fixing bugs in existing bots

If you believe a bot doesn't belong here, please open an issue so we can discuss it.

### Current Bot Overview

Here is a summary of current bot implementations included in this directory. The implementation and time complexity values below are rough **per-turn strict worst-case** estimates. Here, $n$ is the number of map tiles, and $k$ is the number of candidate source stacks considered by a multi-source planner. Some advanced bots are typically faster on ordinary maps.

| Bot | Author | Enabled | Impl Complexity | Time Complexity | Algorithm Summary |
| --- | --- | --- | --- | --- | --- |
| DummyBot | AppOfficer | ❌ | Low | $O(n)$ | Example Heuristic Greedy |
| SmartRandomBot | AppOfficer / GoodCoder666 | ✅ | Low | $O(n)$ | Largest-Stack Greedy |
| XrzBot | xiaruize0911 | ❌ | Low | $O(n)$ | Focused Random Greedy |
| ZlyBot | AppOfficer | ✅ | Medium | $O(n)$ | Single-Focus BFS Heuristic |
| ZlyBot v2 | AppOfficer | ✅ | Medium | $O(n \log n)$ | Memory-Aware Weighted Search |
| ZlyBot v2.1 | AppOfficer | ✅ | Medium | $O(n \log n)$ | Dual-Focus Defensive Search |
| SzlyBot | GoodCoder666 | ✅ | Medium | $O(n)$ | Terrain-Weighted BFS Heuristic |
| GcBot | GoodCoder666 | ✅ | Medium | $O(n)$ | Adaptive Heuristic BFS |
| XiaruizeBot | xiaruize0911 | ✅ | High | $O(kn^2)$ | Multi-Source Strategic Search |
| KutuBot | pinkHC | ✅ | High | $O(n \log n)$ | Unified Strategic Objective Planner |
| LyBot | pinkHC | ❌ | High | $O(n^2)$ | Multiplayer Objective Planner |
| oimbot | oimasterkafuu | ✅ | High | $O(n^3)$ | Stance-Based Strategic Planner |

### Adding a New Bot

Before submitting a bot to this folder, make sure it satisfies **all** of the following requirements:

1. The bot is written in C++ (or another language that can be compiled as C++).
2. It uses C++17 features only; the code must compile with a C++17-compliant compiler.
3. The entire implementation lives in a single source file (`*.cpp`).
4. That source file includes the header `src/core/bot.h`.
5. Your bot class
   - Has a unique name (i.e., does not clash with existing bots).
   - Inherits from `BasicBot`.
   - Overrides the `requestMove` method.
6. The bot is registered via the `REGISTER_BOT` macro.

See `dummyBot.cpp` for a reference implementation.

### Submission Checklist

If your bot meets the above requirements:

1. Place the source file in `src/bots/`.
2. Give the file a clear, unique name (e.g., `MyUniqueBot.cpp`).
3. Add it to `LOCALGEN_BOT_SOURCES` in `CMakeLists.txt`.
4. Commit your changes and open a pull request. 🎉

If your bot does **not** meet all of these requirements, please read our [contribution guide](../../CONTRIBUTING.md#contributing-a-bot) for alternative contribution paths.
