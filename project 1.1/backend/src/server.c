#include "../include/server.h"
#include "../include/cJSON.h"
#include "../include/sorting.h"
#include "../include/searching.h"

ServerContext *g_server_ctx = NULL;

static void send_http_response(SOCKET sock, int status_code, const char *status_text, const char *content_type, const char *body, size_t body_len) {
    char header_buf[1024];
    int header_len = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 %d %s\r\n"
        "Server: SmartParking-C-Backend/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: *\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_text, content_type, body_len
    );

    send(sock, header_buf, header_len, 0);
    if (body && body_len > 0) {
        send(sock, body, (int)body_len, 0);
    }
}

static void send_json_response(SOCKET sock, int status_code, cJSON *json_obj) {
    char *json_str = cJSON_PrintUnformatted(json_obj);
    if (json_str) {
        send_http_response(sock, status_code, "OK", "application/json", json_str, strlen(json_str));
        free(json_str);
    } else {
        const char *err = "{\"success\":false,\"error\":\"JSON serialization failed\"}";
        send_http_response(sock, 500, "Internal Server Error", "application/json", err, strlen(err));
    }
    cJSON_Delete(json_obj);
}

static void send_error_json(SOCKET sock, int status_code, const char *message) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", false);
    cJSON_AddStringToObject(resp, "error", message ? message : "Unknown error");
    send_json_response(sock, status_code, resp);
}

/* Static file serving */
static void serve_static_file(SOCKET sock, const char *url_path) {
    char file_path[MAX_PATH_LEN];

    if (strcmp(url_path, "/") == 0 || strcmp(url_path, "/index.html") == 0) {
        snprintf(file_path, sizeof(file_path), "frontend/index.html");
    } else {
        /* Prevent directory traversal */
        if (strstr(url_path, "..") != NULL) {
            send_error_json(sock, 403, "Access denied: invalid path");
            return;
        }
        snprintf(file_path, sizeof(file_path), "frontend%s", url_path);
    }

    FILE *fp = fopen(file_path, "rb");
    if (!fp) {
        send_error_json(sock, 404, "File not found");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    const char *mime = "text/plain";
    if (strstr(file_path, ".html")) mime = "text/html; charset=utf-8";
    else if (strstr(file_path, ".css")) mime = "text/css; charset=utf-8";
    else if (strstr(file_path, ".js")) mime = "application/javascript; charset=utf-8";
    else if (strstr(file_path, ".json")) mime = "application/json";
    else if (strstr(file_path, ".svg")) mime = "image/svg+xml";
    else if (strstr(file_path, ".png")) mime = "image/png";
    else if (strstr(file_path, ".ico")) mime = "image/x-icon";

    char header_buf[512];
    int header_len = snprintf(header_buf, sizeof(header_buf),
        "HTTP/1.1 200 OK\r\n"
        "Server: SmartParking-C-Backend/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: *\r\n"
        "Connection: close\r\n\r\n",
        mime, file_size
    );
    send(sock, header_buf, header_len, 0);

    char chunk[4096];
    size_t bytes_read;
    while ((bytes_read = fread(chunk, 1, sizeof(chunk), fp)) > 0) {
        send(sock, chunk, (int)bytes_read, 0);
    }
    fclose(fp);
}

/* API Route Handlers */

static void handle_health(SOCKET sock) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON_AddStringToObject(resp, "status", "healthy");
    cJSON_AddStringToObject(resp, "backend", "C Native Server (DSA Engine)");
    cJSON_AddNumberToObject(resp, "timestamp", (double)time(NULL));
    send_json_response(sock, 200, resp);
}

static void handle_dashboard(SOCKET sock) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx) {
        send_error_json(sock, 500, "Server context unavailable");
        return;
    }

    int total = 0, avail = 0, occ = 0, res = 0, maint = 0;
    parking_lock(ctx->lot);
    parking_get_stats(ctx->lot, &total, &avail, &occ, &res, &maint);
    parking_unlock(ctx->lot);

    size_t waiting_count = queue_size(ctx->queue);
    size_t active_vehicles_count = vehicle_count_active(ctx->vr);

    /* Calculate today's revenue from history */
    double today_revenue = 0.0;
    int today_checkouts = 0;
    time_t now = time(NULL);
    struct tm now_tm;
#ifdef _WIN32
    localtime_s(&now_tm, &now);
#else
    localtime_r(&now, &now_tm);
#endif

    LinkedList *history = storage_load_history();
    size_t hist_count = history ? ll_size(history) : 0;
    cJSON *recent_activity = cJSON_CreateArray();

    int car_parked = 0, bike_parked = 0, acc_parked = 0;

    size_t active_num = 0;
    ActiveVehicle *actives = vehicle_get_active_array(ctx->vr, &active_num);
    if (actives) {
        for (size_t i = 0; i < active_num; i++) {
            if (actives[i].vehicle_type == VEHICLE_CAR) car_parked++;
            else if (actives[i].vehicle_type == VEHICLE_MOTORCYCLE) bike_parked++;
            else if (actives[i].vehicle_type == VEHICLE_ACCESSIBLE) acc_parked++;
        }
        free(actives);
    }

    if (history) {
        for (size_t i = 0; i < hist_count; i++) {
            ParkingTransaction *t = (ParkingTransaction*)ll_get_at(history, i);
            if (!t) continue;

            struct tm exit_tm;
#ifdef _WIN32
            localtime_s(&exit_tm, &t->exit_time);
#else
            localtime_r(&t->exit_time, &exit_tm);
#endif
            if (exit_tm.tm_year == now_tm.tm_year &&
                exit_tm.tm_yday == now_tm.tm_yday) {
                today_revenue += t->total_fee;
                today_checkouts++;
            }

            /* Add to recent list if in latest 5 */
            if (i >= (hist_count > 5 ? hist_count - 5 : 0)) {
                cJSON *act = cJSON_CreateObject();
                cJSON_AddStringToObject(act, "type", "EXIT");
                cJSON_AddStringToObject(act, "reg_number", t->reg_number);
                cJSON_AddStringToObject(act, "slot_id", t->slot_id);
                cJSON_AddNumberToObject(act, "fee", t->total_fee);
                cJSON_AddStringToObject(act, "time", t->exit_time_str);
                cJSON_AddItemToArray(recent_activity, act);
            }
        }
        ll_destroy(history);
    }

    double occupancy_pct = total > 0 ? ((double)(occ + res) / (double)total) * 100.0 : 0.0;

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON_AddNumberToObject(resp, "total_slots", total);
    cJSON_AddNumberToObject(resp, "available_slots", avail);
    cJSON_AddNumberToObject(resp, "occupied_slots", occ);
    cJSON_AddNumberToObject(resp, "reserved_slots", res);
    cJSON_AddNumberToObject(resp, "maintenance_slots", maint);
    cJSON_AddNumberToObject(resp, "active_vehicles", active_vehicles_count);
    cJSON_AddNumberToObject(resp, "waiting_queue_count", waiting_count);
    cJSON_AddNumberToObject(resp, "today_revenue", today_revenue);
    cJSON_AddNumberToObject(resp, "today_checkouts", today_checkouts);
    cJSON_AddNumberToObject(resp, "occupancy_percentage", occupancy_pct);

    cJSON *distribution = cJSON_CreateObject();
    cJSON_AddNumberToObject(distribution, "cars", car_parked);
    cJSON_AddNumberToObject(distribution, "motorcycles", bike_parked);
    cJSON_AddNumberToObject(distribution, "accessible", acc_parked);
    cJSON_AddItemToObject(resp, "vehicle_distribution", distribution);

    cJSON_AddItemToObject(resp, "recent_activity", recent_activity);

    send_json_response(sock, 200, resp);
}

