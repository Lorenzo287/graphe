#ifndef GRAPHE_DFS_H
#define GRAPHE_DFS_H

#include "graph.h"

void dfs_trace_build(const Graph *graph, DfsTrace *trace);
void dfs_trace_apply_prefix(const Graph *base, const DfsTrace *trace,
                            size_t event_count, Graph *out);

#endif
