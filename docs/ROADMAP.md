# Roadmap

## Current Direction

Build Graphe as a native C/raylib educational visualizer for graph algorithms,
starting with DFS.

## Completed

- Chose C + raylib as the first renderer.
- Bootstrapped the repository with CMake, README, agent instructions, roadmap,
  and initial source files.
- Added a hardcoded sample directed graph that exercises tree, back, forward,
  and cross edge classifications.
- Added a DFS event trace model that is independent of raylib.
- Confirmed the initial raylib app runs locally.
- Added simple GCC/Ninja CMake build commands for debug and release builds.
- Added clangd support through CMake's `compile_commands.json` export and a
  `.clangd` config.
- Aligned the current C source with the preferred style reference: attached
  braces and declarations near first use.

## Next Steps

- Improve edge rendering with curved edges, reciprocal-edge handling, and
  self-loop support.
- Add graph loading from a small text format.
- Add focused tests for DFS timestamps and edge classifications.
- Add optional Graphviz layout import using `dot` or `neato` output.

## Deferred

- Force-directed physics layout with draggable/pinned nodes.
- BFS visualization.
- Tree traversal visualization.
- WebAssembly/browser build target.
