#include "../include/common.h"
#include "../include/linked_list.h"
#include "../include/queue.h"
#include "../include/hash_table.h"
#include "../include/sorting.h"
#include "../include/searching.h"
#include "../include/graph.h"
#include "../include/parking.h"
#include "../include/vehicle.h"
#include "../include/billing.h"
#include "../include/reservation.h"
#include "../include/storage.h"
#include "../include/cJSON.h"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (cond) { \
        printf("  [PASS] %s\n", msg); \
        g_tests_passed++; \
    } else { \
        printf("  [FAIL] %s (Line %d)\n", msg, __LINE__); \
        g_tests_failed++; \
    } \
} while(0)

/* Test 1: Empty parking lot initialization */
void test_01_empty_lot_init(void) {
    printf("\n--- Test 1: Empty parking lot initialization ---\n");
    ParkingLot *lot = parking_lot_create(30);
    TEST_ASSERT(lot != NULL, "Parking lot allocated");
    TEST_ASSERT(lot->total_slots == 30, "Lot contains requested capacity of slots");

    int total, avail, occ, res, maint;
    parking_get_stats(lot, &total, &avail, &occ, &res, &maint);
    TEST_ASSERT(avail == 30, "All initialized slots are AVAILABLE");
    TEST_ASSERT(occ == 0, "Zero occupied slots initially");
    TEST_ASSERT(res == 0, "Zero reserved slots initially");
    parking_lot_destroy(lot);
}

/* Test 2: Allocation of the first available slot */
void test_02_first_slot_allocation(void) {
    printf("\n--- Test 2: Allocation of first available slot ---\n");
    ParkingLot *lot = parking_lot_create(10);
    ParkingSlot *s = parking_find_available_slot(lot, VEHICLE_CAR);
    TEST_ASSERT(s != NULL, "Found available car slot");
    TEST_ASSERT(s->status == SLOT_AVAILABLE, "Target slot is initially available");

    bool ok = parking_allocate_slot(lot, s->slot_id, "KA-01-HH-1234", time(NULL));
    TEST_ASSERT(ok == true, "Slot allocated successfully");
    TEST_ASSERT(s->status == SLOT_OCCUPIED, "Slot status transitioned to OCCUPIED");
    TEST_ASSERT(strcmp(s->occupied_reg, "KA-01-HH-1234") == 0, "Slot contains vehicle registration");
    parking_lot_destroy(lot);
}

/* Test 3: Full parking capacity */
void test_03_full_parking_capacity(void) {
    printf("\n--- Test 3: Full parking capacity ---\n");
    ParkingLot *lot = parking_lot_create(5);
    /* Fill all slots */
    for (size_t i = 0; i < lot->total_slots; i++) {
        char reg[32];
        snprintf(reg, sizeof(reg), "VEH-%zu", i);
        parking_allocate_slot(lot, lot->slots_array[i]->slot_id, reg, time(NULL));
    }

    ParkingSlot *extra = parking_find_available_slot(lot, VEHICLE_CAR);
    TEST_ASSERT(extra == NULL, "Returns NULL when parking capacity is completely full");

    int total, avail, occ, res, maint;
    parking_get_stats(lot, &total, &avail, &occ, &res, &maint);
    TEST_ASSERT(avail == 0, "Available slot count is 0");
    TEST_ASSERT(occ == total, "Occupied slot count equals total slots");
    parking_lot_destroy(lot);
}

/* Test 4: Duplicate vehicle registration */
void test_04_duplicate_registration(void) {
    printf("\n--- Test 4: Duplicate vehicle registration rejection ---\n");
    VehicleRegistry *vr = vehicle_registry_create();

    ActiveVehicle v1;
    memset(&v1, 0, sizeof(ActiveVehicle));
    strncpy(v1.reg_number, "DL01AB9999", sizeof(v1.reg_number) - 1);
    v1.vehicle_type = VEHICLE_CAR;
    strncpy(v1.slot_id, "C-01", sizeof(v1.slot_id) - 1);
    v1.entry_time = time(NULL);

    bool ok1 = vehicle_register_entry(vr, &v1);
    TEST_ASSERT(ok1 == true, "First entry of registration accepted");

    ActiveVehicle v2 = v1;
    bool ok2 = vehicle_register_entry(vr, &v2);
    TEST_ASSERT(ok2 == false, "Duplicate entry of same registration correctly rejected");

    vehicle_registry_destroy(vr);
}

