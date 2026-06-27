#include "graph.h"

#include <stdio.h>
#include <string.h>

void graph_init(Graph *graph) {
    graph->directed = true;
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

int graph_find_node(const Graph *graph, const char *label) {
    for (size_t i = 0; i < graph->node_count; i++) {
        if (strcmp(graph->nodes[i].label, label) == 0) return (int)i;
    }

    return -1;
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

int graph_edge_neighbor(const Graph *graph, int edge_index, int node) {
    if (edge_index < 0 || (size_t)edge_index >= graph->edge_count) return -1;

    const Edge *edge = &graph->edges[edge_index];

    if (edge->from == node) return edge->to;
    if (!graph->directed && edge->to == node) return edge->from;

    return -1;
}

static int edge_next_for_node(const Graph *graph, int edge_index, int node,
                              bool alphabetical) {
    const Edge *edge = &graph->edges[edge_index];

    if (edge->from == node)
        return alphabetical ? edge->next_alpha_from : edge->next_from;

    if (!graph->directed && edge->to == node)
        return alphabetical ? edge->next_alpha_to : edge->next_to;

    return -1;
}

int graph_next_adjacent_edge(const Graph *graph, int edge_index, int node,
                             bool alphabetical) {
    if (edge_index < 0 || (size_t)edge_index >= graph->edge_count) return -1;
    return edge_next_for_node(graph, edge_index, node, alphabetical);
}

static void set_edge_next_for_node(Graph *graph, int edge_index, int node,
                                   int next_edge, bool alphabetical) {
    Edge *edge = &graph->edges[edge_index];

    if (edge->from == node) {
        if (alphabetical) {
            edge->next_alpha_from = next_edge;
        } else {
            edge->next_from = next_edge;
        }
        return;
    }

    if (!graph->directed && edge->to == node) {
        if (alphabetical) {
            edge->next_alpha_to = next_edge;
        } else {
            edge->next_to = next_edge;
        }
    }
}

static int edge_neighbor_follows_label_order(const Graph *graph, int node,
                                             int left_edge, int right_edge) {
    int left_neighbor = graph_edge_neighbor(graph, left_edge, node);
    int right_neighbor = graph_edge_neighbor(graph, right_edge, node);

    if (left_neighbor < 0 || right_neighbor < 0) return 0;

    const Node *left = &graph->nodes[left_neighbor];
    const Node *right = &graph->nodes[right_neighbor];
    return strcmp(left->label, right->label) <= 0;
}

static void graph_link_edge_by_insertion_order(Graph *graph, int node,
                                               int edge_index) {
    if (graph->first_out[node] == -1) {
        graph->first_out[node] = edge_index;
    } else {
        set_edge_next_for_node(graph, graph->last_out[node], node, edge_index,
                               false);
    }
    graph->last_out[node] = edge_index;
}

static void graph_link_edge_by_target_label(Graph *graph, int node, int edge_index) {
    int current = graph->first_alpha_out[node];

    if (current == -1 ||
        edge_neighbor_follows_label_order(graph, node, edge_index, current)) {
        set_edge_next_for_node(graph, edge_index, node, current, true);
        graph->first_alpha_out[node] = edge_index;
        return;
    }

    while (edge_next_for_node(graph, current, node, true) != -1) {
        int next = edge_next_for_node(graph, current, node, true);
        if (edge_neighbor_follows_label_order(graph, node, edge_index, next)) break;
        current = next;
    }

    set_edge_next_for_node(graph, edge_index, node,
                           edge_next_for_node(graph, current, node, true), true);
    set_edge_next_for_node(graph, current, node, edge_index, true);
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
    edge->next_from = -1;
    edge->next_to = -1;
    edge->next_alpha_from = -1;
    edge->next_alpha_to = -1;
    edge->type = EDGE_UNCLASSIFIED;

    graph_link_edge_by_insertion_order(graph, from, edge_index);
    graph_link_edge_by_target_label(graph, from, edge_index);
    if (!graph->directed && to != from) {
        graph_link_edge_by_insertion_order(graph, to, edge_index);
        graph_link_edge_by_target_label(graph, to, edge_index);
    }

    graph->edge_count++;
    return edge_index;
}

bool graph_edge_is_visible(const Graph *graph, int edge_index) {
    return edge_index >= 0 && (size_t)edge_index < graph->edge_count;
}

bool graph_set_directed(Graph *graph, bool directed) {
    if (graph->directed == directed) return true;

    typedef struct SavedNode {
        char label[sizeof(graph->nodes[0].label)];
        float x;
        float y;
    } SavedNode;

    typedef struct SavedEdge {
        int from;
        int to;
    } SavedEdge;

    SavedNode nodes[GRAPHE_MAX_NODES];
    SavedEdge edges[GRAPHE_MAX_EDGES];
    size_t node_count = graph->node_count;
    size_t edge_count = 0;

    for (size_t i = 0; i < graph->node_count; i++) {
        snprintf(nodes[i].label, sizeof(nodes[i].label), "%s",
                 graph->nodes[i].label);
        nodes[i].x = graph->nodes[i].x;
        nodes[i].y = graph->nodes[i].y;
    }

    for (size_t i = 0; i < graph->edge_count; i++) {
        if (!graph_edge_is_visible(graph, (int)i)) continue;

        const Edge *edge = &graph->edges[i];
        bool duplicate_pair = false;
        if (!directed) {
            for (size_t saved = 0; saved < edge_count; saved++) {
                if ((edges[saved].from == edge->from &&
                     edges[saved].to == edge->to) ||
                    (edges[saved].from == edge->to &&
                     edges[saved].to == edge->from)) {
                    duplicate_pair = true;
                    break;
                }
            }
        }
        if (duplicate_pair) continue;

        if (edge_count >= GRAPHE_MAX_EDGES) return false;

        edges[edge_count].from = edge->from;
        edges[edge_count].to = edge->to;
        edge_count++;
    }

    graph_init(graph);
    graph->directed = directed;

    for (size_t i = 0; i < node_count; i++) {
        int node_index = graph_add_node(graph, nodes[i].label);
        if (node_index < 0) return false;
        graph->nodes[node_index].x = nodes[i].x;
        graph->nodes[node_index].y = nodes[i].y;
    }

    for (size_t i = 0; i < edge_count; i++) {
        if (graph_add_edge(graph, edges[i].from, edges[i].to) < 0) return false;
    }

    return true;
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

void graph_build_expression_tree(Graph *graph) {
    graph_init(graph);

    int multiply = graph_add_node(graph, "*");
    int plus = graph_add_node(graph, "+");
    int minus = graph_add_node(graph, "-");
    int a = graph_add_node(graph, "a");
    int b = graph_add_node(graph, "b");
    int c = graph_add_node(graph, "c");
    int d = graph_add_node(graph, "d");

    graph_add_edge(graph, multiply, plus);
    graph_add_edge(graph, multiply, minus);
    graph_add_edge(graph, plus, a);
    graph_add_edge(graph, plus, b);
    graph_add_edge(graph, minus, c);
    graph_add_edge(graph, minus, d);
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

const char *tree_traversal_order_name(TreeTraversalOrder order) {
    switch (order) {
    case TREE_ORDER_PREORDER:
        return "preorder";
    case TREE_ORDER_INORDER:
        return "inorder";
    case TREE_ORDER_POSTORDER:
        return "postorder";
    default:
        return "unknown";
    }
}
