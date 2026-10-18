#include "../include/storage.h"
#include "../include/cJSON.h"

#ifdef _WIN32
#include <direct.h>
#define MKDIR(d) _mkdir(d)
#else
#include <sys/stat.h>
#define MKDIR(d) mkdir(d, 0755)
#endif

bool storage_init(void) {
    MKDIR(DATA_DIR);
    return true;
}

char* storage_read_file(const char *filepath) {
    if (!filepath) return NULL;
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (sz < 0) {
        fclose(fp);
        return NULL;
    }

    char *buf = (char*)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, (size_t)sz, fp);
    buf[read_bytes] = '\0';
    fclose(fp);
    return buf;
}

bool storage_atomic_write(const char *filepath, const char *content) {
    if (!filepath || !content) return false;

    storage_init();

    char temp_path[MAX_PATH_LEN];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", filepath);

    FILE *fp = fopen(temp_path, "wb");
    if (!fp) return false;

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, fp);
    fflush(fp);
    fclose(fp);

    if (written != len) {
        remove(temp_path);
        return false;
    }

#ifdef _WIN32
    /* On Windows, remove target file before rename for atomic replacement */
    remove(filepath);
    if (rename(temp_path, filepath) != 0) {
        /* Fallback MoveFileEx */
        if (!MoveFileExA(temp_path, filepath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            remove(temp_path);
            return false;
        }
    }
#else
    if (rename(temp_path, filepath) != 0) {
        remove(temp_path);
        return false;
    }
#endif

    return true;
}

/* --- SLOTS STORAGE --- */

bool storage_save_slots(ParkingLot *lot) {
    if (!lot) return false;

    cJSON *root = cJSON_CreateArray();
    if (!root) return false;

    parking_lock(lot);
    for (size_t i = 0; i < lot->total_slots; i++) {
        ParkingSlot *s = lot->slots_array[i];
        if (!s) continue;

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "slot_id", s->slot_id);
        cJSON_AddNumberToObject(item, "floor", s->floor);
        cJSON_AddStringToObject(item, "zone", s->zone);
        cJSON_AddStringToObject(item, "category", vehicle_type_to_string(s->category));
        cJSON_AddStringToObject(item, "status", slot_status_to_string(s->status));
        cJSON_AddStringToObject(item, "occupied_reg", s->occupied_reg);
        cJSON_AddNumberToObject(item, "occupied_time", (double)s->occupied_time);
        cJSON_AddStringToObject(item, "reservation_id", s->reservation_id);
        cJSON_AddNumberToObject(item, "distance", s->distance_from_entrance);

        cJSON_AddItemToArray(root, item);
    }
    parking_unlock(lot);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str) return false;
    bool ok = storage_atomic_write(FILE_SLOTS, json_str);
    free(json_str);
    return ok;
}

bool storage_load_slots(ParkingLot *lot) {
    if (!lot) return false;

    char *content = storage_read_file(FILE_SLOTS);
    if (!content) return false;

    cJSON *root = cJSON_Parse(content);
    free(content);
    if (!root || !cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        return false;
    }

    parking_lock(lot);
    int size = cJSON_GetArraySize(root);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsObject(item)) continue;

        cJSON *j_id = cJSON_GetObjectItem(item, "slot_id");
        if (!j_id || !cJSON_IsString(j_id)) continue;

        ParkingSlot *s = parking_find_slot(lot, j_id->valuestring);
        if (!s) continue;

        cJSON *j_status = cJSON_GetObjectItem(item, "status");
        if (j_status && cJSON_IsString(j_status)) {
            s->status = string_to_slot_status(j_status->valuestring);
        }

        cJSON *j_reg = cJSON_GetObjectItem(item, "occupied_reg");
        if (j_reg && cJSON_IsString(j_reg)) {
            strncpy(s->occupied_reg, j_reg->valuestring, sizeof(s->occupied_reg) - 1);
        }

        cJSON *j_time = cJSON_GetObjectItem(item, "occupied_time");
        if (j_time && cJSON_IsNumber(j_time)) {
            s->occupied_time = (time_t)j_time->valuedouble;
        }

        cJSON *j_res = cJSON_GetObjectItem(item, "reservation_id");
        if (j_res && cJSON_IsString(j_res)) {
            strncpy(s->reservation_id, j_res->valuestring, sizeof(s->reservation_id) - 1);
        }
    }
    parking_unlock(lot);

    cJSON_Delete(root);
    return true;
}