/* Test 5: Invalid vehicle type */
void test_05_invalid_vehicle_type(void) {
    printf("\n--- Test 5: Invalid vehicle type handling ---\n");
    VehicleType t1 = string_to_vehicle_type("PLANE");
    TEST_ASSERT(t1 == VEHICLE_CAR, "Unrecognized vehicle type safely falls back to default");

    bool valid_reg = vehicle_validate_registration("!@#$$%");
    TEST_ASSERT(valid_reg == false, "Special characters rejected in registration plate");

    bool valid_short = vehicle_validate_registration("A");
    TEST_ASSERT(valid_short == false, "Too short registration plate rejected");
}

/* Test 6: Vehicle checkout */
void test_06_vehicle_checkout(void) {
    printf("\n--- Test 6: Vehicle checkout workflow ---\n");
    ParkingLot *lot = parking_lot_create(10);
    VehicleRegistry *vr = vehicle_registry_create();

    ParkingSlot *s = parking_find_available_slot(lot, VEHICLE_CAR);
    time_t entry_t = time(NULL) - 7200; /* 2 hours ago */
    parking_allocate_slot(lot, s->slot_id, "MH02CD4567", entry_t);

    ActiveVehicle v;
    memset(&v, 0, sizeof(ActiveVehicle));
    strncpy(v.reg_number, "MH02CD4567", sizeof(v.reg_number) - 1);
    v.vehicle_type = VEHICLE_CAR;
    strncpy(v.slot_id, s->slot_id, sizeof(v.slot_id) - 1);
    v.entry_time = entry_t;
    vehicle_register_entry(vr, &v);

    ActiveVehicle checkout_veh;
    bool removed = vehicle_remove_active(vr, "MH02CD4567", &checkout_veh);
    TEST_ASSERT(removed == true, "Active vehicle successfully removed from registry");

    bool slot_released = parking_release_slot(lot, s->slot_id);
    TEST_ASSERT(slot_released == true, "Slot successfully released and returned to AVAILABLE");
    TEST_ASSERT(s->status == SLOT_AVAILABLE, "Slot status verified AVAILABLE");

    parking_lot_destroy(lot);
    vehicle_registry_destroy(vr);
}

/* Test 7: Incorrect checkout request */
void test_07_incorrect_checkout(void) {
    printf("\n--- Test 7: Incorrect checkout request ---\n");
    VehicleRegistry *vr = vehicle_registry_create();
    ActiveVehicle v;
    bool res = vehicle_remove_active(vr, "NON_EXISTENT_PLATE", &v);
    TEST_ASSERT(res == false, "Checkout fails safely when vehicle is not in registry");

    ParkingLot *lot = parking_lot_create(5);
    bool rel = parking_release_slot(lot, "C-01");
    TEST_ASSERT(rel == false, "Release fails safely on already available slot");

    parking_lot_destroy(lot);
    vehicle_registry_destroy(vr);
}

/* Test 8: Queue enqueue and dequeue */
void test_08_queue_enqueue_dequeue(void) {
    printf("\n--- Test 8: FIFO Queue enqueue and dequeue ---\n");
    Queue *q = queue_create();
    TEST_ASSERT(q != NULL, "Queue created");
    TEST_ASSERT(queue_is_empty(q) == true, "Queue initially empty");

    WaitingVehicle w1 = {"WAIT-1", "REG-01", VEHICLE_CAR, "Alice", "123", time(NULL)};
    WaitingVehicle w2 = {"WAIT-2", "REG-02", VEHICLE_MOTORCYCLE, "Bob", "456", time(NULL)};

    queue_enqueue(q, &w1);
    queue_enqueue(q, &w2);
    TEST_ASSERT(queue_size(q) == 2, "Queue size is 2 after enqueues");

    WaitingVehicle out;
    bool d1 = queue_dequeue(q, &out);
    TEST_ASSERT(d1 == true, "First dequeue succeeded");
    TEST_ASSERT(strcmp(out.reg_number, "REG-01") == 0, "FIFO preserved: Alice dequeued first");

    bool d2 = queue_dequeue(q, &out);
    TEST_ASSERT(d2 == true, "Second dequeue succeeded");
    TEST_ASSERT(strcmp(out.reg_number, "REG-02") == 0, "FIFO preserved: Bob dequeued second");
    TEST_ASSERT(queue_is_empty(q) == true, "Queue is empty after dequeues");

    queue_destroy(q);
}

