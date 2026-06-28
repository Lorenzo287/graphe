#include "render.h"

#include "raygui.h"
#include "raylib.h"

#include <math.h>
#include <stdio.h>

typedef struct Theme {
    Color background;
    Color panel;
    Color panel_border;
    Color text;
    Color muted_text;
    Color node_white;
    Color node_gray;
    Color node_black;
    Color node_outline;
    Color active;
    Color edge_unclassified;
    Color edge_tree;
    Color edge_back;
    Color edge_forward;
    Color edge_cross;
    Color button;
    Color button_hover;
    Color button_active;
    Color overlay;
} Theme;

typedef struct EdgeGeometry {
    Vector2 start;
    Vector2 control;
    Vector2 end;
    Vector2 end_direction;
    float bend;
} EdgeGeometry;

static Theme light_theme(void) {
    return (Theme){
        .background = {250, 250, 249, 255},
        .panel = {245, 247, 250, 255},
        .panel_border = {209, 213, 219, 255},
        .text = {17, 24, 39, 255},
        .muted_text = {75, 85, 99, 255},
        .node_white = {255, 255, 255, 255},
        .node_gray = {255, 202, 58, 255},
        .node_black = {75, 85, 99, 255},
        .node_outline = {31, 41, 55, 255},
        .active = {249, 115, 22, 255},
        .edge_unclassified = {156, 163, 175, 255},
        .edge_tree = {22, 163, 74, 255},
        .edge_back = {239, 68, 68, 255},
        .edge_forward = {37, 99, 235, 255},
        .edge_cross = {147, 51, 234, 255},
        .button = {229, 231, 235, 255},
        .button_hover = {209, 213, 219, 255},
        .button_active = {191, 219, 254, 255},
        .overlay = {17, 24, 39, 170},
    };
}

static Theme dark_theme(void) {
    return (Theme){
        .background = {17, 24, 39, 255},
        .panel = {31, 41, 55, 255},
        .panel_border = {75, 85, 99, 255},
        .text = {243, 244, 246, 255},
        .muted_text = {176, 183, 195, 255},
        .node_white = {243, 244, 246, 255},
        .node_gray = {245, 158, 11, 255},
        .node_black = {55, 65, 81, 255},
        .node_outline = {209, 213, 219, 255},
        .active = {251, 146, 60, 255},
        .edge_unclassified = {107, 114, 128, 255},
        .edge_tree = {34, 197, 94, 255},
        .edge_back = {248, 113, 113, 255},
        .edge_forward = {96, 165, 250, 255},
        .edge_cross = {192, 132, 252, 255},
        .button = {55, 65, 81, 255},
        .button_hover = {75, 85, 99, 255},
        .button_active = {30, 64, 175, 255},
        .overlay = {3, 7, 18, 190},
    };
}

static Theme theme_for_options(const RenderOptions *options) {
    if (options->dark_mode) return dark_theme();
    return light_theme();
}

static float ui_scale_value(const RenderOptions *options) {
    if (options == NULL || options->ui_scale <= 0.0f) return 1.0f;
    if (options->ui_scale < GRAPHE_UI_SCALE_MIN) return GRAPHE_UI_SCALE_MIN;
    if (options->ui_scale > GRAPHE_UI_SCALE_MAX) return GRAPHE_UI_SCALE_MAX;
    return options->ui_scale;
}

static float ui_pixel_scale(const RenderOptions *options) {
    return ui_scale_value(options) * GRAPHE_UI_PIXEL_SCALE;
}

static float ui_size(const RenderOptions *options, float value) {
    return value * ui_pixel_scale(options);
}

static bool change_ui_scale(RenderOptions *options, float delta) {
    float next_scale = options->ui_scale + delta;

    if (next_scale < GRAPHE_UI_SCALE_MIN) next_scale = GRAPHE_UI_SCALE_MIN;
    if (next_scale > GRAPHE_UI_SCALE_MAX) next_scale = GRAPHE_UI_SCALE_MAX;
    if (next_scale == options->ui_scale) return false;

    options->ui_scale = next_scale;
    return true;
}

float render_sidebar_width(const RenderOptions *options) {
    float sidebar_width = GRAPHE_SIDEBAR_WIDTH * ui_pixel_scale(options);
    float max_sidebar_width = (float)GetScreenWidth() - 320.0f;

    if (max_sidebar_width < 280.0f) max_sidebar_width = 280.0f;
    if (sidebar_width > max_sidebar_width) sidebar_width = max_sidebar_width;

    return sidebar_width;
}

float render_graph_area_width(const RenderOptions *options) {
    return (float)GetScreenWidth() - render_sidebar_width(options);
}

float render_graph_area_height(void) {
    return (float)GetScreenHeight();
}

static float sidebar_left(const RenderOptions *options) {
    return render_graph_area_width(options);
}

bool render_point_in_sidebar(Vector2 point, const RenderOptions *options) {
    return point.x >= sidebar_left(options);
}

bool render_point_in_graph_canvas(Vector2 point, const RenderOptions *options) {
    return point.x < sidebar_left(options);
}

Color render_background_color(const RenderOptions *options) {
    return theme_for_options(options).background;
}

static unsigned char color_mix_channel(unsigned char from, unsigned char to,
                                       float amount) {
    return (unsigned char)((float)from + ((float)to - (float)from) * amount);
}

static Color color_mix(Color from, Color to, float amount) {
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;

    return (Color){
        color_mix_channel(from.r, to.r, amount),
        color_mix_channel(from.g, to.g, amount),
        color_mix_channel(from.b, to.b, amount),
        color_mix_channel(from.a, to.a, amount),
    };
}

static bool color_is_dark(Color color) {
    int luminance = (int)color.r * 299 + (int)color.g * 587 + (int)color.b * 114;
    return luminance < 140000;
}

static Color node_text_color(Color fill) {
    return color_is_dark(fill) ? RAYWHITE : (Color){0, 0, 0, 255};
}

static Color node_fill_color(const Theme *theme, NodeColor color) {
    switch (color) {
    case NODE_WHITE:
        return theme->node_white;
    case NODE_GRAY:
        return theme->node_gray;
    case NODE_BLACK:
        return theme->node_black;
    default:
        return theme->node_white;
    }
}

static Color bfs_level_fill_color(const Theme *theme, const Node *node,
                                  int max_level) {
    if (node->level < 0) return theme->node_white;
    if (max_level <= 0) return theme->node_gray;

    Color shallow = theme->edge_tree;
    Color middle = theme->node_gray;
    Color deep = theme->edge_forward;
    float amount = (float)node->level / (float)max_level;

    if (amount <= 0.5f) return color_mix(shallow, middle, amount * 2.0f);
    return color_mix(middle, deep, (amount - 0.5f) * 2.0f);
}