static void handle_get_slots(SOCKET sock) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx) {
        send_error_json(sock, 500, "Server context unavailable");
        return;
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON *slots_arr = cJSON_CreateArray();

    parking_lock(ctx->lot);
    for (size_t i = 0; i < ctx->lot->total_slots; i++) {
        ParkingSlot *s = ctx->lot->slots_array[i];
        if (!s) continue;

        cJSON *slot_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(slot_obj, "slot_id", s->slot_id);
        cJSON_AddNumberToObject(slot_obj, "floor", s->floor);
        cJSON_AddStringToObject(slot_obj, "zone", s->zone);
        cJSON_AddStringToObject(slot_obj, "category", vehicle_type_to_string(s->category));
        cJSON_AddStringToObject(slot_obj, "status", slot_status_to_string(s->status));
        cJSON_AddStringToObject(slot_obj, "occupied_reg", s->occupied_reg);
        cJSON_AddNumberToObject(slot_obj, "occupied_time", (double)s->occupied_time);
        cJSON_AddStringToObject(slot_obj, "reservation_id", s->reservation_id);
        cJSON_AddNumberToObject(slot_obj, "distance", s->distance_from_entrance);

        cJSON_AddItemToArray(slots_arr, slot_obj);
    }
    parking_unlock(ctx->lot);

    cJSON_AddItemToObject(resp, "slots", slots_arr);
    send_json_response(sock, 200, resp);
}

static void handle_vehicle_entry(SOCKET sock, const char *body) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx || !body) {
        send_error_json(sock, 400, "Invalid request body");
        return;
    }

    cJSON *req = cJSON_Parse(body);
    if (!req) {
        send_error_json(sock, 400, "Malformed JSON request");
        return;
    }

    cJSON *j_reg = cJSON_GetObjectItem(req, "reg_number");
    cJSON *j_type = cJSON_GetObjectItem(req, "vehicle_type");
    cJSON *j_owner = cJSON_GetObjectItem(req, "owner_name");
    cJSON *j_contact = cJSON_GetObjectItem(req, "contact");
    cJSON *j_res = cJSON_GetObjectItem(req, "reservation_id");

    if (!j_reg || !cJSON_IsString(j_reg) || strlen(j_reg->valuestring) == 0) {
        cJSON_Delete(req);
        send_error_json(sock, 400, "Vehicle registration number is required");
        return;
    }

    if (!vehicle_validate_registration(j_reg->valuestring)) {
        cJSON_Delete(req);
        send_error_json(sock, 400, "Invalid registration number format (alphanumeric only)");
        return;
    }

    char norm_reg[MAX_REG_LEN];
    normalize_registration(j_reg->valuestring, norm_reg, sizeof(norm_reg));

    /* Check if already parked */
    if (vehicle_is_parked(ctx->vr, norm_reg)) {
        cJSON_Delete(req);
        send_error_json(sock, 409, "Vehicle with this registration number is already parked");
        return;
    }

    VehicleType type = VEHICLE_CAR;
    if (j_type && cJSON_IsString(j_type)) {
        type = string_to_vehicle_type(j_type->valuestring);
    }

    parking_lock(ctx->lot);

    ParkingSlot *slot = NULL;
    /* Check if there is an active reservation */
    if (j_res && cJSON_IsString(j_res) && strlen(j_res->valuestring) > 0) {
        Reservation *res = reservation_find_by_id(ctx->res_mgr, j_res->valuestring);
        if (res && res->status == RESERVATION_ACTIVE) {
            slot = parking_find_slot(ctx->lot, res->slot_id);
            reservation_fulfill(ctx->res_mgr, res->reservation_id);
        }
    }

    if (!slot) {
        slot = parking_find_available_slot(ctx->lot, type);
    }

    if (!slot) {
        parking_unlock(ctx->lot);
        cJSON_Delete(req);

        /* Parking is full for this vehicle type */
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddBoolToObject(resp, "success", false);
        cJSON_AddBoolToObject(resp, "is_full", true);
        cJSON_AddStringToObject(resp, "error", "No available parking slot found for this category. Would you like to enter waiting queue?");
        send_json_response(sock, 409, resp);
        return;
    }

    time_t now = time(NULL);
    parking_allocate_slot(ctx->lot, slot->slot_id, norm_reg, now);
    parking_unlock(ctx->lot);

    ActiveVehicle v;
    memset(&v, 0, sizeof(ActiveVehicle));
    vehicle_generate_ticket_id(ctx->vr, v.ticket_id, sizeof(v.ticket_id));
    strncpy(v.reg_number, norm_reg, sizeof(v.reg_number) - 1);
    v.vehicle_type = type;
    strncpy(v.slot_id, slot->slot_id, sizeof(v.slot_id) - 1);
    if (j_owner && cJSON_IsString(j_owner)) strncpy(v.owner_name, j_owner->valuestring, sizeof(v.owner_name) - 1);
    if (j_contact && cJSON_IsString(j_contact)) strncpy(v.contact, j_contact->valuestring, sizeof(v.contact) - 1);
    if (j_res && cJSON_IsString(j_res)) strncpy(v.reservation_id, j_res->valuestring, sizeof(v.reservation_id) - 1);
    v.entry_time = now;
    format_iso_time(v.entry_time, v.entry_time_str, sizeof(v.entry_time_str));

    vehicle_register_entry(ctx->vr, &v);

    /* Persist state */
    storage_save_slots(ctx->lot);
    storage_save_active_vehicles(ctx->vr);

    cJSON_Delete(req);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON_AddStringToObject(resp, "message", "Vehicle successfully entered");
    cJSON_AddStringToObject(resp, "ticket_id", v.ticket_id);
    cJSON_AddStringToObject(resp, "reg_number", v.reg_number);
    cJSON_AddStringToObject(resp, "slot_id", v.slot_id);
    cJSON_AddStringToObject(resp, "entry_time", v.entry_time_str);
    send_json_response(sock, 201, resp);
}

