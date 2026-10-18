# Smart Parking Management System (DSA Capstone)

A full-stack, enterprise-grade **Smart Parking Management System** developed as an advanced college Data Structures and Algorithms (DSA) capstone project. 

The core operational engine, data structures, parking slot allocation, search algorithms, queue management, billing calculations, and graph routing are built entirely in **pure C**, backed by an embedded HTTP 1.1 server and a responsive web dashboard.

---

## 1. Project Overview & Objectives

Modern urban centers face significant congestion, wasted fuel, and inefficient space utilization due to uncoordinated parking. This project solves that problem through algorithmic automation:
*   **Optimal Slot Allocation:** Instantly locates and reserves the closest matching available parking slot based on vehicle category (Cars, Motorcycles, Accessible).
*   **Zero Double-Allocation:** Strict concurrency locking and state machines prevent conflicting bookings.
*   **FIFO Waiting Queue:** When the facility reaches capacity, incoming drivers enter a fair linked-list queue and are automatically allocated slots upon checkout.
*   **Transparent Timestamp Billing:** Calculates actual durations down to the second with grace periods, category rates, and configurable rounding policies.
*   **In-Facility Navigation:** Employs Dijkstra's Shortest Path algorithm to provide turn-by-turn driving guidance from entrance gates to assigned parking slots.

---

## 2. Technology Stack

*   **Core Logic & DSA Backend:** C Language (C99 / C11)
*   **Compiler Toolchain:** Clang / Zig C Toolchain (native Windows support with Winsock2)
*   **HTTP Server:** Native Windows Sockets (Winsock2 `ws2_32`) HTTP 1.1 multi-threaded server
*   **Serialization:** Lightweight ANSI C JSON parser/serializer (`cJSON`)
*   **Frontend Dashboard:** HTML5, Modern Vanilla CSS3 (Custom Design System), Vanilla JavaScript (ES6+)
*   **Persistence:** File handling in C with atomic temporary file replacements
*   **Platform:** Windows 10 / 11, VS Code / Antigravity IDE

---

## 3. Data Structures & Algorithms (DSA) Implemented

| Data Structure / Algorithm | File Location | Purpose & Complexity |
| :--- | :--- | :--- |
| **Doubly Linked List** | `backend/src/linked_list.c` | Manages parking slots and active vehicles. $O(1)$ insert, $O(n)$ search/delete. |
| **FIFO Queue** | `backend/src/queue.c` | Manages waiting vehicles. $O(1)$ enqueue (tail) and dequeue (head). Includes order-preserving eligibility filter. |
| **Separate Chaining Hash Table** | `backend/src/hash_table.c` | Instant registration lookup using DJB2 polynomial rolling hash. Average $O(1)$ search, $O(n)$ worst. |
| **Merge Sort** | `backend/src/sorting.c` | Stable divide-and-conquer sorting for parking history transactions by fee or timestamp. $O(n \log n)$. |
| **Quick Sort** | `backend/src/sorting.c` | In-place sorting with median-of-three pivot selection for vehicles and slots. Average $O(n \log n)$. |
| **Binary Search** | `backend/src/searching.c` | $O(\log n)$ search with strict monotonic precondition verification. |
| **Dijkstra's Algorithm** | `backend/src/graph.c` | Computes shortest physical driving distance and guidance from entrance to target slot. $O(V^2)$. |
| **Breadth-First Search (BFS)** | `backend/src/graph.c` | Unweighted graph traversal computing minimum waypoint hop counts. $O(V + E)$. |

For full mathematical proofs and complexity analysis, see [docs/DSA_ANALYSIS.md](docs/DSA_ANALYSIS.md).

---

## 4. Project Folder Structure

```
Smart-Parking-Management-System/
|-- frontend/
|   |-- index.html            # Main single-page web dashboard
|   |-- css/
|   |   |-- style.css         # Modern design system (tokens, glassmorphism, responsive grid)
|   |-- js/
|   |   |-- app.js            # UI controller & reactive DOM updates
|   |   |-- api.js            # REST API client connecting to C backend
|-- backend/
|   |-- src/
|   |   |-- main.c            # Server entry point & signal handling
|   |   |-- server.c          # Multi-threaded Winsock HTTP 1.1 server
|   |   |-- parking.c         # Slot management & category allocation
|   |   |-- vehicle.c         # Vehicle registry & ticket generation
|   |   |-- queue.c           # FIFO waiting queue implementation
|   |   |-- linked_list.c     # Doubly linked list implementation
|   |   |-- hash_table.c      # DJB2 separate chaining hash table
|   |   |-- sorting.c         # Merge Sort & Quick Sort implementations
|   |   |-- searching.c       # Linear & Binary search with precondition checks
|   |   |-- graph.c           # Adjacency list, BFS & Dijkstra navigation
|   |   |-- billing.c         # Pricing policy & duration fee calculations
|   |   |-- reservation.c     # Advance slot reservation manager
|   |   |-- storage.c         # Atomic file persistence layer
|   |   |-- cJSON.c           # Embedded ANSI C JSON parser
|   |-- include/              # Header definitions for all modules
|   |-- tests/
|   |   |-- test_main.c       # Automated 24-scenario C unit test suite
|   |-- run_server.py         # Native C server launcher
|   |-- run_tests.py          # Native C test suite runner
|-- data/                     # Persistent JSON files (slots, queue, history, etc.)
|-- docs/
|   |-- DSA_ANALYSIS.md       # Comprehensive algorithms complexity documentation
|   |-- TEST_PLAN.md          # 24 test cases verification matrix
|   |-- ARCHITECTURE.md       # System architecture & Mermaid request flow diagrams
|-- scripts/
|   |-- setup.ps1             # Environment verification script
|   |-- build.ps1             # C compilation script
|   |-- run.ps1               # Server launch script
|   |-- test.ps1              # Automated test runner script
|-- README.md
```

