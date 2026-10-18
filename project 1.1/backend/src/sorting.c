#include "../include/sorting.h"

/* --- MERGE SORT IMPLEMENTATION --- */

static void merge_internal(char *base, size_t left, size_t mid, size_t right, size_t size, Comparator compar, char *temp) {
    size_t i = left;
    size_t j = mid + 1;
    size_t k = left;

    while (i <= mid && j <= right) {
        if (compar(base + i * size, base + j * size) <= 0) {
            memcpy(temp + k * size, base + i * size, size);
            i++;
        } else {
            memcpy(temp + k * size, base + j * size, size);
            j++;
        }
        k++;
    }

    while (i <= mid) {
        memcpy(temp + k * size, base + i * size, size);
        i++;
        k++;
    }

    while (j <= right) {
        memcpy(temp + k * size, base + j * size, size);
        j++;
        k++;
    }

    for (size_t m = left; m <= right; m++) {
        memcpy(base + m * size, temp + m * size, size);
    }
}

static void merge_sort_recurse(char *base, size_t left, size_t right, size_t size, Comparator compar, char *temp) {
    if (left < right) {
        size_t mid = left + (right - left) / 2;
        merge_sort_recurse(base, left, mid, size, compar, temp);
        merge_sort_recurse(base, mid + 1, right, size, compar, temp);
        merge_internal(base, left, mid, right, size, compar, temp);
    }
}

void merge_sort(void *base, size_t num, size_t size, Comparator compar) {
    if (!base || num <= 1 || size == 0 || !compar) return;

    char *temp = (char*)malloc(num * size);
    if (!temp) return; /* fallback if OOM */

    merge_sort_recurse((char*)base, 0, num - 1, size, compar, temp);
    free(temp);
}

/* --- QUICK SORT IMPLEMENTATION --- */

static void swap_bytes(char *a, char *b, size_t size) {
    if (a == b) return;
    char temp[256];
    while (size > 0) {
        size_t chunk = size < sizeof(temp) ? size : sizeof(temp);
        memcpy(temp, a, chunk);
        memcpy(a, b, chunk);
        memcpy(b, temp, chunk);
        a += chunk;
        b += chunk;
        size -= chunk;
    }
}

/* Median of three pivot selection: picks median of left, mid, right */
static size_t choose_pivot_index(char *base, size_t left, size_t right, size_t size, Comparator compar) {
    size_t mid = left + (right - left) / 2;
    char *a = base + left * size;
    char *b = base + mid * size;
    char *c = base + right * size;

    if (compar(a, b) < 0) {
        if (compar(b, c) < 0) return mid;
        if (compar(a, c) < 0) return right;
        return left;
    } else {
        if (compar(a, c) < 0) return left;
        if (compar(b, c) < 0) return right;
        return mid;
    }
}

static size_t partition(char *base, size_t low, size_t high, size_t size, Comparator compar) {
    size_t pivot_idx = choose_pivot_index(base, low, high, size, compar);
    swap_bytes(base + pivot_idx * size, base + high * size, size);

    char *pivot = base + high * size;
    size_t i = low;

    for (size_t j = low; j < high; j++) {
        if (compar(base + j * size, pivot) <= 0) {
            swap_bytes(base + i * size, base + j * size, size);
            i++;
        }
    }
    swap_bytes(base + i * size, base + high * size, size);
    return i;
}

static void quick_sort_recurse(char *base, size_t low, size_t high, size_t size, Comparator compar) {
    if (low < high) {
        size_t pi = partition(base, low, high, size, compar);
        if (pi > 0 && pi - 1 > low) {
            quick_sort_recurse(base, low, pi - 1, size, compar);
        }
        if (pi + 1 < high) {
            quick_sort_recurse(base, pi + 1, high, size, compar);
        }
    }
}

void quick_sort(void *base, size_t num, size_t size, Comparator compar) {
    if (!base || num <= 1 || size == 0 || !compar) return;
    quick_sort_recurse((char*)base, 0, num - 1, size, compar);
}

/* Standard Comparators */

int cmp_int_asc(const void *a, const void *b) {
    int val_a = *(const int*)a;
    int val_b = *(const int*)b;
    return (val_a > val_b) - (val_a < val_b);
}

int cmp_int_desc(const void *a, const void *b) {
    return cmp_int_asc(b, a);
}

int cmp_double_asc(const void *a, const void *b) {
    double val_a = *(const double*)a;
    double val_b = *(const double*)b;
    return (val_a > val_b) - (val_a < val_b);
}

int cmp_double_desc(const void *a, const void *b) {
    return cmp_double_asc(b, a);
}

int cmp_string_asc(const void *a, const void *b) {
    return strcmp((const char*)a, (const char*)b);
}

int cmp_string_desc(const void *a, const void *b) {
    return strcmp((const char*)b, (const char*)a);
}

int cmp_time_asc(const void *a, const void *b) {
    time_t val_a = *(const time_t*)a;
    time_t val_b = *(const time_t*)b;
    return (val_a > val_b) - (val_a < val_b);
}

int cmp_time_desc(const void *a, const void *b) {
    return cmp_time_asc(b, a);
}