static Color edge_color(const Theme *theme, EdgeType type) {
    switch (type) {
    case EDGE_TREE:
        return theme->edge_tree;
    case EDGE_BACK:
        return theme->edge_back;
    case EDGE_FORWARD:
        return theme->edge_forward;
    case EDGE_CROSS:
        return theme->edge_cross;
    case EDGE_UNCLASSIFIED:
    default:
        return theme->edge_unclassified;
    }
}

static Color edge_render_color(const Theme *theme, EdgeType type,
                               const RenderOptions *options) {
    if (options->algorithm_mode == ALGORITHM_BFS) return theme->edge_unclassified;
    return edge_color(theme, type);
}

static int max_bfs_level(const Graph *graph) {
    int max_level = 0;

    for (size_t i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].level > max_level) max_level = graph->nodes[i].level;
    }

    return max_level;
}

static int max_bfs_trace_level(const Trace *trace) {
    int max_level = 0;

    for (size_t i = 0; i < trace->count; i++) {
        const TraceEvent *event = &trace->events[i];
        if (event->type != TRACE_EVENT_DISCOVER_NODE) continue;
        if (event->time > max_level) max_level = event->time;
    }

    return max_level;
}

static int node_count_with_level(const Graph *graph) {
    int count = 0;

    for (size_t i = 0; i < graph->node_count; i++) {
        if (graph->nodes[i].level >= 0) count++;
    }

    return count;
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

static Vector2 quadratic_point(Vector2 start, Vector2 control, Vector2 end,
                               float t) {
    float inv = 1.0f - t;
    return (Vector2){
        inv * inv * start.x + 2.0f * inv * t * control.x + t * t * end.x,
        inv * inv * start.y + 2.0f * inv * t * control.y + t * t * end.y,
    };
}

#define GRAPHE_ARROW_LENGTH 15.0f
#define GRAPHE_ARROW_WIDTH 11.0f
#define GRAPHE_ARROW_BODY_OVERLAP 0.5f

static Vector2 edge_body_arrow_endpoint(Vector2 tip, Vector2 direction) {
    return vector_subtract(
        tip,
        vector_scale(direction, GRAPHE_ARROW_LENGTH - GRAPHE_ARROW_BODY_OVERLAP));
}

static void draw_arrowhead_shape(Vector2 tip, Vector2 direction, float length,
                                 float width, Color color) {
    Vector2 normal = {-direction.y, direction.x};
    Vector2 base = vector_subtract(tip, vector_scale(direction, length));
    Vector2 left = vector_add(base, vector_scale(normal, width * 0.5f));
    Vector2 right = vector_subtract(base, vector_scale(normal, width * 0.5f));

    DrawTriangle(tip, right, left, color);
}

static void draw_arrowhead(Vector2 tip, Vector2 direction, Color color) {
    draw_arrowhead_shape(tip, direction, GRAPHE_ARROW_LENGTH, GRAPHE_ARROW_WIDTH,
                         color);
}

static void draw_arrowhead_glow(Vector2 tip, Vector2 direction, Color color) {
    const float lengths[] = {21.0f, 18.0f, 16.0f};
    const float widths[] = {18.0f, 15.0f, 12.5f};
    const unsigned char alphas[] = {14, 22, 32};

    for (int i = 0; i < 3; i++) {
        Color glow = color;
        glow.a = alphas[i];
        draw_arrowhead_shape(tip, direction, lengths[i], widths[i], glow);
    }
}

static void draw_text(const RenderResources *resources, const char *text,
                      Vector2 position, float size, Color color) {
    DrawTextEx(resources->font, text, position, size, size * 0.04f, color);
}

static Vector2 measure_text(const RenderResources *resources, const char *text,
                            float size) {
    return MeasureTextEx(resources->font, text, size, size * 0.04f);
}

void render_resources_load(RenderResources *resources) {
    const char *font_paths[] = {
        "assets/fonts/AtkinsonHyperlegible-Regular.ttf",
        "assets/fonts/Inter-Regular.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
    };

    resources->font = GetFontDefault();
    resources->custom_font_loaded = false;

    for (size_t i = 0; i < sizeof(font_paths) / sizeof(font_paths[0]); i++) {
        if (!FileExists(font_paths[i])) continue;

        Font font = LoadFontEx(font_paths[i], 96, NULL, 0);
        if (font.texture.id == 0) continue;

        GenTextureMipmaps(&font.texture);
        SetTextureFilter(font.texture, TEXTURE_FILTER_TRILINEAR);
        resources->font = font;
        resources->custom_font_loaded = true;
        return;
    }
}

void render_resources_unload(RenderResources *resources) {
    if (resources->custom_font_loaded) UnloadFont(resources->font);
    resources->custom_font_loaded = false;
}

static int event_matches_visible_edge(const Graph *graph, int event_edge,
                                      size_t edge_index) {
    (void)graph;

    if (event_edge < 0) return 0;
    return event_edge == (int)edge_index;
}

static int is_active_edge(const Graph *graph, const Trace *trace,
                          size_t active_index, size_t edge_index) {
    if (active_index >= trace->count) return 0;

    return event_matches_visible_edge(graph, trace->events[active_index].edge,
                                      edge_index);
}

static int is_active_examine_edge(const Graph *graph, const Trace *trace,
                                  size_t active_index, size_t edge_index) {
    if (active_index >= trace->count) return 0;
    if (trace->events[active_index].type != TRACE_EVENT_EXAMINE_EDGE) return 0;

    return event_matches_visible_edge(graph, trace->events[active_index].edge,
                                      edge_index);
}

static int is_active_node(const Trace *trace, size_t active_index,
                          size_t node_index) {
    if (active_index >= trace->count) return 0;

    return trace->events[active_index].node == (int)node_index;
}

static void draw_quadratic_edge(Vector2 start, Vector2 control, Vector2 end,
                                float thickness, Color color) {
    const int segments = 24;
    Vector2 previous = start;

    for (int i = 1; i <= segments; i++) {
        float t = (float)i / (float)segments;
        Vector2 current = quadratic_point(start, control, end, t);
        DrawLineEx(previous, current, thickness, color);
        previous = current;
    }
}

static void draw_edge_path(EdgeGeometry geometry, Vector2 end, float thickness,
                           Color color) {
    if (geometry.bend <= 0.0f) {
        DrawLineEx(geometry.start, end, thickness, color);
        return;
    }

    draw_quadratic_edge(geometry.start, geometry.control, end, thickness, color);
}

static void draw_edge_glow(EdgeGeometry geometry, Vector2 end, Color color) {
    const float widths[] = {15.0f, 10.0f, 6.0f};
    const unsigned char alphas[] = {20, 32, 50};

    for (int i = 0; i < 3; i++) {
        Color glow = color;
        glow.a = alphas[i];
        draw_edge_path(geometry, end, widths[i], glow);
    }
}

static Vector2 edge_glow_arrow_endpoint(Vector2 tip, Vector2 direction) {
    return vector_subtract(tip,
                           vector_scale(direction, GRAPHE_ARROW_LENGTH * 1.0));
}

static float clamp_float(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static float vector_length(Vector2 value) {
    return sqrtf(value.x * value.x + value.y * value.y);
}

static float edge_bend_amount(EdgeType type, float distance) {
    float factor;

    switch (type) {
    case EDGE_BACK:
        factor = 0.24f;
        break;
    case EDGE_FORWARD:
        factor = 0.18f;
        break;
    case EDGE_CROSS:
        factor = 0.22f;
        break;
    default:
        return 0.0f;
    }

    return clamp_float(distance * factor, 18.0f, 70.0f);
}

static float edge_curve_sign(const Node *from, const Node *to, EdgeType type,
                             const RenderOptions *options) {
    Vector2 mid = {(from->x + to->x) * 0.5f, (from->y + to->y) * 0.5f};

    if (type == EDGE_BACK || type == EDGE_FORWARD) {
        float graph_center = render_graph_area_width(options) * 0.5f;
        return mid.x < graph_center ? -1.0f : 1.0f;
    }

    return from->x <= to->x ? 1.0f : -1.0f;
}

static EdgeGeometry make_edge_geometry(const Graph *graph, size_t edge_index,
                                       int active, const RenderOptions *options) {
    const Edge *edge = &graph->edges[edge_index];
    const Node *from = &graph->nodes[edge->from];
    const Node *to = &graph->nodes[edge->to];
    Vector2 start_center = {from->x, from->y};
    Vector2 end_center = {to->x, to->y};
    Vector2 delta = vector_subtract(end_center, start_center);
    float distance = vector_length(delta);
    Vector2 direction = vector_normalize(delta);
    float node_padding = graph->directed ? 2.0f : 0.0f;
    EdgeGeometry geometry = {
        .start =
            vector_add(start_center, vector_scale(direction, GRAPHE_NODE_RADIUS)),
        .control = {0.0f, 0.0f},
        .end = vector_subtract(
            end_center, vector_scale(direction, GRAPHE_NODE_RADIUS + node_padding)),
        .end_direction = direction,
        .bend = 0.0f,
    };
    float bend = active && edge->type == EDGE_UNCLASSIFIED
                     ? 0.0f
                     : edge_bend_amount(edge->type, distance);

    if (bend <= 0.0f) return geometry;

    Vector2 mid = vector_scale(vector_add(geometry.start, geometry.end), 0.5f);
    Vector2 normal = {-direction.y, direction.x};
    float sign = edge_curve_sign(from, to, edge->type, options);
    geometry.control = vector_add(mid, vector_scale(normal, bend * sign));
    geometry.start =
        vector_add(start_center, vector_scale(vector_normalize(vector_subtract(
                                                  geometry.control, start_center)),
                                              GRAPHE_NODE_RADIUS));
    geometry.end = vector_add(
        end_center,
        vector_scale(vector_normalize(vector_subtract(geometry.control, end_center)),
                     GRAPHE_NODE_RADIUS + node_padding));
    geometry.end_direction =
        vector_normalize(vector_subtract(geometry.end, geometry.control));
    geometry.bend = bend;

    return geometry;
}

static void draw_edge_body(const Graph *graph, size_t edge_index, int active,
                           int active_examine, const Theme *theme,
                           const RenderOptions *options) {
    EdgeGeometry geometry = make_edge_geometry(graph, edge_index, active, options);
    Color color =
        active_examine
            ? theme->active
            : edge_render_color(theme, graph->edges[edge_index].type, options);
    Vector2 end = graph->directed ? edge_body_arrow_endpoint(geometry.end,
                                                             geometry.end_direction)
                                  : geometry.end;
    Vector2 glow_end = graph->directed ? edge_glow_arrow_endpoint(
                                             geometry.end, geometry.end_direction)
                                       : end;

    if (active_examine) draw_edge_glow(geometry, glow_end, theme->active);
    draw_edge_path(geometry, end, 2.5f, color);
}

static void draw_edge_arrow(const Graph *graph, size_t edge_index,
                            int active_examine, const Theme *theme,
                            const RenderOptions *options) {
    EdgeGeometry geometry =
        make_edge_geometry(graph, edge_index, active_examine, options);
    Color color =
        active_examine
            ? theme->active
            : edge_render_color(theme, graph->edges[edge_index].type, options);

    if (active_examine)
        draw_arrowhead_glow(geometry.end, geometry.end_direction, theme->active);
    draw_arrowhead(geometry.end, geometry.end_direction, color);
}

static void draw_time_label(const RenderResources *resources, const Node *node,
                            Color color) {
    char times[64];
    char discover[16];
    char finish[16];

    if (node->discover_time >= 0) {
        snprintf(discover, sizeof(discover), "%d", node->discover_time);
    } else {
        snprintf(discover, sizeof(discover), "-");
    }

    if (node->finish_time >= 0) {
        snprintf(finish, sizeof(finish), "%d", node->finish_time);
    } else {
        snprintf(finish, sizeof(finish), "-");
    }

    snprintf(times, sizeof(times), "%s %s", discover, finish);
    Vector2 size = measure_text(resources, times, 15.0f);
    draw_text(resources, times, (Vector2){node->x - size.x * 0.5f, node->y + 7.0f},
              15.0f, color);
}

static void draw_level_label(const RenderResources *resources, const Node *node,
                             Color color) {
    if (node->level < 0) return;

    char level[16];
    snprintf(level, sizeof(level), "L%d", node->level);
    Vector2 size = measure_text(resources, level, 15.0f);
    draw_text(resources, level, (Vector2){node->x - size.x * 0.5f, node->y + 7.0f},
              15.0f, color);
}

static void draw_node_glow(Vector2 center, Color color) {
    const float inner_radii[] = {
        GRAPHE_NODE_RADIUS + 1.0f,
        GRAPHE_NODE_RADIUS,
        GRAPHE_NODE_RADIUS,
        GRAPHE_NODE_RADIUS,
    };
    const float outer_radii[] = {
        GRAPHE_NODE_RADIUS + 9.0f,
        GRAPHE_NODE_RADIUS + 6.0f,
        GRAPHE_NODE_RADIUS + 4.0f,
        GRAPHE_NODE_RADIUS + 2.0f,
    };
    const unsigned char alphas[] = {18, 25, 37, 52};

    for (int i = 0; i < 4; i++) {
        Color glow = color;
        glow.a = alphas[i];
        DrawRing(center, inner_radii[i], outer_radii[i], 0, 360, 64, glow);
    }
}

static void draw_node(const RenderResources *resources, const Node *node, int active,
                      const Theme *theme, const RenderOptions *options,
                      int max_level) {
    Vector2 center = {node->x, node->y};
    bool show_times = options->algorithm_mode == ALGORITHM_DFS;
    bool show_level = options->algorithm_mode == ALGORITHM_BFS;
    Color fill = show_level ? bfs_level_fill_color(theme, node, max_level)
                            : node_fill_color(theme, node->color);
    Color outline = theme->node_outline;
    Color label_color = show_level                  ? node_text_color(fill)
                        : node->color == NODE_BLACK ? RAYWHITE
                                                    : (Color){0, 0, 0, 255};
    Color time_color = label_color;
    Vector2 label_size = measure_text(resources, node->label, 24.0f);

    time_color.a = 220;

    if (active) draw_node_glow(center, theme->active);
    DrawCircleV(center, GRAPHE_NODE_RADIUS, fill);
    DrawRing(center, GRAPHE_NODE_RADIUS - 2.5f, GRAPHE_NODE_RADIUS, 0, 360, 64,
             outline);

    float label_y = show_times || show_level ? node->y - label_size.y + 3.0f
                                             : node->y - label_size.y * 0.5f;

    draw_text(resources, node->label,
              (Vector2){node->x - label_size.x * 0.5f, label_y}, 24.0f, label_color);
    if (show_times) draw_time_label(resources, node, time_color);
    if (show_level) draw_level_label(resources, node, time_color);
}

static const char *event_type_name(TraceEventType type) {
    switch (type) {
    case TRACE_EVENT_DISCOVER_NODE:
        return "discover node";
    case TRACE_EVENT_EXAMINE_EDGE:
        return "examine edge";
    case TRACE_EVENT_CLASSIFY_EDGE:
        return "classify edge";
    case TRACE_EVENT_FINISH_NODE:
        return "finish node";
    default:
        return "unknown";
    }
}

static const char *layout_mode_name(LayoutMode mode) {
    switch (mode) {
    case LAYOUT_TRACE_FOREST:
        return "Traversal forest";
    case LAYOUT_CIRCULAR:
        return "Circular";
    case LAYOUT_MANUAL:
        return "Manual";
    default:
        return "Unknown";
    }
}

static void build_tree_output(const Graph *graph, const Trace *trace,
                              size_t active_index, char *buffer,
                              size_t buffer_size) {
    if (buffer_size == 0) return;

    buffer[0] = '\0';
    size_t used = 0;

    for (size_t i = 0; i <= active_index && i < trace->count; i++) {
        const TraceEvent *event = &trace->events[i];
        if (event->type != TRACE_EVENT_DISCOVER_NODE) continue;

        const char *label = graph->nodes[event->node].label;
        int written = snprintf(buffer + used, buffer_size - used, "%s%s",
                               used == 0 ? "" : " ", label);
        if (written < 0) return;
        if ((size_t)written >= buffer_size - used) {
            buffer[buffer_size - 1] = '\0';
            return;
        }
        used += (size_t)written;
    }
}

static void describe_event(const Graph *graph, const Trace *trace,
                           size_t active_index, const RenderOptions *options,
                           char *buffer, size_t buffer_size) {
    if (active_index >= trace->count) {
        snprintf(buffer, buffer_size, "No event selected");
        return;
    }

    const TraceEvent *event = &trace->events[active_index];

    if (options->algorithm_mode == ALGORITHM_TREE) {
        char output[96];
        build_tree_output(graph, trace, active_index, output, sizeof(output));

        if (event->type == TRACE_EVENT_DISCOVER_NODE) {
            // snprintf(buffer, buffer_size, "visit %s   %s: %s",
            snprintf(buffer, buffer_size, "%s: %s",
                     // graph->nodes[event->node].label,
                     tree_traversal_order_name(options->tree_order), output);
        } else {
            // snprintf(buffer, buffer_size, "traverse %s -> %s   %s: %s",
            snprintf(
                buffer, buffer_size, "%s: %s",
                // graph->nodes[event->from].label, graph->nodes[event->to].label,
                tree_traversal_order_name(options->tree_order), output);
        }
        return;
    }

    if (options->algorithm_mode == ALGORITHM_BFS) {
        switch (event->type) {
        case TRACE_EVENT_DISCOVER_NODE:
            snprintf(buffer, buffer_size, "discover %s at level %d",
                     graph->nodes[event->node].label, event->time);
            break;
        case TRACE_EVENT_FINISH_NODE: {
            int level = graph->nodes[event->node].level;
            if (level >= 0) {
                snprintf(buffer, buffer_size, "finish %s at level %d",
                         graph->nodes[event->node].label, level);
            } else {
                snprintf(buffer, buffer_size, "finish %s",
                         graph->nodes[event->node].label);
            }
            break;
        }
        case TRACE_EVENT_EXAMINE_EDGE: {
            int level = graph->nodes[event->from].level;
            snprintf(buffer, buffer_size, "scan %s %s %s from level %d",
                     graph->nodes[event->from].label, graph->directed ? "->" : "-",
                     graph->nodes[event->to].label, level);
            break;
        }
        case TRACE_EVENT_CLASSIFY_EDGE:
            if (event->edge_type == EDGE_TREE) {
                snprintf(buffer, buffer_size, "enqueue %s at level %d",
                         graph->nodes[event->to].label, event->time);
            } else {
                snprintf(buffer, buffer_size, "%s already reached at level %d",
                         graph->nodes[event->to].label, event->time);
            }
            break;
        default:
            snprintf(buffer, buffer_size, "Unknown BFS event");
            break;
        }
        return;
    }

    switch (event->type) {
    case TRACE_EVENT_DISCOVER_NODE:
    case TRACE_EVENT_FINISH_NODE:
        snprintf(buffer, buffer_size, "%s %s at t=%d", event_type_name(event->type),
                 graph->nodes[event->node].label, event->time);
        break;
    case TRACE_EVENT_EXAMINE_EDGE: {
        snprintf(buffer, buffer_size, "examine %s %s %s",
                 graph->nodes[event->from].label, graph->directed ? "->" : "-",
                 graph->nodes[event->to].label);
        break;
    }
    case TRACE_EVENT_CLASSIFY_EDGE: {
        snprintf(buffer, buffer_size, "%s %s %s is %s",
                 graph->nodes[event->from].label, graph->directed ? "->" : "-",
                 graph->nodes[event->to].label, edge_type_name(event->edge_type));
        break;
    }
    default:
        snprintf(buffer, buffer_size, "Unknown event");
        break;
    }
}

static int gui_color(Color color) {
    return ((int)color.r << 24) | ((int)color.g << 16) | ((int)color.b << 8) |
           (int)color.a;
}

static Color lighten_color(Color color, float amount) {
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;

    color.r = (unsigned char)((float)color.r + (255.0f - (float)color.r) * amount);
    color.g = (unsigned char)((float)color.g + (255.0f - (float)color.g) * amount);
    color.b = (unsigned char)((float)color.b + (255.0f - (float)color.b) * amount);
    return color;
}

static Color with_alpha(Color color, unsigned char alpha) {
    color.a = alpha;
    return color;
}

static void apply_gui_style(const RenderResources *resources,
                            const RenderOptions *options, const Theme *theme) {
    int text_size = (int)(ui_size(options, 15.0f) + 0.5f);
    int padding = (int)(ui_size(options, 14.0f) + 0.5f);
    Color button_hover = lighten_color(theme->button, 0.12f);
    Color button_active_hover = lighten_color(theme->button_active, 0.10f);

    GuiSetFont(resources->font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, text_size);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
    GuiSetStyle(DEFAULT, TEXT_LINE_SPACING, (int)(ui_size(options, 20.0f) + 0.5f));
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT_VERTICAL, TEXT_ALIGN_MIDDLE);
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, gui_color(theme->panel_border));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, gui_color(theme->button));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, gui_color(theme->text));
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, gui_color(theme->panel_border));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, gui_color(button_hover));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, gui_color(theme->text));
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, gui_color(theme->panel_border));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, gui_color(button_active_hover));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, gui_color(theme->text));
    GuiSetStyle(DEFAULT, LINE_COLOR, gui_color(theme->panel_border));
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, gui_color(theme->panel));
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(TOGGLE, TEXT_PADDING, padding);
    GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(TEXTBOX, TEXT_PADDING, padding);
}

