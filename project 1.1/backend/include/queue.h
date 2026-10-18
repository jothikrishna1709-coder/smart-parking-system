#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

typedef struct WaitingVehicle {
    char waiting_id[MAX_ID_LEN];
    char reg_number[MAX_REG_LEN];
    VehicleType vehicle_type;
    char owner_name[MAX_NAME_LEN];
    char contact[MAX_PHONE_LEN];
    time_t request_time;
} WaitingVehicle;

typedef struct QueueNode {
    WaitingVehicle data;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    size_t count;
} Queue;

/**
 * Creates an empty FIFO linked-list queue.
 * Time Complexity: O(1)
 */
Queue* queue_create(void);

/**
 * Frees all nodes in the queue.
 * Time Complexity: O(n)
 */
void queue_destroy(Queue *q);

/**
 * Enqueues a waiting vehicle at the rear of the queue.
 * Time Complexity: O(1)
 */
bool queue_enqueue(Queue *q, const WaitingVehicle *item);

/**
 * Dequeues the front vehicle from the queue.
 * Time Complexity: O(1)
 */
bool queue_dequeue(Queue *q, WaitingVehicle *out_item);

/**
 * Peeks at the front vehicle without removing it.
 * Time Complexity: O(1)
 */
bool queue_peek(const Queue *q, WaitingVehicle *out_item);

/**
 * Removes a waiting vehicle by ID (for cancellation before allocation).
 * Time Complexity: O(n)
 */
bool queue_remove_by_id(Queue *q, const char *waiting_id);

/**
 * Finds and removes the first eligible waiting vehicle matching the given vehicle type.
 * Preserves FIFO order of all remaining vehicles.
 * Time Complexity: O(n)
 */
bool queue_dequeue_eligible(Queue *q, VehicleType type, WaitingVehicle *out_item);

/**
 * Checks if the queue is empty.
 * Time Complexity: O(1)
 */
bool queue_is_empty(const Queue *q);

/**
 * Returns total count of waiting vehicles.
 * Time Complexity: O(1)
 */
size_t queue_size(const Queue *q);

/**
 * Returns 1-based position of a vehicle in the queue by waiting ID, or 0 if not found.
 * Time Complexity: O(n)
 */
size_t queue_get_position(const Queue *q, const char *waiting_id);

/**
 * Exports all waiting vehicles into an allocated array (caller must free array).
 * Time Complexity: O(n)
 */
WaitingVehicle* queue_to_array(const Queue *q, size_t *out_count);

/**
 * Clears all items in the queue.
 * Time Complexity: O(n)
 */
void queue_clear(Queue *q);

#endif /* QUEUE_H */