static void handle_vehicle_exit(SOCKET sock, const char *body) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx || !body) {
        send_error_json(sock, 400, "Invalid request body");
        return;
    }

    cJSON *req = cJSON_Parse(body);
    if (!req) {
        send_error_json(sock, 400, "Malformed JSON request");
        return;
    }

    cJSON *j_search = cJSON_GetObjectItem(req, "query");
    if (!j_search) j_search = cJSON_GetObjectItem(req, "reg_number");
    if (!j_search) j_search = cJSON_GetObjectItem(req, "ticket_id");
    if (!j_search || !cJSON_IsString(j_search) || strlen(j_search->valuestring) == 0) {
        cJSON_Delete(req);
        send_error_json(sock, 400, "Search query (registration number or ticket ID) is required");
        return;
    }

    char query[MAX_REG_LEN];
    normalize_registration(j_search->valuestring, query, sizeof(query));

    /* Search by registration or ticket ID */
    ActiveVehicle *target_veh = vehicle_find_by_reg(ctx->vr, query);
    if (!target_veh) {
        /* Scan active list for ticket ID */
        size_t count = 0;
        ActiveVehicle *arr = vehicle_get_active_array(ctx->vr, &count);
        if (arr) {
            for (size_t i = 0; i < count; i++) {
                if (stricmp(arr[i].ticket_id, j_search->valuestring) == 0) {
                    target_veh = vehicle_find_by_reg(ctx->vr, arr[i].reg_number);
                    break;
                }
            }
            free(arr);
        }
    }

    if (!target_veh) {
        cJSON_Delete(req);
        send_error_json(sock, 404, "Vehicle is not currently parked or not found");
        return;
    }

    time_t exit_time = time(NULL);

    /* Optional simulated duration for testing/demo if provided in request */
    cJSON *j_sim_exit = cJSON_GetObjectItem(req, "exit_time");
    if (j_sim_exit && cJSON_IsNumber(j_sim_exit) && (time_t)j_sim_exit->valuedouble >= target_veh->entry_time) {
        exit_time = (time_t)j_sim_exit->valuedouble;
    }

    /* Calculate billing fee breakdown */
    ParkingTransaction txn = billing_calculate_fee(
        &ctx->pricing,
        target_veh->ticket_id,
        target_veh->reg_number,
        target_veh->vehicle_type,
        target_veh->slot_id,
        target_veh->owner_name,
        target_veh->contact,
        target_veh->entry_time,
        exit_time
    );

    /* Check if this is a quote/preview or confirmed checkout */
    cJSON *j_confirm = cJSON_GetObjectItem(req, "confirm");
    bool confirm = (j_confirm && cJSON_IsTrue(j_confirm));

    if (!confirm) {
        /* Return billing breakdown for confirmation */
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddBoolToObject(resp, "success", true);
        cJSON_AddBoolToObject(resp, "confirmed", false);
        cJSON_AddStringToObject(resp, "ticket_id", txn.ticket_id);
        cJSON_AddStringToObject(resp, "reg_number", txn.reg_number);
        cJSON_AddStringToObject(resp, "slot_id", txn.slot_id);
        cJSON_AddStringToObject(resp, "vehicle_type", vehicle_type_to_string(txn.vehicle_type));
        cJSON_AddStringToObject(resp, "entry_time", txn.entry_time_str);
        cJSON_AddStringToObject(resp, "exit_time", txn.exit_time_str);
        cJSON_AddNumberToObject(resp, "duration_hours", txn.duration_hours);
        cJSON_AddNumberToObject(resp, "duration_minutes", txn.duration_minutes);
        cJSON_AddNumberToObject(resp, "base_fee", txn.base_fee);
        cJSON_AddNumberToObject(resp, "subsequent_fee", txn.subsequent_fee);
        cJSON_AddNumberToObject(resp, "total_fee", txn.total_fee);
        cJSON_AddBoolToObject(resp, "is_grace_period", txn.is_grace_period);

        cJSON_Delete(req);
        send_json_response(sock, 200, resp);
        return;
    }

    /* Confirm checkout */
    char released_slot_id[MAX_ID_LEN];
    strncpy(released_slot_id, target_veh->slot_id, sizeof(released_slot_id) - 1);
    VehicleType released_type = target_veh->vehicle_type;

    /* Remove from active vehicles */
    ActiveVehicle removed;
    vehicle_remove_active(ctx->vr, target_veh->reg_number, &removed);

    /* Release slot in parking lot */
    parking_lock(ctx->lot);
    parking_release_slot(ctx->lot, released_slot_id);

    /* Auto-assign released slot to next eligible waiting vehicle in FIFO queue */
    WaitingVehicle waiting_veh;
    bool assigned_queue = false;
    if (queue_dequeue_eligible(ctx->queue, released_type, &waiting_veh)) {
        parking_allocate_slot(ctx->lot, released_slot_id, waiting_veh.reg_number, exit_time);

        ActiveVehicle new_active;
        memset(&new_active, 0, sizeof(ActiveVehicle));
        vehicle_generate_ticket_id(ctx->vr, new_active.ticket_id, sizeof(new_active.ticket_id));
        strncpy(new_active.reg_number, waiting_veh.reg_number, sizeof(new_active.reg_number) - 1);
        new_active.vehicle_type = waiting_veh.vehicle_type;
        strncpy(new_active.slot_id, released_slot_id, sizeof(new_active.slot_id) - 1);
        strncpy(new_active.owner_name, waiting_veh.owner_name, sizeof(new_active.owner_name) - 1);
        strncpy(new_active.contact, waiting_veh.contact, sizeof(new_active.contact) - 1);
        new_active.entry_time = exit_time;
        format_iso_time(new_active.entry_time, new_active.entry_time_str, sizeof(new_active.entry_time_str));

        vehicle_register_entry(ctx->vr, &new_active);
        assigned_queue = true;
    }
    parking_unlock(ctx->lot);

    /* Save transaction to history */
    storage_append_history(&txn);

    /* Save updated state */
    storage_save_slots(ctx->lot);
    storage_save_active_vehicles(ctx->vr);
    storage_save_queue(ctx->queue);

    cJSON_Delete(req);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON_AddBoolToObject(resp, "confirmed", true);
    cJSON_AddStringToObject(resp, "transaction_id", txn.transaction_id);
    cJSON_AddStringToObject(resp, "reg_number", txn.reg_number);
    cJSON_AddStringToObject(resp, "slot_id", released_slot_id);
    cJSON_AddNumberToObject(resp, "total_fee", txn.total_fee);
    cJSON_AddStringToObject(resp, "exit_time", txn.exit_time_str);
    cJSON_AddBoolToObject(resp, "queue_promoted", assigned_queue);
    if (assigned_queue) {
        cJSON_AddStringToObject(resp, "promoted_vehicle", waiting_veh.reg_number);
    }
    send_json_response(sock, 200, resp);
}

