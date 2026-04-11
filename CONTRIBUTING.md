# Contributing to LocalGen

> [!IMPORTANT]
>
> *This is an unfinished early draft of our contribution guide, for reference only. Some stuff here may be wrong or outdated; don't take them seriously.* :joy:

## Project overview

*Before contributing, please read this section to get a full view of the project.*

The LocalGen project evolved with different UI frameworks.

| Project Version(s) | UI framework | Build System      | Repo / Branch                                                                      |
| ------------------ | ------------ | ----------------- | ---------------------------------------------------------------------------------- |
| prev               | Terminal     | MinGW Makefiles   | Local-Generals.io / [main](https://github.com/SZXC-WG/Local-Generals.io/tree/main) |
| v1                 | Terminal     | MinGW Makefiles   | LocalGen-new / (no independent branch)                                             |
| v2~v4              | EGE 20.08    | MinGW Makefiles   | LocalGen-new / (no independent branch)                                             |
| v5                 | EGE 24.04    | CMake / Makefiles | LocalGen-new / [v5.0](https://github.com/SZXC-WG/LocalGen-new/tree/v5.0)           |
| v6                 | Qt 6.7+      | CMake / Ninja     | LocalGen-new / master (this branch)                                                |

Versions 1 to 4 are no longer maintained; v5 only accepts fixes for security issues and critical bugs. Version 6 is the current latest version, offering cross-platform support, a new modular backend design, better performance, and smarter bots. We highly encourage you to submit pull requests to `master` (the v6 branch) to contribute.

## Contributing a bot

### Bot Types  

LocalGen v6 recognises two kinds of bots:

1. **Built-in bots**  
   - Source lives in `src/bots/`.  
   - Compiled together with the core executable.  
   - Written in C++ for maximum integration and speed.  
   - Instantly usable in both *Local Game* and *Web Game* modes.  

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

You can use the provided bot simulator to test your bot.

---

### Community Standards  

- **Code quality** — keep it clean, readable, and well-documented.  
- **Thorough testing** — supply results on multiple maps and player counts.  
- **Strategic depth** — we don’t accept “random-move” placeholders.  
- **Robust performance**  
  - Average per-turn time must stay within the engine’s frame limit.  
  - The bot should survive thousands of turns with no significant memory leaks.  

Happy hacking — and may your generals rule the battlefield! 🚩
