#include "graph.h"
#include "graph_io.h"
#include "traversal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *message) {
    fprintf(stderr, "%s\n", message);
    return 1;
}

#define CHECK(condition, message)               \
    do {                                        \
        if (!(condition)) return fail(message); \
    } while (0)

static int test_alphabetical_edge_order(void) {
    Graph graph;
    graph_init(&graph);

    int a = graph_add_node(&graph, "A");
    int b = graph_add_node(&graph, "B");
    int c = graph_add_node(&graph, "C");

    int edge_to_c = graph_add_edge(&graph, a, c);
    int edge_to_b = graph_add_edge(&graph, a, b);

    CHECK(graph.first_alpha_out[a] == edge_to_b,
          "alphabetical adjacency should start with B");
    CHECK(graph_next_adjacent_edge(&graph, edge_to_b, a, true) == edge_to_c,
          "alphabetical adjacency should put C after B");

    graph_free(&graph);
    return 0;
}

static int test_undirected_view_collapses_reciprocal_edges(void) {
    Graph source;
    graph_init(&source);

    int a = graph_add_node(&source, "A");
    int b = graph_add_node(&source, "B");
    int c = graph_add_node(&source, "C");
    source.nodes[a].x = 42.0f;
    source.nodes[a].y = 24.0f;

    graph_add_edge(&source, a, b);
    graph_add_edge(&source, b, a);
    graph_add_edge(&source, a, c);

    Graph undirected;
    graph_init(&undirected);
    CHECK(graph_build_view(&source, false, &undirected),
          "undirected view should build");
    CHECK(!undirected.directed, "undirected view should be marked undirected");
    CHECK(undirected.edge_count == 2,
          "undirected view should collapse reciprocal directed edges");
    CHECK(source.edge_count == 3, "source graph should remain unchanged");
    CHECK(undirected.nodes[a].x == 42.0f && undirected.nodes[a].y == 24.0f,
          "graph view should preserve node positions");

    Graph directed;
    graph_init(&directed);
    CHECK(graph_build_view(&source, true, &directed), "directed view should build");
    CHECK(directed.directed, "directed view should be marked directed");
    CHECK(directed.edge_count == 3,
          "directed view should preserve reciprocal directed edges");

    graph_free(&directed);
    graph_free(&undirected);
    graph_free(&source);
    return 0;
}

static int test_undirected_bfs_classifies_each_edge_once(void) {
    Graph graph;
    graph_init(&graph);
    graph.directed = false;

    int a = graph_add_node(&graph, "A");
    int b = graph_add_node(&graph, "B");
    int c = graph_add_node(&graph, "C");

    graph_add_edge(&graph, a, b);
    graph_add_edge(&graph, b, c);
    graph_add_edge(&graph, a, c);

    TraversalOptions options = {
        .algorithm = ALGORITHM_BFS,
        .alphabetical = 0,
        .tree_order = TREE_ORDER_INORDER,
    };
    Trace trace;
    traversal_trace_init(&trace);
    traversal_trace_build(&graph, &options, &trace);

    int *classifications = calloc(graph.edge_count, sizeof(*classifications));
    CHECK(classifications != NULL, "classification scratch allocation failed");
    for (size_t i = 0; i < trace.count; i++) {
        const TraceEvent *event = &trace.events[i];
        if (event->type != TRACE_EVENT_CLASSIFY_EDGE) continue;
        if (event->edge < 0 || event->edge >= (int)graph.edge_count) {
            free(classifications);
            traversal_trace_free(&trace);
            graph_free(&graph);
            return fail("BFS classification event should reference a valid edge");
        }
        classifications[event->edge]++;
    }

    for (size_t i = 0; i < graph.edge_count; i++) {
        if (classifications[i] != 1) {
            free(classifications);
            traversal_trace_free(&trace);
            graph_free(&graph);
            return fail(
                "undirected BFS should classify each physical edge exactly once");
        }
    }

    free(classifications);
    traversal_trace_free(&trace);
    graph_free(&graph);
    return 0;
}

