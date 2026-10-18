# Data Structures & Algorithms (DSA) Comprehensive Analysis
**Project:** Smart Parking Management System (College Capstone Project)  
**Implementation Language:** Pure C (Core Algorithms & Data Structures)

---

## Executive Summary of Implemented DSA Modules

This document provides a rigorous mathematical and structural analysis of all custom Data Structures and Algorithms engineered from scratch in pure C for the Smart Parking Management System. No third-party data structure libraries were utilized.

| Component | Abstract Data Type / Algorithm | Source File | Header File | Primary Operation | Time Complexity (Avg / Worst) | Auxiliary Space |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Slot Management** | Doubly Linked List | `backend/src/linked_list.c` | `linked_list.h` | Insert / Delete / Traverse | $O(1)$ / $O(n)$ | $O(n)$ |
| **Waiting Vehicles** | FIFO Queue (Head/Tail Ptrs) | `backend/src/queue.c` | `queue.h` | Enqueue / Dequeue | $O(1)$ / $O(1)$ | $O(n)$ |
| **Plate Index** | Separate Chaining Hash Table | `backend/src/hash_table.c` | `hash_table.h` | Search / Insert / Delete | $O(1)$ / $O(n)$ | $O(N + K)$ |
| **History Sorting** | Merge Sort (Stable) | `backend/src/sorting.c` | `sorting.h` | Sort by Fee / Timestamp | $O(n \log n)$ / $O(n \log n)$ | $O(n)$ |
| **Active Sorting** | Quick Sort (Median-of-3) | `backend/src/sorting.c` | `sorting.h` | Sort by Plate / Slot ID | $O(n \log n)$ / $O(n^2)$ | $O(\log n)$ |
| **Key Lookup** | Binary Search | `backend/src/searching.c` | `searching.h` | Preconditioned Key Search | $O(\log n)$ / $O(\log n)$ | $O(1)$ |
| **Attribute Search** | Linear Search | `backend/src/searching.c` | `searching.h` | Multi-attribute scan | $O(n)$ / $O(n)$ | $O(1)$ |
| **Facility Routing** | Dijkstra's Shortest Path | `backend/src/graph.c` | `graph.h` | Entrance-to-Slot Navigation| $O(V^2)$ / $O(V^2)$ | $O(V)$ |
| **Unweighted Reach** | Breadth-First Search (BFS) | `backend/src/graph.c` | `graph.h` | Minimum Hop Reachability | $O(V + E)$ / $O(V + E)$ | $O(V)$ |

---

## 1. Doubly Linked List (`linked_list.c`)

### 1.1 Architectural Rationale
Parking slots and active vehicles represent dynamic linear sequences that experience frequent deletions (upon checkout) and insertions (upon entry). A doubly linked list facilitates $O(1)$ additions to the head or tail and allows safe node removal given a pointer without requiring traversal from the head.

### 1.2 Structure Definition
```c
typedef struct Node {
    void *data;
    struct Node *next;
    struct Node *prev;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    size_t size;
    void (*free_data)(void *data);
} LinkedList;
```

### 1.3 Operation Complexities
*   **Insert Tail (`ll_insert_end`):** $O(1)$ time, $O(1)$ space. Directly updates `tail->next` and adjusts list tail pointer.
*   **Insert Head (`ll_insert_head`):** $O(1)$ time, $O(1)$ space. Updates `head->prev` and adjusts head pointer.
*   **Delete Node by Predicate (`ll_delete`):** $O(n)$ search time, $O(1)$ pointer rearrangement. Updates `prev->next = curr->next` and `next->prev = curr->prev`.
*   **Search (`ll_find`):** $O(n)$ time. Linear scan using user-provided predicate function.
*   **Size Check (`ll_size`):** $O(1)$ time. Maintained via incremental tracking in container struct.

---

## 2. FIFO Queue (`queue.c`)

### 2.1 Architectural Rationale
When the parking facility reaches maximum capacity for a specific category (e.g., all 24 car slots occupied), incoming vehicles must enter a strict First-In, First-Out (FIFO) waiting line. The custom queue maintains separate `front` and `rear` pointers to ensure true $O(1)$ enqueue and dequeue operations.

### 2.2 Order-Preserving Eligibility Dequeue
When an occupied slot is released upon checkout, the system identifies the vehicle category of the newly freed slot (e.g., `MOTORCYCLE`). If the front waiting vehicle is a `CAR`, dequeuing it would place an oversized vehicle in a motorcycle slot. The algorithm implements `queue_dequeue_eligible`:
*   Scans sequentially from `front` to `rear`.
*   Identifies the first vehicle matching the available category.
*   Slices the node out of the queue, splicing `prev->next = curr->next`.
*   Preserves the relative FIFO order of all remaining waiting vehicles.
*   **Complexity:** $O(k)$ where $k \le n$ is the position of the first eligible vehicle; $O(1)$ in the average case where front of queue matches.

---

## 3. Separate Chaining Hash Table (`hash_table.c`)

### 3.1 Mathematical Formulation: DJB2 Polynomial Rolling Hash
Registration plates are normalized uppercase alphanumeric strings (e.g., `"TN01AB1234"`). The hash index $h(k)$ is calculated via Daniel J. Bernstein's DJB2 algorithm:
$$h_0 = 5381$$
$$h_{i} = ((h_{i-1} \ll 5) + h_{i-1}) + c_i = 33 \cdot h_{i-1} + c_i$$
$$\text{Index} = h_{len} \pmod{\text{Capacity}}$$

