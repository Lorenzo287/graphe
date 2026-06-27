#include "render.h"

#include "raygui.h"

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
        .muted_text = {156, 163, 175, 255},
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

static float ui_size(const RenderOptions *options, float value) {
    return value * ui_scale_value(options);
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
    float sidebar_width = GRAPHE_SIDEBAR_WIDTH * ui_scale_value(options);
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

static void draw_arrowhead(Vector2 tip, Vector2 direction, Color color) {
    const float arrow_length = 15.0f;
    const float arrow_width = 11.0f;
    Vector2 normal = {-direction.y, direction.x};
    Vector2 base = vector_subtract(tip, vector_scale(direction, arrow_length));
    Vector2 left = vector_add(base, vector_scale(normal, arrow_width * 0.5f));
    Vector2 right = vector_subtract(base, vector_scale(normal, arrow_width * 0.5f));

    DrawTriangle(tip, left, right, color);
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

static int is_active_edge(const Trace *trace, size_t active_index,
                          size_t edge_index) {
    if (active_index >= trace->count) return 0;

    return trace->events[active_index].edge == (int)edge_index;
}

static int is_active_node(const Trace *trace, size_t active_index,
                          size_t node_index) {
    if (active_index >= trace->count) return 0;

    return trace->events[active_index].node == (int)node_index;
}

static void draw_quadratic_edge(Vector2 start, Vector2 control, Vector2 end,
                                float thickness, Color color, bool directed_graph) {
    const int segments = 24;
    Vector2 previous = start;

    for (int i = 1; i <= segments; i++) {
        float t = (float)i / (float)segments;
        Vector2 current = quadratic_point(start, control, end, t);
        DrawLineEx(previous, current, thickness, color);
        previous = current;
    }

    if (directed_graph) {
        Vector2 near_end = quadratic_point(start, control, end, 0.92f);
        draw_arrowhead(end, vector_normalize(vector_subtract(end, near_end)), color);
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

static void draw_edge(const Graph *graph, size_t edge_index, int active,
                      const Theme *theme, const RenderOptions *options) {
    const Edge *edge = &graph->edges[edge_index];
    const Node *from = &graph->nodes[edge->from];
    const Node *to = &graph->nodes[edge->to];
    Vector2 start_center = {from->x, from->y};
    Vector2 end_center = {to->x, to->y};
    Vector2 delta = vector_subtract(end_center, start_center);
    float distance = vector_length(delta);
    Vector2 direction = vector_normalize(delta);
    Vector2 start =
        vector_add(start_center, vector_scale(direction, GRAPHE_NODE_RADIUS));
    Vector2 end =
        vector_subtract(end_center, vector_scale(direction, GRAPHE_NODE_RADIUS));
    Color color = active ? theme->active : edge_color(theme, edge->type);
    float thickness = active ? 5.0f : 3.0f;
    float bend = active && edge->type == EDGE_UNCLASSIFIED
                     ? 0.0f
                     : edge_bend_amount(edge->type, distance);

    if (bend <= 0.0f) {
        DrawLineEx(start, end, thickness, color);
        if (options->directed_graph) draw_arrowhead(end, direction, color);
        return;
    }

    Vector2 mid = vector_scale(vector_add(start, end), 0.5f);
    Vector2 normal = {-direction.y, direction.x};
    float sign = edge_curve_sign(from, to, edge->type, options);
    Vector2 control = vector_add(mid, vector_scale(normal, bend * sign));
    draw_quadratic_edge(start, control, end, thickness, color,
                        options->directed_graph);
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

    snprintf(times, sizeof(times), "d:%s f:%s", discover, finish);
    Vector2 size = measure_text(resources, times, 12.0f);
    draw_text(resources, times, (Vector2){node->x - size.x * 0.5f, node->y + 7.0f},
              12.0f, color);
}

static void draw_node(const RenderResources *resources, const Node *node, int active,
                      const Theme *theme) {
    Vector2 center = {node->x, node->y};
    Color fill = node_fill_color(theme, node->color);
    Color outline = active ? theme->active : theme->node_outline;
    Color label_color = node->color == NODE_BLACK ? RAYWHITE : (Color){0, 0, 0, 255};
    Color time_color = label_color;
    Vector2 label_size = measure_text(resources, node->label, 24.0f);

    time_color.a = 220;

    DrawCircleV(center, GRAPHE_NODE_RADIUS, fill);
    DrawCircleLinesV(center, GRAPHE_NODE_RADIUS, outline);
    DrawCircleLinesV(center, GRAPHE_NODE_RADIUS - 1.0f, outline);
    if (active) DrawCircleLinesV(center, GRAPHE_NODE_RADIUS + 5.0f, theme->active);

    draw_text(
        resources, node->label,
        (Vector2){node->x - label_size.x * 0.5f, node->y - label_size.y + 3.0f},
        24.0f, label_color);
    draw_time_label(resources, node, time_color);
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

static void describe_event(const Graph *graph, const Trace *trace,
                           size_t active_index, char *buffer, size_t buffer_size) {
    if (active_index >= trace->count) {
        snprintf(buffer, buffer_size, "No event selected");
        return;
    }

    const TraceEvent *event = &trace->events[active_index];
    switch (event->type) {
    case TRACE_EVENT_DISCOVER_NODE:
    case TRACE_EVENT_FINISH_NODE:
        snprintf(buffer, buffer_size, "%s %s at t=%d", event_type_name(event->type),
                 graph->nodes[event->node].label, event->time);
        break;
    case TRACE_EVENT_EXAMINE_EDGE: {
        const Edge *edge = &graph->edges[event->edge];
        snprintf(buffer, buffer_size, "examine %s -> %s",
                 graph->nodes[edge->from].label, graph->nodes[edge->to].label);
        break;
    }
    case TRACE_EVENT_CLASSIFY_EDGE: {
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

static int gui_color(Color color) {
    return ((int)color.r << 24) | ((int)color.g << 16) | ((int)color.b << 8) |
           (int)color.a;
}

static void apply_gui_style(const RenderResources *resources,
                            const RenderOptions *options, const Theme *theme) {
    int text_size = (int)(ui_size(options, 15.0f) + 0.5f);
    int padding = (int)(ui_size(options, 14.0f) + 0.5f);

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
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, gui_color(theme->button_hover));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, gui_color(theme->text));
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, gui_color(theme->panel_border));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, gui_color(theme->button_active));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, gui_color(theme->text));
    GuiSetStyle(DEFAULT, LINE_COLOR, gui_color(theme->panel_border));
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, gui_color(theme->panel));
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(TOGGLE, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(TOGGLE, TEXT_PADDING, padding);
}

static Rectangle sidebar_rect(const RenderOptions *options, float x, float y,
                              float width, float height) {
    return (Rectangle){sidebar_left(options) + ui_size(options, x),
                       ui_size(options, y), ui_size(options, width),
                       ui_size(options, height)};
}

static Rectangle settings_button_rect(const RenderOptions *options) {
    float sidebar_width = render_sidebar_width(options);
    return sidebar_rect(options, sidebar_width / ui_scale_value(options) - 96.0f,
                        28.0f, 68.0f, 36.0f);
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

    float height = ui_size(options, 520.0f);
    float max_height = (float)GetScreenHeight() - ui_size(options, 48.0f);
    if (height > max_height) height = max_height;

    return (Rectangle){(graph_width - width) * 0.5f, ui_size(options, 24.0f), width,
                       height};
}

static Rectangle option_rect(const RenderOptions *options, int index) {
    Rectangle panel = settings_panel_rect(options);
    return (Rectangle){panel.x + ui_size(options, 28.0f),
                       panel.y + ui_size(options, 82.0f + (float)index * 45.0f),
                       panel.width - ui_size(options, 56.0f),
                       ui_size(options, 36.0f)};
}

static Rectangle scale_button_rect(const RenderOptions *options, int index) {
    Rectangle row = option_rect(options, 8);
    float width = ui_size(options, 48.0f);
    float gap = ui_size(options, 8.0f);
    float x =
        row.x + row.width - width * (float)(2 - index) - gap * (float)(1 - index);

    return (Rectangle){x, row.y, width, row.height};
}

RenderUiResult render_update_options(RenderOptions *options) {
    RenderUiResult result = {0};

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

    return result;
}

static void merge_ui_result(RenderUiResult *result, RenderUiResult update) {
    result->consumed_click = result->consumed_click || update.consumed_click;
    result->layout_changed = result->layout_changed || update.layout_changed;
    result->trace_changed = result->trace_changed || update.trace_changed;
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
    GuiToggle(option_rect(options, 0),
              options->dark_mode ? "Dark mode: on" : "Dark mode: off", &dark);
    if (dark != options->dark_mode) {
        options->dark_mode = dark;
        result.consumed_click = true;
    }

    bool alphabetical = options->alphabetical_order;
    GuiToggle(option_rect(options, 1),
              options->alphabetical_order ? "Traversal order: alphabetical"
                                          : "Traversal order: insertion",
              &alphabetical);
    if (alphabetical != options->alphabetical_order) {
        options->alphabetical_order = alphabetical;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    bool dfs = options->algorithm_mode == ALGORITHM_DFS;
    GuiToggle(option_rect(options, 2), "Algorithm: DFS", &dfs);
    if (dfs && options->algorithm_mode != ALGORITHM_DFS) {
        options->algorithm_mode = ALGORITHM_DFS;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    bool bfs = options->algorithm_mode == ALGORITHM_BFS;
    GuiToggle(option_rect(options, 3), "Algorithm: BFS", &bfs);
    if (bfs && options->algorithm_mode != ALGORITHM_BFS) {
        options->algorithm_mode = ALGORITHM_BFS;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    bool tree = options->algorithm_mode == ALGORITHM_TREE;
    GuiToggle(option_rect(options, 4), "Algorithm: tree traversal", &tree);
    if (tree && options->algorithm_mode != ALGORITHM_TREE) {
        options->algorithm_mode = ALGORITHM_TREE;
        result.trace_changed = true;
        result.consumed_click = true;
    }

    bool forest = options->layout_mode == LAYOUT_TRACE_FOREST;
    GuiToggle(option_rect(options, 5), "Layout: traversal forest", &forest);
    if (forest && options->layout_mode != LAYOUT_TRACE_FOREST) {
        options->layout_mode = LAYOUT_TRACE_FOREST;
        result.layout_changed = true;
        result.consumed_click = true;
    }

    bool circular = options->layout_mode == LAYOUT_CIRCULAR;
    GuiToggle(option_rect(options, 6), "Layout: circular", &circular);
    if (circular && options->layout_mode != LAYOUT_CIRCULAR) {
        options->layout_mode = LAYOUT_CIRCULAR;
        result.layout_changed = true;
        result.consumed_click = true;
    }

    bool manual = options->layout_mode == LAYOUT_MANUAL;
    GuiToggle(option_rect(options, 7), "Layout: manual", &manual);
    if (manual && options->layout_mode != LAYOUT_MANUAL) {
        options->layout_mode = LAYOUT_MANUAL;
        result.layout_changed = true;
        result.consumed_click = true;
    }

    Rectangle scale_row = option_rect(options, 8);
    char scale_text[64];
    snprintf(scale_text, sizeof(scale_text), "UI scale: %.0f%%",
             ui_scale_value(options) * 100.0f);
    draw_text(resources, scale_text,
              (Vector2){scale_row.x, scale_row.y + ui_size(options, 8.0f)},
              ui_size(options, 16.0f), theme->text);

    if (GuiButton(scale_button_rect(options, 0), "-")) {
        if (change_ui_scale(options, -0.08f)) result.layout_changed = true;
        result.consumed_click = true;
    }
    if (GuiButton(scale_button_rect(options, 1), "+")) {
        if (change_ui_scale(options, 0.08f)) result.layout_changed = true;
        result.consumed_click = true;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        render_point_in_graph_canvas(mouse, options)) {
        result.consumed_click = true;
    }

    return result;
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
    size_t active_index =
        applied_event_count == 0 ? trace->count : applied_event_count - 1;
    char event_text[128];
    char progress[64];
    char algorithm_text[64];
    char layout_text[64];
    char scale_text[64];

    describe_event(graph, trace, active_index, event_text, sizeof(event_text));
    snprintf(progress, sizeof(progress), "Step %d / %d", (int)applied_event_count,
             (int)trace->count);
    snprintf(algorithm_text, sizeof(algorithm_text), "Algorithm: %s",
             algorithm_mode_name(options->algorithm_mode));
    snprintf(layout_text, sizeof(layout_text), "Layout: %s",
             layout_mode_name(options->layout_mode));
    snprintf(scale_text, sizeof(scale_text), "UI scale: %.0f%%",
             ui_scale_value(options) * 100.0f);

    DrawRectangle((int)sidebar_left(options), 0, (int)sidebar_width,
                  GetScreenHeight(), theme->panel);
    DrawLine((int)sidebar_left(options), 0, (int)sidebar_left(options),
             GetScreenHeight(), theme->panel_border);

    draw_text(resources, "Graphe", (Vector2){x, y}, ui_size(options, 28.0f),
              theme->text);

    if (GuiButton(settings_button_rect(options),
                  options->settings_open ? "Close" : "Menu")) {
        options->settings_open = !options->settings_open;
        result.consumed_click = true;
    }

    draw_text(resources, algorithm_text, (Vector2){x, y + line * 1.8f},
              ui_size(options, 16.0f), theme->muted_text);
    draw_text(resources, progress, (Vector2){x, y + line * 2.65f},
              ui_size(options, 18.0f), theme->muted_text);
    draw_text(resources, event_text, (Vector2){x, y + line * 3.55f},
              ui_size(options, 18.0f), theme->text);
    draw_text(resources, layout_text, (Vector2){x, y + line * 4.55f},
              ui_size(options, 16.0f), theme->muted_text);
    draw_text(resources, scale_text, (Vector2){x, y + line * 5.35f},
              ui_size(options, 16.0f), theme->muted_text);

    draw_text(resources, "Controls", (Vector2){x, y + line * 6.85f},
              ui_size(options, 21.0f), theme->text);
    draw_text(resources, "Space: play / pause", (Vector2){x, y + line * 7.85f},
              ui_size(options, 16.0f), theme->muted_text);
    draw_text(resources, "Right / Left: step", (Vector2){x, y + line * 8.7f},
              ui_size(options, 16.0f), theme->muted_text);
    draw_text(resources, "0 / Enter: jump", (Vector2){x, y + line * 9.55f},
              ui_size(options, 16.0f), theme->muted_text);
    draw_text(resources, "R: reset layout", (Vector2){x, y + line * 10.4f},
              ui_size(options, 16.0f), theme->muted_text);
    draw_text(resources, "M: cycle algorithm", (Vector2){x, y + line * 11.25f},
              ui_size(options, 16.0f), theme->muted_text);
    draw_text(resources, "Ctrl +/-: UI scale", (Vector2){x, y + line * 12.1f},
              ui_size(options, 16.0f), theme->muted_text);
    draw_text(resources, "Tab: settings", (Vector2){x, y + line * 12.95f},
              ui_size(options, 16.0f), theme->muted_text);

    draw_text(resources, "Edge Types", (Vector2){x, y + line * 15.0f},
              ui_size(options, 21.0f), theme->text);

    const EdgeType edge_types[] = {EDGE_TREE, EDGE_BACK, EDGE_FORWARD, EDGE_CROSS};
    const char *labels[] = {"tree", "back", "forward", "cross"};
    for (int i = 0; i < 4; i++) {
        float item_y = y + line * (16.1f + (float)i);
        Color color = edge_color(theme, edge_types[i]);
        DrawCircleV(
            (Vector2){x + ui_size(options, 8.0f), item_y + ui_size(options, 8.0f)},
            ui_size(options, 6.0f), color);
        draw_text(resources, labels[i],
                  (Vector2){x + ui_size(options, 24.0f), item_y},
                  ui_size(options, 16.0f), color);
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

    BeginScissorMode(0, 0, (int)render_graph_area_width(options),
                     (int)render_graph_area_height());
    BeginMode2D(*camera);

    for (size_t i = 0; i < graph->edge_count; i++)
        draw_edge(graph, i, is_active_edge(trace, active_index, i), &theme, options);

    for (size_t i = 0; i < graph->node_count; i++)
        draw_node(resources, &graph->nodes[i],
                  is_active_node(trace, active_index, i), &theme);

    EndMode2D();
    EndScissorMode();

    merge_ui_result(&result, draw_sidebar(graph, trace, applied_event_count,
                                          resources, options, &theme));
    if (options->settings_open)
        merge_ui_result(&result, draw_settings(resources, options, &theme));

    return result;
}