static void handle_queue_get(SOCKET sock) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx) {
        send_error_json(sock, 500, "Server context unavailable");
        return;
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);

    size_t count = 0;
    WaitingVehicle *arr = queue_to_array(ctx->queue, &count);
    cJSON *q_arr = cJSON_CreateArray();

    if (arr) {
        for (size_t i = 0; i < count; i++) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "position", (int)(i + 1));
            cJSON_AddStringToObject(item, "waiting_id", arr[i].waiting_id);
            cJSON_AddStringToObject(item, "reg_number", arr[i].reg_number);
            cJSON_AddStringToObject(item, "vehicle_type", vehicle_type_to_string(arr[i].vehicle_type));
            cJSON_AddStringToObject(item, "owner_name", arr[i].owner_name);
            cJSON_AddStringToObject(item, "contact", arr[i].contact);

            char time_str[32];
            format_iso_time(arr[i].request_time, time_str, sizeof(time_str));
            cJSON_AddStringToObject(item, "request_time", time_str);

            cJSON_AddItemToArray(q_arr, item);
        }
        free(arr);
    }

    cJSON_AddItemToObject(resp, "queue", q_arr);
    cJSON_AddNumberToObject(resp, "total_waiting", count);
    send_json_response(sock, 200, resp);
}

static void handle_queue_post(SOCKET sock, const char *body) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx || !body) {
        send_error_json(sock, 400, "Invalid request body");
        return;
    }

    cJSON *req = cJSON_Parse(body);
    if (!req) {
        send_error_json(sock, 400, "Malformed JSON request");
        return;
    }

    cJSON *j_reg = cJSON_GetObjectItem(req, "reg_number");
    cJSON *j_type = cJSON_GetObjectItem(req, "vehicle_type");
    cJSON *j_owner = cJSON_GetObjectItem(req, "owner_name");
    cJSON *j_contact = cJSON_GetObjectItem(req, "contact");

    if (!j_reg || !cJSON_IsString(j_reg) || strlen(j_reg->valuestring) == 0) {
        cJSON_Delete(req);
        send_error_json(sock, 400, "Registration number is required");
        return;
    }

    WaitingVehicle w;
    memset(&w, 0, sizeof(WaitingVehicle));

    time_t now = time(NULL);
    struct tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &now);
#else
    localtime_r(&now, &tm_info);
#endif
    snprintf(w.waiting_id, sizeof(w.waiting_id), "WAIT-%04d%02d%02d-%03zu",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday, queue_size(ctx->queue) + 1);

    normalize_registration(j_reg->valuestring, w.reg_number, sizeof(w.reg_number));
    w.vehicle_type = j_type && cJSON_IsString(j_type) ? string_to_vehicle_type(j_type->valuestring) : VEHICLE_CAR;
    if (j_owner && cJSON_IsString(j_owner)) strncpy(w.owner_name, j_owner->valuestring, sizeof(w.owner_name) - 1);
    if (j_contact && cJSON_IsString(j_contact)) strncpy(w.contact, j_contact->valuestring, sizeof(w.contact) - 1);
    w.request_time = now;

    queue_enqueue(ctx->queue, &w);
    storage_save_queue(ctx->queue);

    cJSON_Delete(req);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON_AddStringToObject(resp, "waiting_id", w.waiting_id);
    cJSON_AddNumberToObject(resp, "position", (int)queue_size(ctx->queue));
    send_json_response(sock, 201, resp);
}

static void handle_queue_delete(SOCKET sock, const char *url_path) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx) {
        send_error_json(sock, 500, "Server context unavailable");
        return;
    }

    /* Extract ID from /api/queue/{id} */
    const char *prefix = "/api/queue/";
    const char *id = url_path + strlen(prefix);
    if (!id || strlen(id) == 0) {
        send_error_json(sock, 400, "Waiting ID required");
        return;
    }

    if (queue_remove_by_id(ctx->queue, id)) {
        storage_save_queue(ctx->queue);
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddBoolToObject(resp, "success", true);
        cJSON_AddStringToObject(resp, "message", "Waiting queue record cancelled");
        send_json_response(sock, 200, resp);
    } else {
        send_error_json(sock, 404, "Waiting ID not found in queue");
    }
}

static void handle_search(SOCKET sock, const char *url_path) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx) {
        send_error_json(sock, 500, "Server context unavailable");
        return;
    }

    /* Extract query from ?q= */
    const char *q_param = strstr(url_path, "q=");
    char search_term[MAX_REG_LEN] = "";
    if (q_param) {
        strncpy(search_term, q_param + 2, sizeof(search_term) - 1);
        /* Cut off at '&' if any */
        char *amp = strchr(search_term, '&');
        if (amp) *amp = '\0';
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON *results = cJSON_CreateArray();

    size_t active_count = 0;
    ActiveVehicle *actives = vehicle_get_active_array(ctx->vr, &active_count);

    if (actives) {
        for (size_t i = 0; i < active_count; i++) {
            bool matches = false;
            if (strlen(search_term) == 0) {
                matches = true;
            } else if (str_contains_case_insensitive(actives[i].reg_number, search_term) ||
                       str_contains_case_insensitive(actives[i].slot_id, search_term) ||
                       str_contains_case_insensitive(actives[i].ticket_id, search_term) ||
                       str_contains_case_insensitive(actives[i].owner_name, search_term) ||
                       str_contains_case_insensitive(vehicle_type_to_string(actives[i].vehicle_type), search_term)) {
                matches = true;
            }

            if (matches) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "status", "PARKED");
                cJSON_AddStringToObject(item, "ticket_id", actives[i].ticket_id);
                cJSON_AddStringToObject(item, "reg_number", actives[i].reg_number);
                cJSON_AddStringToObject(item, "slot_id", actives[i].slot_id);
                cJSON_AddStringToObject(item, "vehicle_type", vehicle_type_to_string(actives[i].vehicle_type));
                cJSON_AddStringToObject(item, "owner_name", actives[i].owner_name);
                cJSON_AddStringToObject(item, "contact", actives[i].contact);
                cJSON_AddStringToObject(item, "entry_time", actives[i].entry_time_str);
                cJSON_AddItemToArray(results, item);
            }
        }
        free(actives);
    }

    cJSON_AddItemToObject(resp, "results", results);
    send_json_response(sock, 200, resp);
}

