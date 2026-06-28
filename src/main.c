#include "graph.h"
#include "graph_io.h"
#include "layout.h"
#include "platform_window.h"
#include "render.h"
#include "traversal.h"

#include "raylib.h"

#include <stdbool.h>
#include <stdio.h>

#define GRAPHE_MIN_WINDOW_WIDTH 880
#define GRAPHE_MIN_WINDOW_HEIGHT 560

static int scaled_icon_value(int icon_size, float value) {
    int scaled = (int)(value * (float)icon_size / 64.0f + 0.5f);
    if (scaled < 1) scaled = 1;
    return scaled;
}

static Vector2 scaled_icon_point(int icon_size, float x, float y) {
    return (Vector2){
        (float)scaled_icon_value(icon_size, x),
        (float)scaled_icon_value(icon_size, y),
    };
}

static Image make_generated_window_icon(int icon_size) {
    Color background = {17, 24, 39, 255};
    Color edge = {34, 197, 94, 255};
    Color accent = {251, 146, 60, 255};
    Color node = {243, 244, 246, 255};
    Color outline = {75, 85, 99, 255};
    Image icon = GenImageColor(icon_size, icon_size, background);
    int node_radius = scaled_icon_value(icon_size, 8.0f);
    int line_thickness = scaled_icon_value(icon_size, 4.0f);

    Vector2 a = scaled_icon_point(icon_size, 20.0f, 18.0f);
    Vector2 b = scaled_icon_point(icon_size, 46.0f, 23.0f);
    Vector2 c = scaled_icon_point(icon_size, 32.0f, 47.0f);

    ImageDrawLineEx(&icon, a, b, line_thickness, edge);
    ImageDrawLineEx(&icon, a, c, line_thickness, accent);
    ImageDrawLineEx(&icon, b, c, line_thickness, edge);

    ImageDrawCircleV(&icon, a, node_radius, node);
    ImageDrawCircleV(&icon, b, node_radius, node);
    ImageDrawCircleV(&icon, c, node_radius, node);
    ImageDrawCircleLinesV(&icon, a, node_radius, outline);
    ImageDrawCircleLinesV(&icon, b, node_radius, outline);
    ImageDrawCircleLinesV(&icon, c, node_radius, outline);

    return icon;
}

static void set_window_icon(void) {
    const char *icon_path = "assets/icons/graphe.png";

    if (FileExists(icon_path)) {
        Image icon = LoadImage(icon_path);
        if (icon.data != NULL) {
            SetWindowIcon(icon);
            UnloadImage(icon);
            return;
        }
    }

    Image icons[] = {
        make_generated_window_icon(16),
        make_generated_window_icon(32),
        make_generated_window_icon(64),
        make_generated_window_icon(128),
    };

    SetWindowIcons(icons, (int)(sizeof(icons) / sizeof(icons[0])));

    for (size_t i = 0; i < sizeof(icons) / sizeof(icons[0]); i++)
        UnloadImage(icons[i]);
}