static bool rounded_button(const RenderResources *resources,
                           const RenderOptions *options, const Theme *theme,
                           Rectangle bounds, const char *text, bool active) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    Color base = active ? theme->button_active : theme->button;
    Color fill = hovered ? lighten_color(base, 0.10f) : base;
    Color border =
        active ? lighten_color(theme->button_active, 0.18f) : theme->panel_border;
    float text_size = ui_size(options, 16.0f);
    Vector2 text_size_px = measure_text(resources, text, text_size);

    DrawRectangleRounded(bounds, 0.22f, 8, fill);
    DrawRectangleRoundedLines(bounds, 0.22f, 8, border);
    draw_text(resources, text,
              (Vector2){bounds.x + (bounds.width - text_size_px.x) * 0.5f,
                        bounds.y + (bounds.height - text_size_px.y) * 0.5f -
                            ui_size(options, 1.0f)},
              text_size, theme->text);

    return hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

static bool rounded_choice_button(const RenderResources *resources,
                                  const RenderOptions *options, const Theme *theme,
                                  Rectangle bounds, const char *text, bool active) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    Color base = active ? theme->button_active : theme->button;
    Color fill = hovered ? lighten_color(base, 0.10f) : base;
    Color border =
        active ? lighten_color(theme->button_active, 0.16f) : theme->panel_border;

    DrawRectangleRounded(bounds, 0.14f, 8, fill);
    DrawRectangleRoundedLines(bounds, 0.14f, 8, border);
    draw_text(resources, text,
              (Vector2){bounds.x + ui_size(options, 16.0f),
                        bounds.y + ui_size(options, 8.0f)},
              ui_size(options, 16.0f), theme->text);

    return hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