static void handle_history(SOCKET sock, const char *url_path) {
    LinkedList *history = storage_load_history();
    if (!history) {
        send_error_json(sock, 500, "Unable to load parking history");
        return;
    }

    size_t count = ll_size(history);
    ParkingTransaction *arr = NULL;
    if (count > 0) {
        arr = (ParkingTransaction*)malloc(sizeof(ParkingTransaction) * count);
        for (size_t i = 0; i < count; i++) {
            ParkingTransaction *t = (ParkingTransaction*)ll_get_at(history, i);
            if (t) arr[i] = *t;
        }
    }

    /* Check sorting query parameters: ?sort=fee&order=desc */
    const char *sort_param = strstr(url_path, "sort=");
    const char *order_param = strstr(url_path, "order=");
    bool desc = (order_param && strncmp(order_param + 6, "desc", 4) == 0);

    if (arr && sort_param) {
        if (strncmp(sort_param + 5, "fee", 3) == 0) {
            /* Sort by fee using Merge Sort */
            merge_sort(arr, count, sizeof(ParkingTransaction), desc ? cmp_double_desc : cmp_double_asc);
        } else if (strncmp(sort_param + 5, "time", 4) == 0) {
            /* Sort by entry time using Quick Sort */
            quick_sort(arr, count, sizeof(ParkingTransaction), desc ? cmp_time_desc : cmp_time_asc);
        }
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON *hist_arr = cJSON_CreateArray();

    if (arr) {
        for (size_t i = 0; i < count; i++) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "transaction_id", arr[i].transaction_id);
            cJSON_AddStringToObject(item, "ticket_id", arr[i].ticket_id);
            cJSON_AddStringToObject(item, "reg_number", arr[i].reg_number);
            cJSON_AddStringToObject(item, "vehicle_type", vehicle_type_to_string(arr[i].vehicle_type));
            cJSON_AddStringToObject(item, "slot_id", arr[i].slot_id);
            cJSON_AddStringToObject(item, "owner_name", arr[i].owner_name);
            cJSON_AddStringToObject(item, "contact", arr[i].contact);
            cJSON_AddStringToObject(item, "entry_time", arr[i].entry_time_str);
            cJSON_AddStringToObject(item, "exit_time", arr[i].exit_time_str);
            cJSON_AddNumberToObject(item, "duration_hours", arr[i].duration_hours);
            cJSON_AddNumberToObject(item, "duration_minutes", arr[i].duration_minutes);
            cJSON_AddNumberToObject(item, "fee", arr[i].total_fee);
            cJSON_AddItemToArray(hist_arr, item);
        }
        free(arr);
    }

    ll_destroy(history);
    cJSON_AddItemToObject(resp, "history", hist_arr);
    cJSON_AddNumberToObject(resp, "total_records", count);
    send_json_response(sock, 200, resp);
}

static void handle_history_export_csv(SOCKET sock) {
    LinkedList *history = storage_load_history();
    if (!history) {
        send_error_json(sock, 500, "Unable to load history");
        return;
    }

    size_t count = ll_size(history);
    size_t est_sz = 1024 + (count * 256);
    char *csv_buf = (char*)malloc(est_sz);
    if (!csv_buf) {
        ll_destroy(history);
        send_error_json(sock, 500, "Memory allocation error");
        return;
    }

    int offset = snprintf(csv_buf, est_sz,
        "Transaction ID,Ticket ID,Registration Number,Vehicle Type,Slot ID,Owner Name,Contact,Entry Time,Exit Time,Fee (USD)\r\n");

    for (size_t i = 0; i < count; i++) {
        ParkingTransaction *t = (ParkingTransaction*)ll_get_at(history, i);
        if (!t) continue;
        offset += snprintf(csv_buf + offset, est_sz - offset,
            "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%.2f\r\n",
            t->transaction_id, t->ticket_id, t->reg_number, vehicle_type_to_string(t->vehicle_type),
            t->slot_id, t->owner_name, t->contact, t->entry_time_str, t->exit_time_str, t->total_fee);
    }
    ll_destroy(history);

    send_http_response(sock, 200, "OK", "text/csv; charset=utf-8", csv_buf, (size_t)offset);
    free(csv_buf);
}

static void handle_navigation(SOCKET sock, const char *url_path) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx || !ctx->nav_graph) {
        send_error_json(sock, 500, "Navigation graph unavailable");
        return;
    }

    char target_slot[MAX_ID_LEN] = "C-01";
    char entrance[MAX_ID_LEN] = "ENT_NORTH";

    const char *slot_param = strstr(url_path, "slot=");
    if (slot_param) {
        strncpy(target_slot, slot_param + 5, sizeof(target_slot) - 1);
        char *amp = strchr(target_slot, '&');
        if (amp) *amp = '\0';
    }

    const char *ent_param = strstr(url_path, "entrance=");
    if (ent_param) {
        strncpy(entrance, ent_param + 9, sizeof(entrance) - 1);
        char *amp = strchr(entrance, '&');
        if (amp) *amp = '\0';
    }

    NavigationPath path = graph_dijkstra(ctx->nav_graph, entrance, target_slot);
    NavigationPath bfs_path = graph_bfs(ctx->nav_graph, entrance, target_slot);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", path.found);
    cJSON_AddStringToObject(resp, "start", entrance);
    cJSON_AddStringToObject(resp, "destination", target_slot);
    cJSON_AddNumberToObject(resp, "shortest_distance_meters", path.total_distance);
    cJSON_AddNumberToObject(resp, "dijkstra_hops", path.path_length);
    cJSON_AddNumberToObject(resp, "bfs_hops", bfs_path.path_length);

    cJSON *steps = cJSON_CreateArray();
    for (int i = 0; i < path.path_length; i++) {
        cJSON *step = cJSON_CreateObject();
        int node_idx = path.path_nodes[i];
        cJSON_AddStringToObject(step, "node_id", ctx->nav_graph->vertices[node_idx].id);
        cJSON_AddStringToObject(step, "node_label", ctx->nav_graph->vertices[node_idx].label);
        cJSON_AddNumberToObject(step, "floor", ctx->nav_graph->vertices[node_idx].floor);
        if (i < path.path_length - 1) {
            cJSON_AddStringToObject(step, "guidance", path.instructions[i]);
        }
        cJSON_AddItemToArray(steps, step);
    }
    cJSON_AddItemToObject(resp, "path", steps);

    send_json_response(sock, 200, resp);
}

