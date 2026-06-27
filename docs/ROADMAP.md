# Roadmap

## Current Direction

Build Graphe as a native C/raylib educational visualizer for graph algorithms.
DFS is the mature graph mode. BFS still needs separate first-class state and
presentation. Tree traversal now has a first expression-tree prototype and
should continue toward a dedicated tree UI.

## Completed

- Chose C + raylib as the first renderer.
- Bootstrapped the repository with CMake, README, agent instructions, roadmap,
  and initial source files.
- Added a hardcoded sample directed graph that exercises tree, back, forward,
  and cross edge classifications.
- Added a traversal event trace model that is independent of raylib.
- Confirmed the initial raylib app runs locally.
- Added simple GCC/Ninja CMake build commands for debug and release builds.
- Added clangd support through CMake's `compile_commands.json` export and a
  `.clangd` config.
- Changed graph traversal to use outgoing adjacency chains with stable edge IDs,
  preserving `O(V + E)` traversal complexity.
- Added a first UI polish pass: dark mode, runtime settings drawer, TTF font
  loading with fallback, MSAA hinting, thicker edges, curved non-tree edges,
  traversal-forest layout, and default alphabetical traversal.
- Added graph canvas pan/zoom, node dragging, a modal settings overlay, larger
  UI text, distance-based edge curvature, and slight deterministic jitter in the
  traversal forest layout.
- Replaced the custom window chrome with native window controls, added raygui
  for runtime settings, and changed `Ctrl` + wheel/`+`/`-` to scale the app UI
  instead of resizing the OS window.
- Bundled Atkinson Hyperlegible regular under `assets/fonts/` with its OFL
  license text.
- Added prototype BFS and tree traversal modes alongside DFS, then identified
  that they need a cleaner mode split before they should be considered finished.
- Audited layout/camera coupling after resize: reset now centers on graph bounds
  and circular layout uses the smaller graph-canvas dimension.
- Aligned the current C source with the preferred style reference: attached
  braces and declarations near first use.
- Added best-effort Windows native title-bar coloring, `Ctrl+0` UI scale reset,
  a directed/undirected rendering toggle, visible directed-arrow rendering, and
  raygui hover colors that lighten from the current button state.
- Added `.graphe` file loading from the settings panel, with directed and
  undirected sample files.
- Moved the current event sentence into a bottom graph-canvas banner and added a
  subtle themed word-pattern background behind the graph.
- Promoted graph directedness into `Graph`: undirected graphs store one edge,
  maintain incident adjacency lists for efficient traversal, render without
  arrows, and DFS now uses undirected-style tree/back classification.
- Changed active DFS classification rendering so the classified edge color
  appears on the classification step instead of only on the following step.
- Added a first expression-tree mode for preorder, inorder, and postorder
  traversal output.

## Next Steps

- Split the runtime modes explicitly:
  - DFS: discovery/finish times, node colors, and edge classification.
  - BFS: depth/level, queue/frontier state, and BFS tree edges.
  - Tree traversal: continue replacing graph-specific UI with tree-specific
    controls and expression output.
- Replace fixed graph/event limits with dynamic storage before adding loading or
  editing.
- Consider an in-app graph editor after file loading and dynamic graph storage
  feel solid.
- Improve edge rendering with curved edges, reciprocal-edge handling, and
  self-loop support.
- Continue UI polish inside the native window: spacing, settings grouping, and
  cross-platform visual checks.
- Add focused tests for traversal timestamps and edge classifications.
- Add optional Graphviz layout import using `dot` or `neato` output.

## Deferred

- Force-directed physics layout with draggable/pinned nodes.
- WebAssembly/browser build target.