/* --- ACTIVE VEHICLES STORAGE --- */

bool storage_save_active_vehicles(const VehicleRegistry *vr) {
    if (!vr) return false;

    cJSON *root = cJSON_CreateArray();
    if (!root) return false;

    size_t count = ll_size(vr->active_list);
    for (size_t i = 0; i < count; i++) {
        ActiveVehicle *v = (ActiveVehicle*)ll_get_at(vr->active_list, i);
        if (!v) continue;

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ticket_id", v->ticket_id);
        cJSON_AddStringToObject(item, "reg_number", v->reg_number);
        cJSON_AddStringToObject(item, "vehicle_type", vehicle_type_to_string(v->vehicle_type));
        cJSON_AddStringToObject(item, "slot_id", v->slot_id);
        cJSON_AddStringToObject(item, "owner_name", v->owner_name);
        cJSON_AddStringToObject(item, "contact", v->contact);
        cJSON_AddStringToObject(item, "reservation_id", v->reservation_id);
        cJSON_AddNumberToObject(item, "entry_time", (double)v->entry_time);
        cJSON_AddStringToObject(item, "entry_time_str", v->entry_time_str);

        cJSON_AddItemToArray(root, item);
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) return false;

    bool ok = storage_atomic_write(FILE_ACTIVE_VEHICLES, json_str);
    free(json_str);
    return ok;
}

bool storage_load_active_vehicles(VehicleRegistry *vr, ParkingLot *lot) {
    if (!vr) return false;

    char *content = storage_read_file(FILE_ACTIVE_VEHICLES);
    if (!content) return false;

    cJSON *root = cJSON_Parse(content);
    free(content);
    if (!root || !cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        return false;
    }

    int size = cJSON_GetArraySize(root);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsObject(item)) continue;

        ActiveVehicle v;
        memset(&v, 0, sizeof(ActiveVehicle));

        cJSON *j_ticket = cJSON_GetObjectItem(item, "ticket_id");
        cJSON *j_reg = cJSON_GetObjectItem(item, "reg_number");
        cJSON *j_type = cJSON_GetObjectItem(item, "vehicle_type");
        cJSON *j_slot = cJSON_GetObjectItem(item, "slot_id");
        cJSON *j_owner = cJSON_GetObjectItem(item, "owner_name");
        cJSON *j_contact = cJSON_GetObjectItem(item, "contact");
        cJSON *j_res = cJSON_GetObjectItem(item, "reservation_id");
        cJSON *j_time = cJSON_GetObjectItem(item, "entry_time");
        cJSON *j_time_str = cJSON_GetObjectItem(item, "entry_time_str");

        if (j_ticket && cJSON_IsString(j_ticket)) strncpy(v.ticket_id, j_ticket->valuestring, sizeof(v.ticket_id) - 1);
        if (j_reg && cJSON_IsString(j_reg)) strncpy(v.reg_number, j_reg->valuestring, sizeof(v.reg_number) - 1);
        if (j_type && cJSON_IsString(j_type)) v.vehicle_type = string_to_vehicle_type(j_type->valuestring);
        if (j_slot && cJSON_IsString(j_slot)) strncpy(v.slot_id, j_slot->valuestring, sizeof(v.slot_id) - 1);
        if (j_owner && cJSON_IsString(j_owner)) strncpy(v.owner_name, j_owner->valuestring, sizeof(v.owner_name) - 1);
        if (j_contact && cJSON_IsString(j_contact)) strncpy(v.contact, j_contact->valuestring, sizeof(v.contact) - 1);
        if (j_res && cJSON_IsString(j_res)) strncpy(v.reservation_id, j_res->valuestring, sizeof(v.reservation_id) - 1);
        if (j_time && cJSON_IsNumber(j_time)) v.entry_time = (time_t)j_time->valuedouble;
        if (j_time_str && cJSON_IsString(j_time_str)) strncpy(v.entry_time_str, j_time_str->valuestring, sizeof(v.entry_time_str) - 1);

        vehicle_register_entry(vr, &v);

        /* Ensure matching slot is marked occupied */
        if (lot && v.slot_id[0] != '\0') {
            parking_allocate_slot(lot, v.slot_id, v.reg_number, v.entry_time);
        }
    }

    cJSON_Delete(root);
    return true;
}

/* --- QUEUE STORAGE --- */