The multiplier $33$ has proven empirical distribution properties for alphanumeric strings, minimizing clustering.

### 3.2 Collision Resolution & Dynamic Resizing
*   **Resolution:** Separate Chaining with singly linked collision chains.
*   **Load Factor Threshold:** $\alpha = \frac{N}{M} \le 0.75$.
*   **Dynamic Rehashing:** When an insertion causes $\frac{N+1}{M} > 0.75$, the table dynamically allocates a new bucket array of size $2M$, rehashes all existing nodes, and frees the previous bucket array in $O(N + M)$ time.
*   **Complexity:**
    *   **Search:** Average $O(1 + \alpha) = O(1)$, Worst case $O(N)$ when all keys hash to a single bucket.
    *   **Insert:** Average $O(1)$, Worst case $O(N)$.
    *   **Delete:** Average $O(1)$, Worst case $O(N)$.

---

## 4. Sorting Algorithms (`sorting.c`)

### 4.1 Merge Sort (Stable Divide-and-Conquer)
Used for sorting historical transaction records by parking fee or duration.
*   **Mechanism:** Divide the array at midpoint $m = \lfloor \frac{l + r}{2} \rfloor$, recursively sort both halves, and merge sorted sub-arrays into an auxiliary buffer of size $N$.
*   **Stability:** Preserves initial arrival sequence when transactions share identical fees.
*   **Recurrence Relation:** $T(n) = 2T(n/2) + \Theta(n) \implies T(n) = \Theta(n \log n)$ across all cases (Best, Average, Worst).
*   **Auxiliary Space:** $O(n)$ for temporary merge buffer.

### 4.2 Quick Sort with Median-of-Three Partitioning
Used for sorting active vehicles by registration number or slot ID.
*   **Median-of-Three Pivot Selection:** Compares the elements at `left`, `mid`, and `right`, selecting their median as the partitioning pivot. This avoids the $O(n^2)$ worst-case degeneration on pre-sorted inputs.
*   **Lomuto/Hoare In-Place Partitioning:** Swaps bytes directly in memory with minimal scratch buffer.
*   **Time Complexity:** Best $O(n \log n)$, Average $O(n \log n)$, Worst $O(n^2)$ (astronomically rare with median-of-three).
*   **Auxiliary Space:** $O(\log n)$ call stack space.

---

## 5. Searching Algorithms (`searching.c`)

### 5.1 Binary Search with Strict Precondition Verification
Binary Search operates in $O(\log n)$ by bisecting the sorted interval:
$$mid = low + \left\lfloor \frac{high - low}{2} \right\rfloor$$
*   **Precondition Enforcement:** Before executing, `is_sorted_array()` validates that the array is strictly monotonic relative to the comparator in $O(n)$ time. If unsorted, the search aborts safely with `-1`, preventing undefined infinite loops.

### 5.2 Linear Search
Used for filtering active vehicles across multi-field text criteria (e.g., partial substring matching on driver name, slot zone, or plate). Operates in $O(n)$ time with $O(1)$ auxiliary space.

---

## 6. Graph Facility Navigation (`graph.c`)

### 6.1 Facility Representation
The multi-level parking facility is modeled as a directed weighted graph $G = (V, E)$ using an Adjacency List:
*   **Vertices ($|V| = 20$):** Entrances (`ENT_NORTH`, `ENT_SOUTH`), Hubs/Junctions (`JUNC_F1_N`, `JUNC_F2`), Ramps (`RAMP_F1_F2`), Parking Zones (`ZONE_A`, `ZONE_B`), and Slots (`C-01`, `M-01`).
*   **Edges ($|E| = 34$):** Driving paths between waypoints with weights representing physical distance in meters ($w(u, v) \ge 0$).

### 6.2 Dijkstra's Algorithm
Computes the exact shortest driving distance and waypoint path from an Entrance to a designated Parking Slot:
1. Initialize $\text{dist}[v] = \infty$, $\text{dist}[start] = 0$.
2. In each iteration, extract the unvisited vertex $u$ with minimum $\text{dist}[u]$.
3. Relax each outgoing edge $(u, v) \in E$:
   $$\text{If } \text{dist}[u] + w(u, v) < \text{dist}[v] \implies \text{dist}[v] = \text{dist}[u] + w(u, v), \quad \text{parent}[v] = u$$
4. Reconstruct path by backtracking from target slot to entrance.
*   **Time Complexity:** $O(V^2)$ with flat vertex set, or $O(E \log V)$ with binary heap. For facility topology ($|V| \le 150$), execution is sub-millisecond.

### 6.3 Breadth-First Search (BFS)
Traverses the graph level-by-level using a FIFO queue to determine the path with the minimum number of navigational turns/hops regardless of physical distance.
*   **Time Complexity:** $O(V + E)$
*   **Space Complexity:** $O(V)$ for queue and visited set.

---

## 7. Dynamic Allocation and Memory Safety Summary

All modules adhere strictly to dynamic allocation best practices:
1. **Explicit Allocation Check:** Every call to `malloc()`, `calloc()`, or `realloc()` is checked against `NULL` before dereferencing.
2. **Dedicated Free Routines:** Every ADT provides an associated destructor (`ll_destroy`, `queue_destroy`, `ht_destroy`, `graph_destroy`) that cleanly traverses and deallocates every dynamically allocated node.
3. **No Double-Frees:** Pointers are zeroed or container structs freed atomically.
4. **Leak-Free Verified:** Confirmed across all 24 unit test cases with zero residual allocations.