static Rectangle sidebar_rect(const RenderOptions *options, float x, float y,
                              float width, float height) {
    return (Rectangle){sidebar_left(options) + ui_size(options, x),
                       ui_size(options, y), ui_size(options, width),
                       ui_size(options, height)};
}

static Rectangle settings_button_rect(const RenderOptions *options) {
    float sidebar_width = render_sidebar_width(options) / ui_pixel_scale(options);
    return sidebar_rect(options, sidebar_width - 130.0f, 28.0f, 102.0f, 38.0f);
}

RenderUiResult render_update_options(RenderOptions *options) {
    RenderUiResult result = {0};

    if (options->graph_path_editing) return result;

    if (IsKeyPressed(KEY_TAB)) options->settings_open = !options->settings_open;
    if (IsKeyPressed(KEY_T)) options->dark_mode = !options->dark_mode;

    if (IsKeyPressed(KEY_A)) {
        options->alphabetical_order = !options->alphabetical_order;
        result.trace_changed = true;
    }

    if (IsKeyPressed(KEY_L)) {
        options->layout_mode = (options->layout_mode + 1) % 3;
        result.layout_changed = true;
    }

    if (IsKeyPressed(KEY_M)) {
        options->algorithm_mode = (options->algorithm_mode + 1) % 3;
        result.trace_changed = true;
    }

    if (IsKeyPressed(KEY_O)) {
        options->tree_order = (options->tree_order + 1) % 3;
        if (options->algorithm_mode == ALGORITHM_TREE) result.trace_changed = true;
    }

    return result;
}

