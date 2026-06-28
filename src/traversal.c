#include "traversal.h"

#include <string.h>

typedef struct TraversalState {
    NodeColor colors[GRAPHE_MAX_NODES];
    int discover_times[GRAPHE_MAX_NODES];
    int levels[GRAPHE_MAX_NODES];
    int parent_edges[GRAPHE_MAX_NODES];
    int edge_classified[GRAPHE_MAX_EDGES];
    int time;
} TraversalState;

static const TraversalOptions default_options = {
    .algorithm = ALGORITHM_DFS,
    .alphabetical = 1,
    .tree_order = TREE_ORDER_INORDER,
};

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
    event.from = -1;
    event.to = -1;
    event.edge_type = EDGE_UNCLASSIFIED;
    event.time = time;

    return event;
}

static TraceEvent make_edge_event(TraceEventType type, int edge, int from, int to,
                                  EdgeType edge_type) {
    TraceEvent event;

    event.type = type;
    event.node = -1;
    event.edge = edge;
    event.from = from;
    event.to = to;
    event.edge_type = edge_type;
    event.time = -1;

    return event;
}

static TraceEvent make_edge_event_with_time(TraceEventType type, int edge, int from,
                                            int to, EdgeType edge_type, int time) {
    TraceEvent event = make_edge_event(type, edge, from, to, edge_type);
    event.time = time;
    return event;
}

static void traversal_state_init(const Graph *graph, TraversalState *state) {
    memset(state, 0, sizeof(*state));
    state->time = 0;

    for (size_t i = 0; i < graph->node_count; i++) {
        state->colors[i] = NODE_WHITE;
        state->discover_times[i] = -1;
        state->levels[i] = -1;
        state->parent_edges[i] = -1;
    }

    for (size_t i = 0; i < graph->edge_count; i++) state->edge_classified[i] = 0;
}

static int first_out_edge(const Graph *graph, int node,
                          const TraversalOptions *options) {
    if (options->alphabetical) return graph->first_alpha_out[node];
    return graph->first_out[node];
}

static int next_out_edge(const Graph *graph, int edge_id, int node,
                         const TraversalOptions *options) {
    return graph_next_adjacent_edge(graph, edge_id, node, options->alphabetical);
}

static int next_out_edge_in_insertion_order(const Graph *graph, int edge_id,
                                            int node) {
    return graph_next_adjacent_edge(graph, edge_id, node, false);
}

