#include "../include/reservation.h"

static void free_reservation(void *data) {
    if (data) {
        free(data);
    }
}

ReservationManager* reservation_mgr_create(void) {
    ReservationManager *mgr = (ReservationManager*)malloc(sizeof(ReservationManager));
    if (!mgr) return NULL;
    mgr->list = ll_create(free_reservation);
    mgr->counter = 100;
    if (!mgr->list) {
        free(mgr);
        return NULL;
    }
    return mgr;
}

void reservation_mgr_destroy(ReservationManager *mgr) {
    if (!mgr) return;
    ll_destroy(mgr->list);
    free(mgr);
}

static bool match_res_id(const void *data, const void *arg) {
    const Reservation *r = (const Reservation*)data;
    const char *id = (const char*)arg;
    return (r && id && strcmp(r->reservation_id, id) == 0);
}

Reservation* reservation_find_by_id(const ReservationManager *mgr, const char *reservation_id) {
    if (!mgr || !reservation_id) return NULL;
    return (Reservation*)ll_find(mgr->list, match_res_id, reservation_id);
}

Reservation* reservation_find_active_by_reg(const ReservationManager *mgr, const char *reg_number) {
    if (!mgr || !reg_number) return NULL;
    char norm[MAX_REG_LEN];
    normalize_registration(reg_number, norm, sizeof(norm));

    size_t count = ll_size(mgr->list);
    for (size_t i = 0; i < count; i++) {
        Reservation *r = (Reservation*)ll_get_at(mgr->list, i);
        if (r && r->status == RESERVATION_ACTIVE) {
            char r_norm[MAX_REG_LEN];
            normalize_registration(r->reg_number, r_norm, sizeof(r_norm));
            if (strcmp(norm, r_norm) == 0) {
                return r;
            }
        }
    }
    return NULL;
}

bool reservation_create(
    ReservationManager *mgr,
    ParkingLot *lot,
    const char *reg_number,
    VehicleType type,
    const char *slot_id_preferred,
    const char *customer_name,
    const char *contact,
    int valid_hours,
    Reservation *out_res
) {
    if (!mgr || !lot || !reg_number) return false;

    /* Prevent duplicate active reservations for same vehicle */
    if (reservation_find_active_by_reg(mgr, reg_number) != NULL) {
        return false;
    }

    parking_lock(lot);

    ParkingSlot *target_slot = NULL;
    if (slot_id_preferred && strlen(slot_id_preferred) > 0) {
        target_slot = parking_find_slot(lot, slot_id_preferred);
        if (!target_slot || target_slot->status != SLOT_AVAILABLE) {
            parking_unlock(lot);
            return false; /* Preferred slot not available */
        }
    } else {
        target_slot = parking_find_available_slot(lot, type);
        if (!target_slot) {
            parking_unlock(lot);
            return false; /* No slot available */
        }
    }

    Reservation *res = (Reservation*)calloc(1, sizeof(Reservation));
    if (!res) {
        parking_unlock(lot);
        return false;
    }

    time_t now = time(NULL);
    struct tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &now);
#else
    localtime_r(&now, &tm_info);
#endif

    snprintf(res->reservation_id, sizeof(res->reservation_id), "RES-%04d%02d%02d-%03zu",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday, ++mgr->counter);

    normalize_registration(reg_number, res->reg_number, sizeof(res->reg_number));
    res->vehicle_type = type;
    strncpy(res->slot_id, target_slot->slot_id, sizeof(res->slot_id) - 1);
    if (customer_name) strncpy(res->customer_name, customer_name, sizeof(res->customer_name) - 1);
    if (contact) strncpy(res->contact, contact, sizeof(res->contact) - 1);

    if (valid_hours <= 0) valid_hours = 2;
    res->reservation_time = now;
    res->expiry_time = now + ((time_t)valid_hours * 3600);
    res->status = RESERVATION_ACTIVE;

    format_iso_time(res->reservation_time, res->res_time_str, sizeof(res->res_time_str));
    format_iso_time(res->expiry_time, res->exp_time_str, sizeof(res->exp_time_str));

    /* Mark slot reserved */
    parking_reserve_slot(lot, target_slot->slot_id, res->reservation_id);
    parking_unlock(lot);

    ll_insert_end(mgr->list, res);

    if (out_res) {
        *out_res = *res;
    }
    return true;
}

bool reservation_cancel(ReservationManager *mgr, ParkingLot *lot, const char *reservation_id) {
    if (!mgr || !lot || !reservation_id) return false;

    Reservation *r = reservation_find_by_id(mgr, reservation_id);
    if (!r || r->status != RESERVATION_ACTIVE) return false;

    parking_lock(lot);
    parking_cancel_slot_reservation(lot, r->slot_id, r->reservation_id);
    parking_unlock(lot);

    r->status = RESERVATION_CANCELLED;
    return true;
}

bool reservation_fulfill(ReservationManager *mgr, const char *reservation_id) {
    if (!mgr || !reservation_id) return false;
    Reservation *r = reservation_find_by_id(mgr, reservation_id);
    if (!r || r->status != RESERVATION_ACTIVE) return false;
    r->status = RESERVATION_FULFILLED;
    return true;
}

void reservation_check_expirations(ReservationManager *mgr, ParkingLot *lot) {
    if (!mgr || !lot) return;
    time_t now = time(NULL);

    parking_lock(lot);
    size_t count = ll_size(mgr->list);
    for (size_t i = 0; i < count; i++) {
        Reservation *r = (Reservation*)ll_get_at(mgr->list, i);
        if (r && r->status == RESERVATION_ACTIVE && now > r->expiry_time) {
            r->status = RESERVATION_EXPIRED;
            parking_cancel_slot_reservation(lot, r->slot_id, r->reservation_id);
        }
    }
    parking_unlock(lot);
}

Reservation* reservation_get_all_array(const ReservationManager *mgr, size_t *out_count) {
    if (!mgr || !out_count) return NULL;
    size_t count = ll_size(mgr->list);
    *out_count = count;
    if (count == 0) return NULL;

    Reservation *arr = (Reservation*)malloc(sizeof(Reservation) * count);
    if (!arr) return NULL;

    for (size_t i = 0; i < count; i++) {
        Reservation *r = (Reservation*)ll_get_at(mgr->list, i);
        if (r) {
            arr[i] = *r;
        }
    }
    return arr;
}
