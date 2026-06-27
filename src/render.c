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

static Vector2 arrow_base_point(Vector2 tip, Vector2 direction) {
    return vector_subtract(tip, vector_scale(direction, GRAPHE_ARROW_LENGTH));
}

static Vector2 edge_body_arrow_endpoint(Vector2 tip, Vector2 direction) {
    return vector_subtract(
        tip,
        vector_scale(direction, GRAPHE_ARROW_LENGTH - GRAPHE_ARROW_BODY_OVERLAP));
}

static void draw_arrowhead(Vector2 tip, Vector2 direction, Color color) {
    Vector2 normal = {-direction.y, direction.x};
    Vector2 base = arrow_base_point(tip, direction);
    Vector2 left = vector_add(base, vector_scale(normal, GRAPHE_ARROW_WIDTH * 0.5f));
    Vector2 right =
        vector_subtract(base, vector_scale(normal, GRAPHE_ARROW_WIDTH * 0.5f));

    DrawTriangle(tip, right, left, color);
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
    Color color = active_examine ? theme->active
                                 : edge_color(theme, graph->edges[edge_index].type);
    float thickness = active ? 5.0f : 2.5f;
    Vector2 end = graph->directed ? edge_body_arrow_endpoint(geometry.end,
                                                             geometry.end_direction)
                                  : geometry.end;

    if (geometry.bend <= 0.0f) {
        DrawLineEx(geometry.start, end, thickness, color);
        return;
    }

    draw_quadratic_edge(geometry.start, geometry.control, end, thickness, color);
}

static void draw_edge_arrow(const Graph *graph, size_t edge_index, int active,
                            int active_examine, const Theme *theme,
                            const RenderOptions *options) {
    EdgeGeometry geometry = make_edge_geometry(graph, edge_index, active, options);
    Color color = active_examine ? theme->active
                                 : edge_color(theme, graph->edges[edge_index].type);

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

static void draw_node(const RenderResources *resources, const Node *node, int active,
                      const Theme *theme, const RenderOptions *options) {
    Vector2 center = {node->x, node->y};
    Color fill = node_fill_color(theme, node->color);
    Color outline = active ? theme->active : theme->node_outline;
    Color label_color = node->color == NODE_BLACK ? RAYWHITE : (Color){0, 0, 0, 255};
    Color time_color = label_color;
    Vector2 label_size = measure_text(resources, node->label, 24.0f);

    time_color.a = 220;

    DrawCircleV(center, GRAPHE_NODE_RADIUS, fill);
    DrawRing(center, GRAPHE_NODE_RADIUS - 2.5f, GRAPHE_NODE_RADIUS, 0, 360, 64,
             outline);
    // DrawCircleLinesV(center, GRAPHE_NODE_RADIUS, outline);
    // DrawCircleLinesV(center, GRAPHE_NODE_RADIUS - 1.0f, outline);
    if (active)
        DrawRing(center, GRAPHE_NODE_RADIUS - 2.5f + 5.0f, GRAPHE_NODE_RADIUS + 4.0f,
                 0, 360, 64, theme->active);
    // DrawCircleLinesV(center, GRAPHE_NODE_RADIUS + 5.0f, theme->active);

    bool show_times = options->algorithm_mode == ALGORITHM_DFS;
    float label_y =
        show_times ? node->y - label_size.y + 3.0f : node->y - label_size.y * 0.5f;

    draw_text(resources, node->label,
              (Vector2){node->x - label_size.x * 0.5f, label_y}, 24.0f, label_color);
    if (show_times) draw_time_label(resources, node, time_color);
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
            snprintf(buffer, buffer_size, "%s: %s",
                     // graph->nodes[event->from].label, graph->nodes[event->to].label,
                     tree_traversal_order_name(options->tree_order), output);
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

static void gui_option_toggle(const RenderOptions *options, const Theme *theme,
                              Rectangle bounds, const char *text, bool *active) {
    (void)options;

    bool was_active = active != NULL && *active;
    bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    Color off_hover = lighten_color(theme->button, 0.12f);
    Color active_hover =
        hovered ? lighten_color(theme->button_active, 0.10f) : theme->button_active;
    Color focused_color = was_active ? active_hover : off_hover;
    Color pressed_color =
        was_active ? active_hover : lighten_color(theme->button_active, 0.10f);

    GuiSetStyle(TOGGLE, BASE_COLOR_NORMAL, gui_color(theme->button));
    GuiSetStyle(TOGGLE, BASE_COLOR_FOCUSED, gui_color(focused_color));
    GuiSetStyle(TOGGLE, BASE_COLOR_PRESSED, gui_color(pressed_color));
    GuiToggle(bounds, text, active);
}

static bool gui_option_button(const RenderOptions *options, const Theme *theme,
                              Rectangle bounds, const char *text) {
    (void)options;

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, gui_color(theme->button));
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,
                gui_color(lighten_color(theme->button, 0.12f)));
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,
                gui_color(lighten_color(theme->button_active, 0.10f)));
    return GuiButton(bounds, text);
}

