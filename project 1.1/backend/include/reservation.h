#ifndef RESERVATION_H
#define RESERVATION_H

#include "common.h"
#include "parking.h"
#include "linked_list.h"

typedef struct {
    char reservation_id[MAX_ID_LEN];
    char reg_number[MAX_REG_LEN];
    VehicleType vehicle_type;
    char slot_id[MAX_ID_LEN];
    char customer_name[MAX_NAME_LEN];
    char contact[MAX_PHONE_LEN];
    time_t reservation_time;
    time_t expiry_time;
    ReservationStatus status;
    char res_time_str[32];
    char exp_time_str[32];
} Reservation;

typedef struct {
    LinkedList *list;
    size_t counter;
} ReservationManager;

/**
 * Creates reservation manager.
 */
ReservationManager* reservation_mgr_create(void);

/**
 * Destroys reservation manager.
 */
void reservation_mgr_destroy(ReservationManager *mgr);

/**
 * Creates and confirms a slot reservation.
 * Enforces conflict checks and prevents overwriting occupied or already reserved slots.
 */
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
);

/**
 * Cancels an active reservation and frees the reserved slot.
 */
bool reservation_cancel(ReservationManager *mgr, ParkingLot *lot, const char *reservation_id);

/**
 * Marks reservation as fulfilled when the vehicle arrives and enters the slot.
 */
bool reservation_fulfill(ReservationManager *mgr, const char *reservation_id);

/**
 * Finds reservation by ID.
 */
Reservation* reservation_find_by_id(const ReservationManager *mgr, const char *reservation_id);

/**
 * Finds active reservation by vehicle registration number.
 */
Reservation* reservation_find_active_by_reg(const ReservationManager *mgr, const char *reg_number);

/**
 * Checks and updates expired reservations.
 */
void reservation_check_expirations(ReservationManager *mgr, ParkingLot *lot);

/**
 * Exports all reservations to array.
 */
Reservation* reservation_get_all_array(const ReservationManager *mgr, size_t *out_count);

#endif /* RESERVATION_H */