static void handle_dsa_stats(SOCKET sock) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx) {
        send_error_json(sock, 500, "Server context unavailable");
        return;
    }

    HashTableStats ht_stats = ht_get_stats(ctx->vr->active_hash);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);

    cJSON *ht = cJSON_CreateObject();
    cJSON_AddStringToObject(ht, "data_structure", "Separate Chaining Hash Table");
    cJSON_AddStringToObject(ht, "hash_function", "DJB2 (Dan Bernstein)");
    cJSON_AddNumberToObject(ht, "capacity", ht_stats.capacity);
    cJSON_AddNumberToObject(ht, "stored_elements", ht_stats.count);
    cJSON_AddNumberToObject(ht, "collisions", ht_stats.collisions);
    cJSON_AddNumberToObject(ht, "load_factor", ht_stats.load_factor);
    cJSON_AddNumberToObject(ht, "max_chain_length", ht_stats.max_chain_length);
    cJSON_AddStringToObject(ht, "time_complexity", "Average O(1), Worst O(n)");
    cJSON_AddItemToObject(resp, "hash_table", ht);

    cJSON *q = cJSON_CreateObject();
    cJSON_AddStringToObject(q, "data_structure", "FIFO Linked-List Queue");
    cJSON_AddNumberToObject(q, "waiting_vehicles", queue_size(ctx->queue));
    cJSON_AddStringToObject(q, "enqueue_complexity", "O(1)");
    cJSON_AddStringToObject(q, "dequeue_complexity", "O(1)");
    cJSON_AddItemToObject(resp, "queue", q);

    cJSON *gr = cJSON_CreateObject();
    cJSON_AddStringToObject(gr, "data_structure", "Adjacency List Directed Weighted Graph");
    cJSON_AddNumberToObject(gr, "vertices", ctx->nav_graph->num_vertices);
    cJSON_AddStringToObject(gr, "shortest_path_algorithm", "Dijkstra O(V^2 / E log V)");
    cJSON_AddStringToObject(gr, "unweighted_search_algorithm", "Breadth-First Search (BFS) O(V + E)");
    cJSON_AddItemToObject(resp, "graph", gr);

    send_json_response(sock, 200, resp);
}

static void handle_reservations_get(SOCKET sock) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx) {
        send_error_json(sock, 500, "Server context unavailable");
        return;
    }

    reservation_check_expirations(ctx->res_mgr, ctx->lot);

    size_t count = 0;
    Reservation *arr = reservation_get_all_array(ctx->res_mgr, &count);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON *res_arr = cJSON_CreateArray();

    if (arr) {
        for (size_t i = 0; i < count; i++) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "reservation_id", arr[i].reservation_id);
            cJSON_AddStringToObject(item, "reg_number", arr[i].reg_number);
            cJSON_AddStringToObject(item, "vehicle_type", vehicle_type_to_string(arr[i].vehicle_type));
            cJSON_AddStringToObject(item, "slot_id", arr[i].slot_id);
            cJSON_AddStringToObject(item, "customer_name", arr[i].customer_name);
            cJSON_AddStringToObject(item, "contact", arr[i].contact);
            cJSON_AddStringToObject(item, "reservation_time", arr[i].res_time_str);
            cJSON_AddStringToObject(item, "expiry_time", arr[i].exp_time_str);
            cJSON_AddNumberToObject(item, "status_code", (int)arr[i].status);
            const char *st_str = "ACTIVE";
            if (arr[i].status == RESERVATION_FULFILLED) st_str = "FULFILLED";
            else if (arr[i].status == RESERVATION_CANCELLED) st_str = "CANCELLED";
            else if (arr[i].status == RESERVATION_EXPIRED) st_str = "EXPIRED";
            cJSON_AddStringToObject(item, "status", st_str);
            cJSON_AddItemToArray(res_arr, item);
        }
        free(arr);
    }

    cJSON_AddItemToObject(resp, "reservations", res_arr);
    send_json_response(sock, 200, resp);
}

static void handle_reservations_post(SOCKET sock, const char *body) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx || !body) {
        send_error_json(sock, 400, "Invalid request body");
        return;
    }

    cJSON *req = cJSON_Parse(body);
    if (!req) {
        send_error_json(sock, 400, "Malformed JSON request");
        return;
    }

    cJSON *j_reg = cJSON_GetObjectItem(req, "reg_number");
    cJSON *j_type = cJSON_GetObjectItem(req, "vehicle_type");
    cJSON *j_slot = cJSON_GetObjectItem(req, "slot_id");
    cJSON *j_name = cJSON_GetObjectItem(req, "customer_name");
    cJSON *j_contact = cJSON_GetObjectItem(req, "contact");
    cJSON *j_hours = cJSON_GetObjectItem(req, "valid_hours");

    if (!j_reg || !cJSON_IsString(j_reg) || strlen(j_reg->valuestring) == 0) {
        cJSON_Delete(req);
        send_error_json(sock, 400, "Registration number is required");
        return;
    }

    VehicleType type = j_type && cJSON_IsString(j_type) ? string_to_vehicle_type(j_type->valuestring) : VEHICLE_CAR;
    const char *slot_id = j_slot && cJSON_IsString(j_slot) ? j_slot->valuestring : NULL;
    const char *name = j_name && cJSON_IsString(j_name) ? j_name->valuestring : NULL;
    const char *contact = j_contact && cJSON_IsString(j_contact) ? j_contact->valuestring : NULL;
    int hours = j_hours && cJSON_IsNumber(j_hours) ? j_hours->valueint : 2;

    Reservation res;
    if (reservation_create(ctx->res_mgr, ctx->lot, j_reg->valuestring, type, slot_id, name, contact, hours, &res)) {
        storage_save_reservations(ctx->res_mgr);
        storage_save_slots(ctx->lot);

        cJSON_Delete(req);
        cJSON *resp = cJSON_CreateObject();
        cJSON_AddBoolToObject(resp, "success", true);
        cJSON_AddStringToObject(resp, "reservation_id", res.reservation_id);
        cJSON_AddStringToObject(resp, "slot_id", res.slot_id);
        cJSON_AddStringToObject(resp, "expiry_time", res.exp_time_str);
        send_json_response(sock, 201, resp);
    } else {
        cJSON_Delete(req);
        send_error_json(sock, 409, "Reservation failed: slot conflict or already active reservation");
    }
}

