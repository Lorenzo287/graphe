# Roadmap

## Current Direction

Build Graphe as a native C/raylib educational visualizer for graph algorithms.

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
- Generalized the traversal trace pipeline and added BFS and tree traversal
  modes alongside DFS.
- Audited layout/camera coupling after resize: reset now centers on graph bounds
  and circular layout uses the smaller graph-canvas dimension.
- Aligned the current C source with the preferred style reference: attached
  braces and declarations near first use.

## Next Steps

- Improve edge rendering with curved edges, reciprocal-edge handling, and
  self-loop support.
- Continue UI polish inside the native window: spacing, settings grouping, and
  cross-platform visual checks.
- Add graph loading from a small text format.
- Add focused tests for traversal timestamps and edge classifications.
- Add optional Graphviz layout import using `dot` or `neato` output.

## Deferred

- Force-directed physics layout with draggable/pinned nodes.
- Runtime directed/undirected graph semantics and undirected-specific DFS edge
  classification.
- WebAssembly/browser build target.
