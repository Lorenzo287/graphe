#include "graph.h"

#include <stdio.h>
#include <string.h>

void graph_init(Graph *graph) {
    graph->node_count = 0;
    graph->edge_count = 0;
}

int graph_add_node(Graph *graph, const char *label) {
    if (graph->node_count >= GRAPHE_MAX_NODES) return -1;

    Node *node = &graph->nodes[graph->node_count];
    snprintf(node->label, sizeof(node->label), "%s", label);
    node->x = 0.0f;
    node->y = 0.0f;
    node->discover_time = -1;
    node->finish_time = -1;
    node->color = NODE_WHITE;

    graph->node_count++;
    return (int)graph->node_count - 1;
}

int graph_add_edge(Graph *graph, int from, int to) {
    if (graph->edge_count >= GRAPHE_MAX_EDGES) return -1;

    if (from < 0 || to < 0) return -1;

    if ((size_t)from >= graph->node_count || (size_t)to >= graph->node_count)
        return -1;

    Edge *edge = &graph->edges[graph->edge_count];
    edge->from = from;
    edge->to = to;
    edge->type = EDGE_UNCLASSIFIED;

    graph->edge_count++;
    return (int)graph->edge_count - 1;
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
