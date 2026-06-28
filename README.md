# Graphe

> *γραφή* (*graphê*): writing, drawing, a traced line.

Graphe is a small C/raylib project for visualizing graph algorithms. The main
mode is DFS, with node colors, discovery and finish times, active
traversal events, and color-coded edges. It supports also BFS and tree traversal.

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
- dark and light themes;
- a runtime mode selector for DFS, BFS, and tree traversal;
- BFS level labels and a depth gradient on visited nodes;
- directed and undirected graph semantics for DFS, including arrow rendering and
  undirected DFS edge classification;
- graph import from a small text format;
- a first expression-tree traversal mode with preorder, inorder, and postorder
  output;
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
- `O`: cycle tree traversal order.
- `T`: toggle dark mode.
- `L`: cycle layout mode.
- `A`: toggle alphabetical traversal order.
- `Ctrl` + `+` / `Ctrl` + `-`: scale the interface.
- `Ctrl` + `0`: reset the interface scale.
- `Ctrl` + mouse wheel: scale the interface.
- Left mouse drag: move a node.
- Left mouse drag on the graph background: pan the canvas.
- Mouse wheel over the graph: zoom the canvas.

Dragging a node switches the layout mode to manual.

## Graph Files

Settings includes a path field and `Load` button for `.graphe` files. The format
is line-oriented:

```text
# comments are ignored
directed
node A
node B
edge A B
```

Use `undirected` instead of `directed` for an undirected graph. `node` lines are
optional for nodes that appear in edges, but useful for isolated nodes. Labels
are single words up to 15 characters.

Example files live in `graphs/sample_directed.graphe` and
`graphs/sample_undirected.graphe`. The current loader still uses the project’s
fixed graph limits: 16 nodes and 64 edges.

## Fonts

The app bundles Atkinson Hyperlegible under `assets/fonts/` and loads it before
falling back to common system fonts such as Segoe UI on Windows or DejaVu Sans
on Linux. If no TrueType font is found, it uses raylib's default font.

## Window Icon

The app sets a small generated graph icon at runtime. To override it, place a
PNG at `assets/icons/graphe.png`; raylib will use it for the running window's
titlebar and taskbar icon. Embedding an icon into the `.exe` file itself is a
separate Windows resource step.

On Windows, Graphe also applies a best-effort native title-bar color matching the
current light or dark theme. This uses DWM attributes and depends on Windows
version support.

## Direction

The traversal implementation should remain separate from rendering. DFS, BFS,
and tree traversal should not be forced through DFS concepts: DFS emphasizes
timestamps and edge classification, BFS emphasizes depth, and tree traversal
emphasizes preorder/inorder/postorder output.
