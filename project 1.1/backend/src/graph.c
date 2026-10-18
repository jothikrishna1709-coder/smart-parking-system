#include "../include/graph.h"

Graph* graph_create(void) {
    Graph *g = (Graph*)malloc(sizeof(Graph));
    if (!g) return NULL;
    g->num_vertices = 0;
    for (int i = 0; i < MAX_GRAPH_NODES; i++) {
        g->vertices[i].edges = NULL;
    }
    return g;
}

void graph_destroy(Graph *g) {
    if (!g) return;
    for (int i = 0; i < g->num_vertices; i++) {
        Edge *curr = g->vertices[i].edges;
        while (curr) {
            Edge *next = curr->next;
            free(curr);
            curr = next;
        }
    }
    free(g);
}

int graph_find_vertex(const Graph *g, const char *id) {
    if (!g || !id) return -1;
    for (int i = 0; i < g->num_vertices; i++) {
        if (stricmp(g->vertices[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}

int graph_add_vertex(Graph *g, const char *id, const char *label, int floor, GraphNodeType type) {
    if (!g || !id || g->num_vertices >= MAX_GRAPH_NODES) return -1;

    int existing = graph_find_vertex(g, id);
    if (existing != -1) return existing;

    int idx = g->num_vertices++;
    strncpy(g->vertices[idx].id, id, sizeof(g->vertices[idx].id) - 1);
    g->vertices[idx].id[sizeof(g->vertices[idx].id) - 1] = '\0';

    strncpy(g->vertices[idx].label, label ? label : id, sizeof(g->vertices[idx].label) - 1);
    g->vertices[idx].label[sizeof(g->vertices[idx].label) - 1] = '\0';

    g->vertices[idx].floor = floor;
    g->vertices[idx].type = type;
    g->vertices[idx].edges = NULL;
    return idx;
}

bool graph_add_edge(Graph *g, const char *from_id, const char *to_id, double weight, const char *instruction) {
    if (!g || !from_id || !to_id || weight < 0) return false;

    int u = graph_find_vertex(g, from_id);
    int v = graph_find_vertex(g, to_id);
    if (u == -1 || v == -1) return false;

    Edge *edge = (Edge*)malloc(sizeof(Edge));
    if (!edge) return false;

    edge->target = v;
    edge->weight = weight;
    if (instruction) {
        strncpy(edge->instruction, instruction, sizeof(edge->instruction) - 1);
        edge->instruction[sizeof(edge->instruction) - 1] = '\0';
    } else {
        snprintf(edge->instruction, sizeof(edge->instruction), "Proceed to %s", g->vertices[v].label);
    }
    edge->next = g->vertices[u].edges;
    g->vertices[u].edges = edge;
    return true;
}

bool graph_add_edge_bidirectional(Graph *g, const char *u_id, const char *v_id, double weight, const char *forward_hint, const char *backward_hint) {
    bool ok1 = graph_add_edge(g, u_id, v_id, weight, forward_hint);
    bool ok2 = graph_add_edge(g, v_id, u_id, weight, backward_hint);
    return ok1 && ok2;
}

NavigationPath graph_bfs(const Graph *g, const char *start_id, const char *end_id) {
    NavigationPath path;
    memset(&path, 0, sizeof(NavigationPath));
    path.found = false;

    if (!g) return path;
    int start = graph_find_vertex(g, start_id);
    int end = graph_find_vertex(g, end_id);
    if (start == -1 || end == -1) return path;

    bool visited[MAX_GRAPH_NODES] = {false};
    int parent[MAX_GRAPH_NODES];
    char edge_instr[MAX_GRAPH_NODES][MAX_DESC_LEN];
    double edge_dist[MAX_GRAPH_NODES];

    for (int i = 0; i < MAX_GRAPH_NODES; i++) {
        parent[i] = -1;
        edge_dist[i] = 0;
    }

    int queue[MAX_GRAPH_NODES];
    int q_front = 0, q_rear = 0;

    visited[start] = true;
    queue[q_rear++] = start;

    bool reached = false;
    while (q_front < q_rear) {
        int u = queue[q_front++];
        if (u == end) {
            reached = true;
            break;
        }

        for (Edge *e = g->vertices[u].edges; e != NULL; e = e->next) {
            int v = e->target;
            if (!visited[v]) {
                visited[v] = true;
                parent[v] = u;
                strncpy(edge_instr[v], e->instruction, MAX_DESC_LEN - 1);
                edge_dist[v] = e->weight;
                queue[q_rear++] = v;
            }
        }
    }

    if (!reached) return path;

    /* Reconstruct path from end to start */
    int temp[MAX_GRAPH_NODES];
    int count = 0;
    int curr = end;
    while (curr != -1) {
        temp[count++] = curr;
        curr = parent[curr];
    }

    path.found = true;
    path.path_length = count;
    path.total_distance = 0;

    for (int i = 0; i < count; i++) {
        int node = temp[count - 1 - i];
        path.path_nodes[i] = node;
        if (i > 0) {
            path.total_distance += edge_dist[node];
            strncpy(path.instructions[i - 1], edge_instr[node], MAX_DESC_LEN - 1);
        }
    }
    return path;
}

NavigationPath graph_dijkstra(const Graph *g, const char *start_id, const char *end_id) {
    NavigationPath path;
    memset(&path, 0, sizeof(NavigationPath));
    path.found = false;

    if (!g) return path;
    int start = graph_find_vertex(g, start_id);
    int end = graph_find_vertex(g, end_id);
    if (start == -1 || end == -1) return path;

    double dist[MAX_GRAPH_NODES];
    bool visited[MAX_GRAPH_NODES];
    int parent[MAX_GRAPH_NODES];
    char edge_instr[MAX_GRAPH_NODES][MAX_DESC_LEN];

    for (int i = 0; i < g->num_vertices; i++) {
        dist[i] = INF_DISTANCE;
        visited[i] = false;
        parent[i] = -1;
    }

    dist[start] = 0.0;

    for (int step = 0; step < g->num_vertices; step++) {
        int u = -1;
        double min_dist = INF_DISTANCE;

        for (int i = 0; i < g->num_vertices; i++) {
            if (!visited[i] && dist[i] < min_dist) {
                min_dist = dist[i];
                u = i;
            }
        }

        if (u == -1 || u == end) break;
        visited[u] = true;

        for (Edge *e = g->vertices[u].edges; e != NULL; e = e->next) {
            int v = e->target;
            if (!visited[v] && dist[u] + e->weight < dist[v]) {
                dist[v] = dist[u] + e->weight;
                parent[v] = u;
                strncpy(edge_instr[v], e->instruction, MAX_DESC_LEN - 1);
            }
        }
    }

    if (dist[end] >= INF_DISTANCE) return path;

    int temp[MAX_GRAPH_NODES];
    int count = 0;
    int curr = end;
    while (curr != -1) {
        temp[count++] = curr;
        curr = parent[curr];
    }

    path.found = true;
    path.path_length = count;
    path.total_distance = dist[end];

    for (int i = 0; i < count; i++) {
        int node = temp[count - 1 - i];
        path.path_nodes[i] = node;
        if (i > 0) {
            strncpy(path.instructions[i - 1], edge_instr[node], MAX_DESC_LEN - 1);
        }
    }
    return path;
}

void graph_init_facility_layout(Graph *g) {
    if (!g) return;

    /* Entrances and Exits */
    graph_add_vertex(g, "ENT_NORTH", "North Main Entrance", 1, NODE_ENTRANCE);
    graph_add_vertex(g, "ENT_SOUTH", "South Gate Entrance", 1, NODE_ENTRANCE);
    graph_add_vertex(g, "EXIT_WEST", "West Exit Gate", 1, NODE_EXIT);

    /* Floor 1 Junctions & Zones */
    graph_add_vertex(g, "JUNC_F1_N", "F1 North Hub", 1, NODE_JUNCTION);
    graph_add_vertex(g, "JUNC_F1_S", "F1 South Hub", 1, NODE_JUNCTION);
    graph_add_vertex(g, "ZONE_A", "Zone A (Ground Cars)", 1, NODE_ZONE);
    graph_add_vertex(g, "ZONE_B", "Zone B (Ground Bikes)", 1, NODE_ZONE);
    graph_add_vertex(g, "ZONE_C", "Zone C (Accessible)", 1, NODE_ZONE);

    /* Ramps */
    graph_add_vertex(g, "RAMP_F1_F2", "Ramp to Floor 2", 1, NODE_RAMP);
    graph_add_vertex(g, "RAMP_F2_F3", "Ramp to Floor 3", 2, NODE_RAMP);

    /* Floor 2 */
    graph_add_vertex(g, "JUNC_F2", "Floor 2 Central Hub", 2, NODE_JUNCTION);
    graph_add_vertex(g, "ZONE_D", "Zone D (Floor 2 Cars)", 2, NODE_ZONE);

    /* Floor 3 */
    graph_add_vertex(g, "JUNC_F3", "Floor 3 Central Hub", 3, NODE_JUNCTION);
    graph_add_vertex(g, "ZONE_E", "Zone E (Floor 3 Cars)", 3, NODE_ZONE);

    /* Sample Slot Vertices for Ground Floor */
    graph_add_vertex(g, "C-01", "Slot C-01 (Car)", 1, NODE_SLOT);
    graph_add_vertex(g, "C-02", "Slot C-02 (Car)", 1, NODE_SLOT);
    graph_add_vertex(g, "C-03", "Slot C-03 (Car)", 1, NODE_SLOT);
    graph_add_vertex(g, "M-01", "Slot M-01 (Bike)", 1, NODE_SLOT);
    graph_add_vertex(g, "M-02", "Slot M-02 (Bike)", 1, NODE_SLOT);
    graph_add_vertex(g, "A-01", "Slot A-01 (Accessible)", 1, NODE_SLOT);

    /* Connections */
    /* From North Entrance */
    graph_add_edge_bidirectional(g, "ENT_NORTH", "JUNC_F1_N", 15.0, "Drive straight 15m into North Hub", "Exit toward North Gate");
    graph_add_edge_bidirectional(g, "JUNC_F1_N", "ZONE_C", 10.0, "Turn slight right into Accessible Zone C (10m)", "Return to North Hub");
    graph_add_edge_bidirectional(g, "JUNC_F1_N", "ZONE_A", 20.0, "Turn left into Zone A (20m)", "Return to North Hub");
    graph_add_edge_bidirectional(g, "JUNC_F1_N", "RAMP_F1_F2", 35.0, "Follow ramp signage straight ahead (35m)", "Return to Ground Floor");

    /* From South Entrance */
    graph_add_edge_bidirectional(g, "ENT_SOUTH", "JUNC_F1_S", 12.0, "Drive straight 12m into South Hub", "Exit toward South Gate");
    graph_add_edge_bidirectional(g, "JUNC_F1_S", "ZONE_B", 10.0, "Turn right into Motorcycle Zone B (10m)", "Return to South Hub");
    graph_add_edge_bidirectional(g, "JUNC_F1_S", "ZONE_A", 25.0, "Turn left toward Zone A (25m)", "Return to South Hub");
    graph_add_edge_bidirectional(g, "JUNC_F1_N", "JUNC_F1_S", 30.0, "Cross central aisle between North and South Hubs (30m)", "Return across central aisle");

    /* Upper Floors */
    graph_add_edge_bidirectional(g, "RAMP_F1_F2", "JUNC_F2", 40.0, "Drive up ramp to Floor 2 Central Hub (40m)", "Drive down ramp to Ground Floor");
    graph_add_edge_bidirectional(g, "JUNC_F2", "ZONE_D", 15.0, "Proceed into Floor 2 Zone D (15m)", "Return to F2 Hub");
    graph_add_edge_bidirectional(g, "JUNC_F2", "RAMP_F2_F3", 35.0, "Take ramp up to Floor 3 (35m)", "Take ramp down to Floor 2");
    graph_add_edge_bidirectional(g, "RAMP_F2_F3", "JUNC_F3", 40.0, "Drive up ramp to Floor 3 Central Hub (40m)", "Drive down ramp to Floor 2");
    graph_add_edge_bidirectional(g, "JUNC_F3", "ZONE_E", 15.0, "Proceed into Floor 3 Zone E (15m)", "Return to F3 Hub");

    /* Slot connections in Zone A */
    graph_add_edge_bidirectional(g, "ZONE_A", "C-01", 5.0, "Park in Slot C-01 on left", "Back out of Slot C-01");
    graph_add_edge_bidirectional(g, "ZONE_A", "C-02", 8.0, "Park in Slot C-02 on left", "Back out of Slot C-02");
    graph_add_edge_bidirectional(g, "ZONE_A", "C-03", 12.0, "Park in Slot C-03 on right", "Back out of Slot C-03");

    /* Slot connections in Zone B */
    graph_add_edge_bidirectional(g, "ZONE_B", "M-01", 4.0, "Park in Bike Slot M-01", "Back out of Bike Slot M-01");
    graph_add_edge_bidirectional(g, "ZONE_B", "M-02", 7.0, "Park in Bike Slot M-02", "Back out of Bike Slot M-02");

    /* Slot connections in Zone C */
    graph_add_edge_bidirectional(g, "ZONE_C", "A-01", 4.0, "Park in Accessible Slot A-01 right near entrance", "Back out of Slot A-01");
}
