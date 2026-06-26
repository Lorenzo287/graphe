#include "dfs.h"
#include "graph.h"
#include "layout.h"
#include "render.h"

#include "raylib.h"

#include <stdbool.h>

static float point_distance_squared(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

static int find_node_at(const Graph *graph, Vector2 point) {
    float hit_radius_squared = GRAPHE_NODE_RADIUS * GRAPHE_NODE_RADIUS;

    for (size_t i = graph->node_count; i > 0; i--) {
        const Node *node = &graph->nodes[i - 1];
        Vector2 center = {node->x, node->y};

        if (point_distance_squared(center, point) <= hit_radius_squared)
            return (int)i - 1;
    }

    return -1;
}

static void step_forward(size_t *step, const DfsTrace *trace) {
    if (*step < trace->count) (*step)++;
}

static void step_backward(size_t *step) {
    if (*step > 0) (*step)--;
}

int main(void) {
    const int screen_width = 1100;
    const int screen_height = 720;
    const float seconds_per_step = 0.65f;

    InitWindow(screen_width, screen_height, "Graphe - DFS Visualizer");
    SetTargetFPS(60);

    Graph base_graph;
    graph_build_sample(&base_graph);
    layout_circle(&base_graph, 390.0f, 360.0f, 230.0f);

    DfsTrace trace;
    dfs_trace_build(&base_graph, &trace);

    Graph scene_graph;
    size_t step = 0;
    bool playing = false;
    float playback_time = 0.0f;
    int dragging_node = -1;
    Vector2 drag_offset = {0.0f, 0.0f};

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_SPACE)) { 
			playing = !playing; 
		}
        if (IsKeyPressed(KEY_RIGHT)) {
            step_forward(&step, &trace);
            playing = false;
        }
        if (IsKeyPressed(KEY_LEFT)) {
            step_backward(&step);
            playing = false;
        }
        if (IsKeyPressed(KEY_HOME)) {
            step = 0;
            playing = false;
        }
        if (IsKeyPressed(KEY_END)) {
            step = trace.count;
            playing = false;
        }
        if (IsKeyPressed(KEY_R)) {
            layout_circle(&base_graph, 390.0f, 360.0f, 230.0f);
        }

        if (playing) {
            playback_time += dt;
            while (playback_time >= seconds_per_step) {
                playback_time -= seconds_per_step;
                step_forward(&step, &trace);
                if (step >= trace.count) {
                    playing = false;
                    break;
                }
            }
        } else {
            playback_time = 0.0f;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            dragging_node = find_node_at(&base_graph, mouse);
            if (dragging_node >= 0) {
                drag_offset.x = base_graph.nodes[dragging_node].x - mouse.x;
                drag_offset.y = base_graph.nodes[dragging_node].y - mouse.y;
            }
        }

        if (dragging_node >= 0) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                base_graph.nodes[dragging_node].x = mouse.x + drag_offset.x;
                base_graph.nodes[dragging_node].y = mouse.y + drag_offset.y;
            } else {
                dragging_node = -1;
            }
        }

        dfs_trace_apply_prefix(&base_graph, &trace, step, &scene_graph);

        BeginDrawing();
        ClearBackground((Color){250, 250, 249, 255});
        render_graph(&scene_graph, &trace, step);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