static bool option_clicked(Rectangle bounds) {
    return IsMouseButtonReleased(MOUSE_LEFT_BUTTON) &&
           CheckCollisionPointRec(GetMousePosition(), bounds);
}

static Rectangle sidebar_rect(const RenderOptions *options, float x, float y,
                              float width, float height) {
    return (Rectangle){sidebar_left(options) + ui_size(options, x),
                       ui_size(options, y), ui_size(options, width),
                       ui_size(options, height)};
}

static Rectangle settings_button_rect(const RenderOptions *options) {
    return sidebar_rect(options, GRAPHE_SIDEBAR_WIDTH - 96.0f, 28.0f, 68.0f, 36.0f);
}

static Rectangle settings_panel_rect(const RenderOptions *options) {
    float graph_width = render_graph_area_width(options);
    float margin = ui_size(options, 34.0f);
    float width = ui_size(options, 520.0f);
    float max_width = graph_width - margin * 2.0f;
    float min_width = ui_size(options, 340.0f);

    if (max_width < min_width) min_width = max_width;
    if (width > max_width) width = max_width;
    if (width < min_width) width = min_width;

    float height = ui_size(options, 620.0f);
    float max_height = (float)GetScreenHeight() - ui_size(options, 48.0f);
    if (height > max_height) height = max_height;

    return (Rectangle){(graph_width - width) * 0.5f, ui_size(options, 24.0f), width,
                       height};
}

static Rectangle option_rect(const RenderOptions *options, int index) {
    Rectangle panel = settings_panel_rect(options);
    return (Rectangle){panel.x + ui_size(options, 28.0f),
                       panel.y + ui_size(options, 82.0f + (float)index * 31.0f),
                       panel.width - ui_size(options, 56.0f),
                       ui_size(options, 29.0f)};
}

static Rectangle scale_button_rect(const RenderOptions *options, int index) {
    Rectangle row = option_rect(options, 12);
    float width = ui_size(options, 48.0f);
    float gap = ui_size(options, 8.0f);
    float x =
        row.x + row.width - width * (float)(2 - index) - gap * (float)(1 - index);

    return (Rectangle){x, row.y, width, row.height};
}

static Rectangle graph_path_textbox_rect(const RenderOptions *options) {
    Rectangle row = option_rect(options, 14);
    float button_width = ui_size(options, 86.0f);
    float gap = ui_size(options, 10.0f);

    return (Rectangle){row.x, row.y, row.width - button_width - gap, row.height};
}

