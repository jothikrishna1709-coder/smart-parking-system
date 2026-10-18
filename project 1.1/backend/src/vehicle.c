#include "../include/vehicle.h"

static void free_active_vehicle(void *data) {
    if (data) {
        free(data);
    }
}

VehicleRegistry* vehicle_registry_create(void) {
    VehicleRegistry *vr = (VehicleRegistry*)malloc(sizeof(VehicleRegistry));
    if (!vr) return NULL;

    vr->active_list = ll_create(free_active_vehicle);
    vr->active_hash = ht_create(HT_INITIAL_CAPACITY, NULL); /* Hash table does not free since list owns pointer */
    vr->ticket_counter = 1000;

    if (!vr->active_list || !vr->active_hash) {
        if (vr->active_list) ll_destroy(vr->active_list);
        if (vr->active_hash) ht_destroy(vr->active_hash);
        free(vr);
        return NULL;
    }
    return vr;
}

void vehicle_registry_destroy(VehicleRegistry *vr) {
    if (!vr) return;
    if (vr->active_hash) ht_destroy(vr->active_hash);
    if (vr->active_list) ll_destroy(vr->active_list);
    free(vr);
}

bool vehicle_validate_registration(const char *reg) {
    if (!reg) return false;
    char norm[MAX_REG_LEN];
    normalize_registration(reg, norm, sizeof(norm));
    size_t len = strlen(norm);
    if (len < 3 || len > 14) return false;

    for (size_t i = 0; i < len; i++) {
        if (!isalnum((unsigned char)norm[i])) {
            return false;
        }
    }
    return true;
}

void vehicle_generate_ticket_id(VehicleRegistry *vr, char *out_buf, size_t buf_len) {
    if (!out_buf || buf_len == 0) return;
    time_t now = time(NULL);
    struct tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &now);
#else
    localtime_r(&now, &tm_info);
#endif
    size_t count = vr ? ++vr->ticket_counter : 1001;
    snprintf(out_buf, buf_len, "TCK-%04d%02d%02d-%04zu",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday, count);
}

bool vehicle_is_parked(const VehicleRegistry *vr, const char *reg_number) {
    if (!vr || !vr->active_hash || !reg_number) return false;
    return ht_contains(vr->active_hash, reg_number);
}

ActiveVehicle* vehicle_find_by_reg(const VehicleRegistry *vr, const char *reg_number) {
    if (!vr || !vr->active_hash || !reg_number) return NULL;
    return (ActiveVehicle*)ht_search(vr->active_hash, reg_number);
}

bool vehicle_register_entry(VehicleRegistry *vr, const ActiveVehicle *vehicle) {
    if (!vr || !vehicle) return false;

    /* Prevent duplicate registration */
    if (vehicle_is_parked(vr, vehicle->reg_number)) {
        return false;
    }

    ActiveVehicle *v = (ActiveVehicle*)malloc(sizeof(ActiveVehicle));
    if (!v) return false;
    *v = *vehicle;

    /* Normalize registration */
    normalize_registration(vehicle->reg_number, v->reg_number, sizeof(v->reg_number));

    /* Insert into linked list */
    if (!ll_insert_end(vr->active_list, v)) {
        free(v);
        return false;
    }

    /* Insert pointer into hash table for O(1) lookup */
    if (!ht_insert(vr->active_hash, v->reg_number, v)) {
        /* Rollback list insert */
        ll_delete(vr->active_list, NULL, NULL);
        return false;
    }

    return true;
}

static bool match_active_reg(const void *data, const void *arg) {
    const ActiveVehicle *v = (const ActiveVehicle*)data;
    const char *reg = (const char*)arg;
    if (!v || !reg) return false;

    char norm1[MAX_REG_LEN], norm2[MAX_REG_LEN];
    normalize_registration(v->reg_number, norm1, sizeof(norm1));
    normalize_registration(reg, norm2, sizeof(norm2));
    return (strcmp(norm1, norm2) == 0);
}

bool vehicle_remove_active(VehicleRegistry *vr, const char *reg_number, ActiveVehicle *out_vehicle) {
    if (!vr || !reg_number) return false;

    ActiveVehicle *v = vehicle_find_by_reg(vr, reg_number);
    if (!v) return false;

    if (out_vehicle) {
        *out_vehicle = *v;
    }

    /* Delete from hash table */
    ht_delete(vr->active_hash, reg_number);

    /* Delete from linked list and free memory */
    return ll_delete(vr->active_list, match_active_reg, reg_number);
}

size_t vehicle_count_active(const VehicleRegistry *vr) {
    if (!vr || !vr->active_list) return 0;
    return ll_size(vr->active_list);
}

ActiveVehicle* vehicle_get_active_array(const VehicleRegistry *vr, size_t *out_count) {
    if (!vr || !out_count) return NULL;
    size_t count = ll_size(vr->active_list);
    *out_count = count;
    if (count == 0) return NULL;

    ActiveVehicle *arr = (ActiveVehicle*)malloc(sizeof(ActiveVehicle) * count);
    if (!arr) return NULL;

    for (size_t i = 0; i < count; i++) {
        ActiveVehicle *v = (ActiveVehicle*)ll_get_at(vr->active_list, i);
        if (v) {
            arr[i] = *v;
        }
    }
    return arr;
}