---

## 5. Prerequisites & Installation

### Requirements
*   Windows 10 or 11 (64-bit)
*   PowerShell 5.1+
*   Python 3.8+ (for process execution under Windows Smart App Control)
*   C Compiler: Zig Clang C Toolchain (automatically verified by setup script) or GCC

### Setup
Run the automated setup script to check your environment:
```powershell
.\scripts\setup.ps1
```

---

## 6. Build, Run, and Test Instructions

### Compile the C Backend
```powershell
.\scripts\build.ps1
```
This compiles the C source code into `parking_backend.dll`, `parking_engine.dll`, and `backend/parking_server.exe`.

### Run the C Backend & Web Dashboard
```powershell
.\scripts\run.ps1
```
The server starts immediately at **http://localhost:8080/**. Open this URL in any modern web browser to access the live dashboard.

### Run Automated C Unit Tests
```powershell
.\scripts\test.ps1
```
Executes all 24 DSA and logic verification tests (69 assertions). Returns exit code `0` on success.

---

## 7. REST API Documentation

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/health` | Returns backend health status and engine info. |
| `GET` | `/api/dashboard` | Aggregated statistics: occupancy, distribution, revenue. |
| `GET` | `/api/slots` | List all 40 slots with real-time status and distance. |
| `POST` | `/api/vehicles/entry` | Enters vehicle, validates plate, allocates optimal slot. |
| `POST` | `/api/vehicles/exit` | Previews billing or confirms checkout with queue promotion. |
| `GET` | `/api/vehicles/search?q=` | Multi-attribute search across active vehicles. |
| `GET` | `/api/queue` | Retrieves all waiting vehicles in FIFO order. |
| `POST` | `/api/queue` | Manually enqueues a vehicle when lot is full. |
| `DELETE`| `/api/queue/{id}` | Cancels a waiting queue reservation. |
| `GET` | `/api/reservations` | Lists all advance slot bookings. |
| `POST` | `/api/reservations` | Reserves a designated slot with expiry time. |
| `POST` | `/api/reservations/cancel` | Cancels reservation and releases slot. |
| `GET` | `/api/history?sort=&order=` | Transaction log sorted via Merge Sort or Quick Sort. |
| `GET` | `/api/history/export` | Downloads history in standard CSV format. |
| `GET` | `/api/navigation?slot=&entrance=` | Turn-by-turn guidance via Dijkstra shortest path. |
| `GET` | `/api/dsa-stats` | Live diagnostics: Hash table load factor, collisions, queue size. |
| `GET` | `/api/settings` | Retrieves current pricing rates and grace period. |
| `PUT` | `/api/settings` | Updates pricing configuration dynamically. |

---

## 8. Sample Request & Response

### Vehicle Entry (`POST /api/vehicles/entry`)
**Request:**
```json
{
  "reg_number": "KA01AB1234",
  "vehicle_type": "CAR",
  "owner_name": "Dr. Sharma",
  "contact": "+91 98765 43210"
}
```

**Response (HTTP 201 Created):**
```json
{
  "success": true,
  "message": "Vehicle successfully entered",
  "ticket_id": "TCK-20261010-1001",
  "reg_number": "KA01AB1234",
  "slot_id": "C-01",
  "entry_time": "2026-10-10 19:54:50"
}
```

---

## 9. Team of Four Organization

*   **Member 1:** Parking Management, Doubly Linked List, and Slot Allocation (`parking.c`, `linked_list.c`)
*   **Member 2:** FIFO Queue, Hash Table, Searching, and Sorting Algorithms (`queue.c`, `hash_table.c`, `sorting.c`, `searching.c`)
*   **Member 3:** C Native HTTP Backend, Storage Layer, Billing Engine, and Graph Algorithms (`server.c`, `storage.c`, `billing.c`, `graph.c`)
*   **Member 4:** Web Dashboard Frontend, REST API Integration, Test Suite, and Documentation (`frontend/`, `tests/`, `docs/`)