static void merge_ui_result(RenderUiResult *result, RenderUiResult update) {
    result->consumed_click = result->consumed_click || update.consumed_click;
    result->layout_changed = result->layout_changed || update.layout_changed;
    result->trace_changed = result->trace_changed || update.trace_changed;
    result->graph_load_requested =
        result->graph_load_requested || update.graph_load_requested;
}

static void draw_settings_section(const RenderResources *resources,
                                  const RenderOptions *options, const Theme *theme,
                                  Rectangle bounds, const char *title) {
    DrawRectangleRounded(bounds, 0.04f, 8, theme->panel);
    DrawRectangleRoundedLines(bounds, 0.04f, 8, theme->panel_border);
    draw_text(resources, title,
              (Vector2){bounds.x + ui_size(options, 20.0f),
                        bounds.y + ui_size(options, 17.0f)},
              ui_size(options, 21.0f), theme->text);
}

static Rectangle settings_section_row(const RenderOptions *options,
                                      Rectangle section, int row) {
    float pad = ui_size(options, 20.0f);
    float row_height = ui_size(options, 32.0f);
    float row_gap = ui_size(options, 8.0f);

    return (Rectangle){
        section.x + pad,
        section.y + ui_size(options, 52.0f) + (row_height + row_gap) * (float)row,
        section.width - pad * 2.0f, row_height};
}