static void handle_reservations_cancel(SOCKET sock, const char *body) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx || !body) {
        send_error_json(sock, 400, "Invalid request body");
        return;
    }

    cJSON *req = cJSON_Parse(body);
    if (!req) {
        send_error_json(sock, 400, "Malformed JSON request");
        return;
    }

    cJSON *j_id = cJSON_GetObjectItem(req, "reservation_id");
    if (!j_id || !cJSON_IsString(j_id)) {
        cJSON_Delete(req);
        send_error_json(sock, 400, "Reservation ID is required");
        return;
    }

    if (reservation_cancel(ctx->res_mgr, ctx->lot, j_id->valuestring)) {
        storage_save_reservations(ctx->res_mgr);
        storage_save_slots(ctx->lot);
        cJSON_Delete(req);

        cJSON *resp = cJSON_CreateObject();
        cJSON_AddBoolToObject(resp, "success", true);
        cJSON_AddStringToObject(resp, "message", "Reservation cancelled and slot released");
        send_json_response(sock, 200, resp);
    } else {
        cJSON_Delete(req);
        send_error_json(sock, 404, "Reservation ID not found or already cancelled/fulfilled");
    }
}

static void handle_settings_get(SOCKET sock) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx) {
        send_error_json(sock, 500, "Server context unavailable");
        return;
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON_AddNumberToObject(resp, "car_first_hour", ctx->pricing.car_first_hour);
    cJSON_AddNumberToObject(resp, "car_subsequent_hour", ctx->pricing.car_subsequent_hour);
    cJSON_AddNumberToObject(resp, "bike_first_hour", ctx->pricing.bike_first_hour);
    cJSON_AddNumberToObject(resp, "bike_subsequent_hour", ctx->pricing.bike_subsequent_hour);
    cJSON_AddNumberToObject(resp, "accessible_first_hour", ctx->pricing.accessible_first_hour);
    cJSON_AddNumberToObject(resp, "accessible_subsequent_hour", ctx->pricing.accessible_subsequent_hour);
    cJSON_AddNumberToObject(resp, "grace_period_minutes", ctx->pricing.grace_period_minutes);
    cJSON_AddNumberToObject(resp, "min_charge", ctx->pricing.min_charge);
    cJSON_AddNumberToObject(resp, "rounding_minutes", ctx->pricing.rounding_minutes);
    send_json_response(sock, 200, resp);
}

static void handle_settings_put(SOCKET sock, const char *body) {
    ServerContext *ctx = g_server_ctx;
    if (!ctx || !body) {
        send_error_json(sock, 400, "Invalid request body");
        return;
    }

    cJSON *req = cJSON_Parse(body);
    if (!req) {
        send_error_json(sock, 400, "Malformed JSON request");
        return;
    }

    cJSON *c1 = cJSON_GetObjectItem(req, "car_first_hour");
    cJSON *c2 = cJSON_GetObjectItem(req, "car_subsequent_hour");
    cJSON *b1 = cJSON_GetObjectItem(req, "bike_first_hour");
    cJSON *b2 = cJSON_GetObjectItem(req, "bike_subsequent_hour");
    cJSON *a1 = cJSON_GetObjectItem(req, "accessible_first_hour");
    cJSON *a2 = cJSON_GetObjectItem(req, "accessible_subsequent_hour");
    cJSON *gr = cJSON_GetObjectItem(req, "grace_period_minutes");
    cJSON *mc = cJSON_GetObjectItem(req, "min_charge");
    cJSON *rm = cJSON_GetObjectItem(req, "rounding_minutes");

    if (c1 && cJSON_IsNumber(c1)) ctx->pricing.car_first_hour = c1->valuedouble;
    if (c2 && cJSON_IsNumber(c2)) ctx->pricing.car_subsequent_hour = c2->valuedouble;
    if (b1 && cJSON_IsNumber(b1)) ctx->pricing.bike_first_hour = b1->valuedouble;
    if (b2 && cJSON_IsNumber(b2)) ctx->pricing.bike_subsequent_hour = b2->valuedouble;
    if (a1 && cJSON_IsNumber(a1)) ctx->pricing.accessible_first_hour = a1->valuedouble;
    if (a2 && cJSON_IsNumber(a2)) ctx->pricing.accessible_subsequent_hour = a2->valuedouble;
    if (gr && cJSON_IsNumber(gr)) ctx->pricing.grace_period_minutes = gr->valueint;
    if (mc && cJSON_IsNumber(mc)) ctx->pricing.min_charge = mc->valuedouble;
    if (rm && cJSON_IsNumber(rm)) ctx->pricing.rounding_minutes = rm->valueint;

    storage_save_settings(&ctx->pricing);
    cJSON_Delete(req);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "success", true);
    cJSON_AddStringToObject(resp, "message", "Settings updated successfully");
    send_json_response(sock, 200, resp);
}