static float point_distance_squared(Vector2 a, Vector2 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

static float clamp_float(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
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

static void step_forward(size_t *step, const Trace *trace) {
    if (*step < trace->count) (*step)++;
}

static void step_backward(size_t *step) {
    if (*step > 0) (*step)--;
}

static Camera2D make_graph_camera(void) {
    return (Camera2D){
        .offset = {0.0f, 0.0f},
        .target = {0.0f, 0.0f},
        .rotation = 0.0f,
        .zoom = 1.0f,
    };
}

static void reset_graph_camera(Camera2D *camera, const Graph *graph,
                               const RenderOptions *options) {
    *camera = make_graph_camera();

    if (graph->node_count == 0) return;

    float min_x = graph->nodes[0].x - GRAPHE_NODE_RADIUS;
    float max_x = graph->nodes[0].x + GRAPHE_NODE_RADIUS;
    float min_y = graph->nodes[0].y - GRAPHE_NODE_RADIUS;
    float max_y = graph->nodes[0].y + GRAPHE_NODE_RADIUS;

    for (size_t i = 1; i < graph->node_count; i++) {
        const Node *node = &graph->nodes[i];

        if (node->x - GRAPHE_NODE_RADIUS < min_x)
            min_x = node->x - GRAPHE_NODE_RADIUS;
        if (node->x + GRAPHE_NODE_RADIUS > max_x)
            max_x = node->x + GRAPHE_NODE_RADIUS;
        if (node->y - GRAPHE_NODE_RADIUS < min_y)
            min_y = node->y - GRAPHE_NODE_RADIUS;
        if (node->y + GRAPHE_NODE_RADIUS > max_y)
            max_y = node->y + GRAPHE_NODE_RADIUS;
    }

    Vector2 graph_center = {
        (min_x + max_x) * 0.5f,
        (min_y + max_y) * 0.5f,
    };
    Vector2 canvas_center = {
        render_graph_area_width(options) * 0.5f,
        render_graph_area_height() * 0.5f,
    };

    camera->target.x = graph_center.x - canvas_center.x;
    camera->target.y = graph_center.y - canvas_center.y;
}

static void apply_layout(Graph *graph, const Trace *trace,
                         const RenderOptions *options) {
    float graph_width = render_graph_area_width(options);
    float graph_height = render_graph_area_height();

    switch (options->layout_mode) {
    case LAYOUT_TRACE_FOREST:
        layout_trace_forest(graph, trace, 80.0f, 76.0f, graph_width - 160.0f,
                            118.0f);
        break;
    case LAYOUT_CIRCULAR: {
        float shortest_side =
            graph_width < graph_height ? graph_width : graph_height;
        layout_circle(graph, graph_width * 0.5f, graph_height * 0.52f,
                      shortest_side * 0.30f);
        break;
    }
    case LAYOUT_MANUAL:
    default:
        break;
    }
}

static Graph *active_base_graph(RenderOptions *options, Graph *graph_base,
                                Graph *tree_base) {
    if (options->algorithm_mode == ALGORITHM_TREE) return tree_base;
    return graph_base;
}

static void rebuild_trace(Graph *base_graph, Trace *trace,
                          const RenderOptions *options, size_t *step) {
    TraversalOptions traversal_options = {
        .algorithm = options->algorithm_mode,
        .alphabetical = options->alphabetical_order,
        .tree_order = options->tree_order,
    };

    traversal_trace_build(base_graph, &traversal_options, trace);
    if (*step > trace->count) *step = trace->count;
}

static size_t visible_edge_count(const Graph *graph) {
    size_t count = 0;

    for (size_t i = 0; i < graph->edge_count; i++) {
        if (graph_edge_is_visible(graph, (int)i)) count++;
    }

    return count;
}

static bool graph_positions_collapsed(const Graph *graph) {
    if (graph->node_count <= 1) return false;

    float x = graph->nodes[0].x;
    float y = graph->nodes[0].y;

    for (size_t i = 1; i < graph->node_count; i++) {
        if (graph->nodes[i].x != x || graph->nodes[i].y != y) return false;
    }

    return true;
}

static void apply_layout_for_collapsed_graph(Graph *graph, const Trace *trace,
                                             RenderOptions *options) {
    if (options->layout_mode == LAYOUT_MANUAL)
        options->layout_mode = LAYOUT_TRACE_FOREST;
    apply_layout(graph, trace, options);
}

static void refresh_layout_after_window_change(Graph *graph, const Trace *trace,
                                               const RenderOptions *options,
                                               Camera2D *camera) {
    if (options->layout_mode != LAYOUT_MANUAL) apply_layout(graph, trace, options);
    reset_graph_camera(camera, graph, options);
}

static bool adjust_ui_scale(RenderOptions *options, float delta) {
    float next_scale = clamp_float(options->ui_scale + delta, GRAPHE_UI_SCALE_MIN,
                                   GRAPHE_UI_SCALE_MAX);

    if (next_scale == options->ui_scale) return false;

    options->ui_scale = next_scale;
    return true;
}

static bool reset_ui_scale(RenderOptions *options) {
    if (options->ui_scale == GRAPHE_UI_SCALE_DEFAULT) return false;

    options->ui_scale = GRAPHE_UI_SCALE_DEFAULT;
    return true;
}

static bool scale_up_key_pressed(void) {
    return IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD) ||
           IsKeyPressed(KEY_RIGHT_BRACKET);
}

static bool scale_down_key_pressed(void) {
    return IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT) ||
           IsKeyPressed(KEY_SLASH);
}

static void zoom_camera_at_mouse(Camera2D *camera, Vector2 mouse, float wheel) {
    Vector2 before = GetScreenToWorld2D(mouse, *camera);
    float zoom_factor = 1.0f + wheel * 0.12f;

    if (zoom_factor < 0.25f) zoom_factor = 0.25f;
    camera->zoom = clamp_float(camera->zoom * zoom_factor, 0.35f, 2.8f);

    Vector2 after = GetScreenToWorld2D(mouse, *camera);
    camera->target.x += before.x - after.x;
    camera->target.y += before.y - after.y;
}

