#include "../include/linked_list.h"

LinkedList* ll_create(void (*free_data)(void *data)) {
    LinkedList *list = (LinkedList*)malloc(sizeof(LinkedList));
    if (!list) return NULL;
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->free_data = free_data;
    return list;
}

void ll_clear(LinkedList *list) {
    if (!list) return;
    Node *curr = list->head;
    while (curr) {
        Node *next = curr->next;
        if (list->free_data && curr->data) {
            list->free_data(curr->data);
        }
        free(curr);
        curr = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

void ll_destroy(LinkedList *list) {
    if (!list) return;
    ll_clear(list);
    free(list);
}

bool ll_insert_end(LinkedList *list, void *data) {
    if (!list) return false;
    Node *node = (Node*)malloc(sizeof(Node));
    if (!node) return false;
    node->data = data;
    node->next = NULL;
    node->prev = list->tail;

    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head = node;
    }
    list->tail = node;
    list->size++;
    return true;
}

bool ll_insert_head(LinkedList *list, void *data) {
    if (!list) return false;
    Node *node = (Node*)malloc(sizeof(Node));
    if (!node) return false;
    node->data = data;
    node->prev = NULL;
    node->next = list->head;

    if (list->head) {
        list->head->prev = node;
    } else {
        list->tail = node;
    }
    list->head = node;
    list->size++;
    return true;
}

void* ll_find(LinkedList *list, bool (*predicate)(const void *data, const void *arg), const void *arg) {
    if (!list || !predicate) return NULL;
    Node *curr = list->head;
    while (curr) {
        if (predicate(curr->data, arg)) {
            return curr->data;
        }
        curr = curr->next;
    }
    return NULL;
}

bool ll_delete(LinkedList *list, bool (*predicate)(const void *data, const void *arg), const void *arg) {
    if (!list || !predicate) return false;
    Node *curr = list->head;
    while (curr) {
        if (predicate(curr->data, arg)) {
            if (curr->prev) {
                curr->prev->next = curr->next;
            } else {
                list->head = curr->next;
            }

            if (curr->next) {
                curr->next->prev = curr->prev;
            } else {
                list->tail = curr->prev;
            }

            if (list->free_data && curr->data) {
                list->free_data(curr->data);
            }
            free(curr);
            list->size--;
            return true;
        }
        curr = curr->next;
    }
    return false;
}

void ll_traverse(LinkedList *list, void (*callback)(void *data, void *context), void *context) {
    if (!list || !callback) return;
    Node *curr = list->head;
    while (curr) {
        callback(curr->data, context);
        curr = curr->next;
    }
}

size_t ll_size(const LinkedList *list) {
    return list ? list->size : 0;
}

void* ll_get_at(const LinkedList *list, size_t index) {
    if (!list || index >= list->size) return NULL;
    Node *curr = list->head;
    for (size_t i = 0; i < index && curr; i++) {
        curr = curr->next;
    }
    return curr ? curr->data : NULL;
}