/* Test 9: Empty queue safety */
void test_09_empty_queue_safety(void) {
    printf("\n--- Test 9: Empty queue safety ---\n");
    Queue *q = queue_create();
    WaitingVehicle out;
    bool res = queue_dequeue(q, &out);
    TEST_ASSERT(res == false, "Dequeue on empty queue returns false without crashing");

    bool peek = queue_peek(q, &out);
    TEST_ASSERT(peek == false, "Peek on empty queue returns false without crashing");
    queue_destroy(q);
}

/* Test 10: Queue allocation after checkout */
void test_10_queue_allocation_after_checkout(void) {
    printf("\n--- Test 10: Queue allocation after checkout ---\n");
    Queue *q = queue_create();
    WaitingVehicle w = {"WAIT-10", "WAIT-CAR-01", VEHICLE_CAR, "Charlie", "789", time(NULL)};
    queue_enqueue(q, &w);

    WaitingVehicle promoted;
    bool eligible = queue_dequeue_eligible(q, VEHICLE_CAR, &promoted);
    TEST_ASSERT(eligible == true, "Eligible car found and dequeued from waiting queue");
    TEST_ASSERT(strcmp(promoted.reg_number, "WAIT-CAR-01") == 0, "Promoted vehicle matches waiting car");
    TEST_ASSERT(queue_is_empty(q) == true, "Queue empty after promotion");
    queue_destroy(q);
}

/* Test 11: Hash table collisions */
void test_11_hash_table_collisions(void) {
    printf("\n--- Test 11: Hash table collision resolution ---\n");
    HashTable *ht = ht_create(4, NULL); /* Small capacity guarantees collisions */

    int val1 = 101, val2 = 202, val3 = 303, val4 = 404, val5 = 505, val9 = 909;
    ht_insert(ht, "REG1", &val1);
    ht_insert(ht, "REG2", &val2);
    ht_insert(ht, "REG3", &val3);
    ht_insert(ht, "REG4", &val4);
    ht_insert(ht, "REG5", &val5);
    ht_insert(ht, "REG9", &val9); /* Collides with REG1 modulo powers of 2 */

    HashTableStats stats = ht_get_stats(ht);
    TEST_ASSERT(stats.count == 6, "All 6 items inserted");
    TEST_ASSERT(stats.collisions > 0, "Collisions occurred and were tracked");

    int *found3 = (int*)ht_search(ht, "REG3");
    TEST_ASSERT(found3 != NULL && *found3 == 303, "Value retrieved accurately across collision chain");

    int *found9 = (int*)ht_search(ht, "REG9");
    TEST_ASSERT(found9 != NULL && *found9 == 909, "Colliding key REG9 retrieved accurately");

    ht_delete(ht, "REG3");
    TEST_ASSERT(ht_search(ht, "REG3") == NULL, "Deleted key is no longer found");
    TEST_ASSERT(ht_search(ht, "REG4") != NULL, "Neighboring collision chain entries remain intact");

    ht_destroy(ht);
}

/* Test 12: Search for missing vehicles */
void test_12_search_missing(void) {
    printf("\n--- Test 12: Search for missing vehicles ---\n");
    HashTable *ht = ht_create(16, NULL);
    void *val = ht_search(ht, "NOT_PRESENT");
    TEST_ASSERT(val == NULL, "Hash table returns NULL for non-existent key");

    int arr[] = {10, 20, 30, 40, 50};
    int target = 99;
    int64_t idx = linear_search(arr, 5, sizeof(int), &target, cmp_int_asc);
    TEST_ASSERT(idx == -1, "Linear search returns -1 when element is absent");
    ht_destroy(ht);
}

/* Test 13: Sorting correctness */
void test_13_sorting_correctness(void) {
    printf("\n--- Test 13: Sorting correctness (Merge Sort & Quick Sort) ---\n");
    int data1[] = {85, 12, 59, 73, 2, 45, 99, 1};
    int expected[] = {1, 2, 12, 45, 59, 73, 85, 99};

    merge_sort(data1, 8, sizeof(int), cmp_int_asc);
    bool m_ok = true;
    for (int i = 0; i < 8; i++) {
        if (data1[i] != expected[i]) m_ok = false;
    }
    TEST_ASSERT(m_ok == true, "Merge Sort sorts array in ascending order correctly");

    int data2[] = {85, 12, 59, 73, 2, 45, 99, 1};
    quick_sort(data2, 8, sizeof(int), cmp_int_asc);
    bool q_ok = true;
    for (int i = 0; i < 8; i++) {
        if (data2[i] != expected[i]) q_ok = false;
    }
    TEST_ASSERT(q_ok == true, "Quick Sort sorts array in ascending order correctly");
}

