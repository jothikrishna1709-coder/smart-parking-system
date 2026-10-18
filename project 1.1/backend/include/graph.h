#ifndef GRAPH_H
#define GRAPH_H

#include "common.h"

#define MAX_GRAPH_NODES 150
#define INF_DISTANCE    1e9

typedef enum {
    NODE_ENTRANCE = 0,
    NODE_EXIT = 1,
    NODE_JUNCTION = 2,
    NODE_RAMP = 3,
    NODE_ZONE = 4,
    NODE_SLOT = 5
} GraphNodeType;

typedef struct Edge {
    int target;                 /* Destination vertex index */
    double weight;              /* Distance in meters (non-negative) */
    char instruction[MAX_DESC_LEN]; /* Guidance: e.g. "Turn right into Zone A" */
    struct Edge *next;
} Edge;

typedef struct {
    char id[MAX_ID_LEN];        /* e.g., "ENT_NORTH", "SLOT_C-01" */
    char label[MAX_NAME_LEN];   /* Human readable name */
    int floor;                  /* Floor level: 1, 2, 3 */
    GraphNodeType type;
    Edge *edges;                /* Adjacency list head */
} Vertex;

typedef struct {
    Vertex vertices[MAX_GRAPH_NODES];
    int num_vertices;
} Graph;

typedef struct {
    int path_nodes[MAX_GRAPH_NODES];
    int path_length;
    double total_distance;      /* Total distance in meters */
    bool found;
    char instructions[MAX_GRAPH_NODES][MAX_DESC_LEN];
} NavigationPath;

/**
 * Creates an empty facility navigation graph.
 * Time Complexity: O(1)
 */
Graph* graph_create(void);

/**
 * Frees all allocated edges and the graph container.
 * Time Complexity: O(V + E)
 */
void graph_destroy(Graph *g);

/**
 * Adds a vertex to the graph. Returns vertex index, or -1 if capacity exceeded.
 * Time Complexity: O(1)
 */
int graph_add_vertex(Graph *g, const char *id, const char *label, int floor, GraphNodeType type);

/**
 * Finds vertex index by ID.
 * Time Complexity: O(V)
 */
int graph_find_vertex(const Graph *g, const char *id);

/**
 * Adds a directed weighted edge with navigation guidance instruction.
 * Time Complexity: O(1)
 */
bool graph_add_edge(Graph *g, const char *from_id, const char *to_id, double weight, const char *instruction);

/**
 * Adds an undirected (bidirectional) weighted edge.
 * Time Complexity: O(1)
 */
bool graph_add_edge_bidirectional(Graph *g, const char *u_id, const char *v_id, double weight, const char *forward_hint, const char *backward_hint);

/**
 * Breadth-First Search (BFS): Computes path with minimum hop count.
 * Time Complexity: O(V + E)
 * Space Complexity: O(V)
 */
NavigationPath graph_bfs(const Graph *g, const char *start_id, const char *end_id);

/**
 * Dijkstra's Shortest Path Algorithm: Computes path with minimum total distance.
 * Precondition: All edge weights >= 0.
 * Time Complexity: O(V^2) or O(E log V)
 * Space Complexity: O(V)
 */
NavigationPath graph_dijkstra(const Graph *g, const char *start_id, const char *end_id);

/**
 * Initializes default parking facility layout with entrances, ramps, zones, and sample slots.
 */
void graph_init_facility_layout(Graph *g);

#endif /* GRAPH_H */