bool storage_save_queue(const Queue *q) {
    if (!q) return false;

    cJSON *root = cJSON_CreateArray();
    if (!root) return false;

    size_t count = 0;
    WaitingVehicle *arr = queue_to_array(q, &count);
    if (arr) {
        for (size_t i = 0; i < count; i++) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "waiting_id", arr[i].waiting_id);
            cJSON_AddStringToObject(item, "reg_number", arr[i].reg_number);
            cJSON_AddStringToObject(item, "vehicle_type", vehicle_type_to_string(arr[i].vehicle_type));
            cJSON_AddStringToObject(item, "owner_name", arr[i].owner_name);
            cJSON_AddStringToObject(item, "contact", arr[i].contact);
            cJSON_AddNumberToObject(item, "request_time", (double)arr[i].request_time);
            cJSON_AddItemToArray(root, item);
        }
        free(arr);
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) return false;

    bool ok = storage_atomic_write(FILE_QUEUE, json_str);
    free(json_str);
    return ok;
}

bool storage_load_queue(Queue *q) {
    if (!q) return false;

    char *content = storage_read_file(FILE_QUEUE);
    if (!content) return false;

    cJSON *root = cJSON_Parse(content);
    free(content);
    if (!root || !cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        return false;
    }

    queue_clear(q);
    int size = cJSON_GetArraySize(root);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsObject(item)) continue;

        WaitingVehicle w;
        memset(&w, 0, sizeof(WaitingVehicle));

        cJSON *j_id = cJSON_GetObjectItem(item, "waiting_id");
        cJSON *j_reg = cJSON_GetObjectItem(item, "reg_number");
        cJSON *j_type = cJSON_GetObjectItem(item, "vehicle_type");
        cJSON *j_owner = cJSON_GetObjectItem(item, "owner_name");
        cJSON *j_contact = cJSON_GetObjectItem(item, "contact");
        cJSON *j_time = cJSON_GetObjectItem(item, "request_time");

        if (j_id && cJSON_IsString(j_id)) strncpy(w.waiting_id, j_id->valuestring, sizeof(w.waiting_id) - 1);
        if (j_reg && cJSON_IsString(j_reg)) strncpy(w.reg_number, j_reg->valuestring, sizeof(w.reg_number) - 1);
        if (j_type && cJSON_IsString(j_type)) w.vehicle_type = string_to_vehicle_type(j_type->valuestring);
        if (j_owner && cJSON_IsString(j_owner)) strncpy(w.owner_name, j_owner->valuestring, sizeof(w.owner_name) - 1);
        if (j_contact && cJSON_IsString(j_contact)) strncpy(w.contact, j_contact->valuestring, sizeof(w.contact) - 1);
        if (j_time && cJSON_IsNumber(j_time)) w.request_time = (time_t)j_time->valuedouble;

        queue_enqueue(q, &w);
    }

    cJSON_Delete(root);
    return true;
}

/* --- RESERVATIONS STORAGE --- */

bool storage_save_reservations(const ReservationManager *mgr) {
    if (!mgr) return false;

    cJSON *root = cJSON_CreateArray();
    if (!root) return false;

    size_t count = 0;
    Reservation *arr = reservation_get_all_array(mgr, &count);
    if (arr) {
        for (size_t i = 0; i < count; i++) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "reservation_id", arr[i].reservation_id);
            cJSON_AddStringToObject(item, "reg_number", arr[i].reg_number);
            cJSON_AddStringToObject(item, "vehicle_type", vehicle_type_to_string(arr[i].vehicle_type));
            cJSON_AddStringToObject(item, "slot_id", arr[i].slot_id);
            cJSON_AddStringToObject(item, "customer_name", arr[i].customer_name);
            cJSON_AddStringToObject(item, "contact", arr[i].contact);
            cJSON_AddNumberToObject(item, "reservation_time", (double)arr[i].reservation_time);
            cJSON_AddNumberToObject(item, "expiry_time", (double)arr[i].expiry_time);
            cJSON_AddNumberToObject(item, "status", (int)arr[i].status);
            cJSON_AddStringToObject(item, "res_time_str", arr[i].res_time_str);
            cJSON_AddStringToObject(item, "exp_time_str", arr[i].exp_time_str);
            cJSON_AddItemToArray(root, item);
        }
        free(arr);
    }

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) return false;

    bool ok = storage_atomic_write(FILE_RESERVATIONS, json_str);
    free(json_str);
    return ok;
}

