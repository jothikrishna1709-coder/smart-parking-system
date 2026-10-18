#ifndef SORTING_H
#define SORTING_H

#include "common.h"

typedef int (*Comparator)(const void *a, const void *b);

/**
 * Merge Sort: Divide and conquer stable sorting algorithm.
 * Time Complexity:
 *   Best:    O(n log n)
 *   Average: O(n log n)
 *   Worst:   O(n log n)
 * Space Complexity: O(n) auxiliary buffer
 * 
 * @param base   Pointer to first element of array
 * @param num    Number of elements
 * @param size   Size of each element in bytes
 * @param compar Comparison function returning <0, 0, >0
 */
void merge_sort(void *base, size_t num, size_t size, Comparator compar);

/**
 * Quick Sort: In-place divide and conquer sorting algorithm with median-of-three pivot.
 * Time Complexity:
 *   Best:    O(n log n)
 *   Average: O(n log n)
 *   Worst:   O(n^2) (mitigated by median-of-three)
 * Space Complexity: O(log n) call stack space
 * 
 * @param base   Pointer to first element of array
 * @param num    Number of elements
 * @param size   Size of each element in bytes
 * @param compar Comparison function returning <0, 0, >0
 */
void quick_sort(void *base, size_t num, size_t size, Comparator compar);

/* Utility comparators for primitives and common types */
int cmp_int_asc(const void *a, const void *b);
int cmp_int_desc(const void *a, const void *b);
int cmp_double_asc(const void *a, const void *b);
int cmp_double_desc(const void *a, const void *b);
int cmp_string_asc(const void *a, const void *b);
int cmp_string_desc(const void *a, const void *b);
int cmp_time_asc(const void *a, const void *b);
int cmp_time_desc(const void *a, const void *b);

#endif /* SORTING_H */
