#include "dfs.h"

#include <string.h>

typedef struct TraversalState {
    NodeColor colors[GRAPHE_MAX_NODES];
    int discover_times[GRAPHE_MAX_NODES];
    int time;
} TraversalState;

static const TraversalOptions default_options = {
    .algorithm = ALGORITHM_DFS,
    .alphabetical = 1,
};

static const DfsOptions default_dfs_options = {.alphabetical = 1};

static void push_event(Trace *trace, TraceEvent event) {
    if (trace->count >= GRAPHE_MAX_EVENTS) return;

    trace->events[trace->count] = event;
    trace->count++;
}

static TraceEvent make_node_event(TraceEventType type, int node, int time) {
    TraceEvent event;

    event.type = type;
    event.node = node;
    event.edge = -1;
    event.edge_type = EDGE_UNCLASSIFIED;
    event.time = time;

    return event;
}

static TraceEvent make_edge_event(TraceEventType type, int edge,
                                  EdgeType edge_type) {
    TraceEvent event;

    event.type = type;
    event.node = -1;
    event.edge = edge;
    event.edge_type = edge_type;
    event.time = -1;

    return event;
}

static void traversal_state_init(const Graph *graph, TraversalState *state) {
    memset(state, 0, sizeof(*state));
    state->time = 0;

    for (size_t i = 0; i < graph->node_count; i++) {
        state->colors[i] = NODE_WHITE;
        state->discover_times[i] = -1;
    }
}

static int first_out_edge(const Graph *graph, int node,
                          const TraversalOptions *options) {
    if (options->alphabetical) return graph->first_alpha_out[node];
    return graph->first_out[node];
}

static int next_out_edge(const Graph *graph, int edge_id,
                         const TraversalOptions *options) {
    if (options->alphabetical) return graph->edges[edge_id].next_alpha_out;
    return graph->edges[edge_id].next_out;
}

static int first_root_node(const Graph *graph, const TraversalOptions *options) {
    if (graph->node_count == 0) return -1;
    if (options->alphabetical) return graph->first_alpha_node;
    return 0;
}

static int next_root_node(const Graph *graph, int node,
                          const TraversalOptions *options) {
    if (options->alphabetical) return graph->next_alpha_node[node];

    node++;
    if ((size_t)node >= graph->node_count) return -1;
    return node;
}

static void discover_node(TraversalState *state, Trace *trace, int node) {
    state->colors[node] = NODE_GRAY;
    state->time++;
    state->discover_times[node] = state->time;
    push_event(trace, make_node_event(TRACE_EVENT_DISCOVER_NODE, node, state->time));
}

static void finish_node(TraversalState *state, Trace *trace, int node) {
    state->colors[node] = NODE_BLACK;
    state->time++;
    push_event(trace, make_node_event(TRACE_EVENT_FINISH_NODE, node, state->time));
}

static EdgeType classify_dfs_seen_edge(const TraversalState *state, int from,
                                       int to) {
    if (state->colors[to] == NODE_GRAY) return EDGE_BACK;

    if (state->discover_times[from] < state->discover_times[to]) return EDGE_FORWARD;

    return EDGE_CROSS;
}

static void dfs_visit_node(const Graph *graph, int node,
                           const TraversalOptions *options, TraversalState *state,
                           Trace *trace) {
    discover_node(state, trace, node);

    for (int edge_id = first_out_edge(graph, node, options); edge_id != -1;
         edge_id = next_out_edge(graph, edge_id, options)) {
        const Edge *edge = &graph->edges[edge_id];

        push_event(trace, make_edge_event(TRACE_EVENT_EXAMINE_EDGE, edge_id,
                                          EDGE_UNCLASSIFIED));

        if (state->colors[edge->to] == NODE_WHITE) {
            push_event(trace, make_edge_event(TRACE_EVENT_CLASSIFY_EDGE, edge_id,
                                              EDGE_TREE));
            dfs_visit_node(graph, edge->to, options, state, trace);
        } else {
            EdgeType edge_type = classify_dfs_seen_edge(state, edge->from, edge->to);
            push_event(trace, make_edge_event(TRACE_EVENT_CLASSIFY_EDGE, edge_id,
                                              edge_type));
        }
    }

    finish_node(state, trace, node);
}

static void build_dfs_trace(const Graph *graph, const TraversalOptions *options,
                            Trace *trace) {
    TraversalState state;
    traversal_state_init(graph, &state);

    for (int node = first_root_node(graph, options); node != -1;
         node = next_root_node(graph, node, options)) {
        if (state.colors[node] == NODE_WHITE)
            dfs_visit_node(graph, node, options, &state, trace);
    }
}

