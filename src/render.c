#include "render.h"

#include "raylib.h"

#include <math.h>
#include <stdio.h>

static Color node_fill_color(NodeColor color) {
    switch (color) {
    case NODE_WHITE:
        return RAYWHITE;
    case NODE_GRAY:
        return (Color){255, 202, 58, 255};
    case NODE_BLACK:
        return (Color){75, 85, 99, 255};
    default:
        return RAYWHITE;
    }
}

static Color edge_color(EdgeType type) {
    switch (type) {
    case EDGE_TREE:
        return (Color){34, 197, 94, 255};
    case EDGE_BACK:
        return (Color){239, 68, 68, 255};
    case EDGE_FORWARD:
        return (Color){59, 130, 246, 255};
    case EDGE_CROSS:
        return (Color){168, 85, 247, 255};
    case EDGE_UNCLASSIFIED:
    default:
        return (Color){156, 163, 175, 255};
    }
}

static Vector2 vector_add(Vector2 a, Vector2 b) {
    return (Vector2){a.x + b.x, a.y + b.y};
}

static Vector2 vector_subtract(Vector2 a, Vector2 b) {
    return (Vector2){a.x - b.x, a.y - b.y};
}

static Vector2 vector_scale(Vector2 value, float scale) {
    return (Vector2){value.x * scale, value.y * scale};
}

static Vector2 vector_normalize(Vector2 value) {
    float length = sqrtf(value.x * value.x + value.y * value.y);

    if (length <= 0.0001f) return (Vector2){1.0f, 0.0f};

    return (Vector2){value.x / length, value.y / length};
}

static void draw_arrowhead(Vector2 tip, Vector2 direction, Color color) {
    const float arrow_length = 14.0f;
    const float arrow_width = 9.0f;
    Vector2 normal = {-direction.y, direction.x};
    Vector2 base = vector_subtract(tip, vector_scale(direction, arrow_length));
    Vector2 left = vector_add(base, vector_scale(normal, arrow_width * 0.5f));
    Vector2 right = vector_subtract(base, vector_scale(normal, arrow_width * 0.5f));

    DrawTriangle(tip, left, right, color);
}

static int is_active_edge(const DfsTrace *trace, size_t active_index,
                          size_t edge_index) {
    if (active_index >= trace->count) return 0;

    return trace->events[active_index].edge == (int)edge_index;
}

static int is_active_node(const DfsTrace *trace, size_t active_index,
                          size_t node_index) {
    if (active_index >= trace->count) return 0;

    return trace->events[active_index].node == (int)node_index;
}

static void draw_edge(const Graph *graph, size_t edge_index, int active) {
    const Edge *edge = &graph->edges[edge_index];
    const Node *from = &graph->nodes[edge->from];
    const Node *to = &graph->nodes[edge->to];
    Vector2 start_center = {from->x, from->y};
    Vector2 end_center = {to->x, to->y};
    Vector2 direction = vector_normalize(vector_subtract(end_center, start_center));
    Vector2 start =
        vector_add(start_center, vector_scale(direction, GRAPHE_NODE_RADIUS));
    Vector2 end =
        vector_subtract(end_center, vector_scale(direction, GRAPHE_NODE_RADIUS));
    Color color = active ? ORANGE : edge_color(edge->type);
    float thickness = active ? 4.0f : 2.0f;

    DrawLineEx(start, end, thickness, color);
    draw_arrowhead(end, direction, color);
}

static void draw_node(const Node *node, int active) {
    Vector2 center = {node->x, node->y};
    Color fill = node_fill_color(node->color);
    Color outline = active ? ORANGE : (Color){31, 41, 55, 255};
    int label_width = MeasureText(node->label, 22);
    char times[32];

    DrawCircleV(center, GRAPHE_NODE_RADIUS, fill);
    DrawCircleLinesV(center, GRAPHE_NODE_RADIUS, outline);
    if (active) DrawCircleLinesV(center, GRAPHE_NODE_RADIUS + 4.0f, ORANGE);

    DrawText(node->label, (int)(node->x - (float)label_width * 0.5f),
             (int)(node->y - 13.0f), 22, BLACK);

    snprintf(times, sizeof(times), "d:%d f:%d", node->discover_time,
             node->finish_time);
    DrawText(times, (int)(node->x - 26.0f), (int)(node->y + 34.0f), 12, DARKGRAY);
}

