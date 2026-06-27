#include "graph.h"

#include <stdio.h>
#include <string.h>

void graph_init(Graph *graph) {
    graph->node_count = 0;
    graph->edge_count = 0;
    graph->first_alpha_node = -1;

    for (size_t i = 0; i < GRAPHE_MAX_NODES; i++) {
        graph->next_alpha_node[i] = -1;
        graph->first_out[i] = -1;
        graph->last_out[i] = -1;
        graph->first_alpha_out[i] = -1;
    }
}

static int node_follows_label_order(const Graph *graph, int left_node,
                                    int right_node) {
    return strcmp(graph->nodes[left_node].label, graph->nodes[right_node].label) <=
           0;
}

static void graph_link_node_by_label(Graph *graph, int node_index) {
    int current = graph->first_alpha_node;

    if (current == -1 || node_follows_label_order(graph, node_index, current)) {
        graph->next_alpha_node[node_index] = current;
        graph->first_alpha_node = node_index;
        return;
    }

    while (graph->next_alpha_node[current] != -1) {
        int next = graph->next_alpha_node[current];
        if (node_follows_label_order(graph, node_index, next)) break;
        current = next;
    }

    graph->next_alpha_node[node_index] = graph->next_alpha_node[current];
    graph->next_alpha_node[current] = node_index;
}

int graph_add_node(Graph *graph, const char *label) {
    if (graph->node_count >= GRAPHE_MAX_NODES) return -1;

    int node_index = (int)graph->node_count;
    Node *node = &graph->nodes[node_index];
    snprintf(node->label, sizeof(node->label), "%s", label);
    node->x = 0.0f;
    node->y = 0.0f;
    node->discover_time = -1;
    node->finish_time = -1;
    node->color = NODE_WHITE;
    graph->next_alpha_node[node_index] = -1;
    graph->first_out[node_index] = -1;
    graph->last_out[node_index] = -1;
    graph->first_alpha_out[node_index] = -1;

    graph_link_node_by_label(graph, node_index);

    graph->node_count++;
    return node_index;
}

static int edge_target_follows_label_order(const Graph *graph, int left_edge,
                                           int right_edge) {
    const Node *left = &graph->nodes[graph->edges[left_edge].to];
    const Node *right = &graph->nodes[graph->edges[right_edge].to];
    return strcmp(left->label, right->label) <= 0;
}

static void graph_link_edge_by_insertion_order(Graph *graph, int from,
                                               int edge_index) {
    if (graph->first_out[from] == -1) {
        graph->first_out[from] = edge_index;
    } else {
        graph->edges[graph->last_out[from]].next_out = edge_index;
    }
    graph->last_out[from] = edge_index;
}

static void graph_link_edge_by_target_label(Graph *graph, int from, int edge_index) {
    int current = graph->first_alpha_out[from];

    if (current == -1 ||
        edge_target_follows_label_order(graph, edge_index, current)) {
        graph->edges[edge_index].next_alpha_out = current;
        graph->first_alpha_out[from] = edge_index;
        return;
    }

    while (graph->edges[current].next_alpha_out != -1) {
        int next = graph->edges[current].next_alpha_out;
        if (edge_target_follows_label_order(graph, edge_index, next)) break;
        current = next;
    }

    graph->edges[edge_index].next_alpha_out = graph->edges[current].next_alpha_out;
    graph->edges[current].next_alpha_out = edge_index;
}

int graph_add_edge(Graph *graph, int from, int to) {
    if (graph->edge_count >= GRAPHE_MAX_EDGES) return -1;

    if (from < 0 || to < 0) return -1;

    if ((size_t)from >= graph->node_count || (size_t)to >= graph->node_count)
        return -1;

    int edge_index = (int)graph->edge_count;
    Edge *edge = &graph->edges[edge_index];
    edge->from = from;
    edge->to = to;
    edge->next_out = -1;
    edge->next_alpha_out = -1;
    edge->type = EDGE_UNCLASSIFIED;

    graph_link_edge_by_insertion_order(graph, from, edge_index);
    graph_link_edge_by_target_label(graph, from, edge_index);

    graph->edge_count++;
    return edge_index;
}

void graph_reset_visual_state(Graph *graph) {
    for (size_t i = 0; i < graph->node_count; i++) {
        graph->nodes[i].discover_time = -1;
        graph->nodes[i].finish_time = -1;
        graph->nodes[i].color = NODE_WHITE;
    }

    for (size_t i = 0; i < graph->edge_count; i++)
        graph->edges[i].type = EDGE_UNCLASSIFIED;
}

void graph_build_sample(Graph *graph) {
    graph_init(graph);

    int a = graph_add_node(graph, "A");
    int b = graph_add_node(graph, "B");
    int c = graph_add_node(graph, "C");
    int d = graph_add_node(graph, "D");
    int e = graph_add_node(graph, "E");
    int f = graph_add_node(graph, "F");

    graph_add_edge(graph, a, b);
    graph_add_edge(graph, a, c);
    graph_add_edge(graph, b, d);
    graph_add_edge(graph, b, e);
    graph_add_edge(graph, c, b);
    graph_add_edge(graph, c, f);
    graph_add_edge(graph, d, c);
    graph_add_edge(graph, e, d);
    graph_add_edge(graph, f, c);
}

const char *edge_type_name(EdgeType type) {
    switch (type) {
    case EDGE_UNCLASSIFIED:
        return "unclassified";
    case EDGE_TREE:
        return "tree";
    case EDGE_BACK:
        return "back";
    case EDGE_FORWARD:
        return "forward";
    case EDGE_CROSS:
        return "cross";
    default:
        return "unknown";
    }
}

const char *node_color_name(NodeColor color) {
    switch (color) {
    case NODE_WHITE:
        return "white";
    case NODE_GRAY:
        return "gray";
    case NODE_BLACK:
        return "black";
    default:
        return "unknown";
    }
}

const char *algorithm_mode_name(AlgorithmMode mode) {
    switch (mode) {
    case ALGORITHM_DFS:
        return "DFS";
    case ALGORITHM_BFS:
        return "BFS";
    case ALGORITHM_TREE:
        return "Tree traversal";
    default:
        return "Unknown";
    }
}