bool storage_load_reservations(ReservationManager *mgr, ParkingLot *lot) {
    if (!mgr) return false;

    char *content = storage_read_file(FILE_RESERVATIONS);
    if (!content) return false;

    cJSON *root = cJSON_Parse(content);
    free(content);
    if (!root || !cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        return false;
    }

    int size = cJSON_GetArraySize(root);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsObject(item)) continue;

        Reservation *r = (Reservation*)calloc(1, sizeof(Reservation));
        if (!r) continue;

        cJSON *j_id = cJSON_GetObjectItem(item, "reservation_id");
        cJSON *j_reg = cJSON_GetObjectItem(item, "reg_number");
        cJSON *j_type = cJSON_GetObjectItem(item, "vehicle_type");
        cJSON *j_slot = cJSON_GetObjectItem(item, "slot_id");
        cJSON *j_name = cJSON_GetObjectItem(item, "customer_name");
        cJSON *j_contact = cJSON_GetObjectItem(item, "contact");
        cJSON *j_res_t = cJSON_GetObjectItem(item, "reservation_time");
        cJSON *j_exp_t = cJSON_GetObjectItem(item, "expiry_time");
        cJSON *j_stat = cJSON_GetObjectItem(item, "status");
        cJSON *j_res_str = cJSON_GetObjectItem(item, "res_time_str");
        cJSON *j_exp_str = cJSON_GetObjectItem(item, "exp_time_str");

        if (j_id && cJSON_IsString(j_id)) strncpy(r->reservation_id, j_id->valuestring, sizeof(r->reservation_id) - 1);
        if (j_reg && cJSON_IsString(j_reg)) strncpy(r->reg_number, j_reg->valuestring, sizeof(r->reg_number) - 1);
        if (j_type && cJSON_IsString(j_type)) r->vehicle_type = string_to_vehicle_type(j_type->valuestring);
        if (j_slot && cJSON_IsString(j_slot)) strncpy(r->slot_id, j_slot->valuestring, sizeof(r->slot_id) - 1);
        if (j_name && cJSON_IsString(j_name)) strncpy(r->customer_name, j_name->valuestring, sizeof(r->customer_name) - 1);
        if (j_contact && cJSON_IsString(j_contact)) strncpy(r->contact, j_contact->valuestring, sizeof(r->contact) - 1);
        if (j_res_t && cJSON_IsNumber(j_res_t)) r->reservation_time = (time_t)j_res_t->valuedouble;
        if (j_exp_t && cJSON_IsNumber(j_exp_t)) r->expiry_time = (time_t)j_exp_t->valuedouble;
        if (j_stat && cJSON_IsNumber(j_stat)) r->status = (ReservationStatus)j_stat->valueint;
        if (j_res_str && cJSON_IsString(j_res_str)) strncpy(r->res_time_str, j_res_str->valuestring, sizeof(r->res_time_str) - 1);
        if (j_exp_str && cJSON_IsString(j_exp_str)) strncpy(r->exp_time_str, j_exp_str->valuestring, sizeof(r->exp_time_str) - 1);

        ll_insert_end(mgr->list, r);

        if (lot && r->status == RESERVATION_ACTIVE && r->slot_id[0] != '\0') {
            parking_reserve_slot(lot, r->slot_id, r->reservation_id);
        }
    }

    cJSON_Delete(root);
    return true;
}

/* --- HISTORY STORAGE --- */

static void free_history_item(void *data) {
    if (data) free(data);
}