static const char *event_type_name(DfsEventType type) {
    switch (type) {
    case DFS_EVENT_DISCOVER_NODE:
        return "discover node";
    case DFS_EVENT_EXAMINE_EDGE:
        return "examine edge";
    case DFS_EVENT_CLASSIFY_EDGE:
        return "classify edge";
    case DFS_EVENT_FINISH_NODE:
        return "finish node";
    default:
        return "unknown";
    }
}

static void describe_event(const Graph *graph, const DfsTrace *trace,
                           size_t active_index, char *buffer, size_t buffer_size) {
    if (active_index >= trace->count) {
        snprintf(buffer, buffer_size, "No event selected");
        return;
    }

    const DfsEvent *event = &trace->events[active_index];
    switch (event->type) {
    case DFS_EVENT_DISCOVER_NODE:
    case DFS_EVENT_FINISH_NODE:
        snprintf(buffer, buffer_size, "%s %s at t=%d", event_type_name(event->type),
                 graph->nodes[event->node].label, event->time);
        break;
    case DFS_EVENT_EXAMINE_EDGE: {
        const Edge *edge = &graph->edges[event->edge];
        snprintf(buffer, buffer_size, "examine %s -> %s",
                 graph->nodes[edge->from].label, graph->nodes[edge->to].label);
        break;
    }
    case DFS_EVENT_CLASSIFY_EDGE: {
        const Edge *edge = &graph->edges[event->edge];
        snprintf(buffer, buffer_size, "%s -> %s is %s",
                 graph->nodes[edge->from].label, graph->nodes[edge->to].label,
                 edge_type_name(event->edge_type));
        break;
    }
    default:
        snprintf(buffer, buffer_size, "Unknown event");
        break;
    }
}

static void draw_sidebar(const Graph *graph, const DfsTrace *trace,
                         size_t applied_event_count) {
    const int x = 820;
    const int y = 32;
    const int line = 24;
    size_t active_index =
        applied_event_count == 0 ? trace->count : applied_event_count - 1;
    char event_text[128];
    char progress[64];

    describe_event(graph, trace, active_index, event_text, sizeof(event_text));
    snprintf(progress, sizeof(progress), "Step %d / %d", (int)applied_event_count,
             (int)trace->count);

    DrawRectangle(790, 0, 310, 720, (Color){245, 247, 250, 255});
    DrawLine(790, 0, 790, 720, (Color){209, 213, 219, 255});

    DrawText("Graphe DFS", x, y, 26, (Color){17, 24, 39, 255});
    DrawText(progress, x, y + line * 2, 18, DARKGRAY);
    DrawText(event_text, x, y + line * 3, 16, (Color){17, 24, 39, 255});

    DrawText("Controls", x, y + line * 5, 20, (Color){17, 24, 39, 255});
    DrawText("Space: play / pause", x, y + line * 6, 16, DARKGRAY);
    DrawText("Right / Left: step", x, y + line * 7, 16, DARKGRAY);
    DrawText("Home / End: jump", x, y + line * 8, 16, DARKGRAY);
    DrawText("R: reset layout", x, y + line * 9, 16, DARKGRAY);
    DrawText("Mouse: drag nodes", x, y + line * 10, 16, DARKGRAY);

    DrawText("Edge Types", x, y + line * 12, 20, (Color){17, 24, 39, 255});
    DrawText("tree", x + 24, y + line * 13, 16, edge_color(EDGE_TREE));
    DrawText("back", x + 24, y + line * 14, 16, edge_color(EDGE_BACK));
    DrawText("forward", x + 24, y + line * 15, 16, edge_color(EDGE_FORWARD));
    DrawText("cross", x + 24, y + line * 16, 16, edge_color(EDGE_CROSS));

    DrawCircle(x + 8, y + line * 13 + 8, 6.0f, edge_color(EDGE_TREE));
    DrawCircle(x + 8, y + line * 14 + 8, 6.0f, edge_color(EDGE_BACK));
    DrawCircle(x + 8, y + line * 15 + 8, 6.0f, edge_color(EDGE_FORWARD));
    DrawCircle(x + 8, y + line * 16 + 8, 6.0f, edge_color(EDGE_CROSS));
}

void render_graph(const Graph *graph, const DfsTrace *trace,
                  size_t applied_event_count) {
    size_t active_index =
        applied_event_count == 0 ? trace->count : applied_event_count - 1;

    for (size_t i = 0; i < graph->edge_count; i++)
        draw_edge(graph, i, is_active_edge(trace, active_index, i));

    for (size_t i = 0; i < graph->node_count; i++)
        draw_node(&graph->nodes[i], is_active_node(trace, active_index, i));

    draw_sidebar(graph, trace, applied_event_count);
}