static int test_load_tree_file_with_free_form_values(void) {
    Graph tree;
    graph_init(&tree);
    GraphFileKind kind = GRAPH_FILE_GRAPH;
    char error[160];

    CHECK(graph_load_from_file("graphs/sample_tree.graphe", &tree, error,
                               sizeof(error), &kind),
          error);
    CHECK(kind == GRAPH_FILE_TREE, "sample_tree should load as a tree file");
    CHECK(tree.directed, "tree files should be directed from parent to child");
    CHECK(tree.node_count == 7, "sample_tree should contain seven nodes");
    CHECK(tree.edge_count == 6, "sample_tree should contain six edges");
    CHECK(strcmp(tree.nodes[2].label, "total score") == 0,
          "tree node values should keep spaces");

    Trace trace;
    traversal_trace_init(&trace);
    TraversalOptions options = {
        .algorithm = ALGORITHM_TREE,
        .alphabetical = 1,
        .tree_order = TREE_ORDER_INORDER,
    };
    traversal_trace_build(&tree, &options, &trace);
    CHECK(trace.count > 0, "tree traversal should produce events");

    traversal_trace_free(&trace);
    graph_free(&tree);
    return 0;
}

static int test_load_showcase_graph_file(void) {
    Graph graph;
    graph_init(&graph);
    GraphFileKind kind = GRAPH_FILE_TREE;
    char error[160];

    CHECK(graph_load_from_file("graphs/showcase_directed.graphe", &graph, error,
                               sizeof(error), &kind),
          error);
    CHECK(kind == GRAPH_FILE_GRAPH, "showcase_directed should load as a graph file");
    CHECK(graph.directed, "showcase_directed should be directed");
    CHECK(graph.node_count == 84, "showcase_directed should contain 84 nodes");
    CHECK(graph.edge_count == 126, "showcase_directed should contain 126 edges");

    Trace trace;
    traversal_trace_init(&trace);
    TraversalOptions options = {
        .algorithm = ALGORITHM_DFS,
        .alphabetical = 1,
        .tree_order = TREE_ORDER_INORDER,
    };
    traversal_trace_build(&graph, &options, &trace);
    CHECK(trace.count > graph.edge_count,
          "showcase graph traversal should include edge and node events");

    int *depths = malloc(graph.node_count * sizeof(*depths));
    CHECK(depths != NULL, "could not allocate showcase graph test depths");
    for (size_t i = 0; i < graph.node_count; i++) depths[i] = -1;
    depths[0] = 0;

    int tree_edges = 0;
    int back_edges = 0;
    int forward_edges = 0;
    int cross_edges = 0;
    int root_tree_children = 0;
    int first_branch_tree_children = 0;
    int max_depth = 0;
    for (size_t i = 0; i < trace.count; i++) {
        if (trace.events[i].type != TRACE_EVENT_CLASSIFY_EDGE) continue;

        if (trace.events[i].edge_type == EDGE_TREE) tree_edges++;
        if (trace.events[i].edge_type == EDGE_BACK) back_edges++;
        if (trace.events[i].edge_type == EDGE_FORWARD) forward_edges++;
        if (trace.events[i].edge_type == EDGE_CROSS) cross_edges++;
        if (trace.events[i].edge_type == EDGE_TREE && trace.events[i].from == 0) {
            root_tree_children++;
        }
        if (trace.events[i].edge_type == EDGE_TREE && trace.events[i].from == 1) {
            first_branch_tree_children++;
        }
        if (trace.events[i].edge_type == EDGE_TREE && trace.events[i].from >= 0 &&
            trace.events[i].to >= 0 && depths[trace.events[i].from] >= 0) {
            depths[trace.events[i].to] = depths[trace.events[i].from] + 1;
            if (depths[trace.events[i].to] > max_depth)
                max_depth = depths[trace.events[i].to];
        }
    }
    CHECK(tree_edges == (int)graph.node_count - 1,
          "showcase graph should produce one DFS tree");
    CHECK(back_edges > 0, "showcase graph should produce back edges");
    CHECK(forward_edges > 0, "showcase graph should produce forward edges");
    CHECK(cross_edges > 0, "showcase graph should produce cross edges");
    CHECK(root_tree_children >= 4, "showcase graph DFS tree should start wide");
    CHECK(first_branch_tree_children >= 3,
          "showcase graph DFS tree should keep branching below the root");
    CHECK(max_depth >= 5, "showcase graph DFS tree should span several levels");

    free(depths);
    traversal_trace_free(&trace);
    graph_free(&graph);
    return 0;
}

int main(void) {
    if (test_alphabetical_edge_order() != 0) return 1;
    if (test_undirected_view_collapses_reciprocal_edges() != 0) return 1;
    if (test_undirected_bfs_classifies_each_edge_once() != 0) return 1;
    if (test_load_tree_file_with_free_form_values() != 0) return 1;
    if (test_load_showcase_graph_file() != 0) return 1;

    return 0;
}