static RenderUiResult draw_settings(const RenderResources *resources,
                                    RenderOptions *options, const Theme *theme) {
    RenderUiResult result = {0};
    float screen_width = (float)GetScreenWidth();
    float margin = ui_size(options, 48.0f);
    float max_content_width = ui_size(options, 1040.0f);
    float content_width = screen_width - margin * 2.0f;

    if (content_width > max_content_width) content_width = max_content_width;

    float content_x = (screen_width - content_width) * 0.5f;
    float top = ui_size(options, 96.0f);
    float column_gap = ui_size(options, 24.0f);
    float column_width = (content_width - column_gap) * 0.5f;
    Rectangle algorithms = {content_x, top, column_width, ui_size(options, 178.0f)};
    Rectangle layouts = {content_x, algorithms.y + algorithms.height + column_gap,
                         column_width, ui_size(options, 166.0f)};
    Rectangle utilities = {content_x + column_width + column_gap, top, column_width,
                           ui_size(options, 238.0f)};
    Rectangle graph_options = {utilities.x,
                               utilities.y + utilities.height + column_gap,
                               column_width, ui_size(options, 144.0f)};
    Rectangle close_button = {screen_width - margin - ui_size(options, 104.0f),
                              ui_size(options, 29.0f), ui_size(options, 104.0f),
                              ui_size(options, 38.0f)};

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), theme->background);

    draw_text(resources, "Settings", (Vector2){margin, ui_size(options, 33.0f)},
              ui_size(options, 30.0f), theme->text);
    // draw_text(resources,
    //           "Configure the visualizer without the graph competing for space",
    //           (Vector2){margin, ui_size(options, 61.0f)}, ui_size(options, 16.0f),
    //           theme->muted_text);

    if (rounded_button(resources, options, theme, close_button, "Close", false)) {
        options->settings_open = false;
        result.consumed_click = true;
    }

    // DrawLine(0, (int)ui_size(options, 84.0f), GetScreenWidth(),
    //          (int)ui_size(options, 84.0f), theme->panel_border);

    draw_settings_section(resources, options, theme, algorithms, "Algorithm");
    if (rounded_choice_button(resources, options, theme,
                              settings_section_row(options, algorithms, 0),
                              "DFS: times and edge classes",
                              options->algorithm_mode == ALGORITHM_DFS) &&
        options->algorithm_mode != ALGORITHM_DFS) {
        options->algorithm_mode = ALGORITHM_DFS;
        result.trace_changed = true;
        result.consumed_click = true;
    }
    if (rounded_choice_button(
            resources, options, theme, settings_section_row(options, algorithms, 1),
            "BFS: levels and depth", options->algorithm_mode == ALGORITHM_BFS) &&
        options->algorithm_mode != ALGORITHM_BFS) {
        options->algorithm_mode = ALGORITHM_BFS;
        result.trace_changed = true;
        result.consumed_click = true;
    }
    if (rounded_choice_button(resources, options, theme,
                              settings_section_row(options, algorithms, 2),
                              "Tree traversal: expression output",
                              options->algorithm_mode == ALGORITHM_TREE) &&
        options->algorithm_mode != ALGORITHM_TREE) {
        options->algorithm_mode = ALGORITHM_TREE;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    draw_settings_section(resources, options, theme, layouts, "Layout");
    if (rounded_choice_button(
            resources, options, theme, settings_section_row(options, layouts, 0),
            "Traversal forest", options->layout_mode == LAYOUT_TRACE_FOREST) &&
        options->layout_mode != LAYOUT_TRACE_FOREST) {
        options->layout_mode = LAYOUT_TRACE_FOREST;
        result.layout_changed = true;
        result.consumed_click = true;
    }
    if (rounded_choice_button(
            resources, options, theme, settings_section_row(options, layouts, 1),
            "Circular layout", options->layout_mode == LAYOUT_CIRCULAR) &&
        options->layout_mode != LAYOUT_CIRCULAR) {
        options->layout_mode = LAYOUT_CIRCULAR;
        result.layout_changed = true;
        result.consumed_click = true;
    }
    draw_text(resources, "Dragging a node switches to manual layout.",
              (Vector2){layouts.x + ui_size(options, 20.0f),
                        layouts.y + ui_size(options, 136.0f)},
              ui_size(options, 16.0f), theme->muted_text);

    draw_settings_section(resources, options, theme, utilities, "Utilities");
    if (rounded_choice_button(
            resources, options, theme, settings_section_row(options, utilities, 0),
            options->dark_mode ? "Dark mode: on" : "Dark mode: off",
            options->dark_mode)) {
        options->dark_mode = !options->dark_mode;
        result.consumed_click = true;
    }

    Rectangle scale_row = settings_section_row(options, utilities, 1);
    char scale_text[64];
    snprintf(scale_text, sizeof(scale_text), "UI scale: %.0f%%",
             ui_scale_value(options) * 100.0f);
    draw_text(resources, scale_text,
              (Vector2){scale_row.x + ui_size(options, 16.0f),
                        scale_row.y + ui_size(options, 8.0f)},
              ui_size(options, 16.0f), theme->text);

    Rectangle minus_button = {
        scale_row.x + scale_row.width - ui_size(options, 104.0f), scale_row.y,
        ui_size(options, 46.0f), scale_row.height};
    Rectangle plus_button = {scale_row.x + scale_row.width - ui_size(options, 46.0f),
                             scale_row.y, ui_size(options, 46.0f), scale_row.height};
    if (rounded_button(resources, options, theme, minus_button, "-", false)) {
        if (change_ui_scale(options, -0.08f)) result.layout_changed = true;
        result.consumed_click = true;
    }
    if (rounded_button(resources, options, theme, plus_button, "+", false)) {
        if (change_ui_scale(options, 0.08f)) result.layout_changed = true;
        result.consumed_click = true;
    }

    Rectangle import_label = settings_section_row(options, utilities, 2);
    draw_text(resources, "Import graph file",
              (Vector2){import_label.x, import_label.y + ui_size(options, 7.0f)},
              ui_size(options, 16.0f), theme->text);

    Rectangle import_row = settings_section_row(options, utilities, 3);
    float load_width = ui_size(options, 94.0f);
    Rectangle path_box = {import_row.x, import_row.y,
                          import_row.width - load_width - ui_size(options, 10.0f),
                          import_row.height};
    Rectangle load_button = {path_box.x + path_box.width + ui_size(options, 10.0f),
                             import_row.y, load_width, import_row.height};

    if (GuiTextBox(path_box, options->graph_path, GRAPHE_GRAPH_PATH_MAX,
                   options->graph_path_editing)) {
        options->graph_path_editing = !options->graph_path_editing;
        result.consumed_click = true;
    }
    if (options->graph_file_dirty && !options->graph_path_editing) {
        draw_text(resources, "*",
                  (Vector2){path_box.x + path_box.width - ui_size(options, 20.0f),
                            path_box.y + ui_size(options, 5.0f)},
                  ui_size(options, 22.0f), theme->active);
    }
    if (rounded_button(resources, options, theme, load_button, "Load", false)) {
        result.graph_load_requested = true;
        result.consumed_click = true;
    }

    if (options->status_message[0] != '\0') {
        draw_text(resources, options->status_message,
                  (Vector2){import_row.x, import_row.y + import_row.height +
                                              ui_size(options, 9.0f)},
                  ui_size(options, 14.0f), theme->muted_text);
    }

    draw_settings_section(resources, options, theme, graph_options, "Graph Options");
    if (rounded_choice_button(resources, options, theme,
                              settings_section_row(options, graph_options, 0),
                              options->alphabetical_order
                                  ? "Traversal order: alphabetical"
                                  : "Traversal order: insertion",
                              options->alphabetical_order)) {
        options->alphabetical_order = !options->alphabetical_order;
        result.trace_changed = true;
        result.consumed_click = true;
    }
    if (rounded_choice_button(
            resources, options, theme,
            settings_section_row(options, graph_options, 1),
            options->directed_graph ? "Graph: directed" : "Graph: undirected",
            options->directed_graph)) {
        options->directed_graph = !options->directed_graph;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    return result;
}

static void draw_graph_word_background(const RenderResources *resources,
                                       const RenderOptions *options,
                                       const Theme *theme) {
    float graph_width = render_graph_area_width(options);
    float graph_height = render_graph_area_height();
    const char *words[] = {"graphe", "graph", "dfs", "tree", "edge", "node"};
    const int word_count = 6;
    const float angle = -18.0f;
    float radians = angle * DEG2RAD;
    float cos_a = cosf(radians);
    float sin_a = sinf(radians);
    float step_x = ui_size(options, 82.0f);
    float step_y = ui_size(options, 27.0f);
    int cols = (int)((graph_width + graph_height) / step_x) + 5;
    int rows = (int)((graph_width + graph_height) / step_y) + 5;
    Color colors[] = {
        with_alpha(lighten_color(theme->background, 0.14f),
                   options->dark_mode ? 38 : 34),
        with_alpha(lighten_color(theme->panel, 0.08f), options->dark_mode ? 34 : 30),
        with_alpha(theme->edge_unclassified, options->dark_mode ? 28 : 24),
    };

    BeginScissorMode(0, 0, (int)graph_width, (int)graph_height);

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            float local_x = -graph_height * 0.55f + (float)col * step_x +
                            (float)(row % 2) * step_x * 0.5f;
            float local_y = -graph_width * 0.18f + (float)row * step_y;
            float center_x = graph_width * 0.5f;
            float center_y = graph_height * 0.5f;
            float dx = local_x - center_x;
            float dy = local_y - center_y;
            float x = center_x + dx * cos_a - dy * sin_a;
            float y = center_y + dx * sin_a + dy * cos_a;
            int word_index = (row + col * 2) % word_count;
            int color_index = (row * 3 + col * 5) % 3;
            float font_size = ui_size(options, 15.0f + (float)((row + col) % 5));

            DrawTextPro(resources->font, words[word_index], (Vector2){x, y},
                        (Vector2){0.0f, 0.0f}, angle, font_size, font_size * 0.04f,
                        colors[color_index]);
        }
    }

    EndScissorMode();
}

