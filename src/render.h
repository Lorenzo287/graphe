#ifndef GRAPHE_RENDER_H
#define GRAPHE_RENDER_H

#include "graph.h"

#include "raylib.h"

#include <stdbool.h>

#define GRAPHE_NODE_RADIUS 30.0f
#define GRAPHE_SIDEBAR_WIDTH 360.0f
#define GRAPHE_UI_PIXEL_SCALE 1.2f
#define GRAPHE_UI_SCALE_MIN 0.70f
#define GRAPHE_UI_SCALE_DEFAULT 1.0f
#define GRAPHE_UI_SCALE_MAX 2.0f
#define GRAPHE_GRAPH_PATH_MAX 260
#define GRAPHE_STATUS_MESSAGE_MAX 160

typedef enum LayoutMode {
    LAYOUT_TRACE_FOREST,
    LAYOUT_CIRCULAR,
    LAYOUT_MANUAL
} LayoutMode;

typedef struct RenderOptions {
    bool dark_mode;
    bool settings_open;
    bool alphabetical_order;
    bool directed_graph;
    bool graph_path_editing;
    AlgorithmMode algorithm_mode;
    TreeTraversalOrder tree_order;
    LayoutMode layout_mode;
    float ui_scale;
    char graph_path[GRAPHE_GRAPH_PATH_MAX];
    char status_message[GRAPHE_STATUS_MESSAGE_MAX];
} RenderOptions;

typedef struct RenderResources {
    Font font;
    bool custom_font_loaded;
} RenderResources;

typedef struct RenderUiResult {
    bool consumed_click;
    bool layout_changed;
    bool trace_changed;
    bool graph_load_requested;
} RenderUiResult;

void render_resources_load(RenderResources *resources);
void render_resources_unload(RenderResources *resources);

RenderUiResult render_update_options(RenderOptions *options);
RenderUiResult render_graph(const Graph *graph, const Trace *trace,
                            size_t applied_event_count,
                            const RenderResources *resources, RenderOptions *options,
                            const Camera2D *camera);

Color render_background_color(const RenderOptions *options);
float render_sidebar_width(const RenderOptions *options);
float render_graph_area_width(const RenderOptions *options);
float render_graph_area_height(void);
bool render_point_in_sidebar(Vector2 point, const RenderOptions *options);
bool render_point_in_graph_canvas(Vector2 point, const RenderOptions *options);

#endif
