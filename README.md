<div align="center">
  <h1>Local Generals.io</h1>
  <a href="http://github.com/SZXC-WG/LocalGen-new/releases"><img alt="GitHub release" src="https://img.shields.io/github/release/SZXC-WG/LocalGen-new.svg" /></a>
  <a href="http://github.com/SZXC-WG/LocalGen-new/releases"><img alt="GitHub Release Date" src="https://img.shields.io/github/release-date/SZXC-WG/LocalGen-new.svg" /></a>
  <a href="http://github.com/SZXC-WG/LocalGen-new/LICENSE.md"><img alt="License" src="https://img.shields.io/github/license/SZXC-WG/LocalGen-new.svg" /></a>
  <a href="http://github.com/SZXC-WG/LocalGen-new/stargazers"><img alt="GitHub stars" src="https://img.shields.io/github/stars/SZXC-WG/LocalGen-new.svg?style=social" /></a>
  <h2><font color="red">branch <code>v5-EGE</code></font></h2>
</div>

Welcome to Local Generals.io, branch `v5-EGE`.

This branch contains the source code and (some) documentation of LocalGen Version 5.

GUI here is built with [EGE Graphics library](http://xege.org).

To contribute to this project, please create an issue or create a pull request. **Thank you for supporting LocalGen.**

## How to build the project

We now use CMake to build the project. Make sure you have CMake installed on your system before processing the following commands.

Make sure MinGW-w64 is installed on your computer and added to the system path before building the project.

Clone the project and switch to its directory in the terminal.

Then, run the following command to initialize:

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
```

You don't need to initialize again after your first build, unless `CMakeLists.txt` is changed.

After that, run the following command to build the executable `LGen.exe`:

```bash
cmake --build .
```

## Building within VS Code

We've recently migrated to the CMake build system. The best way to build the project is to use VS Code with the [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) extension.

`settings.json` is already configured to build with MinGW Makefiles. Please manually select the `GCC` compiler kit and set variant to `Release` for compilation.

## Known issues

- Web mode may not work when using speed 1. Try 2 or higher.
