#ifndef SEARCHING_H
#define SEARCHING_H

#include "common.h"
#include "sorting.h"
#include "hash_table.h"

/**
 * Validates that an array is sorted according to the comparator.
 * Precondition check for binary search.
 * Time Complexity: O(n)
 */
bool is_sorted_array(const void *base, size_t num, size_t size, Comparator compar);

/**
 * Linear Search: Scans elements sequentially.
 * Time Complexity:
 *   Best:    O(1)
 *   Average: O(n)
 *   Worst:   O(n)
 * Space Complexity: O(1)
 * 
 * @return 0-based index if found, or -1 if not found.
 */
int64_t linear_search(const void *base, size_t num, size_t size, const void *target, Comparator compar);

/**
 * Binary Search: Divides search space in half each iteration.
 * REQUIREMENT: Array must be sorted by comparator!
 * Time Complexity:
 *   Best:    O(1)
 *   Average: O(log n)
 *   Worst:   O(log n)
 * Space Complexity: O(1)
 * 
 * @return 0-based index if found, or -1 if not found or if sorted precondition fails.
 */
int64_t binary_search(const void *base, size_t num, size_t size, const void *target, Comparator compar);

/**
 * Case-insensitive substring match helper for text searching (e.g., partial reg or name).
 */
bool str_contains_case_insensitive(const char *haystack, const char *needle);

#endif /* SEARCHING_H */
