# Graphe: Agent Notes

Graphe is a C99/raylib educational visualizer for graph algorithms. DFS is the
main mode. BFS and tree traversal are separate teaching modes with their own
state and UI needs.

Keep public usage in `README.md`. Keep current plans and deferred work in
`docs/ROADMAP.md`. Use this file only for internal workflow and architecture
guidance.

## Work Loop

- Start with `git status --short`; do not overwrite user changes.
- Read `README.md` for public behavior and `docs/ROADMAP.md` before broad or
  directional work.
- Find relevant files and tests before editing. Prefer `rg` or `rg --files`.
- Keep diffs scoped. Preserve public contracts unless the task explicitly
  changes them.
- Ask only when missing information affects behavior, architecture, data model,
  or another hard-to-reverse choice.
- Record deferred verification or follow-up work in the roadmap or final
  response.

## Project Map

- `src/main.c`: raylib app loop, playback controls, and input handling.
- `src/graph.*`: graph storage, labels, visual state, and sample graph setup.
- `src/graph_io.*`: `.graphe` file loading.
- `src/traversal.*`: DFS, BFS, and tree traversal trace generation/application.
- `src/layout.*`: graph layout helpers.
- `src/render.*`: drawing code for nodes, edges, labels, and UI.
- `src/platform_window.*`: platform hooks such as Windows title-bar styling.
- `src/raygui_impl.c`: single raygui implementation translation unit.
- `graphs/`: sample `.graphe` files.
- `tests/`: focused C tests.

## Engineering Rules

- Keep traversal logic independent of raylib. Rendering consumes graph/tree
  state and trace data; it does not run algorithms.
- Keep DFS, BFS, and tree traversal concepts distinct. Share helpers only where
  the model is genuinely common.
- Directedness belongs to `Graph`. Undirected graphs use one edge linked into
  both endpoint adjacency lists; traversal should stay `O(V + E)` without
  duplicate undirected edge events.
- Use plain C APIs first. Avoid new dependencies unless they clearly support the
  visualizer.
- Use `graphe_` or module-local names for new public helpers. Keep structs and
  enums explicit and small.
- Follow the existing C style: attached braces and local variables near first
  use. Run `clang-format -i src/*.c src/*.h` only when a formatting pass is
  useful.
- Do not commit generated build directories, compiled binaries, fetched
  dependency trees, secrets, or machine-specific paths.
- Preserve third-party license notices and attribution.

## Design Notes

- Make each animation frame explainable: colors, labels, active event text, and
  controls should agree with the current trace step.
- DFS emphasizes discovery/finish times and edge classification.
- BFS should emphasize levels, queue/frontier state, and the BFS tree.
- Tree traversal should emphasize preorder/inorder/postorder output on a tree
  layout, not graph-specific classifications.
- Keep the main graph event sentence near the graph canvas. Use the sidebar for
  secondary state, controls, and settings.

## Verification

- Use the README build and CTest commands for normal verification.
- For docs-only edits, a consistency review is enough.
- When a check cannot be run, say why and identify the most relevant untested
  risk.

## Roadmap

Use `docs/ROADMAP.md` as the current state and next-work source, not as public
documentation. Update it after planned work, direction changes, or intentional
deferrals.
