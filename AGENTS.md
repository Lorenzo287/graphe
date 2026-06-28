# Graphe: Agent Manual

Graphe is a C/raylib educational visualizer for graph algorithms. The mature
mode is an interactive DFS visualizer with discovery and finish times,
color-coded nodes, and classified directed edges. BFS and tree traversal should
be treated as separate modes with their own educational state.

This file is for AI agents and maintainers working inside the repository. Keep
public usage information in `README.md`; keep internal workflow, architecture,
and verification guidance here.

## Documentation Roles

- `README.md`: public overview, setup, build, and usage.
- `AGENTS.md`: internal development and AI-agent instructions.
- `docs/ROADMAP.md`: temporal context, current plan, deferred decisions, and next
  work.

## Project Map

- `CMakeLists.txt`: cross-platform build configuration.
- `README.md`: public project overview and usage.
- `AGENTS.md`: internal development and AI-agent instructions.
- `docs/ROADMAP.md`: current plan and next steps.
- `src/main.c`: raylib application loop, playback controls, and input handling.
- `src/graph.*`: graph storage, visual state, labels, and sample graph setup.
- `src/graph_io.*`: `.graphe` file loading.
- `src/traversal.*`: algorithm trace generation/application for DFS, BFS, and
  the expression-tree traversal prototype.
- `src/platform_window.*`: small platform hooks such as Windows title-bar
  styling.
- `src/layout.*`: graph layout helpers.
- `src/render.*`: raylib drawing code for nodes, edges, labels, and UI text.
- `src/raygui_impl.c`: single raygui implementation translation unit.

## Fast Context

- Main entry point: `src/main.c`.
- Public user-facing surface: the native raylib window launched by `graphe`.
- Core traversal implementation: `src/traversal.c` and `src/graph.c`.
- Rendering surface: `src/render.c`.
- Layout logic: `src/layout.c`.
- Graph import format examples: `graphs/*.graphe`.
- Tests: none yet; add focused tests before making traversal behavior more
  complex.
- Build or task runner: CMake.
- Configuration: `CMakeLists.txt`.
- Generated files: `build/`; source of truth is the C source and CMake files.

## Operating Loop

- Start by checking the working tree with `git status --short` when the project
  is in git. Do not overwrite user changes.
- Read `AGENTS.md`, then `README.md` for public behavior, then `docs/ROADMAP.md`
  when choosing or continuing broad project work.
- Find the relevant files and tests before editing. Prefer `rg` or `rg --files`
  when available.
- State assumptions or ask a concise question when missing information affects
  architecture, behavior, data model, or public contracts.
- Make small, coherent source changes. Keep a lightweight note of follow-up
  docs, generated files, tooling metadata, and broader tests that should be
  handled before handoff or before switching topics.
- For small or trivial edits, use the smallest useful verification first rather
  than rebuilding the whole project after every change.
- After a substantial source change, several related small changes, or a change
  of topic, run the broader relevant checks and update docs/tooling that were
  intentionally batched.
- If deferred verification or follow-up work remains, record it in the roadmap,
  a task list, or the final response.

## Change Control

- Keep the diff focused on the current goal, but do not ignore nearby issues
  that directly affect the requested work.
- If you notice an obvious bug, inconsistency, risky design, or code that
  conflicts with these instructions, mention it and ask before changing it
  unless it directly blocks the requested task.
- Do not make unrequested behavioral changes, broad refactors, formatting
  sweeps, dependency changes, or cleanup passes without confirmation.
- Remove only the imports, variables, files, or generated outputs made obsolete
  by your own changes.
- Preserve existing public contracts unless the task explicitly changes them or
  the user agrees to the change.

## Project Rules

- Language/runtime versions: C99 for project code.
- Graphics dependencies: raylib for drawing/input/windowing and raygui for
  runtime controls, linked through CMake. Use plain C APIs first.
- Formatting command: `clang-format -i src/*.c src/*.h` when a formatting pass
  is useful. Treat `.clang-format` as the style reference, not as a reason for
  unrelated formatting churn.
- Lint command: none yet.
- Test command: none yet.
- Build command: `cmake -S . -B build/gcc-debug -G Ninja
  -DCMAKE_C_COMPILER=gcc` then `cmake --build build/gcc-debug`.
- Release build: `cmake -S . -B build/gcc-release -G Ninja
  -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Release` then
  `cmake --build build/gcc-release`.
- Editor support: CMake exports `compile_commands.json`; `.clangd` points clangd
  at `build/gcc-debug`.
- Naming and style: use `graphe_` or module-local names for new public helpers;
  keep structs and enums explicit and small. Use attached braces and declare
  local variables near first use.
- Architecture boundaries: algorithm code should produce/apply mode-specific
  state without depending on raylib. Rendering should consume graph or tree
  state and trace data, not run the traversal.
- Directedness belongs to `Graph`, not only the renderer. Undirected graphs are
  represented by one edge linked into both endpoint adjacency lists; keep
  traversal `O(V + E)` without duplicating undirected edge events.
- Dependency policy: avoid extra dependencies unless they clearly support the
  visualizer. raygui is the current UI helper. Graphviz may be added later as an
  optional layout helper, not as the primary renderer.
- Generated-file policy: do not commit `build/`, compiled binaries, or fetched
  dependency trees.

## Design Philosophy

- Keep algorithm state separate from rendering state. Share small helpers only
  where the concepts are genuinely common; do not force DFS, BFS, and tree
  traversal through one abstract event model when the educational point differs.
- Prefer simple visible progress over premature graphics complexity. Start with
  clear circles, lines, labels, and controls; add curves, physics, and Graphviz
  layout after the basic visualizer is solid.
- Make each animation frame explainable. In DFS, node colors, edge colors,
  active event, discovery times, finish times, and playback controls should
  agree with the DFS trace. BFS should emphasize depth and queue/frontier state.
  Tree traversal should emphasize preorder/inorder/postorder output on the
  expression tree sample.
- Keep the main graph event sentence visible near the graph canvas. The sidebar
  is for secondary state, controls, and settings.

## Verification

- Fast local check: inspect the relevant C files and build configuration.
- Focused tests: none yet.
- Full test suite: none yet.
- Build: `cmake -S . -B build/gcc-debug -G Ninja -DCMAKE_C_COMPILER=gcc` then
  `cmake --build build/gcc-debug`.
- Release or packaging check: none yet.

When a check cannot be run, say why and identify the most relevant untested
risk.

## Roadmap Use

- Treat the roadmap as the source of temporal context, not public
  documentation.
- Before starting broad work, read the roadmap. After completing planned work or
  changing direction, update it.
- Keep entries concrete enough that another agent can resume the project
  without reconstructing intent from old chats.
- If work is intentionally deferred, record the condition for returning to it.

## Safety

- Do not commit secrets, credentials, private keys, local environment files, or
  machine-specific paths.
- Prefer documented configuration over hard-coded local values.
- Confirm before destructive commands, migrations, data deletion, external
  writes, or changes that are difficult to reverse.
- Preserve third-party license notices and attribution requirements.
