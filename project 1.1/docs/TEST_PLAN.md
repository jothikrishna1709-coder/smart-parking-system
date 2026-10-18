# Test Plan & Verification Report
**Project:** Smart Parking Management System (College DSA Capstone)  
**Test Suite:** `backend/tests/test_main.c` (69 Assertions, 24 Mandatory Scenarios)  
**Verification Result:** 100% PASSED (0 Failures)

---

## 1. Test Suite Summary

The Smart Parking Management System includes an automated C verification suite implementing all 24 mandatory test scenarios outlined in the project specification. All tests run natively against compiled C source code.

```
=======================================================
 SMART PARKING MANAGEMENT SYSTEM - C UNIT TEST SUITE
 24 Capstone DSA & Logic Requirement Verification
=======================================================
 Total Tests Executed: 24
 Total Assertions Evaluated: 69
 Total Passed: 69
 Total Failed: 0
 Success Rate: 100.0%
=======================================================
```

---

## 2. Test Cases Specification & Verification Matrix

| Test ID | Test Scenario | Description & Input | Expected Output | Actual Result |
| :---: | :--- | :--- | :--- | :---: |
| **TC-01** | Empty Parking Lot Initialization | Initialize lot with 30 slots capacity. | All 30 slots created, status AVAILABLE, 0 occupied, 0 reserved. | **PASSED** |
| **TC-02** | First Available Slot Allocation | Allocate incoming car `KA-01-HH-1234`. | Nearest car slot `C-01` transitioned to OCCUPIED; plate recorded. | **PASSED** |
| **TC-03** | Full Parking Capacity | Allocate all 5 slots of a small lot and request 6th slot. | Returns `NULL`, available count equals 0, rejects further allocations. | **PASSED** |
| **TC-04** | Duplicate Vehicle Registration | Register plate `DL01AB9999` twice. | First registration succeeds; second attempt rejected with error. | **PASSED** |
| **TC-05** | Invalid Vehicle Type & Plate | Input invalid type `PLANE` and invalid plate `!@#$$%`. | Safely falls back to default category; rejects invalid plate format. | **PASSED** |
| **TC-06** | Vehicle Checkout Workflow | Check out parked vehicle `MH02CD4567` after 2 hours. | Removed from registry; slot released to AVAILABLE; record saved. | **PASSED** |
| **TC-07** | Incorrect Checkout Request | Attempt checkout for unparked plate `NON_EXISTENT_PLATE`. | Fails safely with `false`; no crash; maintains state integrity. | **PASSED** |
| **TC-08** | Queue Enqueue & Dequeue | Enqueue 2 vehicles (`Alice`, `Bob`); dequeue both. | FIFO order strictly preserved: Alice dequeued first, Bob second. | **PASSED** |
| **TC-09** | Empty Queue Safety | Dequeue and peek operations on an empty queue. | Operations return `false` gracefully without segmentation faults. | **PASSED** |
| **TC-10** | Queue Allocation After Checkout | Frees slot when a car is waiting in FIFO queue. | Eligible car dequeued and promoted to newly freed slot automatically. | **PASSED** |
| **TC-11** | Hash Table Collision Resolution | Insert 6 items with conflicting hash buckets (e.g., `REG1` & `REG9`). | Collisions tracked; all values retrieved accurately along chain. | **PASSED** |
| **TC-12** | Search for Missing Vehicles | Hash table lookup for `NOT_PRESENT`; linear search on missing item. | Hash search returns `NULL`; linear search returns `-1`. | **PASSED** |
| **TC-13** | Sorting Correctness | Sort unsorted array `[85, 12, 59, 73, 2, 45, 99, 1]`. | Both Merge Sort and Quick Sort produce `[1, 2, 12, 45, 59, 73, 85, 99]`. | **PASSED** |
| **TC-14** | Binary Search Preconditions | Execute binary search on unsorted array `[40, 10, 50, 20, 30]`. | Precondition check fails, returns `-1`; succeeds on sorted array. | **PASSED** |
| **TC-15** | BFS Facility Path Finding | Compute route from `ENT_NORTH` to `C-01` using BFS. | Returns path with minimal waypoints through North Hub and Zone A. | **PASSED** |
| **TC-16** | Dijkstra Shortest Path | Compute shortest distance from `ENT_NORTH` to `C-01`. | Finds exact minimum weighted distance: 15m + 20m + 5m = 40.0m. | **PASSED** |
| **TC-17** | File Persistence After Restart | Save modified pricing rate ($45.00/hr) and reload from disk. | Values restored accurately from JSON storage after restart. | **PASSED** |
| **TC-18** | Missing Data Files Handling | Attempt to read non-existent file `data/non_existent.xyz`. | Returns `NULL` safely; falls back to default settings without crashing. | **PASSED** |
| **TC-19** | Corrupted Records Handling | Pass malformed JSON `{ invalid_syntax: true ` to parser. | cJSON safely returns `NULL`; errors logged without memory fault. | **PASSED** |
| **TC-20** | Failed Storage Writes Handling | Attempt atomic write to invalid empty filename `""` or `NULL`. | Rejects operation safely; preserves original data files. | **PASSED** |
| **TC-21** | Invalid JSON Requests | Provide number `12345` instead of string for `reg_number`. | Type mismatch detected and rejected with HTTP 400 Bad Request. | **PASSED** |
| **TC-22** | Repeated Checkout Prevention | Attempt to check out the same vehicle `REG-REPEAT` twice. | First checkout succeeds; second checkout rejected as not parked. | **PASSED** |
| **TC-23** | Reservation Conflicts | Attempt two reservations for the same slot `C-01`. | First reservation succeeds; second attempt rejected with conflict. | **PASSED** |
| **TC-24** | Pricing Boundary Cases | Test 5-minute exit (grace period) and 3-hour car exit. | Grace period charged $0.00; 3-hour charged exactly $30+$20+$20 = $70.00. | **PASSED** |

---

## 3. Execution Instructions

To execute the test suite in PowerShell:
```powershell
.\scripts\test.ps1
```
Or directly via Python test runner:
```powershell
python backend/run_tests.py
```
Exit code is `0` when all tests pass, and non-zero if any assertion fails.
