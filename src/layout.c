#include "layout.h"

#include <math.h>

void layout_circle(Graph *graph, float center_x, float center_y, float radius) {
    const float two_pi = 6.28318530718f;

    if (graph->node_count == 0) return;

    for (size_t i = 0; i < graph->node_count; i++) {
        float angle = -1.57079632679f + two_pi * (float)i / (float)graph->node_count;
        graph->nodes[i].x = center_x + cosf(angle) * radius;
        graph->nodes[i].y = center_y + sinf(angle) * radius;
    }
}
