#ifndef VEHICLE_H
#define VEHICLE_H

#include "common.h"
#include "parking.h"
#include "linked_list.h"
#include "hash_table.h"
#include "queue.h"

typedef struct {
    char ticket_id[MAX_ID_LEN];      /* Unique entry ticket e.g. "TCK-..." */
    char reg_number[MAX_REG_LEN];    /* Vehicle registration plate */
    VehicleType vehicle_type;        /* CAR, MOTORCYCLE, ACCESSIBLE */
    char slot_id[MAX_ID_LEN];        /* Allocated slot */
    char owner_name[MAX_NAME_LEN];
    char contact[MAX_PHONE_LEN];
    char reservation_id[MAX_ID_LEN];
    time_t entry_time;
    char entry_time_str[32];
} ActiveVehicle;

typedef struct {
    LinkedList *active_list;         /* Doubly linked list of active parked vehicles */
    HashTable *active_hash;          /* Hash table for O(1) reg_number lookup */
    size_t ticket_counter;
} VehicleRegistry;

/**
 * Creates vehicle registry.
 */
VehicleRegistry* vehicle_registry_create(void);

/**
 * Destroys registry and frees all active vehicle memory.
 */
void vehicle_registry_destroy(VehicleRegistry *vr);

/**
 * Validates vehicle registration number format (must be 3-12 alphanumeric characters).
 */
bool vehicle_validate_registration(const char *reg);

/**
 * Generates unique parking ticket ID.
 */
void vehicle_generate_ticket_id(VehicleRegistry *vr, char *out_buf, size_t buf_len);

/**
 * Checks if a vehicle with the given registration is already parked.
 * Time Complexity: O(1) via hash table.
 */
bool vehicle_is_parked(const VehicleRegistry *vr, const char *reg_number);

/**
 * Finds active vehicle record by registration number.
 * Time Complexity: O(1) via hash table.
 */
ActiveVehicle* vehicle_find_by_reg(const VehicleRegistry *vr, const char *reg_number);

/**
 * Registers an active vehicle entry.
 * Time Complexity: O(1)
 */
bool vehicle_register_entry(VehicleRegistry *vr, const ActiveVehicle *vehicle);

/**
 * Removes an active vehicle upon checkout.
 * Time Complexity: O(1) in hash table, O(n) in linked list.
 */
bool vehicle_remove_active(VehicleRegistry *vr, const char *reg_number, ActiveVehicle *out_vehicle);

/**
 * Returns total number of active parked vehicles.
 */
size_t vehicle_count_active(const VehicleRegistry *vr);

/**
 * Exports active vehicles to an array for searching and sorting.
 */
ActiveVehicle* vehicle_get_active_array(const VehicleRegistry *vr, size_t *out_count);

#endif /* VEHICLE_H */
