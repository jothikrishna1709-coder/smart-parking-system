#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "common.h"

#define HT_INITIAL_CAPACITY 64
#define HT_MAX_LOAD_FACTOR  0.75

typedef struct HashNode {
    char key[MAX_REG_LEN];       /* Vehicle registration number */
    void *value;                 /* Pointer to ActiveVehicle or Slot */
    struct HashNode *next;       /* Separate chaining collision resolution */
} HashNode;

typedef struct {
    HashNode **buckets;
    size_t capacity;
    size_t count;
    size_t collisions;
    void (*free_value)(void *value);
} HashTable;

typedef struct {
    size_t capacity;
    size_t count;
    size_t collisions;
    double load_factor;
    size_t max_chain_length;
} HashTableStats;

/**
 * Creates an empty hash table with default capacity.
 * Time Complexity: O(capacity)
 */
HashTable* ht_create(size_t initial_capacity, void (*free_value)(void *value));

/**
 * Destroys hash table and frees all nodes and values.
 * Time Complexity: O(capacity + count)
 */
void ht_destroy(HashTable *ht);

/**
 * Computes DJB2 hash for string key.
 * Time Complexity: O(len(key))
 */
uint64_t ht_hash(const char *key, size_t capacity);

/**
 * Inserts a key-value pair. Returns false if duplicate key or memory failure.
 * Time Complexity: Average O(1), Worst O(n)
 */
bool ht_insert(HashTable *ht, const char *key, void *value);

/**
 * Searches for a key and returns pointer to value, or NULL if not found.
 * Time Complexity: Average O(1), Worst O(n)
 */
void* ht_search(const HashTable *ht, const char *key);

/**
 * Deletes an entry by key and optionally frees its value.
 * Time Complexity: Average O(1), Worst O(n)
 */
bool ht_delete(HashTable *ht, const char *key);

/**
 * Checks if key exists.
 * Time Complexity: Average O(1), Worst O(n)
 */
bool ht_contains(const HashTable *ht, const char *key);

/**
 * Dynamically resizes hash table and rehashes existing entries.
 * Time Complexity: O(capacity + count)
 */
bool ht_resize(HashTable *ht, size_t new_capacity);

/**
 * Retrieves diagnostic statistics for DSA reporting.
 * Time Complexity: O(capacity + count)
 */
HashTableStats ht_get_stats(const HashTable *ht);

/**
 * Clears all entries from the hash table.
 * Time Complexity: O(capacity + count)
 */
void ht_clear(HashTable *ht);

#endif /* HASH_TABLE_H */
