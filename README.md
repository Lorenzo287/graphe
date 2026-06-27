# Graphe

> *γραφή* (*graphê*): writing, drawing, a traced line.

Graphe is a small C/raylib project for visualizing graph algorithms. It started
as a DFS visualizer and now supports DFS, BFS, and tree traversal modes with node
colors, discovery and finish times, active traversal events, and color-coded
edges.

The project is intentionally low-level enough to be educational: it uses a
native event loop, per-frame drawing, simple geometry, and explicit algorithm
state instead of a web framework or a full game engine.

## Current Status

The initial version contains:

- a hardcoded sample directed graph;
- a traversal trace with discover, examine edge, classify edge, and finish
  events;
- color-coded nodes and edges;
- keyboard playback controls;
- draggable nodes;
- dark and light themes;
- DFS, BFS, and tree traversal modes;
- circular, manual, and traversal-forest layouts;
- alphabetical traversal order by default.

## Build

Requirements:

- CMake 3.20 or newer;
- Ninja;
- GCC through MinGW/UCRT for the default build commands;
- raylib, either installed on the system or fetched into the build directory by
  CMake;
- raygui, fetched into the build directory by CMake.

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

- `Space`: play or pause the traversal trace.
- `Right`: step forward.
- `Left`: step backward.
- `0`: jump to the beginning.
- `Enter`: jump to the end.
- `R`: reset the current layout.
- `Tab`: open or close settings.
- `M`: cycle algorithm mode.
- `T`: toggle dark mode.
- `L`: cycle layout mode.
- `A`: toggle alphabetical traversal order.
- `Ctrl` + `+` / `Ctrl` + `-`: scale the interface.
- `Ctrl` + mouse wheel: scale the interface.
- Left mouse drag: move a node.
- Left mouse drag on the graph background: pan the canvas.
- Mouse wheel over the graph: zoom the canvas.

Dragging a node switches the layout mode to manual.

## Fonts

The app bundles Atkinson Hyperlegible under `assets/fonts/` and loads it before
falling back to common system fonts such as Segoe UI on Windows or DejaVu Sans
on Linux. If no TrueType font is found, it uses raylib's default font.

## Window Icon

The app sets a small generated graph icon at runtime. To override it, place a
PNG at `assets/icons/graphe.png`; raylib will use it for the running window's
titlebar and taskbar icon. Embedding an icon into the `.exe` file itself is a
separate Windows resource step.

## Direction

The code separates the traversal event trace from rendering. That should make it
reasonable to add more graph algorithms, Graphviz-assisted layout, or a
different renderer later without rewriting the rendering core.
