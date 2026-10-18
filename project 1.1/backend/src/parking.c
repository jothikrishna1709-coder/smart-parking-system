#include "../include/parking.h"

static void free_slot_node(void *data) {
    if (data) {
        free(data);
    }
}

ParkingLot* parking_lot_create(size_t capacity) {
    if (capacity == 0 || capacity > MAX_SLOTS) {
        capacity = 40; /* Default 40 slots */
    }

    ParkingLot *lot = (ParkingLot*)malloc(sizeof(ParkingLot));
    if (!lot) return NULL;

    lot->slot_list = ll_create(free_slot_node);
    if (!lot->slot_list) {
        free(lot);
        return NULL;
    }

    lot->total_slots = 0;
    for (size_t i = 0; i < MAX_SLOTS; i++) {
        lot->slots_array[i] = NULL;
    }

#ifdef _WIN32
    InitializeCriticalSection(&lot->lock);
#else
    pthread_mutex_init(&lot->lock, NULL);
#endif

    /* Setup initial slots:
     * - Car slots: C-01 to C-24 (Floor 1: 1-12, Floor 2: 13-24)
     * - Motorcycle slots: M-01 to M-10 (Floor 1)
     * - Accessible slots: A-01 to A-06 (Floor 1, nearest to entrance)
     */
    int car_count = (int)(capacity * 0.60);
    int bike_count = (int)(capacity * 0.25);
    int access_count = capacity - car_count - bike_count;
    if (access_count < 2) access_count = 2;

    int idx = 0;

    /* Accessible slots first (closest to entrance) */
    for (int i = 1; i <= access_count && idx < (int)capacity; i++) {
        ParkingSlot *s = (ParkingSlot*)calloc(1, sizeof(ParkingSlot));
        snprintf(s->slot_id, sizeof(s->slot_id), "A-%02d", i);
        s->floor = 1;
        snprintf(s->zone, sizeof(s->zone), "Zone C (Accessible)");
        s->category = VEHICLE_ACCESSIBLE;
        s->status = SLOT_AVAILABLE;
        s->distance_from_entrance = 8.0 + (i * 2.0);

        ll_insert_end(lot->slot_list, s);
        lot->slots_array[idx++] = s;
    }

    /* Motorcycle slots */
    for (int i = 1; i <= bike_count && idx < (int)capacity; i++) {
        ParkingSlot *s = (ParkingSlot*)calloc(1, sizeof(ParkingSlot));
        snprintf(s->slot_id, sizeof(s->slot_id), "M-%02d", i);
        s->floor = 1;
        snprintf(s->zone, sizeof(s->zone), "Zone B (Bikes)");
        s->category = VEHICLE_MOTORCYCLE;
        s->status = SLOT_AVAILABLE;
        s->distance_from_entrance = 15.0 + (i * 2.5);

        ll_insert_end(lot->slot_list, s);
        lot->slots_array[idx++] = s;
    }

    /* Car slots */
    for (int i = 1; i <= car_count && idx < (int)capacity; i++) {
        ParkingSlot *s = (ParkingSlot*)calloc(1, sizeof(ParkingSlot));
        snprintf(s->slot_id, sizeof(s->slot_id), "C-%02d", i);
        s->floor = (i <= car_count / 2) ? 1 : 2;
        snprintf(s->zone, sizeof(s->zone), s->floor == 1 ? "Zone A (Ground Cars)" : "Zone D (Floor 2 Cars)");
        s->category = VEHICLE_CAR;
        s->status = SLOT_AVAILABLE;
        s->distance_from_entrance = 20.0 + (i * 3.0);

        ll_insert_end(lot->slot_list, s);
        lot->slots_array[idx++] = s;
    }

    lot->total_slots = idx;
    return lot;
}

void parking_lot_destroy(ParkingLot *lot) {
    if (!lot) return;
    parking_lock(lot);
    ll_destroy(lot->slot_list);
    lot->slot_list = NULL;
    lot->total_slots = 0;
    parking_unlock(lot);

#ifdef _WIN32
    DeleteCriticalSection(&lot->lock);
#else
    pthread_mutex_destroy(&lot->lock);
#endif
    free(lot);
}

void parking_lock(ParkingLot *lot) {
    if (!lot) return;
#ifdef _WIN32
    EnterCriticalSection(&lot->lock);
#else
    pthread_mutex_lock(&lot->lock);
#endif
}

void parking_unlock(ParkingLot *lot) {
    if (!lot) return;
#ifdef _WIN32
    LeaveCriticalSection(&lot->lock);
#else
    pthread_mutex_unlock(&lot->lock);
#endif
}