/* Dispatch incoming request */
static void dispatch_request(SOCKET sock, const char *method, const char *path, const char *body) {
    /* CORS pre-flight */
    if (strcmp(method, "OPTIONS") == 0) {
        send_http_response(sock, 204, "No Content", "text/plain", NULL, 0);
        return;
    }

    if (strncmp(path, "/api/", 5) == 0) {
        if (strcmp(path, "/api/health") == 0 && strcmp(method, "GET") == 0) {
            handle_health(sock);
        } else if (strcmp(path, "/api/dashboard") == 0 && strcmp(method, "GET") == 0) {
            handle_dashboard(sock);
        } else if (strcmp(path, "/api/slots") == 0 && strcmp(method, "GET") == 0) {
            handle_get_slots(sock);
        } else if (strcmp(path, "/api/vehicles/entry") == 0 && strcmp(method, "POST") == 0) {
            handle_vehicle_entry(sock, body);
        } else if (strcmp(path, "/api/vehicles/exit") == 0 && strcmp(method, "POST") == 0) {
            handle_vehicle_exit(sock, body);
        } else if (strncmp(path, "/api/vehicles/search", 20) == 0 && strcmp(method, "GET") == 0) {
            handle_search(sock, path);
        } else if (strcmp(path, "/api/queue") == 0 && strcmp(method, "GET") == 0) {
            handle_queue_get(sock);
        } else if (strcmp(path, "/api/queue") == 0 && strcmp(method, "POST") == 0) {
            handle_queue_post(sock, body);
        } else if (strncmp(path, "/api/queue/", 11) == 0 && strcmp(method, "DELETE") == 0) {
            handle_queue_delete(sock, path);
        } else if (strncmp(path, "/api/history/export", 19) == 0 && strcmp(method, "GET") == 0) {
            handle_history_export_csv(sock);
        } else if (strncmp(path, "/api/history", 12) == 0 && strcmp(method, "GET") == 0) {
            handle_history(sock, path);
        } else if (strncmp(path, "/api/navigation", 15) == 0 && strcmp(method, "GET") == 0) {
            handle_navigation(sock, path);
        } else if (strcmp(path, "/api/dsa-stats") == 0 && strcmp(method, "GET") == 0) {
            handle_dsa_stats(sock);
        } else if (strcmp(path, "/api/reservations") == 0 && strcmp(method, "GET") == 0) {
            handle_reservations_get(sock);
        } else if (strcmp(path, "/api/reservations") == 0 && strcmp(method, "POST") == 0) {
            handle_reservations_post(sock, body);
        } else if (strcmp(path, "/api/reservations/cancel") == 0 && strcmp(method, "POST") == 0) {
            handle_reservations_cancel(sock, body);
        } else if (strcmp(path, "/api/settings") == 0 && strcmp(method, "GET") == 0) {
            handle_settings_get(sock);
        } else if (strcmp(path, "/api/settings") == 0 && (strcmp(method, "PUT") == 0 || strcmp(method, "POST") == 0)) {
            handle_settings_put(sock, body);
        } else {
            send_error_json(sock, 404, "Endpoint not found");
        }
    } else {
        /* Static file serving */
        serve_static_file(sock, path);
    }
}

#ifdef _WIN32
static DWORD WINAPI client_thread(LPVOID arg) {
    SOCKET client_sock = (SOCKET)arg;
#else
static void* client_thread(void *arg) {
    int client_sock = (int)(intptr_t)arg;
#endif

    char buffer[16384];
    int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        char method[16] = {0};
        char path[MAX_PATH_LEN] = {0};
        char protocol[16] = {0};

        if (sscanf(buffer, "%15s %511s %15s", method, path, protocol) >= 2) {
            /* Find body if POST/PUT */
            char *body = strstr(buffer, "\r\n\r\n");
            if (body) {
                body += 4;
            }
            dispatch_request(client_sock, method, path, body);
        }
    }

#ifdef _WIN32
    closesocket(client_sock);
    return 0;
#else
    close(client_sock);
    return NULL;
#endif
}

ServerContext* server_init(int port) {
    ServerContext *ctx = (ServerContext*)calloc(1, sizeof(ServerContext));
    if (!ctx) return NULL;

    ctx->port = (port > 0) ? port : DEFAULT_PORT;
    ctx->pricing = billing_get_default_policy();

    /* Initialize storage and directories */
    storage_init();

    /* Create parking lot (40 slots capacity) */
    ctx->lot = parking_lot_create(40);

    /* Create vehicle registry */
    ctx->vr = vehicle_registry_create();

    /* Create queue */
    ctx->queue = queue_create();

    /* Create reservations manager */
    ctx->res_mgr = reservation_mgr_create();

    /* Create and initialize navigation graph */
    ctx->nav_graph = graph_create();
    graph_init_facility_layout(ctx->nav_graph);

    /* Load persistent files if available */
    storage_load_settings(&ctx->pricing);
    storage_load_slots(ctx->lot);
    storage_load_active_vehicles(ctx->vr, ctx->lot);
    storage_load_queue(ctx->queue);
    storage_load_reservations(ctx->res_mgr, ctx->lot);

    /* If first run (no slots saved), save initial default slots */
    FILE *fp = fopen(FILE_SLOTS, "rb");
    if (!fp) {
        storage_save_slots(ctx->lot);
    } else {
        fclose(fp);
    }

    g_server_ctx = ctx;
    return ctx;
}

int server_run(ServerContext *ctx) {
    if (!ctx) return -1;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
#endif

    ctx->server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
    if (ctx->server_socket == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }
#else
    if (ctx->server_socket < 0) return -1;
#endif

    int opt = 1;
    setsockopt(ctx->server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((uint16_t)ctx->port);

    if (bind(ctx->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        fprintf(stderr, "Failed to bind to port %d\n", ctx->port);
#ifdef _WIN32
        closesocket(ctx->server_socket);
        WSACleanup();
#else
        close(ctx->server_socket);
#endif
        return -1;
    }

    if (listen(ctx->server_socket, SOMAXCONN) != 0) {
        fprintf(stderr, "Failed to listen on socket\n");
        return -1;
    }

    ctx->running = true;
    printf("\n=======================================================\n");
    printf(" Smart Parking Management System (DSA Capstone)\n");
    printf(" Server listening at: http://localhost:%d\n", ctx->port);
    printf(" API Endpoint:        http://localhost:%d/api/health\n", ctx->port);
    printf("=======================================================\n\n");

    while (ctx->running) {
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client_sock = accept(ctx->server_socket, (struct sockaddr*)&client_addr, &client_len);

#ifdef _WIN32
        if (client_sock == INVALID_SOCKET) {
            if (!ctx->running) break;
            continue;
        }
        HANDLE thread = CreateThread(NULL, 0, client_thread, (LPVOID)client_sock, 0, NULL);
        if (thread) CloseHandle(thread);
#else
        if (client_sock < 0) {
            if (!ctx->running) break;
            continue;
        }
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, (void*)(intptr_t)client_sock);
        pthread_detach(tid);
#endif
    }

    return 0;
}

void server_stop(ServerContext *ctx) {
    if (!ctx) return;
    ctx->running = false;
#ifdef _WIN32
    if (ctx->server_socket != INVALID_SOCKET) {
        closesocket(ctx->server_socket);
    }
    WSACleanup();
#else
    if (ctx->server_socket >= 0) {
        close(ctx->server_socket);
    }
#endif

    if (ctx->lot) parking_lot_destroy(ctx->lot);
    if (ctx->vr) vehicle_registry_destroy(ctx->vr);
    if (ctx->queue) queue_destroy(ctx->queue);
    if (ctx->res_mgr) reservation_mgr_destroy(ctx->res_mgr);
    if (ctx->nav_graph) graph_destroy(ctx->nav_graph);
    free(ctx);
    g_server_ctx = NULL;
}
