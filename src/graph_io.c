#include "graph_io.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *message,
                      int line_number) {
    if (error == NULL || error_size == 0) return;

    if (line_number > 0) {
        snprintf(error, error_size, "line %d: %s", line_number, message);
    } else {
        snprintf(error, error_size, "%s", message);
    }
}

static char *trim_left(char *text) {
    while (*text != '\0' && isspace((unsigned char)*text)) text++;
    return text;
}

static void trim_right(char *text) {
    size_t length = strlen(text);

    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[length - 1] = '\0';
        length--;
    }
}

static bool label_is_valid(const char *label) {
    if (label[0] == '\0') return false;
    if (strlen(label) >= sizeof(((Node *)0)->label)) return false;

    for (const char *cursor = label; *cursor != '\0'; cursor++) {
        if (isspace((unsigned char)*cursor)) return false;
    }

    return true;
}

static int find_or_add_node(Graph *graph, const char *label, char *error,
                            size_t error_size, int line_number) {
    if (!label_is_valid(label)) {
        set_error(error, error_size, "invalid node label", line_number);
        return -1;
    }

    int existing = graph_find_node(graph, label);
    if (existing >= 0) return existing;

    int node = graph_add_node(graph, label);
    if (node < 0)
        set_error(error, error_size, "too many nodes for current build limits",
                  line_number);

    return node;
}

bool graph_load_from_file(const char *path, Graph *graph, char *error,
                          size_t error_size) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        set_error(error, error_size, "could not open graph file", 0);
        return false;
    }

    Graph loaded;
    graph_init(&loaded);

    char line[256];
    int line_number = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line_number++;

        char *comment = strchr(line, '#');
        if (comment != NULL) *comment = '\0';

        char *content = trim_left(line);
        trim_right(content);
        if (content[0] == '\0') continue;

        char command[32] = {0};
        char first[32] = {0};
        char second[32] = {0};
        char extra[32] = {0};
        int fields =
            sscanf(content, "%31s %31s %31s %31s", command, first, second, extra);

        if (strcmp(command, "directed") == 0 || strcmp(command, "undirected") == 0) {
            if (fields != 1) {
                set_error(error, error_size,
                          "direction line must be just directed or undirected",
                          line_number);
                fclose(file);
                return false;
            }
            if (loaded.edge_count > 0) {
                set_error(error, error_size,
                          "direction must be declared before edge lines",
                          line_number);
                fclose(file);
                return false;
            }

            loaded.directed = strcmp(command, "directed") == 0;
            continue;
        }

        if (strcmp(command, "node") == 0) {
            if (fields != 2) {
                set_error(error, error_size, "node line must be: node LABEL",
                          line_number);
                fclose(file);
                return false;
            }
            if (graph_find_node(&loaded, first) >= 0) {
                set_error(error, error_size, "duplicate node label", line_number);
                fclose(file);
                return false;
            }
            if (find_or_add_node(&loaded, first, error, error_size, line_number) <
                0) {
                fclose(file);
                return false;
            }
            continue;
        }

        if (strcmp(command, "edge") == 0) {
            if (fields != 3) {
                set_error(error, error_size, "edge line must be: edge FROM TO",
                          line_number);
                fclose(file);
                return false;
            }

            int from =
                find_or_add_node(&loaded, first, error, error_size, line_number);
            int to =
                find_or_add_node(&loaded, second, error, error_size, line_number);
            if (from < 0 || to < 0) {
                fclose(file);
                return false;
            }

            if (graph_add_edge(&loaded, from, to) < 0) {
                set_error(error, error_size,
                          "too many edges for current build limits", line_number);
                fclose(file);
                return false;
            }
            continue;
        }

        set_error(error, error_size, "unknown graph file command", line_number);
        fclose(file);
        return false;
    }

    fclose(file);

    if (loaded.node_count == 0) {
        set_error(error, error_size, "graph file contains no nodes", 0);
        return false;
    }

    *graph = loaded;
    if (error != NULL && error_size > 0) error[0] = '\0';
    return true;
}