static Rectangle graph_load_button_rect(const RenderOptions *options) {
    Rectangle row = option_rect(options, 14);
    float button_width = ui_size(options, 86.0f);

    return (Rectangle){row.x + row.width - button_width, row.y, button_width,
                       row.height};
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

static RenderUiResult draw_settings(const RenderResources *resources,
                                    RenderOptions *options, const Theme *theme) {
    RenderUiResult result = {0};
    Rectangle panel = settings_panel_rect(options);
    Vector2 mouse = GetMousePosition();

    DrawRectangle(0, 0, (int)render_graph_area_width(options),
                  (int)render_graph_area_height(), theme->overlay);
    GuiPanel(panel, NULL);

    draw_text(resources, "Settings",
              (Vector2){panel.x + ui_size(options, 28.0f),
                        panel.y + ui_size(options, 24.0f)},
              ui_size(options, 27.0f), theme->text);
    draw_text(resources, "Appearance and traversal options",
              (Vector2){panel.x + ui_size(options, 28.0f),
                        panel.y + ui_size(options, 56.0f)},
              ui_size(options, 16.0f), theme->muted_text);

    bool dark = options->dark_mode;
    gui_option_toggle(options, theme, option_rect(options, 0),
                      options->dark_mode ? "Dark mode: on" : "Dark mode: off",
                      &dark);
    if (dark != options->dark_mode) {
        options->dark_mode = dark;
        result.consumed_click = true;
    }

    bool alphabetical = options->alphabetical_order;
    gui_option_toggle(options, theme, option_rect(options, 1),
                      options->alphabetical_order ? "Traversal order: alphabetical"
                                                  : "Traversal order: insertion",
                      &alphabetical);
    if (alphabetical != options->alphabetical_order) {
        options->alphabetical_order = alphabetical;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    bool directed = options->directed_graph;
    gui_option_toggle(
        options, theme, option_rect(options, 2),
        options->directed_graph ? "Graph: directed" : "Graph: undirected",
        &directed);
    if (directed != options->directed_graph) {
        options->directed_graph = directed;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    bool dfs = options->algorithm_mode == ALGORITHM_DFS;
    gui_option_toggle(options, theme, option_rect(options, 3), "Algorithm: DFS",
                      &dfs);
    if (dfs && options->algorithm_mode != ALGORITHM_DFS) {
        options->algorithm_mode = ALGORITHM_DFS;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    bool bfs = options->algorithm_mode == ALGORITHM_BFS;
    gui_option_toggle(options, theme, option_rect(options, 4), "Algorithm: BFS",
                      &bfs);
    if (bfs && options->algorithm_mode != ALGORITHM_BFS) {
        options->algorithm_mode = ALGORITHM_BFS;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    bool tree = options->algorithm_mode == ALGORITHM_TREE;
    gui_option_toggle(options, theme, option_rect(options, 5),
                      "Algorithm: tree traversal", &tree);
    if (tree && options->algorithm_mode != ALGORITHM_TREE) {
        options->algorithm_mode = ALGORITHM_TREE;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    Rectangle preorder_rect = option_rect(options, 6);
    bool preorder = options->tree_order == TREE_ORDER_PREORDER;
    gui_option_toggle(options, theme, preorder_rect, "Tree: preorder", &preorder);
    if (option_clicked(preorder_rect)) {
        options->tree_order = TREE_ORDER_PREORDER;
        options->algorithm_mode = ALGORITHM_TREE;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    Rectangle inorder_rect = option_rect(options, 7);
    bool inorder = options->tree_order == TREE_ORDER_INORDER;
    gui_option_toggle(options, theme, inorder_rect, "Tree: inorder", &inorder);
    if (option_clicked(inorder_rect)) {
        options->tree_order = TREE_ORDER_INORDER;
        options->algorithm_mode = ALGORITHM_TREE;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    Rectangle postorder_rect = option_rect(options, 8);
    bool postorder = options->tree_order == TREE_ORDER_POSTORDER;
    gui_option_toggle(options, theme, postorder_rect, "Tree: postorder", &postorder);
    if (option_clicked(postorder_rect)) {
        options->tree_order = TREE_ORDER_POSTORDER;
        options->algorithm_mode = ALGORITHM_TREE;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    bool forest = options->layout_mode == LAYOUT_TRACE_FOREST;
    gui_option_toggle(options, theme, option_rect(options, 9),
                      "Layout: traversal forest", &forest);
    if (forest && options->layout_mode != LAYOUT_TRACE_FOREST) {
        options->layout_mode = LAYOUT_TRACE_FOREST;
        result.layout_changed = true;
        result.consumed_click = true;
    }

    bool circular = options->layout_mode == LAYOUT_CIRCULAR;
    gui_option_toggle(options, theme, option_rect(options, 10), "Layout: circular",
                      &circular);
    if (circular && options->layout_mode != LAYOUT_CIRCULAR) {
        options->layout_mode = LAYOUT_CIRCULAR;
        result.layout_changed = true;
        result.consumed_click = true;
    }

    bool manual = options->layout_mode == LAYOUT_MANUAL;
    gui_option_toggle(options, theme, option_rect(options, 11), "Layout: manual",
                      &manual);
    if (manual && options->layout_mode != LAYOUT_MANUAL) {
        options->layout_mode = LAYOUT_MANUAL;
        result.layout_changed = true;
        result.consumed_click = true;
    }

    Rectangle scale_row = option_rect(options, 12);
    char scale_text[64];
    snprintf(scale_text, sizeof(scale_text), "UI scale: %.0f%%",
             ui_scale_value(options) * 100.0f);
    draw_text(resources, scale_text,
              (Vector2){scale_row.x, scale_row.y + ui_size(options, 8.0f)},
              ui_size(options, 16.0f), theme->text);

    if (gui_option_button(options, theme, scale_button_rect(options, 0), "-")) {
        if (change_ui_scale(options, -0.08f)) result.layout_changed = true;
        result.consumed_click = true;
    }
    if (gui_option_button(options, theme, scale_button_rect(options, 1), "+")) {
        if (change_ui_scale(options, 0.08f)) result.layout_changed = true;
        result.consumed_click = true;
    }

    Rectangle import_label = option_rect(options, 13);
    draw_text(resources, "Import graph file",
              (Vector2){import_label.x, import_label.y + ui_size(options, 8.0f)},
              ui_size(options, 16.0f), theme->text);

    Rectangle path_box = graph_path_textbox_rect(options);
    if (GuiTextBox(path_box, options->graph_path, GRAPHE_GRAPH_PATH_MAX,
                   options->graph_path_editing)) {
        options->graph_path_editing = !options->graph_path_editing;
        result.consumed_click = true;
    }

    if (gui_option_button(options, theme, graph_load_button_rect(options), "Load")) {
        result.graph_load_requested = true;
        result.consumed_click = true;
    }

    if (options->status_message[0] != '\0') {
        Rectangle status_row = option_rect(options, 15);
        draw_text(resources, options->status_message,
                  (Vector2){status_row.x, status_row.y + ui_size(options, 8.0f)},
                  ui_size(options, 15.0f), theme->muted_text);
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        render_point_in_graph_canvas(mouse, options)) {
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

    draw_text(resources, "Graphe", (Vector2){x, y}, ui_size(options, 30.0f),
              theme->text);

    if (gui_option_button(options, theme, settings_button_rect(options),
                          options->settings_open ? "Close" : "Menu")) {
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

    if (options->algorithm_mode == ALGORITHM_TREE) return result;

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

    apply_gui_style(resources, options, &theme);

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
                  is_active_node(trace, active_index, i), &theme, options);

    if (graph->directed) {
        for (size_t i = 0; i < graph->edge_count; i++) {
            if (!graph_edge_is_visible(graph, (int)i)) continue;
            draw_edge_arrow(graph, i, is_active_edge(graph, trace, active_index, i),
                            is_active_examine_edge(graph, trace, active_index, i),
                            &theme, options);
        }
    }

    EndMode2D();
    EndScissorMode();

    draw_event_banner(graph, trace, active_index, resources, options, &theme);

    merge_ui_result(&result, draw_sidebar(graph, trace, applied_event_count,
                                          resources, options, &theme));
    if (options->settings_open)
        merge_ui_result(&result, draw_settings(resources, options, &theme));

    DrawRectangle(0, 0, GetScreenWidth(), 1, theme.panel_border);

    return result;
}
