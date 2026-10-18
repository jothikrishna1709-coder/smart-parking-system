#include "../include/searching.h"

bool is_sorted_array(const void *base, size_t num, size_t size, Comparator compar) {
    if (!base || num <= 1 || size == 0 || !compar) return true;
    const char *ptr = (const char*)base;
    for (size_t i = 0; i < num - 1; i++) {
        if (compar(ptr + i * size, ptr + (i + 1) * size) > 0) {
            return false;
        }
    }
    return true;
}

int64_t linear_search(const void *base, size_t num, size_t size, const void *target, Comparator compar) {
    if (!base || num == 0 || size == 0 || !target || !compar) return -1;
    const char *ptr = (const char*)base;
    for (size_t i = 0; i < num; i++) {
        if (compar(ptr + i * size, target) == 0) {
            return (int64_t)i;
        }
    }
    return -1;
}

int64_t binary_search(const void *base, size_t num, size_t size, const void *target, Comparator compar) {
    if (!base || num == 0 || size == 0 || !target || !compar) return -1;

    /* Enforce precondition: array must be sorted */
    if (!is_sorted_array(base, num, size, compar)) {
        return -1;
    }

    const char *ptr = (const char*)base;
    int64_t low = 0;
    int64_t high = (int64_t)num - 1;

    while (low <= high) {
        int64_t mid = low + (high - low) / 2;
        int cmp = compar(ptr + (size_t)mid * size, target);

        if (cmp == 0) {
            return mid;
        } else if (cmp < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return -1;
}

bool str_contains_case_insensitive(const char *haystack, const char *needle) {
    if (!haystack || !needle) return false;
    if (*needle == '\0') return true;

    size_t n_len = strlen(needle);
    size_t h_len = strlen(haystack);
    if (n_len > h_len) return false;

    for (size_t i = 0; i <= h_len - n_len; i++) {
        size_t j = 0;
        while (j < n_len && tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == n_len) return true;
    }
    return false;
}