bool storage_append_history(const ParkingTransaction *txn) {
    if (!txn) return false;

    LinkedList *history = storage_load_history();
    if (!history) {
        history = ll_create(free_history_item);
    }

    ParkingTransaction *copy = (ParkingTransaction*)malloc(sizeof(ParkingTransaction));
    if (!copy) {
        ll_destroy(history);
        return false;
    }
    *copy = *txn;
    ll_insert_end(history, copy);

    cJSON *root = cJSON_CreateArray();
    size_t count = ll_size(history);
    for (size_t i = 0; i < count; i++) {
        ParkingTransaction *t = (ParkingTransaction*)ll_get_at(history, i);
        if (!t) continue;

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "transaction_id", t->transaction_id);
        cJSON_AddStringToObject(item, "ticket_id", t->ticket_id);
        cJSON_AddStringToObject(item, "reg_number", t->reg_number);
        cJSON_AddStringToObject(item, "vehicle_type", vehicle_type_to_string(t->vehicle_type));
        cJSON_AddStringToObject(item, "slot_id", t->slot_id);
        cJSON_AddStringToObject(item, "owner_name", t->owner_name);
        cJSON_AddStringToObject(item, "contact", t->contact);
        cJSON_AddNumberToObject(item, "entry_time", (double)t->entry_time);
        cJSON_AddNumberToObject(item, "exit_time", (double)t->exit_time);
        cJSON_AddNumberToObject(item, "duration_seconds", (double)t->duration_seconds);
        cJSON_AddNumberToObject(item, "duration_hours", t->duration_hours);
        cJSON_AddNumberToObject(item, "duration_minutes", t->duration_minutes);
        cJSON_AddNumberToObject(item, "total_fee", t->total_fee);
        cJSON_AddStringToObject(item, "entry_time_str", t->entry_time_str);
        cJSON_AddStringToObject(item, "exit_time_str", t->exit_time_str);
        cJSON_AddBoolToObject(item, "is_grace_period", t->is_grace_period);

        cJSON_AddItemToArray(root, item);
    }

    ll_destroy(history);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) return false;

    bool ok = storage_atomic_write(FILE_HISTORY, json_str);
    free(json_str);
    return ok;
}

LinkedList* storage_load_history(void) {
    LinkedList *list = ll_create(free_history_item);
    if (!list) return NULL;

    char *content = storage_read_file(FILE_HISTORY);
    if (!content) return list;

    cJSON *root = cJSON_Parse(content);
    free(content);
    if (!root || !cJSON_IsArray(root)) {
        if (root) cJSON_Delete(root);
        return list;
    }

    int size = cJSON_GetArraySize(root);
    for (int i = 0; i < size; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsObject(item)) continue;

        ParkingTransaction *t = (ParkingTransaction*)calloc(1, sizeof(ParkingTransaction));
        if (!t) continue;

        cJSON *j_txn = cJSON_GetObjectItem(item, "transaction_id");
        cJSON *j_tck = cJSON_GetObjectItem(item, "ticket_id");
        cJSON *j_reg = cJSON_GetObjectItem(item, "reg_number");
        cJSON *j_type = cJSON_GetObjectItem(item, "vehicle_type");
        cJSON *j_slot = cJSON_GetObjectItem(item, "slot_id");
        cJSON *j_owner = cJSON_GetObjectItem(item, "owner_name");
        cJSON *j_contact = cJSON_GetObjectItem(item, "contact");
        cJSON *j_en_t = cJSON_GetObjectItem(item, "entry_time");
        cJSON *j_ex_t = cJSON_GetObjectItem(item, "exit_time");
        cJSON *j_dur = cJSON_GetObjectItem(item, "duration_seconds");
        cJSON *j_fee = cJSON_GetObjectItem(item, "total_fee");
        cJSON *j_en_str = cJSON_GetObjectItem(item, "entry_time_str");
        cJSON *j_ex_str = cJSON_GetObjectItem(item, "exit_time_str");

        if (j_txn && cJSON_IsString(j_txn)) strncpy(t->transaction_id, j_txn->valuestring, sizeof(t->transaction_id) - 1);
        if (j_tck && cJSON_IsString(j_tck)) strncpy(t->ticket_id, j_tck->valuestring, sizeof(t->ticket_id) - 1);
        if (j_reg && cJSON_IsString(j_reg)) strncpy(t->reg_number, j_reg->valuestring, sizeof(t->reg_number) - 1);
        if (j_type && cJSON_IsString(j_type)) t->vehicle_type = string_to_vehicle_type(j_type->valuestring);
        if (j_slot && cJSON_IsString(j_slot)) strncpy(t->slot_id, j_slot->valuestring, sizeof(t->slot_id) - 1);
        if (j_owner && cJSON_IsString(j_owner)) strncpy(t->owner_name, j_owner->valuestring, sizeof(t->owner_name) - 1);
        if (j_contact && cJSON_IsString(j_contact)) strncpy(t->contact, j_contact->valuestring, sizeof(t->contact) - 1);
        if (j_en_t && cJSON_IsNumber(j_en_t)) t->entry_time = (time_t)j_en_t->valuedouble;
        if (j_ex_t && cJSON_IsNumber(j_ex_t)) t->exit_time = (time_t)j_ex_t->valuedouble;
        if (j_dur && cJSON_IsNumber(j_dur)) t->duration_seconds = (int64_t)j_dur->valuedouble;
        if (j_fee && cJSON_IsNumber(j_fee)) t->total_fee = j_fee->valuedouble;
        if (j_en_str && cJSON_IsString(j_en_str)) strncpy(t->entry_time_str, j_en_str->valuestring, sizeof(t->entry_time_str) - 1);
        if (j_ex_str && cJSON_IsString(j_ex_str)) strncpy(t->exit_time_str, j_ex_str->valuestring, sizeof(t->exit_time_str) - 1);

        ll_insert_end(list, t);
    }

    cJSON_Delete(root);
    return list;
}

