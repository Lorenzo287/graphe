# Graphe

> *γραφή* (*graphê*): writing, drawing, a traced line.

Graphe is a small C/raylib project for visualizing graph algorithms. The first
goal is an interactive DFS visualizer that shows node colors, discovery and
finish times, active traversal events, and directed edge classifications.

The project is intentionally low-level enough to be educational: it uses a
native event loop, per-frame drawing, simple geometry, and explicit algorithm
state instead of a web framework or a full game engine.

## Current Status

The initial version contains:

- a hardcoded sample directed graph;
- a DFS trace with discover, examine edge, classify edge, and finish events;
- color-coded nodes and edges;
- keyboard playback controls;
- draggable nodes;
- a simple circular layout.

## Build

Requirements:

- CMake 3.20 or newer;
- Ninja;
- GCC through MinGW/UCRT for the default build commands;
- raylib, either installed on the system or fetched into the build directory by
  CMake.

Configure and build a debug executable:

```sh
cmake -S . -B build/gcc-debug -G Ninja -DCMAKE_C_COMPILER=gcc
cmake --build build/gcc-debug
```

This uses GCC + Ninja and does not require the Visual Studio developer shell.
CMake also writes `build/gcc-debug/compile_commands.json` for clangd.

Configure and build an optimized release executable:

```sh
cmake -S . -B build/gcc-release -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release
cmake --build build/gcc-release
```

Release uses CMake's normal `Release` build type, which enables the compiler's
release optimizations and defines `NDEBUG`.

You can still configure manually with `cmake -S . -B some-build-dir`, but on
Windows CMake may auto-select a Visual Studio generator when no generator is
specified.

## Editor Support

clangd reads the compilation database through `.clangd`, which currently points
to `build/gcc-debug`. Run the debug configure command once before expecting
clangd to resolve project includes and compiler flags.

## Controls

- `Space`: play or pause the DFS trace.
- `Right`: step forward.
- `Left`: step backward.
- `Home`: jump to the beginning.
- `End`: jump to the end.
- `R`: reset the circular layout.
- Left mouse drag: move a node.

## Direction

The code separates the DFS event trace from rendering. That should make it
reasonable to add BFS, tree traversal, Graphviz-assisted layout, or a different
renderer later without rewriting the algorithm core.
