## Local Generals.io v6 (Qt) Bot 贡献指南  

*A Guide to Contributing Bots to Local Generals.io v6*

---

### 引言  

自 LocalGen 项目诞生以来，Bot 始终是这款本地化 Generals.io 的核心组成部分。  
在 v5 时代，由于架构设计不佳，Bot 的开发几乎完全依赖核心开发者：

- 只能使用 C++ 编写  
- 必须与主项目一同编译  
- 接口写死在源码中  

这些限制显著降低了社区的参与度，也限制了 Bot 整体水平。  

在 v6 中，我们决定彻底重构 Bot 体系，向所有开发者敞开大门。本文档将说明如何在 LocalGen v6 中编写、提交和维护 Bot。

---

### Bot 分类  

LocalGen v6 支持两种类型的 Bot：

1. **内置 Bot（built-in bots）**  
   - 源码位于 `src/bots/`  
   - 与 LocalGen 主程序一同编译  
   - 使用 C++ 编写，接入程度最深  
   - 可在 *Local Game* 与 *Web Game* 中直接选用  

2. **外部 Bot（external bots）**  
   - 独立可执行文件，可使用任何语言编写  
   - 通过网络接口与游戏通信  
   - 可加入 LocalGen 提供的外部 Bot 列表，由游戏进程托管启动  

---

### 如何贡献？  

欢迎以下形式的贡献（不限于）：

- 新 Bot  
- 现有 Bot 的功能改进  
- Bug 修复或性能优化  

#### 贡献内置 Bot  

1. 阅读 `src/bots/README.md` 获取接口与编译说明。  
2. 按项目 C++ 代码规范实现 Bot。  
3. 提交 Pull Request，并附带：  
   - 单元测试 / 对局复盘  
   - 性能评估结果  

#### 贡献外部 Bot  

1. 实现与 LocalGen 的通信协议（详见 *External-Bot API* 文档）。  
2. 提供可执行文件或源代码构建脚本。  
3. 在 PR 中注明：  
   - 依赖与启动方式  
   - 兼容的操作系统 / 运行环境  
   - 预期性能（回合耗时、峰值内存等）  

---

### Bot 社区贡献规范  

- **代码质量**  
  - 遵循项目编码风格；必要时附注释与文档  
- **测试充分**  
  - 提供不同地图、不同玩家数的测试记录  
- **策略水平**  
  - 不接受仅随机走子的「占位」Bot  
- **性能可靠**  
  - 单回合计算时长须低于游戏设定帧限  
  - 在千回合长局中稳定运行、无显著内存泄漏  

---

## Local Generals.io v6 (Qt) Bot Contribution Guide  

### Introduction  

Bots have always been at the core of LocalGen, our localized take on Generals.io.  
In version 5, however, a handful of architectural decisions placed heavy shackles on bot development:

- Bots could only be written in C++.  
- They had to be statically linked into the main binary.  
- All APIs were hard-coded.  

These constraints made community contributions difficult and stunted overall bot quality.

Version 6 removes those barriers. We rebuilt the entire bot subsystem so that anyone can write a bot—in any language—and drop it straight into the game. This guide shows you how.

---

### Bot Types  

LocalGen v6 recognises two kinds of bots:

1. **Built-in bots**  
   - Source lives in `src/bots/`.  
   - Compiled together with the core executable.  
   - Written in C++ for maximum integration and speed.  
   - Instantly usable in both *Local Game* and *Web Game* modes.  

2. **External bots**  
   - Stand-alone executables written in whatever language you prefer.  
   - Communicate with the game via a lightweight network protocol.  
   - Can be listed in the external-bot registry so the game launches and supervises them automatically.  

---

### How You Can Contribute  

We gladly accept:

- Brand-new bots.  
- Feature upgrades to existing bots.  
- Bug fixes and performance tweaks.  

#### Contributing a Built-in Bot  

1. Read `src/bots/README.md` for API details and build instructions.  
2. Implement your bot following the project’s C++ style guide.  
3. Open a pull request that includes:  
   - Unit tests and/or replay files demonstrating behaviour.  
   - A brief performance report (CPU time, memory usage).  

#### Contributing an External Bot  

1. Implement the communication protocol defined in the *External-Bot API* spec.  
2. Provide either a ready-to-run binary or a reliable build script.  
3. In your PR state clearly:  
   - How to install dependencies and launch the bot.  
   - Supported OSes / runtime environments.  
   - Expected performance (average turn time, peak memory).  

---

### Community Standards  

- **Code quality** — keep it clean, readable, and well-documented.  
- **Thorough testing** — supply results on multiple maps and player counts.  
- **Strategic depth** — we don’t accept “random-move” placeholders.  
- **Robust performance**  
  - Average per-turn time must stay within the engine’s frame limit.  
  - The bot should survive thousands of turns with no significant memory leaks.  

Happy hacking — and may your generals rule the battlefield! 🚩
