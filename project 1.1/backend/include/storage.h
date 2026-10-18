#ifndef STORAGE_H
#define STORAGE_H

#include "common.h"
#include "parking.h"
#include "vehicle.h"
#include "queue.h"
#include "reservation.h"
#include "billing.h"
#include "linked_list.h"

#define DATA_DIR               "data"
#define FILE_SLOTS             "data/slots.json"
#define FILE_ACTIVE_VEHICLES   "data/active_vehicles.json"
#define FILE_QUEUE             "data/queue.json"
#define FILE_RESERVATIONS      "data/reservations.json"
#define FILE_HISTORY           "data/history.json"
#define FILE_SETTINGS          "data/settings.json"

/**
 * Ensures data directory and all default files exist.
 */
bool storage_init(void);

/**
 * Writes data safely using atomic temp-file replace strategy.
 */
bool storage_atomic_write(const char *filepath, const char *content);

/**
 * Reads entire file content into allocated string (caller frees).
 */
char* storage_read_file(const char *filepath);

/* Slots Persistence */
bool storage_save_slots(ParkingLot *lot);
bool storage_load_slots(ParkingLot *lot);

/* Active Vehicles Persistence */
bool storage_save_active_vehicles(const VehicleRegistry *vr);
bool storage_load_active_vehicles(VehicleRegistry *vr, ParkingLot *lot);

/* Queue Persistence */
bool storage_save_queue(const Queue *q);
bool storage_load_queue(Queue *q);

/* Reservations Persistence */
bool storage_save_reservations(const ReservationManager *mgr);
bool storage_load_reservations(ReservationManager *mgr, ParkingLot *lot);

/* History Persistence */
bool storage_append_history(const ParkingTransaction *txn);
LinkedList* storage_load_history(void);

/* Settings Persistence */
bool storage_save_settings(const PricingPolicy *policy);
bool storage_load_settings(PricingPolicy *policy);

/**
 * Saves all system state in one atomic coordinated sequence.
 */
bool storage_save_all(
    ParkingLot *lot,
    const VehicleRegistry *vr,
    const Queue *q,
    const ReservationManager *mgr,
    const PricingPolicy *policy
);

#endif /* STORAGE_H */
