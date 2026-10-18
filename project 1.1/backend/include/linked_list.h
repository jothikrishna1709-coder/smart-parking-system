#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "common.h"

typedef struct Node {
    void *data;
    struct Node *next;
    struct Node *prev;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    size_t size;
    void (*free_data)(void *data);
} LinkedList;

/**
 * Creates a new doubly linked list.
 * Time Complexity: O(1)
 */
LinkedList* ll_create(void (*free_data)(void *data));

/**
 * Frees all nodes and memory in the list.
 * Time Complexity: O(n)
 */
void ll_destroy(LinkedList *list);

/**
 * Appends data to the tail of the list.
 * Time Complexity: O(1)
 */
bool ll_insert_end(LinkedList *list, void *data);

/**
 * Prepends data to the head of the list.
 * Time Complexity: O(1)
 */
bool ll_insert_head(LinkedList *list, void *data);

/**
 * Finds first element matching the predicate.
 * Time Complexity: O(n)
 */
void* ll_find(LinkedList *list, bool (*predicate)(const void *data, const void *arg), const void *arg);

/**
 * Deletes first element matching predicate and frees it.
 * Time Complexity: O(n)
 */
bool ll_delete(LinkedList *list, bool (*predicate)(const void *data, const void *arg), const void *arg);

/**
 * Traverses list and executes callback for each item.
 * Time Complexity: O(n)
 */
void ll_traverse(LinkedList *list, void (*callback)(void *data, void *context), void *context);

/**
 * Returns number of elements.
 * Time Complexity: O(1)
 */
size_t ll_size(const LinkedList *list);

/**
 * Retrieves data at 0-based index.
 * Time Complexity: O(n)
 */
void* ll_get_at(const LinkedList *list, size_t index);

/**
 * Clears all nodes in the list without destroying the list container itself.
 * Time Complexity: O(n)
 */
void ll_clear(LinkedList *list);

#endif /* LINKED_LIST_H */
