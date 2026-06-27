#ifndef GRAPHE_LAYOUT_H
#define GRAPHE_LAYOUT_H

#include "graph.h"

void layout_circle(Graph *graph, float center_x, float center_y, float radius);
void layout_trace_forest(Graph *graph, const Trace *trace, float left, float top,
                         float width, float level_gap);

#endif
