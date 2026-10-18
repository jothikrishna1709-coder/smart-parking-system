#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "parking.h"
#include "vehicle.h"
#include "queue.h"
#include "reservation.h"
#include "billing.h"
#include "graph.h"
#include "storage.h"

typedef struct {
    int port;
    ParkingLot *lot;
    VehicleRegistry *vr;
    Queue *queue;
    ReservationManager *res_mgr;
    PricingPolicy pricing;
    Graph *nav_graph;
    bool running;
#ifdef _WIN32
    SOCKET server_socket;
#else
    int server_socket;
#endif
} ServerContext;

/**
 * Global server context pointer.
 */
extern ServerContext *g_server_ctx;

/**
 * Initializes server context and loads persistent data.
 */
ServerContext* server_init(int port);

/**
 * Starts HTTP server and listens for incoming requests.
 */
int server_run(ServerContext *ctx);

/**
 * Stops server and cleans up all resources.
 */
void server_stop(ServerContext *ctx);

#endif /* SERVER_H */