static bool match_slot_id(const void *data, const void *arg) {
    const ParkingSlot *s = (const ParkingSlot*)data;
    const char *id = (const char*)arg;
    return (s && id && stricmp(s->slot_id, id) == 0);
}

ParkingSlot* parking_find_slot(ParkingLot *lot, const char *slot_id) {
    if (!lot || !slot_id) return NULL;
    return (ParkingSlot*)ll_find(lot->slot_list, match_slot_id, slot_id);
}

ParkingSlot* parking_find_available_slot(ParkingLot *lot, VehicleType type) {
    if (!lot) return NULL;

    /* First pass: exact category match */
    for (size_t i = 0; i < lot->total_slots; i++) {
        ParkingSlot *s = lot->slots_array[i];
        if (s && s->status == SLOT_AVAILABLE && s->category == type) {
            return s;
        }
    }

    /* Second pass: accessible vehicles can take regular car slots */
    if (type == VEHICLE_ACCESSIBLE) {
        for (size_t i = 0; i < lot->total_slots; i++) {
            ParkingSlot *s = lot->slots_array[i];
            if (s && s->status == SLOT_AVAILABLE && s->category == VEHICLE_CAR) {
                return s;
            }
        }
    }

    return NULL;
}

bool parking_allocate_slot(ParkingLot *lot, const char *slot_id, const char *reg_number, time_t entry_time) {
    if (!lot || !slot_id || !reg_number) return false;

    ParkingSlot *s = parking_find_slot(lot, slot_id);
    if (!s) return false;

    /* Prevent double allocation or overwriting reserved slot unless by owner */
    if (s->status == SLOT_OCCUPIED || s->status == SLOT_MAINTENANCE) {
        return false;
    }

    s->status = SLOT_OCCUPIED;
    strncpy(s->occupied_reg, reg_number, sizeof(s->occupied_reg) - 1);
    s->occupied_reg[sizeof(s->occupied_reg) - 1] = '\0';
    s->occupied_time = entry_time;
    s->reservation_id[0] = '\0'; /* Clear reservation once occupied */
    return true;
}

bool parking_release_slot(ParkingLot *lot, const char *slot_id) {
    if (!lot || !slot_id) return false;

    ParkingSlot *s = parking_find_slot(lot, slot_id);
    if (!s) return false;

    /* Prevent release of slot that is not occupied */
    if (s->status != SLOT_OCCUPIED) {
        return false;
    }

    s->status = SLOT_AVAILABLE;
    s->occupied_reg[0] = '\0';
    s->occupied_time = 0;
    s->reservation_id[0] = '\0';
    return true;
}

bool parking_reserve_slot(ParkingLot *lot, const char *slot_id, const char *reservation_id) {
    if (!lot || !slot_id || !reservation_id) return false;

    ParkingSlot *s = parking_find_slot(lot, slot_id);
    if (!s) return false;

    /* Reservation must not silently overwrite an occupied or already reserved slot */
    if (s->status != SLOT_AVAILABLE) {
        return false;
    }

    s->status = SLOT_RESERVED;
    strncpy(s->reservation_id, reservation_id, sizeof(s->reservation_id) - 1);
    s->reservation_id[sizeof(s->reservation_id) - 1] = '\0';
    return true;
}

bool parking_cancel_slot_reservation(ParkingLot *lot, const char *slot_id, const char *reservation_id) {
    if (!lot || !slot_id) return false;

    ParkingSlot *s = parking_find_slot(lot, slot_id);
    if (!s) return false;

    if (s->status != SLOT_RESERVED) {
        return false;
    }

    if (reservation_id && s->reservation_id[0] != '\0' && strcmp(s->reservation_id, reservation_id) != 0) {
        return false;
    }

    s->status = SLOT_AVAILABLE;
    s->reservation_id[0] = '\0';
    return true;
}

void parking_get_stats(ParkingLot *lot, int *out_total, int *out_avail, int *out_occ, int *out_res, int *out_maint) {
    int total = 0, avail = 0, occ = 0, res = 0, maint = 0;
    if (lot) {
        total = (int)lot->total_slots;
        for (size_t i = 0; i < lot->total_slots; i++) {
            ParkingSlot *s = lot->slots_array[i];
            if (!s) continue;
            switch (s->status) {
                case SLOT_AVAILABLE: avail++; break;
                case SLOT_OCCUPIED: occ++; break;
                case SLOT_RESERVED: res++; break;
                case SLOT_MAINTENANCE: maint++; break;
            }
        }
    }
    if (out_total) *out_total = total;
    if (out_avail) *out_avail = avail;
    if (out_occ) *out_occ = occ;
    if (out_res) *out_res = res;
    if (out_maint) *out_maint = maint;
}