/* Test 14: Binary Search preconditions */
void test_14_binary_search_preconditions(void) {
    printf("\n--- Test 14: Binary Search preconditions ---\n");
    int unsorted[] = {40, 10, 50, 20, 30};
    int target = 20;

    /* Should fail precondition check */
    int64_t res = binary_search(unsorted, 5, sizeof(int), &target, cmp_int_asc);
    TEST_ASSERT(res == -1, "Binary search rejects unsorted data precondition");

    int sorted[] = {10, 20, 30, 40, 50};
    int64_t valid_idx = binary_search(sorted, 5, sizeof(int), &target, cmp_int_asc);
    TEST_ASSERT(valid_idx == 1, "Binary search succeeds on properly sorted data");
}

/* Test 15: BFS path finding */
void test_15_bfs_path_finding(void) {
    printf("\n--- Test 15: BFS path finding in facility graph ---\n");
    Graph *g = graph_create();
    graph_init_facility_layout(g);

    NavigationPath path = graph_bfs(g, "ENT_NORTH", "C-01");
    TEST_ASSERT(path.found == true, "BFS found route from North Entrance to Slot C-01");
    TEST_ASSERT(path.path_length > 1, "BFS path contains multiple waypoints");
    graph_destroy(g);
}

/* Test 16: Dijkstra shortest path */
void test_16_dijkstra_shortest_path(void) {
    printf("\n--- Test 16: Dijkstra shortest path algorithm ---\n");
    Graph *g = graph_create();
    graph_init_facility_layout(g);

    NavigationPath path = graph_dijkstra(g, "ENT_NORTH", "C-01");
    TEST_ASSERT(path.found == true, "Dijkstra found shortest path");
    TEST_ASSERT(path.total_distance > 0.0, "Total distance is positive");
    TEST_ASSERT(path.total_distance == 40.0, "Dijkstra calculated exact minimum route distance (15m + 20m + 5m = 40m)");
    graph_destroy(g);
}

/* Test 17: File persistence after restart */
void test_17_file_persistence(void) {
    printf("\n--- Test 17: File persistence after restart ---\n");
    storage_init();

    PricingPolicy policy = billing_get_default_policy();
    policy.car_first_hour = 45.0;
    policy.min_charge = 15.0;
    storage_save_settings(&policy);

    PricingPolicy loaded;
    bool ok = storage_load_settings(&loaded);
    TEST_ASSERT(ok == true, "Settings loaded from persistent file");
    TEST_ASSERT(loaded.car_first_hour == 45.0, "Modified rate persisted accurately");
    TEST_ASSERT(loaded.min_charge == 15.0, "Modified min charge persisted accurately");
}

/* Test 18: Missing data files */
void test_18_missing_data_files(void) {
    printf("\n--- Test 18: Missing data files handling ---\n");
    char *data = storage_read_file("data/non_existent_file.xyz");
    TEST_ASSERT(data == NULL, "Missing file safely returns NULL");

    PricingPolicy p;
    bool res = storage_load_settings(&p);
    /* Should succeed or fall back safely */
    TEST_ASSERT(p.car_first_hour > 0, "System continues safely with default parameters");
}

/* Test 19: Corrupted records */
void test_19_corrupted_records(void) {
    printf("\n--- Test 19: Corrupted records handling ---\n");
    const char *corrupted_json = "{ invalid_json_syntax: true ";
    cJSON *parsed = cJSON_Parse(corrupted_json);
    TEST_ASSERT(parsed == NULL, "cJSON safely rejects invalid JSON without crashing");
}

/* Test 20: Failed storage writes */
void test_20_failed_storage_writes(void) {
    printf("\n--- Test 20: Failed storage writes handling ---\n");
    bool res = storage_atomic_write("", "content");
    TEST_ASSERT(res == false, "Empty filename write rejected safely");

    bool null_res = storage_atomic_write(NULL, NULL);
    TEST_ASSERT(null_res == false, "NULL parameters rejected safely");
}

/* Test 21: Invalid JSON requests */
void test_21_invalid_json_requests(void) {
    printf("\n--- Test 21: Invalid JSON requests validation ---\n");
    const char *bad_input = "{\"reg_number\": 12345}"; /* Wrong type: number instead of string */
    cJSON *root = cJSON_Parse(bad_input);
    cJSON *reg = cJSON_GetObjectItem(root, "reg_number");
    TEST_ASSERT(reg != NULL && !cJSON_IsString(reg), "Type mismatch detected safely");
    cJSON_Delete(root);
}

