#include "../include/queue.h"

Queue* queue_create(void) {
    Queue *q = (Queue*)malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
    return q;
}

void queue_clear(Queue *q) {
    if (!q) return;
    QueueNode *curr = q->front;
    while (curr) {
        QueueNode *next = curr->next;
        free(curr);
        curr = next;
    }
    q->front = NULL;
    q->rear = NULL;
    q->count = 0;
}

void queue_destroy(Queue *q) {
    if (!q) return;
    queue_clear(q);
    free(q);
}

bool queue_enqueue(Queue *q, const WaitingVehicle *item) {
    if (!q || !item) return false;
    QueueNode *node = (QueueNode*)malloc(sizeof(QueueNode));
    if (!node) return false;

    node->data = *item;
    node->next = NULL;

    if (q->rear) {
        q->rear->next = node;
    } else {
        q->front = node;
    }
    q->rear = node;
    q->count++;
    return true;
}

bool queue_dequeue(Queue *q, WaitingVehicle *out_item) {
    if (!q || !q->front) return false;
    QueueNode *target = q->front;
    if (out_item) {
        *out_item = target->data;
    }
    q->front = target->next;
    if (!q->front) {
        q->rear = NULL;
    }
    free(target);
    q->count--;
    return true;
}

bool queue_peek(const Queue *q, WaitingVehicle *out_item) {
    if (!q || !q->front || !out_item) return false;
    *out_item = q->front->data;
    return true;
}

bool queue_remove_by_id(Queue *q, const char *waiting_id) {
    if (!q || !q->front || !waiting_id) return false;

    QueueNode *curr = q->front;
    QueueNode *prev = NULL;

    while (curr) {
        if (strcmp(curr->data.waiting_id, waiting_id) == 0) {
            if (prev) {
                prev->next = curr->next;
            } else {
                q->front = curr->next;
            }

            if (curr == q->rear) {
                q->rear = prev;
            }

            free(curr);
            q->count--;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}

bool queue_dequeue_eligible(Queue *q, VehicleType type, WaitingVehicle *out_item) {
    if (!q || !q->front) return false;

    QueueNode *curr = q->front;
    QueueNode *prev = NULL;

    while (curr) {
        /* Check if vehicle type matches slot type */
        bool eligible = false;
        if (curr->data.vehicle_type == type) {
            eligible = true;
        } else if (type == VEHICLE_CAR && curr->data.vehicle_type == VEHICLE_ACCESSIBLE) {
            /* Accessible vehicles can also take regular car slots if needed */
            eligible = true;
        }

        if (eligible) {
            if (out_item) {
                *out_item = curr->data;
            }

            if (prev) {
                prev->next = curr->next;
            } else {
                q->front = curr->next;
            }

            if (curr == q->rear) {
                q->rear = prev;
            }

            free(curr);
            q->count--;
            return true;
        }

        prev = curr;
        curr = curr->next;
    }
    return false;
}

bool queue_is_empty(const Queue *q) {
    return (!q || q->count == 0);
}

size_t queue_size(const Queue *q) {
    return q ? q->count : 0;
}

size_t queue_get_position(const Queue *q, const char *waiting_id) {
    if (!q || !waiting_id) return 0;
    size_t pos = 1;
    QueueNode *curr = q->front;
    while (curr) {
        if (strcmp(curr->data.waiting_id, waiting_id) == 0) {
            return pos;
        }
        pos++;
        curr = curr->next;
    }
    return 0;
}

WaitingVehicle* queue_to_array(const Queue *q, size_t *out_count) {
    if (!q || !out_count) return NULL;
    *out_count = q->count;
    if (q->count == 0) return NULL;

    WaitingVehicle *arr = (WaitingVehicle*)malloc(sizeof(WaitingVehicle) * q->count);
    if (!arr) return NULL;

    QueueNode *curr = q->front;
    size_t i = 0;
    while (curr && i < q->count) {
        arr[i++] = curr->data;
        curr = curr->next;
    }
    return arr;
}
