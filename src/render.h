#ifndef GRAPHE_RENDER_H
#define GRAPHE_RENDER_H

#include "graph.h"

#define GRAPHE_NODE_RADIUS 28.0f

void render_graph(const Graph *graph, const DfsTrace *trace,
                  size_t applied_event_count);

#endif