static int first_out_edge_in_insertion_order(const Graph *graph, int node) {
    return graph->first_out[node];
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

static void discover_node_at_level(TraversalState *state, Trace *trace, int node,
                                   int level) {
    state->colors[node] = NODE_GRAY;
    state->levels[node] = level;
    push_event(trace, make_node_event(TRACE_EVENT_DISCOVER_NODE, node, level));
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

static bool is_reverse_parent_edge(const Graph *graph, int edge_id,
                                   int parent_edge) {
    if (graph->directed || parent_edge < 0) return false;

    return parent_edge == edge_id;
}

static void dfs_visit_node(const Graph *graph, int node, int parent_edge,
                           const TraversalOptions *options, TraversalState *state,
                           Trace *trace) {
    discover_node(state, trace, node);

    for (int edge_id = first_out_edge(graph, node, options); edge_id != -1;
         edge_id = next_out_edge(graph, edge_id, node, options)) {
        int neighbor = graph_edge_neighbor(graph, edge_id, node);

        if (neighbor < 0) continue;
        if (is_reverse_parent_edge(graph, edge_id, parent_edge)) continue;
        if (!graph->directed && state->edge_classified[edge_id]) continue;
        if (!graph->directed && state->colors[neighbor] == NODE_BLACK) continue;

        push_event(trace, make_edge_event(TRACE_EVENT_EXAMINE_EDGE, edge_id, node,
                                          neighbor, EDGE_UNCLASSIFIED));

        if (state->colors[neighbor] == NODE_WHITE) {
            state->parent_edges[neighbor] = edge_id;
            state->edge_classified[edge_id] = 1;
            push_event(trace, make_edge_event(TRACE_EVENT_CLASSIFY_EDGE, edge_id,
                                              node, neighbor, EDGE_TREE));
            dfs_visit_node(graph, neighbor, edge_id, options, state, trace);
        } else {
            EdgeType edge_type;

            if (!graph->directed && state->colors[neighbor] != NODE_GRAY) continue;

            if (graph->directed) {
                edge_type = classify_dfs_seen_edge(state, node, neighbor);
            } else {
                edge_type = EDGE_BACK;
                state->edge_classified[edge_id] = 1;
            }

            push_event(trace, make_edge_event(TRACE_EVENT_CLASSIFY_EDGE, edge_id,
                                              node, neighbor, edge_type));
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
            dfs_visit_node(graph, node, -1, options, &state, trace);
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
        discover_node_at_level(&state, trace, root, 0);
        queue[tail] = root;
        tail++;

        while (head < tail) {
            int node = queue[head];
            head++;

            for (int edge_id = first_out_edge(graph, node, options); edge_id != -1;
                 edge_id = next_out_edge(graph, edge_id, node, options)) {
                int neighbor = graph_edge_neighbor(graph, edge_id, node);

                if (neighbor < 0) continue;
                if (is_reverse_parent_edge(graph, edge_id, state.parent_edges[node]))
                    continue;
                if (!graph->directed && state.edge_classified[edge_id]) continue;

                push_event(trace,
                           make_edge_event(TRACE_EVENT_EXAMINE_EDGE, edge_id, node,
                                           neighbor, EDGE_UNCLASSIFIED));

                if (state.colors[neighbor] == NODE_WHITE) {
                    int neighbor_level = state.levels[node] + 1;
                    state.parent_edges[neighbor] = edge_id;
                    state.edge_classified[edge_id] = 1;
                    push_event(trace, make_edge_event_with_time(
                                          TRACE_EVENT_CLASSIFY_EDGE, edge_id, node,
                                          neighbor, EDGE_TREE, neighbor_level));
                    discover_node_at_level(&state, trace, neighbor, neighbor_level);
                    queue[tail] = neighbor;
                    tail++;
                } else {
                    if (!graph->directed) continue;
                    push_event(trace,
                               make_edge_event_with_time(
                                   TRACE_EVENT_CLASSIFY_EDGE, edge_id, node,
                                   neighbor, EDGE_CROSS, state.levels[neighbor]));
                }
            }

            finish_node(&state, trace, node);
        }
    }
}

static int collect_tree_child_edges(const Graph *graph, int node, int *child_edges,
                                    int capacity) {
    int count = 0;

    for (int edge_id = first_out_edge_in_insertion_order(graph, node); edge_id != -1;
         edge_id = next_out_edge_in_insertion_order(graph, edge_id, node)) {
        int neighbor = graph_edge_neighbor(graph, edge_id, node);
        if (neighbor < 0) continue;
        if (count >= capacity) break;

        child_edges[count] = edge_id;
        count++;
    }

    return count;
}

static void tree_visit_node(const Graph *graph, int node,
                            const TraversalOptions *options, TraversalState *state,
                            Trace *trace) {
    int child_edges[GRAPHE_MAX_NODES];
    int child_count =
        collect_tree_child_edges(graph, node, child_edges, GRAPHE_MAX_NODES);
    int visited_node = 0;

    if (options->tree_order == TREE_ORDER_PREORDER) {
        discover_node(state, trace, node);
        visited_node = 1;
    }

    for (int i = 0; i < child_count; i++) {
        if (options->tree_order == TREE_ORDER_INORDER && i == 1) {
            discover_node(state, trace, node);
            visited_node = 1;
        }

        int edge_id = child_edges[i];
        int neighbor = graph_edge_neighbor(graph, edge_id, node);
        if (neighbor < 0 || state->colors[neighbor] != NODE_WHITE) continue;

        push_event(trace, make_edge_event(TRACE_EVENT_EXAMINE_EDGE, edge_id, node,
                                          neighbor, EDGE_UNCLASSIFIED));
        push_event(trace, make_edge_event(TRACE_EVENT_CLASSIFY_EDGE, edge_id, node,
                                          neighbor, EDGE_TREE));
        tree_visit_node(graph, neighbor, options, state, trace);
    }

    if (options->tree_order == TREE_ORDER_INORDER && !visited_node) {
        discover_node(state, trace, node);
        visited_node = 1;
    }
    if (options->tree_order == TREE_ORDER_POSTORDER)
        discover_node(state, trace, node);
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
            out->nodes[event->node].level = event->time;
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
