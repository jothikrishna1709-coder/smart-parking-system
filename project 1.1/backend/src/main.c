#include "../include/server.h"
#include <signal.h>

static void handle_sigint(int sig) {
    (void)sig;
    printf("\nReceived interrupt signal (SIGINT). Shutting down server gracefully...\n");
    if (g_server_ctx) {
        /* Save all state before exit */
        storage_save_all(
            g_server_ctx->lot,
            g_server_ctx->vr,
            g_server_ctx->queue,
            g_server_ctx->res_mgr,
            &g_server_ctx->pricing
        );
        printf("All application data persisted to storage.\n");
        server_stop(g_server_ctx);
    }
    exit(0);
}

#ifdef _WIN32
__declspec(dllexport) int start_c_server(int port);
#endif

int start_c_server(int port) {
    signal(SIGINT, handle_sigint);
#ifdef SIGBREAK
    signal(SIGBREAK, handle_sigint);
#endif

    printf("Initializing Smart Parking Management System...\n");
    ServerContext *ctx = server_init(port);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize server context.\n");
        return 1;
    }

    printf("Backend initialized successfully.\n");
    int res = server_run(ctx);
    server_stop(ctx);
    return res;
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) && i + 1 < argc) {
            port = atoi(argv[++i]);
        }
    }

    return start_c_server(port);
}
