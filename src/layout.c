#include "layout.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void layout_circle(Graph *graph, float center_x, float center_y, float radius) {
    const float two_pi = 6.28318530718f;

    if (graph->node_count == 0) return;

    for (size_t i = 0; i < graph->node_count; i++) {
        float angle = -1.57079632679f + two_pi * (float)i / (float)graph->node_count;
        graph->nodes[i].x = center_x + cosf(angle) * radius;
        graph->nodes[i].y = center_y + sinf(angle) * radius;
    }
}

typedef struct ForestLayoutState {
    int *first_child;
    int *last_child;
    int *next_sibling;
    int *child_counts;
    int *roots;
    int root_count;
    int *parent;
    int *subtree_leaves;
    size_t node_count;
} ForestLayoutState;

static void forest_layout_state_free(ForestLayoutState *state) {
    free(state->first_child);
    free(state->last_child);
    free(state->next_sibling);
    free(state->child_counts);
    free(state->roots);
    free(state->parent);
    free(state->subtree_leaves);
    memset(state, 0, sizeof(*state));
}

static bool forest_layout_state_init(ForestLayoutState *state, size_t node_count) {
    memset(state, 0, sizeof(*state));
    state->node_count = node_count;

    if (node_count == 0) return true;

    state->first_child = malloc(node_count * sizeof(*state->first_child));
    state->last_child = malloc(node_count * sizeof(*state->last_child));
    state->next_sibling = malloc(node_count * sizeof(*state->next_sibling));
    state->child_counts = calloc(node_count, sizeof(*state->child_counts));
    state->roots = malloc(node_count * sizeof(*state->roots));
    state->parent = malloc(node_count * sizeof(*state->parent));
    state->subtree_leaves = malloc(node_count * sizeof(*state->subtree_leaves));

    if (state->first_child == NULL || state->last_child == NULL ||
        state->next_sibling == NULL || state->child_counts == NULL ||
        state->roots == NULL || state->parent == NULL ||
        state->subtree_leaves == NULL) {
        forest_layout_state_free(state);
        return false;
    }

    for (size_t i = 0; i < node_count; i++) {
        state->first_child[i] = -1;
        state->last_child[i] = -1;
        state->next_sibling[i] = -1;
        state->parent[i] = -1;
        state->subtree_leaves[i] = 1;
    }

    return true;
}

static void append_child(ForestLayoutState *state, int parent, int child) {
    if (state->parent[child] != -1) return;

    state->parent[child] = parent;
    state->next_sibling[child] = -1;

    if (state->first_child[parent] == -1) {
        state->first_child[parent] = child;
    } else {
        state->next_sibling[state->last_child[parent]] = child;
    }

    state->last_child[parent] = child;
    state->child_counts[parent]++;
}

/*
 * Derives a forest from EDGE_TREE classification events instead of from raw
 * graph edges. That keeps the layout aligned with what the current traversal is
 * teaching, even when the graph has many non-tree edges.
 */
static void build_forest_from_trace(const Graph *graph, const Trace *trace,
                                    ForestLayoutState *state) {
    for (size_t i = 0; i < trace->count; i++) {
        const TraceEvent *event = &trace->events[i];

        if (event->type != TRACE_EVENT_CLASSIFY_EDGE ||
            event->edge_type != EDGE_TREE) {
            continue;
        }
        if (event->from < 0 || event->to < 0) continue;
        if ((size_t)event->from >= graph->node_count ||
            (size_t)event->to >= graph->node_count) {
            continue;
        }

        append_child(state, event->from, event->to);
    }

    for (size_t i = 0; i < trace->count; i++) {
        const TraceEvent *event = &trace->events[i];

        if (event->type != TRACE_EVENT_DISCOVER_NODE) continue;
        if (event->node < 0 || (size_t)event->node >= graph->node_count) continue;
        if (state->parent[event->node] != -1) continue;
        if ((size_t)state->root_count >= state->node_count) continue;

        state->roots[state->root_count] = event->node;
        state->root_count++;
    }
}

/*
 * Counts leaves in each traversal subtree so horizontal space can be allocated
 * by visible breadth rather than by raw node count.
 */
static int measure_subtree(int node, ForestLayoutState *state) {
    int leaves = 0;

    if (state->child_counts[node] == 0) {
        state->subtree_leaves[node] = 1;
        return 1;
    }

    for (int child = state->first_child[node]; child != -1;
         child = state->next_sibling[child]) {
        leaves += measure_subtree(child, state);
    }

    state->subtree_leaves[node] = leaves;
    return leaves;
}

static float node_jitter(int node, int axis, float amount) {
    unsigned int value = (unsigned int)(node + 1) * 1103515245u;
    value ^= (unsigned int)(axis + 3) * 2654435761u;
    value = (value >> 16) & 1023u;
    return (((float)value / 1023.0f) - 0.5f) * amount;
}

/*
 * Places one measured subtree in its horizontal slot. The small deterministic
 * jitter keeps large examples from looking perfectly mechanical without making
 * the layout change between frames.
 */
static void place_subtree(Graph *graph, ForestLayoutState *state, int node,
                          int depth, float slot_width, float level_gap, float left,
                          float top, float *cursor) {
    float subtree_width = (float)state->subtree_leaves[node] * slot_width;
    float jitter_scale = depth == 0 ? 0.35f : 1.0f;
    graph->nodes[node].x = left + *cursor + subtree_width * 0.5f +
                           node_jitter(node, 0, 30.0f) * jitter_scale;
    graph->nodes[node].y =
        top + (float)depth * level_gap + node_jitter(node, 1, 18.0f);

    for (int child = state->first_child[node]; child != -1;
         child = state->next_sibling[child]) {
        place_subtree(graph, state, child, depth + 1, slot_width, level_gap, left,
                      top, cursor);
    }

    if (state->child_counts[node] == 0) {
        *cursor += slot_width;
        return;
    }

    float child_leaves = 0.0f;
    for (int child = state->first_child[node]; child != -1;
         child = state->next_sibling[child]) {
        child_leaves += (float)state->subtree_leaves[child];
    }
    *cursor += subtree_width - child_leaves * slot_width;
}

/*
 * Main traversal-forest layout entry point. It falls back to a circular layout
 * when no tree edges are available, which covers empty or not-yet-classified
 * traces.
 */
void layout_trace_forest(Graph *graph, const Trace *trace, float left, float top,
                         float width, float level_gap) {
    if (graph->node_count == 0) return;

    ForestLayoutState state;
    if (!forest_layout_state_init(&state, graph->node_count)) {
        layout_circle(graph, left + width * 0.5f, top + level_gap, width * 0.25f);
        return;
    }

    build_forest_from_trace(graph, trace, &state);

    if (state.root_count == 0) {
        layout_circle(graph, left + width * 0.5f, top + level_gap, width * 0.25f);
        forest_layout_state_free(&state);
        return;
    }

    int total_leaves = 0;
    for (int i = 0; i < state.root_count; i++)
        total_leaves += measure_subtree(state.roots[i], &state);

    if (total_leaves <= 0) total_leaves = 1;

    float slot_width = width / (float)total_leaves;
    float cursor = 0.0f;

    for (int i = 0; i < state.root_count; i++)
        place_subtree(graph, &state, state.roots[i], 0, slot_width, level_gap, left,
                      top, &cursor);

    forest_layout_state_free(&state);
}