static void apply_ui_result(RenderUiResult result, Graph *graph_base,
                            Graph *tree_base, Trace *trace, RenderOptions *options,
                            size_t *step, Camera2D *camera, bool *playing) {
    Graph *base_graph = active_base_graph(options, graph_base, tree_base);

    if (result.graph_load_requested) {
        Graph loaded_graph;
        char error[GRAPHE_STATUS_MESSAGE_MAX];

        options->graph_path_editing = false;

        if (graph_load_from_file(options->graph_path, &loaded_graph, error,
                                 sizeof(error))) {
            *graph_base = loaded_graph;
            options->directed_graph = graph_base->directed;
            options->loaded_graph_directed = graph_base->directed;
            options->graph_file_dirty = false;
            if (options->layout_mode == LAYOUT_MANUAL)
                options->layout_mode = LAYOUT_TRACE_FOREST;
            base_graph = active_base_graph(options, graph_base, tree_base);
            *step = 0;
            rebuild_trace(base_graph, trace, options, step);
            apply_layout(base_graph, trace, options);
            reset_graph_camera(camera, base_graph, options);
            *playing = false;
            snprintf(options->status_message, sizeof(options->status_message),
                     "Loaded %d nodes, %d edges", (int)graph_base->node_count,
                     (int)visible_edge_count(graph_base));
        } else {
            snprintf(options->status_message, sizeof(options->status_message),
                     "Load failed: %.120s", error);
        }

        return;
    }

    if (options->algorithm_mode != ALGORITHM_TREE &&
        graph_base->directed != options->directed_graph) {
        if (graph_set_directed(graph_base, options->directed_graph)) {
            result.trace_changed = true;
            options->graph_file_dirty =
                graph_base->directed != options->loaded_graph_directed;
            options->status_message[0] = '\0';
        } else {
            options->directed_graph = graph_base->directed;
            snprintf(options->status_message, sizeof(options->status_message),
                     "Could not change graph direction");
        }
    }

    base_graph = active_base_graph(options, graph_base, tree_base);

    if (result.trace_changed) {
        rebuild_trace(base_graph, trace, options, step);
        *step = 0;
        if (graph_positions_collapsed(base_graph)) {
            apply_layout_for_collapsed_graph(base_graph, trace, options);
        } else if (options->layout_mode == LAYOUT_TRACE_FOREST) {
            apply_layout(base_graph, trace, options);
        }
        reset_graph_camera(camera, base_graph, options);
        *playing = false;
    }

    if (result.layout_changed) {
        apply_layout(base_graph, trace, options);
        reset_graph_camera(camera, base_graph, options);
        *playing = false;
    }
}