static void draw_event_banner(const Graph *graph, const Trace *trace,
                              size_t active_index, const RenderResources *resources,
                              const RenderOptions *options, const Theme *theme) {
    char event_text[128];
    describe_event(graph, trace, active_index, options, event_text,
                   sizeof(event_text));

    float text_size = ui_size(options, 22.0f);
    Vector2 text = measure_text(resources, event_text, text_size);
    float graph_width = render_graph_area_width(options);
    float graph_height = render_graph_area_height();
    float padding_x = ui_size(options, 22.0f);
    float padding_y = ui_size(options, 13.0f);
    float max_width = graph_width - ui_size(options, 80.0f);
    float width = text.x + padding_x * 2.0f;

    if (width > max_width) width = max_width;

    float height = text_size + padding_y * 2.0f;
    Rectangle box = {
        (graph_width - width) * 0.5f,
        graph_height - height - ui_size(options, 28.0f),
        width,
        height,
    };
    // Color fill = with_alpha(theme->panel, options->dark_mode ? 224 : 235);
    Color fill = with_alpha(theme->panel, 255);

    DrawRectangleRounded(box, 0.18f, 8, fill);
    DrawRectangleRoundedLines(box, 0.18f, 8, theme->panel_border);
    draw_text(
        resources, event_text,
        (Vector2){box.x + padding_x, box.y + padding_y - ui_size(options, 1.0f)},
        text_size, theme->text);
}

