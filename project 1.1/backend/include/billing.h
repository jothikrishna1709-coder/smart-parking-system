#ifndef BILLING_H
#define BILLING_H

#include "common.h"

typedef struct {
    double car_first_hour;
    double car_subsequent_hour;
    double bike_first_hour;
    double bike_subsequent_hour;
    double accessible_first_hour;
    double accessible_subsequent_hour;
    int grace_period_minutes;      /* Free if under this threshold */
    double min_charge;             /* Minimum billable charge */
    int rounding_minutes;          /* Round partial hours up to 30 or 60 min */
} PricingPolicy;

typedef struct {
    char transaction_id[MAX_ID_LEN];
    char ticket_id[MAX_ID_LEN];
    char reg_number[MAX_REG_LEN];
    VehicleType vehicle_type;
    char slot_id[MAX_ID_LEN];
    char owner_name[MAX_NAME_LEN];
    char contact[MAX_PHONE_LEN];
    time_t entry_time;
    time_t exit_time;
    int64_t duration_seconds;
    int duration_hours;
    int duration_minutes;
    double base_rate;
    double subsequent_rate;
    double base_fee;
    double subsequent_fee;
    double total_fee;
    char entry_time_str[32];
    char exit_time_str[32];
    bool is_grace_period;
} ParkingTransaction;

/**
 * Returns default system pricing policy.
 */
PricingPolicy billing_get_default_policy(void);

/**
 * Calculates parking fee from actual timestamps and produces detailed breakdown.
 * Time Complexity: O(1)
 */
ParkingTransaction billing_calculate_fee(
    const PricingPolicy *policy,
    const char *ticket_id,
    const char *reg_number,
    VehicleType type,
    const char *slot_id,
    const char *owner_name,
    const char *contact,
    time_t entry_time,
    time_t exit_time
);

/**
 * Generates unique transaction ID.
 */
void billing_generate_transaction_id(char *out_buf, size_t buf_len);

#endif /* BILLING_H */
