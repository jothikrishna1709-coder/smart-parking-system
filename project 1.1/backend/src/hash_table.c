#include "../include/hash_table.h"

HashTable* ht_create(size_t initial_capacity, void (*free_value)(void *value)) {
    if (initial_capacity == 0) {
        initial_capacity = HT_INITIAL_CAPACITY;
    }
    HashTable *ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) return NULL;

    ht->buckets = (HashNode**)calloc(initial_capacity, sizeof(HashNode*));
    if (!ht->buckets) {
        free(ht);
        return NULL;
    }

    ht->capacity = initial_capacity;
    ht->count = 0;
    ht->collisions = 0;
    ht->free_value = free_value;
    return ht;
}

uint64_t ht_hash(const char *key, size_t capacity) {
    if (!key || capacity == 0) return 0;
    /* DJB2 hash algorithm with case-insensitivity */
    uint64_t hash = 5381;
    while (*key) {
        int c = tolower((unsigned char)*key);
        hash = ((hash << 5) + hash) + (uint64_t)c; /* hash * 33 + c */
        key++;
    }
    return hash % capacity;
}

bool ht_resize(HashTable *ht, size_t new_capacity) {
    if (!ht || new_capacity <= ht->count) return false;

    HashNode **new_buckets = (HashNode**)calloc(new_capacity, sizeof(HashNode*));
    if (!new_buckets) return false;

    size_t new_collisions = 0;

    for (size_t i = 0; i < ht->capacity; i++) {
        HashNode *curr = ht->buckets[i];
        while (curr) {
            HashNode *next = curr->next;

            uint64_t new_idx = ht_hash(curr->key, new_capacity);
            if (new_buckets[new_idx] != NULL) {
                new_collisions++;
            }
            curr->next = new_buckets[new_idx];
            new_buckets[new_idx] = curr;

            curr = next;
        }
    }

    free(ht->buckets);
    ht->buckets = new_buckets;
    ht->capacity = new_capacity;
    /* Maintain cumulative collision count or current chain collisions */
    if (new_collisions > ht->collisions) {
        ht->collisions = new_collisions;
    }
    return true;
}

bool ht_insert(HashTable *ht, const char *key, void *value) {
    if (!ht || !key) return false;

    char norm_key[MAX_REG_LEN];
    normalize_registration(key, norm_key, sizeof(norm_key));
    if (strlen(norm_key) == 0) return false;

    /* Check load factor and auto-resize if needed */
    if ((double)(ht->count + 1) / (double)ht->capacity > HT_MAX_LOAD_FACTOR) {
        ht_resize(ht, ht->capacity * 2);
    }

    uint64_t idx = ht_hash(norm_key, ht->capacity);
    HashNode *curr = ht->buckets[idx];

    /* Check for duplicate key */
    while (curr) {
        if (stricmp(curr->key, norm_key) == 0) {
            return false; /* Duplicate key rejected */
        }
        curr = curr->next;
    }

    /* Create new node */
    HashNode *node = (HashNode*)malloc(sizeof(HashNode));
    if (!node) return false;

    strncpy(node->key, norm_key, sizeof(node->key) - 1);
    node->key[sizeof(node->key) - 1] = '\0';
    node->value = value;

    if (ht->buckets[idx] != NULL) {
        ht->collisions++;
    }

    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
    ht->count++;
    return true;
}

void* ht_search(const HashTable *ht, const char *key) {
    if (!ht || !key) return NULL;

    char norm_key[MAX_REG_LEN];
    normalize_registration(key, norm_key, sizeof(norm_key));

    uint64_t idx = ht_hash(norm_key, ht->capacity);
    HashNode *curr = ht->buckets[idx];

    while (curr) {
        if (stricmp(curr->key, norm_key) == 0) {
            return curr->value;
        }
        curr = curr->next;
    }
    return NULL;
}

bool ht_contains(const HashTable *ht, const char *key) {
    return (ht_search(ht, key) != NULL);
}

bool ht_delete(HashTable *ht, const char *key) {
    if (!ht || !key) return false;

    char norm_key[MAX_REG_LEN];
    normalize_registration(key, norm_key, sizeof(norm_key));

    uint64_t idx = ht_hash(norm_key, ht->capacity);
    HashNode *curr = ht->buckets[idx];
    HashNode *prev = NULL;

    while (curr) {
        if (stricmp(curr->key, norm_key) == 0) {
            if (prev) {
                prev->next = curr->next;
            } else {
                ht->buckets[idx] = curr->next;
            }

            if (ht->free_value && curr->value) {
                ht->free_value(curr->value);
            }
            free(curr);
            ht->count--;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}

void ht_clear(HashTable *ht) {
    if (!ht) return;
    for (size_t i = 0; i < ht->capacity; i++) {
        HashNode *curr = ht->buckets[i];
        while (curr) {
            HashNode *next = curr->next;
            if (ht->free_value && curr->value) {
                ht->free_value(curr->value);
            }
            free(curr);
            curr = next;
        }
        ht->buckets[i] = NULL;
    }
    ht->count = 0;
    ht->collisions = 0;
}

void ht_destroy(HashTable *ht) {
    if (!ht) return;
    ht_clear(ht);
    free(ht->buckets);
    free(ht);
}

HashTableStats ht_get_stats(const HashTable *ht) {
    HashTableStats stats = {0, 0, 0, 0.0, 0};
    if (!ht) return stats;

    stats.capacity = ht->capacity;
    stats.count = ht->count;
    stats.load_factor = (double)ht->count / (double)ht->capacity;

    size_t max_chain = 0;
    size_t chained_collisions = 0;
    for (size_t i = 0; i < ht->capacity; i++) {
        size_t chain = 0;
        HashNode *curr = ht->buckets[i];
        while (curr) {
            chain++;
            curr = curr->next;
        }
        if (chain > 1) {
            chained_collisions += (chain - 1);
        }
        if (chain > max_chain) {
            max_chain = chain;
        }
    }
    stats.max_chain_length = max_chain;
    stats.collisions = (ht->collisions > chained_collisions) ? ht->collisions : chained_collisions;
    return stats;
}
