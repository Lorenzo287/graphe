#ifndef GRAPHE_DFS_H
#define GRAPHE_DFS_H

#include "graph.h"

typedef struct DfsOptions {
    int alphabetical;
} DfsOptions;

typedef struct TraversalOptions {
    AlgorithmMode algorithm;
    int alphabetical;
    TreeTraversalOrder tree_order;
} TraversalOptions;

void traversal_trace_build(const Graph *graph, const TraversalOptions *options,
                           Trace *trace);
void traversal_trace_apply_prefix(const Graph *base, const Trace *trace,
                                  size_t event_count, Graph *out);

void dfs_trace_build(const Graph *graph, Trace *trace);
void dfs_trace_build_with_options(const Graph *graph, const DfsOptions *options,
                                  Trace *trace);
void dfs_trace_apply_prefix(const Graph *base, const Trace *trace,
                            size_t event_count, Graph *out);

#endif
