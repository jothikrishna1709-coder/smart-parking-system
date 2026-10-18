#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#define PATH_SEP "\\"
#else
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define PATH_SEP "/"
#endif

#define MAX_REG_LEN        32
#define MAX_NAME_LEN       64
#define MAX_PHONE_LEN      24
#define MAX_ID_LEN         32
#define MAX_DESC_LEN       128
#define MAX_PATH_LEN       512
#define MAX_SLOTS          120
#define DEFAULT_PORT       8080

typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR_NOT_FOUND = -1,
    STATUS_ERROR_ALREADY_EXISTS = -2,
    STATUS_ERROR_FULL = -3,
    STATUS_ERROR_INVALID_INPUT = -4,
    STATUS_ERROR_FILE_IO = -5,
    STATUS_ERROR_MEMORY = -6,
    STATUS_ERROR_UNAUTHORIZED = -7,
    STATUS_ERROR_CONFLICT = -8
} StatusCode;

typedef enum {
    VEHICLE_CAR = 0,
    VEHICLE_MOTORCYCLE = 1,
    VEHICLE_ACCESSIBLE = 2
} VehicleType;

typedef enum {
    SLOT_AVAILABLE = 0,
    SLOT_OCCUPIED = 1,
    SLOT_RESERVED = 2,
    SLOT_MAINTENANCE = 3
} SlotStatus;

typedef enum {
    RESERVATION_ACTIVE = 0,
    RESERVATION_FULFILLED = 1,
    RESERVATION_CANCELLED = 2,
    RESERVATION_EXPIRED = 3
} ReservationStatus;

/* Helper to convert vehicle type to string */
static inline const char* vehicle_type_to_string(VehicleType t) {
    switch (t) {
        case VEHICLE_CAR: return "CAR";
        case VEHICLE_MOTORCYCLE: return "MOTORCYCLE";
        case VEHICLE_ACCESSIBLE: return "ACCESSIBLE";
        default: return "UNKNOWN";
    }
}

static inline VehicleType string_to_vehicle_type(const char *str) {
    if (!str) return VEHICLE_CAR;
    if (stricmp(str, "MOTORCYCLE") == 0 || stricmp(str, "BIKE") == 0) return VEHICLE_MOTORCYCLE;
    if (stricmp(str, "ACCESSIBLE") == 0 || stricmp(str, "HANDICAPPED") == 0) return VEHICLE_ACCESSIBLE;
    return VEHICLE_CAR;
}

static inline const char* slot_status_to_string(SlotStatus s) {
    switch (s) {
        case SLOT_AVAILABLE: return "AVAILABLE";
        case SLOT_OCCUPIED: return "OCCUPIED";
        case SLOT_RESERVED: return "RESERVED";
        case SLOT_MAINTENANCE: return "MAINTENANCE";
        default: return "UNKNOWN";
    }
}

static inline SlotStatus string_to_slot_status(const char *str) {
    if (!str) return SLOT_AVAILABLE;
    if (stricmp(str, "OCCUPIED") == 0) return SLOT_OCCUPIED;
    if (stricmp(str, "RESERVED") == 0) return SLOT_RESERVED;
    if (stricmp(str, "MAINTENANCE") == 0) return SLOT_MAINTENANCE;
    return SLOT_AVAILABLE;
}

/* Helper to format ISO timestamp */
static inline void format_iso_time(time_t t, char *buf, size_t max_len) {
    struct tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &t);
#else
    localtime_r(&t, &tm_info);
#endif
    strftime(buf, max_len, "%Y-%m-%d %H:%M:%S", &tm_info);
}

/* Normalizes vehicle registration: trims and uppercases */
static inline void normalize_registration(const char *src, char *dest, size_t dest_len) {
    size_t d = 0;
    while (*src && d + 1 < dest_len) {
        if (!isspace((unsigned char)*src) && *src != '-') {
            dest[d++] = (char)toupper((unsigned char)*src);
        }
        src++;
    }
    dest[d] = '\0';
}

#endif /* COMMON_H */
