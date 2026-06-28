#ifndef GRAPHE_GRAPH_IO_H
#define GRAPHE_GRAPH_IO_H

#include "graph.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum GraphFileKind { GRAPH_FILE_GRAPH, GRAPH_FILE_TREE } GraphFileKind;

bool graph_load_from_file(const char *path, Graph *graph, char *error,
                          size_t error_size, GraphFileKind *kind);

#endif