/* Test 22: Repeated checkout */
void test_22_repeated_checkout(void) {
    printf("\n--- Test 22: Repeated checkout prevention ---\n");
    VehicleRegistry *vr = vehicle_registry_create();
    ActiveVehicle v = {"TCK-1", "REG-REPEAT", VEHICLE_CAR, "C-01", "User", "999", "", time(NULL), "2026-10-10 10:00:00"};
    vehicle_register_entry(vr, &v);

    ActiveVehicle out;
    bool first_checkout = vehicle_remove_active(vr, "REG-REPEAT", &out);
    TEST_ASSERT(first_checkout == true, "First checkout succeeds");

    bool second_checkout = vehicle_remove_active(vr, "REG-REPEAT", &out);
    TEST_ASSERT(second_checkout == false, "Repeated checkout correctly rejected");
    vehicle_registry_destroy(vr);
}

/* Test 23: Reservation conflicts */
void test_23_reservation_conflicts(void) {
    printf("\n--- Test 23: Reservation conflict enforcement ---\n");
    ReservationManager *mgr = reservation_mgr_create();
    ParkingLot *lot = parking_lot_create(10);

    Reservation r1;
    bool ok1 = reservation_create(mgr, lot, "RES-CAR-1", VEHICLE_CAR, "C-01", "Owner1", "111", 2, &r1);
    TEST_ASSERT(ok1 == true, "First reservation for Slot C-01 accepted");

    Reservation r2;
    bool ok2 = reservation_create(mgr, lot, "RES-CAR-2", VEHICLE_CAR, "C-01", "Owner2", "222", 2, &r2);
    TEST_ASSERT(ok2 == false, "Conflict on already reserved slot C-01 correctly rejected");

    reservation_mgr_destroy(mgr);
    parking_lot_destroy(lot);
}

/* Test 24: Invalid pricing settings */
void test_24_invalid_pricing(void) {
    printf("\n--- Test 24: Pricing calculation boundary cases ---\n");
    PricingPolicy p = billing_get_default_policy();

    /* Zero duration checkout (immediate exit) */
    time_t t = time(NULL);
    ParkingTransaction txn_grace = billing_calculate_fee(&p, "TCK-1", "CAR-1", VEHICLE_CAR, "C-01", "Bob", "", t, t + 300); /* 5 mins */
    TEST_ASSERT(txn_grace.is_grace_period == true, "Exited within grace period (5 mins) charged $0");
    TEST_ASSERT(txn_grace.total_fee == 0.0, "Grace period fee is 0.0");

    /* 3 hours checkout */
    ParkingTransaction txn_3h = billing_calculate_fee(&p, "TCK-2", "CAR-2", VEHICLE_CAR, "C-01", "Bob", "", t, t + 10800); /* 3 hours */
    /* 1st hour: $30, 2nd hour: $20, 3rd hour: $20 = $70 */
    TEST_ASSERT(txn_3h.total_fee == 70.0, "3-hour car parking correctly calculated: $30 + $20 + $20 = $70.00");
}

#ifdef _WIN32
__declspec(dllexport) int run_all_c_tests(void);
#endif

int run_all_c_tests(void) {
    g_tests_passed = 0;
    g_tests_failed = 0;

    printf("=======================================================\n");
    printf(" SMART PARKING MANAGEMENT SYSTEM - C UNIT TEST SUITE\n");
    printf(" 24 Capstone DSA & Logic Requirement Verification\n");
    printf("=======================================================\n");

    test_01_empty_lot_init();
    test_02_first_slot_allocation();
    test_03_full_parking_capacity();
    test_04_duplicate_registration();
    test_05_invalid_vehicle_type();
    test_06_vehicle_checkout();
    test_07_incorrect_checkout();
    test_08_queue_enqueue_dequeue();
    test_09_empty_queue_safety();
    test_10_queue_allocation_after_checkout();
    test_11_hash_table_collisions();
    test_12_search_missing();
    test_13_sorting_correctness();
    test_14_binary_search_preconditions();
    test_15_bfs_path_finding();
    test_16_dijkstra_shortest_path();
    test_17_file_persistence();
    test_18_missing_data_files();
    test_19_corrupted_records();
    test_20_failed_storage_writes();
    test_21_invalid_json_requests();
    test_22_repeated_checkout();
    test_23_reservation_conflicts();
    test_24_invalid_pricing();

    printf("\n=======================================================\n");
    printf(" TEST SUITE RESULTS:\n");
    printf(" Passed: %d\n", g_tests_passed);
    printf(" Failed: %d\n", g_tests_failed);
    printf("=======================================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}

int main(void) {
    return run_all_c_tests();
}
