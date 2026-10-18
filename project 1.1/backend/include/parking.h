#ifndef PARKING_H
#define PARKING_H

#include "common.h"
#include "linked_list.h"

typedef struct {
    char slot_id[MAX_ID_LEN];       /* e.g., "C-01", "M-05", "A-02" */
    int floor;                      /* 1, 2, 3 */
    char zone[MAX_ID_LEN];          /* "Zone A", "Zone B", etc. */
    VehicleType category;           /* CAR, MOTORCYCLE, ACCESSIBLE */
    SlotStatus status;              /* AVAILABLE, OCCUPIED, RESERVED, MAINTENANCE */
    char occupied_reg[MAX_REG_LEN]; /* Active vehicle registration */
    time_t occupied_time;           /* Time entered */
    char reservation_id[MAX_ID_LEN];/* Associated reservation ID if reserved */
    double distance_from_entrance;  /* Illustrative distance in meters */
} ParkingSlot;

typedef struct {
    LinkedList *slot_list;          /* Doubly linked list of all parking slots */
    ParkingSlot *slots_array[MAX_SLOTS]; /* Fast index array */
    size_t total_slots;
#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
} ParkingLot;

/**
 * Initializes parking lot with default slot configurations.
 * Creates cars, motorcycles, and accessible slots across multiple floors.
 */
ParkingLot* parking_lot_create(size_t capacity);

/**
 * Destroys parking lot and frees all slot resources.
 */
void parking_lot_destroy(ParkingLot *lot);

/**
 * Finds a slot by unique slot ID.
 * Time Complexity: O(n) via linked list, or O(1) via cached array
 */
ParkingSlot* parking_find_slot(ParkingLot *lot, const char *slot_id);

/**
 * Finds the nearest/first available slot matching the requested vehicle category.
 * If exact category is not found:
 *   - Accessible vehicle can take Car slot
 *   - Car cannot take Motorcycle slot
 * Returns pointer to available slot, or NULL if full.
 */
ParkingSlot* parking_find_available_slot(ParkingLot *lot, VehicleType type);

/**
 * Allocates an available slot to a vehicle.
 * Enforces no double-allocation.
 */
bool parking_allocate_slot(ParkingLot *lot, const char *slot_id, const char *reg_number, time_t entry_time);

/**
 * Releases an occupied slot and marks it AVAILABLE.
 * Enforces that slot was actually OCCUPIED.
 */
bool parking_release_slot(ParkingLot *lot, const char *slot_id);

/**
 * Marks an AVAILABLE slot as RESERVED.
 * Rejects reservation if slot is already OCCUPIED or RESERVED.
 */
bool parking_reserve_slot(ParkingLot *lot, const char *slot_id, const char *reservation_id);

/**
 * Cancels a reservation on a RESERVED slot and returns it to AVAILABLE.
 */
bool parking_cancel_slot_reservation(ParkingLot *lot, const char *slot_id, const char *reservation_id);

/**
 * Counts slot statuses and returns aggregated breakdown.
 */
void parking_get_stats(ParkingLot *lot, int *out_total, int *out_avail, int *out_occ, int *out_res, int *out_maint);

/**
 * Thread-safety lock and unlock wrappers.
 */
void parking_lock(ParkingLot *lot);
void parking_unlock(ParkingLot *lot);

#endif /* PARKING_H */