static void build_bfs_trace(const Graph *graph, const TraversalOptions *options,
                            Trace *trace) {
    TraversalState state;
    int queue[GRAPHE_MAX_NODES];

    traversal_state_init(graph, &state);

    for (int root = first_root_node(graph, options); root != -1;
         root = next_root_node(graph, root, options)) {
        if (state.colors[root] != NODE_WHITE) continue;

        int head = 0;
        int tail = 0;
        discover_node(&state, trace, root);
        queue[tail] = root;
        tail++;

        while (head < tail) {
            int node = queue[head];
            head++;

            for (int edge_id = first_out_edge(graph, node, options); edge_id != -1;
                 edge_id = next_out_edge(graph, edge_id, options)) {
                const Edge *edge = &graph->edges[edge_id];

                push_event(trace, make_edge_event(TRACE_EVENT_EXAMINE_EDGE, edge_id,
                                                  EDGE_UNCLASSIFIED));

                if (state.colors[edge->to] == NODE_WHITE) {
                    push_event(trace, make_edge_event(TRACE_EVENT_CLASSIFY_EDGE,
                                                      edge_id, EDGE_TREE));
                    discover_node(&state, trace, edge->to);
                    queue[tail] = edge->to;
                    tail++;
                } else {
                    push_event(trace, make_edge_event(TRACE_EVENT_CLASSIFY_EDGE,
                                                      edge_id, EDGE_CROSS));
                }
            }

            finish_node(&state, trace, node);
        }
    }
}

static void tree_visit_node(const Graph *graph, int node,
                            const TraversalOptions *options, TraversalState *state,
                            Trace *trace) {
    discover_node(state, trace, node);

    for (int edge_id = first_out_edge(graph, node, options); edge_id != -1;
         edge_id = next_out_edge(graph, edge_id, options)) {
        const Edge *edge = &graph->edges[edge_id];

        if (state->colors[edge->to] != NODE_WHITE) continue;

        push_event(trace, make_edge_event(TRACE_EVENT_EXAMINE_EDGE, edge_id,
                                          EDGE_UNCLASSIFIED));
        push_event(trace,
                   make_edge_event(TRACE_EVENT_CLASSIFY_EDGE, edge_id, EDGE_TREE));
        tree_visit_node(graph, edge->to, options, state, trace);
    }

    finish_node(state, trace, node);
}

static void build_tree_trace(const Graph *graph, const TraversalOptions *options,
                             Trace *trace) {
    TraversalState state;
    traversal_state_init(graph, &state);

    for (int node = first_root_node(graph, options); node != -1;
         node = next_root_node(graph, node, options)) {
        if (state.colors[node] == NODE_WHITE)
            tree_visit_node(graph, node, options, &state, trace);
    }
}

void traversal_trace_build(const Graph *graph, const TraversalOptions *options,
                           Trace *trace) {
    if (options == NULL) options = &default_options;

    trace->count = 0;

    switch (options->algorithm) {
    case ALGORITHM_BFS:
        build_bfs_trace(graph, options, trace);
        break;
    case ALGORITHM_TREE:
        build_tree_trace(graph, options, trace);
        break;
    case ALGORITHM_DFS:
    default:
        build_dfs_trace(graph, options, trace);
        break;
    }
}

void traversal_trace_apply_prefix(const Graph *base, const Trace *trace,
                                  size_t event_count, Graph *out) {
    *out = *base;
    graph_reset_visual_state(out);

    if (event_count > trace->count) event_count = trace->count;

    for (size_t i = 0; i < event_count; i++) {
        const TraceEvent *event = &trace->events[i];

        switch (event->type) {
        case TRACE_EVENT_DISCOVER_NODE:
            out->nodes[event->node].color = NODE_GRAY;
            out->nodes[event->node].discover_time = event->time;
            break;
        case TRACE_EVENT_EXAMINE_EDGE:
            break;
        case TRACE_EVENT_CLASSIFY_EDGE:
            out->edges[event->edge].type = event->edge_type;
            break;
        case TRACE_EVENT_FINISH_NODE:
            out->nodes[event->node].color = NODE_BLACK;
            out->nodes[event->node].finish_time = event->time;
            break;
        default:
            break;
        }
    }
}

void dfs_trace_build(const Graph *graph, Trace *trace) {
    dfs_trace_build_with_options(graph, &default_dfs_options, trace);
}

void dfs_trace_build_with_options(const Graph *graph, const DfsOptions *options,
                                  Trace *trace) {
    if (options == NULL) options = &default_dfs_options;

    TraversalOptions traversal_options = {
        .algorithm = ALGORITHM_DFS,
        .alphabetical = options->alphabetical,
    };

    traversal_trace_build(graph, &traversal_options, trace);
}

void dfs_trace_apply_prefix(const Graph *base, const Trace *trace,
                            size_t event_count, Graph *out) {
    traversal_trace_apply_prefix(base, trace, event_count, out);
}