static RenderUiResult draw_sidebar(const Graph *graph, const Trace *trace,
                                   size_t applied_event_count,
                                   const RenderResources *resources,
                                   RenderOptions *options, const Theme *theme) {
    RenderUiResult result = {0};
    float x = sidebar_left(options) + ui_size(options, 28.0f);
    float y = ui_size(options, 30.0f);
    float line = ui_size(options, 28.0f);
    float sidebar_width = render_sidebar_width(options);
    char progress[64];
    char algorithm_text[64];
    char graph_text[64];
    char layout_text[64];
    char scale_text[64];

    snprintf(progress, sizeof(progress), "Step %d / %d", (int)applied_event_count,
             (int)trace->count);
    snprintf(algorithm_text, sizeof(algorithm_text), "Algorithm: %s",
             algorithm_mode_name(options->algorithm_mode));
    snprintf(graph_text, sizeof(graph_text), "Graph: %s",
             graph->directed ? "directed" : "undirected");
    snprintf(layout_text, sizeof(layout_text), "Layout: %s",
             layout_mode_name(options->layout_mode));
    snprintf(scale_text, sizeof(scale_text), "UI scale: %.0f%%",
             ui_scale_value(options) * 100.0f);

    DrawRectangle((int)sidebar_left(options), 0, (int)sidebar_width,
                  GetScreenHeight(), theme->panel);
    DrawLine((int)sidebar_left(options), 0, (int)sidebar_left(options),
             GetScreenHeight(), theme->panel_border);

    draw_text(resources, "Graphe", (Vector2){x, y}, ui_size(options, 34.0f),
              theme->text);

    if (rounded_button(resources, options, theme, settings_button_rect(options),
                       "Settings", false)) {
        options->settings_open = !options->settings_open;
        result.consumed_click = true;
    }

    draw_text(resources, algorithm_text, (Vector2){x, y + line * 1.8f},
              ui_size(options, 19.0f), theme->muted_text);
    draw_text(resources, progress, (Vector2){x, y + line * 2.65f},
              ui_size(options, 19.0f), theme->muted_text);
    draw_text(resources, graph_text, (Vector2){x, y + line * 3.55f},
              ui_size(options, 19.0f), theme->muted_text);
    if (options->algorithm_mode == ALGORITHM_TREE) {
        char order_text[64];
        snprintf(order_text, sizeof(order_text), "Tree order: %s",
                 tree_traversal_order_name(options->tree_order));
        draw_text(resources, order_text, (Vector2){x, y + line * 4.45f},
                  ui_size(options, 19.0f), theme->muted_text);
    } else if (options->algorithm_mode == ALGORITHM_BFS) {
        char level_text[80];
        snprintf(level_text, sizeof(level_text), "Reached: %d/%d, max level: %d",
                 node_count_with_level(graph), (int)graph->node_count,
                 max_bfs_level(graph));
        draw_text(resources, level_text, (Vector2){x, y + line * 4.45f},
                  ui_size(options, 19.0f), theme->muted_text);
    } else {
        draw_text(resources, layout_text, (Vector2){x, y + line * 4.45f},
                  ui_size(options, 19.0f), theme->muted_text);
    }
    draw_text(resources, scale_text, (Vector2){x, y + line * 5.35f},
              ui_size(options, 19.0f), theme->muted_text);

    draw_text(resources, "Controls", (Vector2){x, y + line * 6.85f},
              ui_size(options, 22.0f), theme->text);
    draw_text(resources, "Space: play / pause", (Vector2){x, y + line * 7.85f},
              ui_size(options, 19.0f), theme->muted_text);
    draw_text(resources, "Right / Left: step", (Vector2){x, y + line * 8.7f},
              ui_size(options, 19.0f), theme->muted_text);
    draw_text(resources, "0 / Enter: jump", (Vector2){x, y + line * 9.55f},
              ui_size(options, 19.0f), theme->muted_text);
    draw_text(resources, "R: reset layout", (Vector2){x, y + line * 10.4f},
              ui_size(options, 19.0f), theme->muted_text);
    draw_text(resources, "M: cycle algorithm", (Vector2){x, y + line * 11.25f},
              ui_size(options, 19.0f), theme->muted_text);
    draw_text(resources, "Ctrl +/-/0: UI scale", (Vector2){x, y + line * 12.1f},
              ui_size(options, 19.0f), theme->muted_text);
    draw_text(resources, "Tab: settings", (Vector2){x, y + line * 12.95f},
              ui_size(options, 19.0f), theme->muted_text);

    if (options->algorithm_mode == ALGORITHM_TREE) {
        float inner_width = sidebar_width - ui_size(options, 56.0f);
        float gap = ui_size(options, 8.0f);
        float button_width = (inner_width - gap * 2.0f) / 3.0f;
        float button_y = y + line * 16.0f;

        draw_text(resources, "Traversal Order", (Vector2){x, y + line * 15.0f},
                  ui_size(options, 22.0f), theme->text);

        Rectangle preorder = {x, button_y, button_width, ui_size(options, 34.0f)};
        Rectangle inorder = {x + button_width + gap, button_y, button_width,
                             ui_size(options, 34.0f)};
        Rectangle postorder = {x + (button_width + gap) * 2.0f, button_y,
                               button_width, ui_size(options, 34.0f)};

        if (rounded_button(resources, options, theme, preorder, "Pre",
                           options->tree_order == TREE_ORDER_PREORDER) &&
            options->tree_order != TREE_ORDER_PREORDER) {
            options->tree_order = TREE_ORDER_PREORDER;
            result.trace_changed = true;
            result.consumed_click = true;
        }
        if (rounded_button(resources, options, theme, inorder, "In",
                           options->tree_order == TREE_ORDER_INORDER) &&
            options->tree_order != TREE_ORDER_INORDER) {
            options->tree_order = TREE_ORDER_INORDER;
            result.trace_changed = true;
            result.consumed_click = true;
        }
        if (rounded_button(resources, options, theme, postorder, "Post",
                           options->tree_order == TREE_ORDER_POSTORDER) &&
            options->tree_order != TREE_ORDER_POSTORDER) {
            options->tree_order = TREE_ORDER_POSTORDER;
            result.trace_changed = true;
            result.consumed_click = true;
        }

        return result;
    }

    if (options->algorithm_mode != ALGORITHM_DFS) return result;

    draw_text(resources, "Edge Types", (Vector2){x, y + line * 15.0f},
              ui_size(options, 22.0f), theme->text);

    const EdgeType edge_types[] = {EDGE_TREE, EDGE_BACK, EDGE_FORWARD, EDGE_CROSS};
    const char *labels[] = {"Tree", "Back", "Forward", "Cross"};
    for (int i = 0; i < 4; i++) {
        float item_y = y + 1.0f + line * (16.0f + (float)i * 0.8f);
        Color color = edge_color(theme, edge_types[i]);
        // DrawCircleV(
        //     (Vector2){x + ui_size(options, 8.0f), item_y +
        //     ui_size(options, 8.0f)}, ui_size(options, 6.0f), color);
        draw_text(resources, labels[i],
                  // (Vector2){x + ui_size(options, 24.0f), item_y},
                  (Vector2){x, item_y}, ui_size(options, 19.0f), color);
    }

    return result;
}

RenderUiResult render_graph(const Graph *graph, const Trace *trace,
                            size_t applied_event_count,
                            const RenderResources *resources, RenderOptions *options,
                            const Camera2D *camera) {
    RenderUiResult result = {0};
    Theme theme = theme_for_options(options);
    size_t active_index =
        applied_event_count == 0 ? trace->count : applied_event_count - 1;
    int bfs_max_level = max_bfs_trace_level(trace);

    apply_gui_style(resources, options, &theme);

    if (options->settings_open) {
        merge_ui_result(&result, draw_settings(resources, options, &theme));
        DrawRectangle(0, 0, GetScreenWidth(), 1, theme.panel_border);
        return result;
    }

    draw_graph_word_background(resources, options, &theme);

    BeginScissorMode(0, 0, (int)render_graph_area_width(options),
                     (int)render_graph_area_height());
    BeginMode2D(*camera);

    for (size_t i = 0; i < graph->edge_count; i++) {
        if (!graph_edge_is_visible(graph, (int)i)) continue;
        draw_edge_body(graph, i, is_active_edge(graph, trace, active_index, i),
                       is_active_examine_edge(graph, trace, active_index, i), &theme,
                       options);
    }

    for (size_t i = 0; i < graph->node_count; i++)
        draw_node(resources, &graph->nodes[i],
                  is_active_node(trace, active_index, i), &theme, options,
                  bfs_max_level);

    if (graph->directed) {
        for (size_t i = 0; i < graph->edge_count; i++) {
            if (!graph_edge_is_visible(graph, (int)i)) continue;
            draw_edge_arrow(graph, i,
                            is_active_examine_edge(graph, trace, active_index, i),
                            &theme, options);
        }
    }

    EndMode2D();
    EndScissorMode();

    draw_event_banner(graph, trace, active_index, resources, options, &theme);

    merge_ui_result(&result, draw_sidebar(graph, trace, applied_event_count,
                                          resources, options, &theme));

    DrawRectangle(0, 0, GetScreenWidth(), 1, theme.panel_border);

    return result;
}
