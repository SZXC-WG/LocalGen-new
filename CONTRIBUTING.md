# Contributing to LocalGen

> [!IMPORTANT]
>
> *This is an unfinished early draft of our contribution guide, for reference only. Some stuff here may be wrong or outdated; don't take them seriously.* :joy:

LocalGen’s development is organized across three groups of branches:

- **`master` | `v6.x`**:  
  The `master` branch serves as the main development branch for version 6.  
  **Starting from v6, we will be migrating the UI to the Qt library**, aiming for better performance and cross-platform support. The source code will also undergo a significant refactor to improve maintainability.

- **`v5.x`**:  
  This branch covers version 5, which is now going to **long-term maintenance**.  
  Versions 2 to 5 of LocalGen are built with [EGE Graphics](https://xege.org) (24.04), and we’re working on creating an object-oriented system based on it.  
  **Note:** Releases for this branch are **Windows-only** due to EGE 24.04’s limitations. However, if you want to run it on Linux or macOS, [**Wine**](https://www.winehq.org) might be an option. (We have tested, it worked well on a Ubuntu 22.04 LTS machine.)

If you'd like to contribute, please submit a pull request to the `master` branch. We appreciate your help!

## Contributing a bot

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
