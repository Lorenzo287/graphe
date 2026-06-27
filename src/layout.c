#include "layout.h"

#include <math.h>
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
    int children[GRAPHE_MAX_NODES][GRAPHE_MAX_NODES];
    int child_counts[GRAPHE_MAX_NODES];
    int roots[GRAPHE_MAX_NODES];
    int root_count;
    int parent[GRAPHE_MAX_NODES];
    int subtree_leaves[GRAPHE_MAX_NODES];
} ForestLayoutState;

static void build_forest_from_trace(const Graph *graph, const Trace *trace,
                                    ForestLayoutState *state) {
    memset(state, 0, sizeof(*state));

    for (size_t i = 0; i < graph->node_count; i++) {
        state->parent[i] = -1;
        state->subtree_leaves[i] = 1;
    }

    for (size_t i = 0; i < trace->count; i++) {
        const TraceEvent *event = &trace->events[i];

        if (event->type != TRACE_EVENT_CLASSIFY_EDGE ||
            event->edge_type != EDGE_TREE) {
            continue;
        }

        const Edge *edge = &graph->edges[event->edge];
        int child_count = state->child_counts[edge->from];
        state->children[edge->from][child_count] = edge->to;
        state->child_counts[edge->from]++;
        state->parent[edge->to] = edge->from;
    }

    for (size_t i = 0; i < trace->count; i++) {
        const TraceEvent *event = &trace->events[i];

        if (event->type != TRACE_EVENT_DISCOVER_NODE) continue;
        if (state->parent[event->node] != -1) continue;

        state->roots[state->root_count] = event->node;
        state->root_count++;
    }
}

static int measure_subtree(int node, ForestLayoutState *state) {
    int leaves = 0;

    if (state->child_counts[node] == 0) {
        state->subtree_leaves[node] = 1;
        return 1;
    }

    for (int i = 0; i < state->child_counts[node]; i++)
        leaves += measure_subtree(state->children[node][i], state);

    state->subtree_leaves[node] = leaves;
    return leaves;
}

static float node_jitter(int node, int axis, float amount) {
    unsigned int value = (unsigned int)(node + 1) * 1103515245u;
    value ^= (unsigned int)(axis + 3) * 2654435761u;
    value = (value >> 16) & 1023u;
    return (((float)value / 1023.0f) - 0.5f) * amount;
}

static void place_subtree(Graph *graph, ForestLayoutState *state, int node,
                          int depth, float slot_width, float level_gap, float left,
                          float top, float *cursor) {
    float subtree_width = (float)state->subtree_leaves[node] * slot_width;
    float jitter_scale = depth == 0 ? 0.35f : 1.0f;
    graph->nodes[node].x = left + *cursor + subtree_width * 0.5f +
                           node_jitter(node, 0, 30.0f) * jitter_scale;
    graph->nodes[node].y =
        top + (float)depth * level_gap + node_jitter(node, 1, 18.0f);

    for (int i = 0; i < state->child_counts[node]; i++)
        place_subtree(graph, state, state->children[node][i], depth + 1, slot_width,
                      level_gap, left, top, cursor);

    if (state->child_counts[node] == 0) {
        *cursor += slot_width;
        return;
    }

    float child_leaves = 0.0f;
    for (int i = 0; i < state->child_counts[node]; i++)
        child_leaves += (float)state->subtree_leaves[state->children[node][i]];
    *cursor += subtree_width - child_leaves * slot_width;
}

void layout_trace_forest(Graph *graph, const Trace *trace, float left, float top,
                         float width, float level_gap) {
    if (graph->node_count == 0) return;

    ForestLayoutState state;
    build_forest_from_trace(graph, trace, &state);

    if (state.root_count == 0) {
        layout_circle(graph, left + width * 0.5f, top + level_gap, width * 0.25f);
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
}
