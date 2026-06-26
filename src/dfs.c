#include "dfs.h"

#include <string.h>

typedef struct DfsState {
    NodeColor colors[GRAPHE_MAX_NODES];
    int discover_times[GRAPHE_MAX_NODES];
    int time;
} DfsState;

static void push_event(DfsTrace *trace, DfsEvent event) {
    if (trace->count >= GRAPHE_MAX_EVENTS) return;

    trace->events[trace->count] = event;
    trace->count++;
}

static DfsEvent make_node_event(DfsEventType type, int node, int time) {
    DfsEvent event;

    event.type = type;
    event.node = node;
    event.edge = -1;
    event.edge_type = EDGE_UNCLASSIFIED;
    event.time = time;

    return event;
}

static DfsEvent make_edge_event(DfsEventType type, int edge, EdgeType edge_type) {
    DfsEvent event;

    event.type = type;
    event.node = -1;
    event.edge = edge;
    event.edge_type = edge_type;
    event.time = -1;

    return event;
}

static EdgeType classify_seen_edge(const DfsState *state, int from, int to) {
    if (state->colors[to] == NODE_GRAY) return EDGE_BACK;

    if (state->discover_times[from] < state->discover_times[to]) return EDGE_FORWARD;

    return EDGE_CROSS;
}

static void visit_node(const Graph *graph, int node, DfsState *state,
                       DfsTrace *trace) {
    state->colors[node] = NODE_GRAY;
    state->time++;
    state->discover_times[node] = state->time;
    push_event(trace, make_node_event(DFS_EVENT_DISCOVER_NODE, node, state->time));

    for (size_t i = 0; i < graph->edge_count; i++) {
        const Edge *edge = &graph->edges[i];

        if (edge->from != node) continue;

        push_event(trace, make_edge_event(DFS_EVENT_EXAMINE_EDGE, (int)i,
                                          EDGE_UNCLASSIFIED));

        if (state->colors[edge->to] == NODE_WHITE) {
            EdgeType edge_type = EDGE_TREE;
            push_event(trace,
                       make_edge_event(DFS_EVENT_CLASSIFY_EDGE, (int)i, edge_type));
            visit_node(graph, edge->to, state, trace);
        } else {
            EdgeType edge_type = classify_seen_edge(state, edge->from, edge->to);
            push_event(trace,
                       make_edge_event(DFS_EVENT_CLASSIFY_EDGE, (int)i, edge_type));
        }
    }

    state->colors[node] = NODE_BLACK;
    state->time++;
    push_event(trace, make_node_event(DFS_EVENT_FINISH_NODE, node, state->time));
}

void dfs_trace_build(const Graph *graph, DfsTrace *trace) {
    trace->count = 0;

    DfsState state;
    memset(&state, 0, sizeof(state));
    state.time = 0;

    for (size_t i = 0; i < graph->node_count; i++) {
        state.colors[i] = NODE_WHITE;
        state.discover_times[i] = -1;
    }

    for (size_t i = 0; i < graph->node_count; i++) {
        if (state.colors[i] == NODE_WHITE) visit_node(graph, (int)i, &state, trace);
    }
}

void dfs_trace_apply_prefix(const Graph *base, const DfsTrace *trace,
                            size_t event_count, Graph *out) {
    *out = *base;
    graph_reset_visual_state(out);

    if (event_count > trace->count) event_count = trace->count;

    for (size_t i = 0; i < event_count; i++) {
        const DfsEvent *event = &trace->events[i];

        switch (event->type) {
        case DFS_EVENT_DISCOVER_NODE:
            out->nodes[event->node].color = NODE_GRAY;
            out->nodes[event->node].discover_time = event->time;
            break;
        case DFS_EVENT_EXAMINE_EDGE:
            break;
        case DFS_EVENT_CLASSIFY_EDGE:
            out->edges[event->edge].type = event->edge_type;
            break;
        case DFS_EVENT_FINISH_NODE:
            out->nodes[event->node].color = NODE_BLACK;
            out->nodes[event->node].finish_time = event->time;
            break;
        default:
            break;
        }
    }
}
