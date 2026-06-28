#ifndef GRAPHE_GRAPH_H
#define GRAPHE_GRAPH_H

#include <stddef.h>
#include <stdbool.h>

#define GRAPHE_MAX_NODES 16
#define GRAPHE_MAX_EDGES 64
#define GRAPHE_MAX_EVENTS 256

typedef enum NodeColor { NODE_WHITE, NODE_GRAY, NODE_BLACK } NodeColor;

typedef enum EdgeType {
    EDGE_UNCLASSIFIED,
    EDGE_TREE,
    EDGE_BACK,
    EDGE_FORWARD,
    EDGE_CROSS
} EdgeType;

typedef enum AlgorithmMode {
    ALGORITHM_DFS,
    ALGORITHM_BFS,
    ALGORITHM_TREE
} AlgorithmMode;

typedef enum TreeTraversalOrder {
    TREE_ORDER_PREORDER,
    TREE_ORDER_INORDER,
    TREE_ORDER_POSTORDER
} TreeTraversalOrder;

typedef enum TraceEventType {
    TRACE_EVENT_DISCOVER_NODE,
    TRACE_EVENT_EXAMINE_EDGE,
    TRACE_EVENT_CLASSIFY_EDGE,
    TRACE_EVENT_FINISH_NODE
} TraceEventType;

typedef struct Node {
    char label[16];
    float x;
    float y;
    int discover_time;
    int finish_time;
    int level;
    NodeColor color;
} Node;

typedef struct Edge {
    int from;
    int to;
    int next_from;
    int next_to;
    int next_alpha_from;
    int next_alpha_to;
    EdgeType type;
} Edge;

typedef struct Graph {
    bool directed;
    Node nodes[GRAPHE_MAX_NODES];
    int next_alpha_node[GRAPHE_MAX_NODES];
    int first_alpha_node;
    int first_out[GRAPHE_MAX_NODES];
    int last_out[GRAPHE_MAX_NODES];
    int first_alpha_out[GRAPHE_MAX_NODES];
    size_t node_count;
    Edge edges[GRAPHE_MAX_EDGES];
    size_t edge_count;
} Graph;

typedef struct TraceEvent {
    TraceEventType type;
    int node;
    int edge;
    int from;
    int to;
    EdgeType edge_type;
    int time;
} TraceEvent;

typedef struct Trace {
    TraceEvent events[GRAPHE_MAX_EVENTS];
    size_t count;
} Trace;

void graph_init(Graph *graph);
int graph_add_node(Graph *graph, const char *label);
int graph_add_edge(Graph *graph, int from, int to);
int graph_find_node(const Graph *graph, const char *label);
bool graph_set_directed(Graph *graph, bool directed);
bool graph_edge_is_visible(const Graph *graph, int edge_index);
int graph_edge_neighbor(const Graph *graph, int edge_index, int node);
int graph_next_adjacent_edge(const Graph *graph, int edge_index, int node,
                             bool alphabetical);
void graph_reset_visual_state(Graph *graph);
void graph_build_sample(Graph *graph);
void graph_build_expression_tree(Graph *graph);

const char *edge_type_name(EdgeType type);
const char *node_color_name(NodeColor color);
const char *algorithm_mode_name(AlgorithmMode mode);
const char *tree_traversal_order_name(TreeTraversalOrder order);

#endif
