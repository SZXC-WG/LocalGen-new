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

Versions 1 to 4 are no longer maintained; v5 only accepts fixes for security issues and critical bugs. Version 6 is the current latest version, offering cross-platform support, a new modular backend design, better performance, and smarter bots. We highly encourage you to submit pull requests to `master` (the v6 branch) to contribute. The guide below is for v6.

## Building the project

> This section is for developers/contributors. The "Building the project" section in the README is for users who want to build the project themselves.

You'll need Qt 6.7+, CMake 3.19+, and Ninja 1.10+ installed under PATH. The project can be built with the following commands, using the actual Qt6 toolchain path on your system (typically `$QT_ROOT_DIR/lib/cmake/Qt6/qt.toolchain.cmake`):

```bash
cmake -B build -S . -G "Ninja Multi-Config" -DCMAKE_TOOLCHAIN_FILE=/path/to/qt.toolchain.cmake
cmake --build build --config [config]
```

Here, `[config]` can be `Debug` or `Release`. Use `Debug` to test for bugs, and `Release` to measure performance. You should test in both configs before submitting a PR.

If you use VS Code (or any of its derivatives), there is the [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) extension as an alternative for building the project. We've already configured `settings.json` for it to work out-of-the-box. (You still need to manually select the `Debug`/`Release` variant.)

## Contributing a bot

### Bot Types  

Currently, LocalGen v6 only accepts bots directly compiled into the executable, also known as *built-in bots*:

- Source lives in `src/bots/`.

- Compiled together with the core executable.
  
- Written in C++ for maximum integration and speed.

- Instantly usable in both *Local Game* and *Web Game* modes.

### How You Can Contribute  

We gladly accept:

- Brand-new bots.  
- Feature upgrades to existing bots.  
- Bug fixes and performance tweaks.  

**To create a bot from scratch:**

1. Read the [bot README](./src/bots/README.md) for API details and build instructions.  
2. Implement your bot following the project’s C++ style guide.  
3. Open a pull request that includes:  
   - Unit tests and/or replay files demonstrating behaviour.  
   - A brief performance report (CPU time, memory usage).  

You can use the provided bot simulator to test your bot.

### Bot Community Standards  

- **Code quality** — keep it clean, readable, and well-documented.  
- **Thorough testing** — supply results on multiple maps and player counts.  
- **Strategic depth** — we don’t accept “random-move” placeholders.  
- **Robust performance**  
  - Average per-turn time must stay within the engine’s frame limit.  
  - The bot should survive thousands of turns with no significant memory leaks.  

Happy hacking — and may your generals rule the battlefield! 🚩
