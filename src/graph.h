#ifndef GRAPHE_GRAPH_H
#define GRAPHE_GRAPH_H

#include <stddef.h>

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

typedef enum DfsEventType {
    DFS_EVENT_DISCOVER_NODE,
    DFS_EVENT_EXAMINE_EDGE,
    DFS_EVENT_CLASSIFY_EDGE,
    DFS_EVENT_FINISH_NODE
} DfsEventType;

typedef struct Node {
    char label[16];
    float x;
    float y;
    int discover_time;
    int finish_time;
    NodeColor color;
} Node;

typedef struct Edge {
    int from;
    int to;
    EdgeType type;
} Edge;

typedef struct Graph {
    Node nodes[GRAPHE_MAX_NODES];
    size_t node_count;
    Edge edges[GRAPHE_MAX_EDGES];
    size_t edge_count;
} Graph;

typedef struct DfsEvent {
    DfsEventType type;
    int node;
    int edge;
    EdgeType edge_type;
    int time;
} DfsEvent;

typedef struct DfsTrace {
    DfsEvent events[GRAPHE_MAX_EVENTS];
    size_t count;
} DfsTrace;

void graph_init(Graph *graph);
int graph_add_node(Graph *graph, const char *label);
int graph_add_edge(Graph *graph, int from, int to);
void graph_reset_visual_state(Graph *graph);
void graph_build_sample(Graph *graph);

const char *edge_type_name(EdgeType type);
const char *node_color_name(NodeColor color);

#endif