int main(void) {
    const int screen_width = 1180;
    const int screen_height = 760;
    const float seconds_per_step = 0.65f;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screen_width, screen_height, "Graphe");
    set_window_icon();
    SetWindowMinSize(GRAPHE_MIN_WINDOW_WIDTH, GRAPHE_MIN_WINDOW_HEIGHT);
    SetTargetFPS(60);

    RenderResources render_resources;
    render_resources_load(&render_resources);

    RenderOptions options = {
        .dark_mode = true,
        .settings_open = false,
        .alphabetical_order = true,
        .directed_graph = true,
        .graph_path_editing = false,
        .graph_file_dirty = false,
        .loaded_graph_directed = true,
        .algorithm_mode = ALGORITHM_DFS,
        .tree_order = TREE_ORDER_INORDER,
        .layout_mode = LAYOUT_TRACE_FOREST,
        .ui_scale = GRAPHE_UI_SCALE_DEFAULT,
        .graph_path = "graphs/sample_directed.graphe",
        .status_message = "",
    };
    platform_window_apply_style(options.dark_mode);
    bool native_style_dark_mode = options.dark_mode;

    Graph graph_base;
    graph_build_sample(&graph_base);
    Graph tree_base;
    graph_build_expression_tree(&tree_base);

    Trace trace;
    size_t step = 0;
    Graph *base_graph = active_base_graph(&options, &graph_base, &tree_base);
    rebuild_trace(base_graph, &trace, &options, &step);
    apply_layout(base_graph, &trace, &options);

    Camera2D graph_camera = make_graph_camera();
    reset_graph_camera(&graph_camera, base_graph, &options);
    Graph scene_graph;
    bool playing = false;
    float playback_time = 0.0f;
    int dragging_node = -1;
    bool panning_graph = false;
    Vector2 drag_offset = {0.0f, 0.0f};
    int window_width = GetScreenWidth();
    int window_height = GetScreenHeight();

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        Vector2 mouse_world = GetScreenToWorld2D(mouse, graph_camera);
        float dt = GetFrameTime();
        float wheel = GetMouseWheelMove();
        bool control_down =
            IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        bool shortcuts_enabled = !options.graph_path_editing;
        RenderUiResult ui_result = render_update_options(&options);

        base_graph = active_base_graph(&options, &graph_base, &tree_base);

        apply_ui_result(ui_result, &graph_base, &tree_base, &trace, &options, &step,
                        &graph_camera, &playing);
        if (native_style_dark_mode != options.dark_mode) {
            platform_window_apply_style(options.dark_mode);
            native_style_dark_mode = options.dark_mode;
        }

        if (shortcuts_enabled && control_down) {
            bool scale_changed = false;

            if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0))
                scale_changed = reset_ui_scale(&options) || scale_changed;
            if (scale_up_key_pressed())
                scale_changed = adjust_ui_scale(&options, 0.08f) || scale_changed;
            if (scale_down_key_pressed())
                scale_changed = adjust_ui_scale(&options, -0.08f) || scale_changed;
            if (wheel != 0.0f)
                scale_changed =
                    adjust_ui_scale(&options, wheel * 0.06f) || scale_changed;

            if (scale_changed) {
                base_graph = active_base_graph(&options, &graph_base, &tree_base);
                refresh_layout_after_window_change(base_graph, &trace, &options,
                                                   &graph_camera);
            }
        }

        int current_window_width = GetScreenWidth();
        int current_window_height = GetScreenHeight();
        if (current_window_width > 0 && current_window_height > 0 &&
            (current_window_width != window_width ||
             current_window_height != window_height)) {
            window_width = current_window_width;
            window_height = current_window_height;
            base_graph = active_base_graph(&options, &graph_base, &tree_base);
            refresh_layout_after_window_change(base_graph, &trace, &options,
                                               &graph_camera);
        }

        if (shortcuts_enabled && IsKeyPressed(KEY_SPACE)) playing = !playing;
        if (shortcuts_enabled && IsKeyPressed(KEY_RIGHT)) {
            step_forward(&step, &trace);
            playing = false;
        }
        if (shortcuts_enabled && IsKeyPressed(KEY_LEFT)) {
            step_backward(&step);
            playing = false;
        }
        if (shortcuts_enabled && !control_down &&
            (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0))) {
            step = 0;
            playing = false;
        }
        if (shortcuts_enabled &&
            (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER))) {
            step = trace.count;
            playing = false;
        }
        if (shortcuts_enabled && IsKeyPressed(KEY_R)) {
            base_graph = active_base_graph(&options, &graph_base, &tree_base);
            apply_layout(base_graph, &trace, &options);
            reset_graph_camera(&graph_camera, base_graph, &options);
            playing = false;
        }

        if (!control_down && render_point_in_graph_canvas(mouse, &options) &&
            !options.settings_open && wheel != 0.0f)
            zoom_camera_at_mouse(&graph_camera, mouse, wheel);

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

        if (!ui_result.consumed_click && !options.settings_open &&
            render_point_in_graph_canvas(mouse, &options) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            base_graph = active_base_graph(&options, &graph_base, &tree_base);
            dragging_node = find_node_at(base_graph, mouse_world);
            if (dragging_node >= 0) {
                options.layout_mode = LAYOUT_MANUAL;
                drag_offset.x = base_graph->nodes[dragging_node].x - mouse_world.x;
                drag_offset.y = base_graph->nodes[dragging_node].y - mouse_world.y;
            } else {
                panning_graph = true;
            }
        }

        if (dragging_node >= 0) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                mouse_world = GetScreenToWorld2D(mouse, graph_camera);
                base_graph = active_base_graph(&options, &graph_base, &tree_base);
                base_graph->nodes[dragging_node].x = mouse_world.x + drag_offset.x;
                base_graph->nodes[dragging_node].y = mouse_world.y + drag_offset.y;
            } else {
                dragging_node = -1;
            }
        }

        if (panning_graph) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                Vector2 delta = GetMouseDelta();
                graph_camera.target.x -= delta.x / graph_camera.zoom;
                graph_camera.target.y -= delta.y / graph_camera.zoom;
            } else {
                panning_graph = false;
            }
        }

        base_graph = active_base_graph(&options, &graph_base, &tree_base);
        traversal_trace_apply_prefix(base_graph, &trace, step, &scene_graph);

        BeginDrawing();
        ClearBackground(render_background_color(&options));
        RenderUiResult draw_result = render_graph(
            &scene_graph, &trace, step, &render_resources, &options, &graph_camera);
        EndDrawing();

        apply_ui_result(draw_result, &graph_base, &tree_base, &trace, &options,
                        &step, &graph_camera, &playing);
        if (native_style_dark_mode != options.dark_mode) {
            platform_window_apply_style(options.dark_mode);
            native_style_dark_mode = options.dark_mode;
        }
    }

    render_resources_unload(&render_resources);
    CloseWindow();
    return 0;
}
