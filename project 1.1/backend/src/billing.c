#include "../include/billing.h"
#include <math.h>

static uint64_t g_transaction_counter = 1000;

PricingPolicy billing_get_default_policy(void) {
    PricingPolicy policy;
    policy.car_first_hour = 30.0;
    policy.car_subsequent_hour = 20.0;
    policy.bike_first_hour = 15.0;
    policy.bike_subsequent_hour = 10.0;
    policy.accessible_first_hour = 20.0;
    policy.accessible_subsequent_hour = 15.0;
    policy.grace_period_minutes = 15;
    policy.min_charge = 10.0;
    policy.rounding_minutes = 30; /* Round to nearest 30 mins */
    return policy;
}

void billing_generate_transaction_id(char *out_buf, size_t buf_len) {
    if (!out_buf || buf_len == 0) return;
    time_t now = time(NULL);
    struct tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &now);
#else
    localtime_r(&now, &tm_info);
#endif
    snprintf(out_buf, buf_len, "TXN-%04d%02d%02d-%05llu",
             tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
             (unsigned long long)(++g_transaction_counter));
}

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
) {
    ParkingTransaction txn;
    memset(&txn, 0, sizeof(ParkingTransaction));

    PricingPolicy p = policy ? *policy : billing_get_default_policy();

    billing_generate_transaction_id(txn.transaction_id, sizeof(txn.transaction_id));
    if (ticket_id) strncpy(txn.ticket_id, ticket_id, sizeof(txn.ticket_id) - 1);
    if (reg_number) strncpy(txn.reg_number, reg_number, sizeof(txn.reg_number) - 1);
    txn.vehicle_type = type;
    if (slot_id) strncpy(txn.slot_id, slot_id, sizeof(txn.slot_id) - 1);
    if (owner_name) strncpy(txn.owner_name, owner_name, sizeof(txn.owner_name) - 1);
    if (contact) strncpy(txn.contact, contact, sizeof(txn.contact) - 1);

    txn.entry_time = entry_time;
    txn.exit_time = exit_time >= entry_time ? exit_time : entry_time;

    format_iso_time(txn.entry_time, txn.entry_time_str, sizeof(txn.entry_time_str));
    format_iso_time(txn.exit_time, txn.exit_time_str, sizeof(txn.exit_time_str));

    txn.duration_seconds = (int64_t)(txn.exit_time - txn.entry_time);
    int total_minutes = (int)(txn.duration_seconds / 60);
    txn.duration_hours = total_minutes / 60;
    txn.duration_minutes = total_minutes % 60;

    /* Rates based on category */
    switch (type) {
        case VEHICLE_MOTORCYCLE:
            txn.base_rate = p.bike_first_hour;
            txn.subsequent_rate = p.bike_subsequent_hour;
            break;
        case VEHICLE_ACCESSIBLE:
            txn.base_rate = p.accessible_first_hour;
            txn.subsequent_rate = p.accessible_subsequent_hour;
            break;
        case VEHICLE_CAR:
        default:
            txn.base_rate = p.car_first_hour;
            txn.subsequent_rate = p.car_subsequent_hour;
            break;
    }

    /* Grace period check */
    if (total_minutes <= p.grace_period_minutes) {
        txn.is_grace_period = true;
        txn.base_fee = 0.0;
        txn.subsequent_fee = 0.0;
        txn.total_fee = 0.0;
        return txn;
    }

    txn.is_grace_period = false;

    /* First hour is charged at base rate */
    txn.base_fee = txn.base_rate;

    if (total_minutes <= 60) {
        txn.subsequent_fee = 0.0;
        txn.total_fee = txn.base_fee;
    } else {
        int extra_minutes = total_minutes - 60;
        /* Partial hours rounded up to nearest configured increment (e.g., 30 mins) */
        int rounding = p.rounding_minutes > 0 ? p.rounding_minutes : 60;
        int billable_extra_units = (extra_minutes + rounding - 1) / rounding;
        double billable_extra_hours = (double)(billable_extra_units * rounding) / 60.0;

        txn.subsequent_fee = billable_extra_hours * txn.subsequent_rate;
        txn.total_fee = txn.base_fee + txn.subsequent_fee;
    }

    /* Minimum charge enforcement */
    if (txn.total_fee < p.min_charge) {
        txn.total_fee = p.min_charge;
    }

    return txn;
}