/* --- SETTINGS STORAGE --- */

bool storage_save_settings(const PricingPolicy *policy) {
    if (!policy) return false;

    cJSON *root = cJSON_CreateObject();
    if (!root) return false;

    cJSON_AddNumberToObject(root, "car_first_hour", policy->car_first_hour);
    cJSON_AddNumberToObject(root, "car_subsequent_hour", policy->car_subsequent_hour);
    cJSON_AddNumberToObject(root, "bike_first_hour", policy->bike_first_hour);
    cJSON_AddNumberToObject(root, "bike_subsequent_hour", policy->bike_subsequent_hour);
    cJSON_AddNumberToObject(root, "accessible_first_hour", policy->accessible_first_hour);
    cJSON_AddNumberToObject(root, "accessible_subsequent_hour", policy->accessible_subsequent_hour);
    cJSON_AddNumberToObject(root, "grace_period_minutes", policy->grace_period_minutes);
    cJSON_AddNumberToObject(root, "min_charge", policy->min_charge);
    cJSON_AddNumberToObject(root, "rounding_minutes", policy->rounding_minutes);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_str) return false;

    bool ok = storage_atomic_write(FILE_SETTINGS, json_str);
    free(json_str);
    return ok;
}

bool storage_load_settings(PricingPolicy *policy) {
    if (!policy) return false;

    char *content = storage_read_file(FILE_SETTINGS);
    if (!content) return false;

    cJSON *root = cJSON_Parse(content);
    free(content);
    if (!root || !cJSON_IsObject(root)) {
        if (root) cJSON_Delete(root);
        return false;
    }

    cJSON *c1 = cJSON_GetObjectItem(root, "car_first_hour");
    cJSON *c2 = cJSON_GetObjectItem(root, "car_subsequent_hour");
    cJSON *b1 = cJSON_GetObjectItem(root, "bike_first_hour");
    cJSON *b2 = cJSON_GetObjectItem(root, "bike_subsequent_hour");
    cJSON *a1 = cJSON_GetObjectItem(root, "accessible_first_hour");
    cJSON *a2 = cJSON_GetObjectItem(root, "accessible_subsequent_hour");
    cJSON *gr = cJSON_GetObjectItem(root, "grace_period_minutes");
    cJSON *mc = cJSON_GetObjectItem(root, "min_charge");
    cJSON *rm = cJSON_GetObjectItem(root, "rounding_minutes");

    if (c1 && cJSON_IsNumber(c1)) policy->car_first_hour = c1->valuedouble;
    if (c2 && cJSON_IsNumber(c2)) policy->car_subsequent_hour = c2->valuedouble;
    if (b1 && cJSON_IsNumber(b1)) policy->bike_first_hour = b1->valuedouble;
    if (b2 && cJSON_IsNumber(b2)) policy->bike_subsequent_hour = b2->valuedouble;
    if (a1 && cJSON_IsNumber(a1)) policy->accessible_first_hour = a1->valuedouble;
    if (a2 && cJSON_IsNumber(a2)) policy->accessible_subsequent_hour = a2->valuedouble;
    if (gr && cJSON_IsNumber(gr)) policy->grace_period_minutes = gr->valueint;
    if (mc && cJSON_IsNumber(mc)) policy->min_charge = mc->valuedouble;
    if (rm && cJSON_IsNumber(rm)) policy->rounding_minutes = rm->valueint;

    cJSON_Delete(root);
    return true;
}

bool storage_save_all(
    ParkingLot *lot,
    const VehicleRegistry *vr,
    const Queue *q,
    const ReservationManager *mgr,
    const PricingPolicy *policy
) {
    bool ok = true;
    if (lot) ok = ok && storage_save_slots(lot);
    if (vr) ok = ok && storage_save_active_vehicles(vr);
    if (q) ok = ok && storage_save_queue(q);
    if (mgr) ok = ok && storage_save_reservations(mgr);
    if (policy) ok = ok && storage_save_settings(policy);
    return ok;
}
