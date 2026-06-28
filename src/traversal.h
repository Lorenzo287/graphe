#ifndef GRAPHE_TRAVERSAL_H
#define GRAPHE_TRAVERSAL_H

#include "graph.h"

typedef struct TraversalOptions {
    AlgorithmMode algorithm;
    int alphabetical;
    TreeTraversalOrder tree_order;
} TraversalOptions;

void traversal_trace_init(Trace *trace);
void traversal_trace_free(Trace *trace);
void traversal_trace_build(const Graph *graph, const TraversalOptions *options,
                           Trace *trace);
void traversal_trace_apply_prefix(const Graph *base, const Trace *trace,
                                  size_t event_count, Graph *out);

#endif
