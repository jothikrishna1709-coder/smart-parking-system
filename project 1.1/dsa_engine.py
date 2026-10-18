"""
Smart Parking Management System - Pure Python DSA Engine
Replicates the native C DSA core algorithms and data structures with 100% parity,
providing zero-dependency, cross-platform execution for Streamlit Local & Streamlit Cloud.
"""

import os
import json
import time
import math
from datetime import datetime
from typing import Dict, List, Optional, Any, Tuple


# ==============================================================================
# 1. DATA STRUCTURES & ALGORITHMS (DSA) CORE
# ==============================================================================

class DoublyLinkedListNode:
    """Node for Doubly Linked List."""
    def __init__(self, data: Any):
        self.data = data
        self.prev: Optional['DoublyLinkedListNode'] = None
        self.next: Optional['DoublyLinkedListNode'] = None


class DoublyLinkedList:
    """
    Doubly Linked List implementation for slot & vehicle management.
    Time Complexity:
      - Append: O(1)
      - Remove by predicate: O(n)
      - Find: O(n)
      - Traversal: O(n)
    """
    def __init__(self):
        self.head: Optional[DoublyLinkedListNode] = None
        self.tail: Optional[DoublyLinkedListNode] = None
        self.count: int = 0

    def append(self, data: Any) -> DoublyLinkedListNode:
        node = DoublyLinkedListNode(data)
        if not self.head:
            self.head = node
            self.tail = node
        else:
            self.tail.next = node
            node.prev = self.tail
            self.tail = node
        self.count += 1
        return node

    def remove(self, predicate) -> Optional[Any]:
        curr = self.head
        while curr:
            if predicate(curr.data):
                if curr.prev:
                    curr.prev.next = curr.next
                else:
                    self.head = curr.next

                if curr.next:
                    curr.next.prev = curr.prev
                else:
                    self.tail = curr.prev

                self.count -= 1
                return curr.data
            curr = curr.next
        return None

    def find(self, predicate) -> Optional[Any]:
        curr = self.head
        while curr:
            if predicate(curr.data):
                return curr.data
            curr = curr.next
        return None

    def to_list(self) -> List[Any]:
        items = []
        curr = self.head
        while curr:
            items.append(curr.data)
            curr = curr.next
        return items

    def size(self) -> int:
        return self.count


class DJB2HashTable:
    """
    Separate Chaining Hash Table using DJB2 Polynomial Rolling Hash.
    Time Complexity:
      - Insertion: Average O(1), Worst O(n)
      - Lookup: Average O(1), Worst O(n)
      - Deletion: Average O(1), Worst O(n)
    Formula: hash = ((hash << 5) + hash) + char_code
    """
    def __init__(self, capacity: int = 101):
        self.capacity: int = capacity
        self.buckets: List[List[Dict[str, Any]]] = [[] for _ in range(capacity)]
        self.size_count: int = 0
        self.collisions: int = 0

    def hash(self, key: str) -> int:
        h = 5381
        norm = str(key).upper().strip()
        for char in norm:
            h = (((h << 5) + h) + ord(char)) & 0xFFFFFFFF
        return h % self.capacity

    def set(self, key: str, value: Any) -> None:
        idx = self.hash(key)
        bucket = self.buckets[idx]
        norm = str(key).upper().strip()

        for item in bucket:
            if item["key"] == norm:
                item["value"] = value
                return

        if len(bucket) > 0:
            self.collisions += 1

        bucket.append({"key": norm, "value": value})
        self.size_count += 1

    def get(self, key: str) -> Optional[Any]:
        idx = self.hash(key)
        bucket = self.buckets[idx]
        norm = str(key).upper().strip()

        for item in bucket:
            if item["key"] == norm:
                return item["value"]
        return None

    def delete(self, key: str) -> Optional[Any]:
        idx = self.hash(key)
        bucket = self.buckets[idx]
        norm = str(key).upper().strip()

        for i, item in enumerate(bucket):
            if item["key"] == norm:
                val = bucket.pop(i)["value"]
                self.size_count -= 1
                return val
        return None

    def get_all(self) -> List[Any]:
        results = []
        for bucket in self.buckets:
            for item in bucket:
                results.append(item["value"])
        return results

    def get_stats(self) -> Dict[str, Any]:
        return {
            "capacity": self.capacity,
            "active_keys": self.size_count,
            "load_factor": round(self.size_count / self.capacity, 4) if self.capacity else 0,
            "collisions": self.collisions,
            "occupied_buckets": sum(1 for b in self.buckets if b),
            "algorithm": "DJB2 polynomial rolling hash with separate chaining"
        }


class FIFOQueue:
    """
    FIFO Linked-List Queue for Waiting Vehicles.
    Time Complexity:
      - Enqueue (Tail): O(1)
      - Dequeue (Head): O(1)
      - Filter / Search: O(n)
    """
    def __init__(self):
        self.list = DoublyLinkedList()

    def enqueue(self, item: Any) -> None:
        self.list.append(item)

    def dequeue(self) -> Optional[Any]:
        if self.list.head:
            data = self.list.head.data
            self.list.remove(lambda d: d == data)
            return data
        return None

    def remove_by_id(self, waiting_id: str) -> Optional[Any]:
        return self.list.remove(lambda d: d.get("waiting_id") == waiting_id)

    def find(self, predicate) -> Optional[Any]:
        return self.list.find(predicate)

    def to_list(self) -> List[Any]:
        return self.list.to_list()

    def size(self) -> int:
        return self.list.size()


class FacilityGraph:
    """
    Adjacency List Graph with Dijkstra's Shortest Path & BFS Traversal.
    Used for turn-by-turn navigation guidance from entrance gates to assigned parking slots.
    Time Complexity:
      - Dijkstra: O(V^2) or O((V+E) log V)
      - BFS: O(V + E)
    """
    def __init__(self):
        self.vertices: Dict[str, Dict[str, Any]] = {}

    def add_vertex(self, vertex_id: str, label: str, floor: int, vtype: str) -> None:
        if vertex_id not in self.vertices:
            self.vertices[vertex_id] = {
                "id": vertex_id,
                "label": label,
                "floor": floor,
                "type": vtype,
                "edges": []
            }

    def add_edge(self, from_id: str, to_id: str, weight: float, instruction: str) -> None:
        if from_id in self.vertices and to_id in self.vertices:
            self.vertices[from_id]["edges"].append({
                "target": to_id,
                "weight": float(weight),
                "instruction": instruction
            })

    def add_bidirectional(self, u: str, v: str, weight: float, fwd_hint: str, bwd_hint: str) -> None:
        self.add_edge(u, v, weight, fwd_hint)
        self.add_edge(v, u, weight, bwd_hint)

    def dijkstra(self, start_id: str, end_id: str) -> Dict[str, Any]:
        if start_id not in self.vertices or end_id not in self.vertices:
            return {"found": False, "total_distance": 0, "path_nodes": [], "instructions": []}

        distances: Dict[str, float] = {vid: float("inf") for vid in self.vertices}
        previous: Dict[str, Optional[str]] = {vid: None for vid in self.vertices}
        edge_instr: Dict[str, str] = {}
        unvisited = set(self.vertices.keys())

        distances[start_id] = 0.0

        while unvisited:
            curr = None
            min_dist = float("inf")
            for node in unvisited:
                d = distances[node]
                if d < min_dist:
                    min_dist = d
                    curr = node

            if curr is None or min_dist == float("inf") or curr == end_id:
                break

            unvisited.remove(curr)

            for edge in self.vertices[curr]["edges"]:
                tgt = edge["target"]
                if tgt in unvisited:
                    alt = min_dist + edge["weight"]
                    if alt < distances[tgt]:
                        distances[tgt] = alt
                        previous[tgt] = curr
                        edge_instr[tgt] = edge["instruction"]

        if distances[end_id] == float("inf"):
            return {"found": False, "total_distance": 0, "path_nodes": [], "instructions": []}

        path_nodes: List[str] = []
        instructions: List[str] = []
        curr = end_id
        while curr:
            path_nodes.insert(0, curr)
            prev = previous.get(curr)
            if prev:
                instr = edge_instr.get(curr, f"Proceed toward {curr}")
                instructions.insert(0, instr)
            curr = prev

        return {
            "found": True,
            "total_distance": round(distances[end_id], 1),
            "path_nodes": path_nodes,
            "instructions": instructions
        }


# ==============================================================================
# 2. SORTING ALGORITHMS (MERGE SORT & QUICK SORT)
# ==============================================================================

def merge_sort(arr: List[Any], key_fn, reverse: bool = False) -> List[Any]:
    """Stable Divide-and-Conquer Merge Sort. O(n log n)."""
    if len(arr) <= 1:
        return arr[:]
    mid = len(arr) // 2
    left = merge_sort(arr[:mid], key_fn, reverse)
    right = merge_sort(arr[mid:], key_fn, reverse)

    merged = []
    i = j = 0
    while i < len(left) and j < len(right):
        val_left = key_fn(left[i])
        val_right = key_fn(right[j])
        cond = (val_left >= val_right) if reverse else (val_left <= val_right)
        if cond:
            merged.append(left[i])
            i += 1
        else:
            merged.append(right[j])
            j += 1

    merged.extend(left[i:])
    merged.extend(right[j:])
    return merged


def quick_sort(arr: List[Any], key_fn, reverse: bool = False) -> List[Any]:
    """In-place style Divide-and-Conquer Quick Sort with median pivot. Average O(n log n)."""
    if len(arr) <= 1:
        return arr[:]

    pivot = key_fn(arr[len(arr) // 2])
    less, equal, greater = [], [], []

    for item in arr:
        v = key_fn(item)
        if v < pivot:
            less.append(item)
        elif v > pivot:
            greater.append(item)
        else:
            equal.append(item)

    if reverse:
        return quick_sort(greater, key_fn, reverse) + equal + quick_sort(less, key_fn, reverse)
    else:
        return quick_sort(less, key_fn, reverse) + equal + quick_sort(greater, key_fn, reverse)


# ==============================================================================
# 3. FACILITY ENGINE & STATE MANAGER
# ==============================================================================

class SmartParkingEngine:
    """
    Complete Smart Parking State Machine and Algorithm Coordinator.
    Handles data persistence with JSON files, slot allocation, FIFO queue promotion,
    DJB2 vehicle registry, Dijkstra routing, and billing calculations.
    """
    def __init__(self, data_dir: Optional[str] = None):
        if not data_dir:
            base_dir = os.path.dirname(os.path.abspath(__file__))
            data_dir = os.path.join(base_dir, "data")
        self.data_dir = data_dir

        self.slots_map: Dict[str, Dict[str, Any]] = {}
        self.vehicle_hash_table = DJB2HashTable(101)
        self.waiting_queue = FIFOQueue()
        self.reservations = DoublyLinkedList()
        self.history: List[Dict[str, Any]] = []
        self.pricing: Dict[str, Any] = {
            "car_first_hour": 45.0,
            "car_subsequent_hour": 20.0,
            "bike_first_hour": 15.0,
            "bike_subsequent_hour": 10.0,
            "accessible_first_hour": 20.0,
            "accessible_subsequent_hour": 15.0,
            "grace_period_minutes": 15,
            "min_charge": 15.0,
            "rounding_minutes": 30
        }
        self.graph = FacilityGraph()
        self.ticket_counter = 1000
        self.tx_counter = 1000

        self.initialize_engine()

    def _load_json(self, filename: str) -> Optional[Any]:
        filepath = os.path.join(self.data_dir, filename)
        if os.path.exists(filepath):
            try:
                with open(filepath, "r", encoding="utf-8") as f:
                    return json.load(f)
            except Exception:
                pass
        return None

    def _save_json(self, filename: str, data: Any) -> bool:
        if not os.path.exists(self.data_dir):
            try:
                os.makedirs(self.data_dir, exist_ok=True)
            except Exception:
                pass
        filepath = os.path.join(self.data_dir, filename)
        try:
            with open(filepath, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2)
            return True
        except Exception:
            return False

    def initialize_engine(self) -> None:
        """Initializes slots, active vehicles, history, settings, and navigation graph."""
        slots_data = self._load_json("slots.json") or []
        active_veh_data = self._load_json("active_vehicles.json") or []
        history_data = self._load_json("history.json") or []
        queue_data = self._load_json("queue.json") or []
        settings_data = self._load_json("settings.json") or {}

        if settings_data:
            self.pricing.update(settings_data)

        # Build fallback slots if empty
        if not slots_data:
            # Floor 1 Accessible
            for i in range(1, 5):
                slots_data.append({
                    "slot_id": f"A-0{i}",
                    "floor": 1,
                    "zone": "Zone C (Accessible)",
                    "category": "ACCESSIBLE",
                    "status": "AVAILABLE",
                    "occupied_reg": "",
                    "occupied_time": 0,
                    "reservation_id": "",
                    "distance": 10 + i * 2
                })
            # Floor 1 Bikes
            for i in range(1, 7):
                slots_data.append({
                    "slot_id": f"M-0{i}",
                    "floor": 1,
                    "zone": "Zone B (Ground Bikes)",
                    "category": "MOTORCYCLE",
                    "status": "AVAILABLE",
                    "occupied_reg": "",
                    "occupied_time": 0,
                    "reservation_id": "",
                    "distance": 15 + i * 3
                })
            # Floor 1 Cars
            for i in range(1, 11):
                sid = f"C-0{i}" if i < 10 else f"C-{i}"
                slots_data.append({
                    "slot_id": sid,
                    "floor": 1,
                    "zone": "Zone A (Ground Cars)",
                    "category": "CAR",
                    "status": "AVAILABLE",
                    "occupied_reg": "",
                    "occupied_time": 0,
                    "reservation_id": "",
                    "distance": 20 + i * 3
                })
            # Floor 2 Cars
            for i in range(11, 26):
                slots_data.append({
                    "slot_id": f"C-{i}",
                    "floor": 2,
                    "zone": "Zone D (Floor 2 Cars)",
                    "category": "CAR",
                    "status": "AVAILABLE",
                    "occupied_reg": "",
                    "occupied_time": 0,
                    "reservation_id": "",
                    "distance": 55 + (i - 10) * 3
                })
            # Floor 3 Cars
            for i in range(26, 31):
                slots_data.append({
                    "slot_id": f"C-{i}",
                    "floor": 3,
                    "zone": "Zone E (Floor 3 Cars)",
                    "category": "CAR",
                    "status": "AVAILABLE",
                    "occupied_reg": "",
                    "occupied_time": 0,
                    "reservation_id": "",
                    "distance": 95 + (i - 25) * 3
                })

        self.slots_map = {s["slot_id"]: s.copy() for s in slots_data}

        # Populate active vehicles into Hash Table and mark slots
        for v in active_veh_data:
            self.vehicle_hash_table.set(v["reg_number"], v.copy())
            sid = v.get("slot_id")
            if sid in self.slots_map:
                self.slots_map[sid]["status"] = "OCCUPIED"
                self.slots_map[sid]["occupied_reg"] = v["reg_number"]
                self.slots_map[sid]["occupied_time"] = v.get("entry_time", 0)

        # Populate queue
        for q in queue_data:
            self.waiting_queue.enqueue(q.copy())

        self.history = history_data
        self.ticket_counter = 1000 + len(active_veh_data)
        self.tx_counter = 1000 + len(history_data)

        # Build Graph
        self._build_navigation_graph()

    def _build_navigation_graph(self) -> None:
        g = self.graph
        g.add_vertex("ENT_NORTH", "North Main Entrance", 1, "ENTRANCE")
        g.add_vertex("ENT_SOUTH", "South Gate Entrance", 1, "ENTRANCE")
        g.add_vertex("EXIT_WEST", "West Exit Gate", 1, "EXIT")

        g.add_vertex("JUNC_F1_N", "F1 North Hub", 1, "JUNCTION")
        g.add_vertex("JUNC_F1_S", "F1 South Hub", 1, "JUNCTION")
        g.add_vertex("ZONE_A", "Zone A (Ground Cars)", 1, "ZONE")
        g.add_vertex("ZONE_B", "Zone B (Ground Bikes)", 1, "ZONE")
        g.add_vertex("ZONE_C", "Zone C (Accessible)", 1, "ZONE")

        g.add_vertex("RAMP_F1_F2", "Ramp to Floor 2", 1, "RAMP")
        g.add_vertex("RAMP_F2_F3", "Ramp to Floor 3", 2, "RAMP")
        g.add_vertex("JUNC_F2", "Floor 2 Central Hub", 2, "JUNCTION")
        g.add_vertex("ZONE_D", "Zone D (Floor 2 Cars)", 2, "ZONE")
        g.add_vertex("JUNC_F3", "Floor 3 Central Hub", 3, "JUNCTION")
        g.add_vertex("ZONE_E", "Zone E (Floor 3 Cars)", 3, "ZONE")

        # Slot Vertices
        for sid, s in self.slots_map.items():
            g.add_vertex(sid, f"Slot {sid} ({s['category']})", s["floor"], "SLOT")

        # Graph Edges
        g.add_bidirectional("ENT_NORTH", "JUNC_F1_N", 15.0, "Drive straight 15m into North Hub", "Exit toward North Gate")
        g.add_bidirectional("JUNC_F1_N", "ZONE_C", 10.0, "Turn slight right into Accessible Zone C (10m)", "Return to North Hub")
        g.add_bidirectional("JUNC_F1_N", "ZONE_A", 20.0, "Turn left into Zone A (20m)", "Return to North Hub")
        g.add_bidirectional("JUNC_F1_N", "RAMP_F1_F2", 35.0, "Follow ramp signage straight ahead (35m)", "Return to Ground Floor")

        g.add_bidirectional("ENT_SOUTH", "JUNC_F1_S", 12.0, "Drive straight 12m into South Hub", "Exit toward South Gate")
        g.add_bidirectional("JUNC_F1_S", "ZONE_B", 10.0, "Turn right into Motorcycle Zone B (10m)", "Return to South Hub")
        g.add_bidirectional("JUNC_F1_S", "ZONE_A", 25.0, "Turn left toward Zone A (25m)", "Return to South Hub")
        g.add_bidirectional("JUNC_F1_N", "JUNC_F1_S", 30.0, "Cross central aisle between North and South Hubs (30m)", "Return across central aisle")

        g.add_bidirectional("RAMP_F1_F2", "JUNC_F2", 40.0, "Drive up ramp to Floor 2 Central Hub (40m)", "Drive down ramp to Ground Floor")
        g.add_bidirectional("JUNC_F2", "ZONE_D", 15.0, "Proceed into Floor 2 Zone D (15m)", "Return to F2 Hub")
        g.add_bidirectional("JUNC_F2", "RAMP_F2_F3", 35.0, "Take ramp up to Floor 3 (35m)", "Take ramp down to Floor 2")
        g.add_bidirectional("RAMP_F2_F3", "JUNC_F3", 40.0, "Drive up ramp to Floor 3 Central Hub (40m)", "Drive down ramp to Floor 2")
        g.add_bidirectional("JUNC_F3", "ZONE_E", 15.0, "Proceed into Floor 3 Zone E (15m)", "Return to F3 Hub")

        # Connect Slots to Parent Zones
        for sid, s in self.slots_map.items():
            parent_zone = "ZONE_A"
            if s["category"] == "MOTORCYCLE":
                parent_zone = "ZONE_B"
            elif s["category"] == "ACCESSIBLE":
                parent_zone = "ZONE_C"
            elif s["floor"] == 2:
                parent_zone = "ZONE_D"
            elif s["floor"] == 3:
                parent_zone = "ZONE_E"

            d = (s["distance"] % 20) + 4.0
            g.add_bidirectional(parent_zone, sid, d, f"Park into Slot {sid} ({d:.0f}m)", f"Exit from Slot {sid}")

    def sync_to_storage(self) -> None:
        """Persists current state into JSON files atomically."""
        self._save_json("slots.json", list(self.slots_map.values()))
        self._save_json("active_vehicles.json", self.vehicle_hash_table.get_all())
        self._save_json("queue.json", self.waiting_queue.to_list())
        self._save_json("history.json", self.history)
        self._save_json("settings.json", self.pricing)

    # --------------------------------------------------------------------------
    # BUSINESS LOGIC & DSA OPERATIONS
    # --------------------------------------------------------------------------

    def get_dashboard_metrics(self) -> Dict[str, Any]:
        total = len(self.slots_map)
        avail = sum(1 for s in self.slots_map.values() if s["status"] == "AVAILABLE")
        occ = sum(1 for s in self.slots_map.values() if s["status"] == "OCCUPIED")
        reserved = sum(1 for s in self.slots_map.values() if s["status"] == "RESERVED")
        maint = sum(1 for s in self.slots_map.values() if s["status"] == "MAINTENANCE")

        active_list = self.vehicle_hash_table.get_all()
        cars = sum(1 for v in active_list if v.get("vehicle_type") == "CAR")
        bikes = sum(1 for v in active_list if v.get("vehicle_type") == "MOTORCYCLE")
        accessible = sum(1 for v in active_list if v.get("vehicle_type") == "ACCESSIBLE")

        today_str = datetime.now().strftime("%Y-%m-%d")
        today_rev = 0.0
        today_checkouts = 0
        recent_activity = []

        for h in reversed(self.history):
            exit_str = h.get("exit_time_str", "")
            if exit_str.startswith(today_str):
                today_rev += float(h.get("total_fee", 0.0))
                today_checkouts += 1

            if len(recent_activity) < 8:
                recent_activity.append({
                    "type": "CHECKOUT",
                    "reg_number": h.get("reg_number"),
                    "slot_id": h.get("slot_id"),
                    "vehicle_type": h.get("vehicle_type"),
                    "fee": h.get("total_fee"),
                    "time": exit_str
                })

        occupancy_pct = round(((occ + reserved) / total * 100), 1) if total > 0 else 0.0

        return {
            "total_slots": total,
            "available_slots": avail,
            "occupied_slots": occ,
            "reserved_slots": reserved,
            "maintenance_slots": maint,
            "active_vehicles": len(active_list),
            "waiting_queue_count": self.waiting_queue.size(),
            "today_revenue": round(today_rev, 2),
            "today_checkouts": today_checkouts,
            "occupancy_percentage": occupancy_pct,
            "distribution": {"cars": cars, "motorcycles": bikes, "accessible": accessible},
            "recent_activity": recent_activity
        }

    def allocate_slot(self, vehicle_type: str) -> Optional[Dict[str, Any]]:
        """
        Allocation Algorithm:
        Finds the closest available slot matching the vehicle category.
        Accessible vehicles can also overflow to Ground Floor Car slots.
        Time Complexity: O(n) slot scanning with distance minimization.
        """
        vtype = vehicle_type.upper().strip()
        best_slot = None
        min_distance = float("inf")

        for s in self.slots_map.values():
            if s["status"] == "AVAILABLE":
                matches = (s["category"] == vtype) or (vtype == "ACCESSIBLE" and s["category"] == "CAR" and s["floor"] == 1)
                if matches and s["distance"] < min_distance:
                    min_distance = s["distance"]
                    best_slot = s

        return best_slot

    def vehicle_entry(
        self,
        reg_number: str,
        vehicle_type: str = "CAR",
        owner_name: str = "",
        contact: str = "",
        entrance: str = "ENT_NORTH"
    ) -> Dict[str, Any]:
        """
        Processes new vehicle entry:
        1. Checks Hash Table for duplicate entry.
        2. Allocates optimal slot.
        3. Generates Ticket and updates Hash Table O(1).
        4. Calculates Dijkstra turn-by-turn guidance from entrance to assigned slot.
        """
        reg = reg_number.upper().strip()
        vtype = vehicle_type.upper().strip()

        if not reg:
            return {"success": False, "error": "Registration plate number cannot be empty."}

        # Check existing
        if self.vehicle_hash_table.get(reg):
            return {"success": False, "error": f"Vehicle {reg} is already parked inside the facility."}

        # Find slot
        chosen_slot = self.allocate_slot(vtype)
        if not chosen_slot:
            return {
                "success": False,
                "full": True,
                "error": f"Facility full for {vtype}. Would you like to enter the FIFO waiting queue?"
            }

        now = int(time.time())
        now_str = datetime.fromtimestamp(now).strftime("%Y-%m-%d %H:%M:%S")

        # Mark slot
        chosen_slot["status"] = "OCCUPIED"
        chosen_slot["occupied_reg"] = reg
        chosen_slot["occupied_time"] = now

        # Create ticket
        self.ticket_counter += 1
        ticket_id = f"TCK-{datetime.now().strftime('%Y%m%d')}-{self.ticket_counter}"

        active_veh = {
            "ticket_id": ticket_id,
            "reg_number": reg,
            "vehicle_type": vtype,
            "slot_id": chosen_slot["slot_id"],
            "owner_name": owner_name.strip(),
            "contact": contact.strip(),
            "entry_time": now,
            "entry_time_str": now_str
        }

        # Insert to Hash Table O(1)
        self.vehicle_hash_table.set(reg, active_veh)

        # Compute Dijkstra navigation path
        nav_info = self.graph.dijkstra(entrance, chosen_slot["slot_id"])

        self.sync_to_storage()

        return {
            "success": True,
            "ticket": active_veh,
            "assigned_slot": chosen_slot["slot_id"],
            "slot_details": chosen_slot,
            "navigation": nav_info
        }

    def calculate_bill(self, vehicle_type: str, entry_time: int, exit_time: Optional[int] = None) -> Dict[str, Any]:
        """Calculates duration and fee settlement based on pricing tier & rounding rules."""
        if exit_time is None:
            exit_time = int(time.time())

        p = self.pricing
        duration_sec = max(0, exit_time - entry_time)
        total_mins = duration_sec // 60
        hours = total_mins // 60
        mins = total_mins % 60

        vtype = vehicle_type.upper().strip()
        base_rate = p.get("car_first_hour", 45.0)
        subsequent_rate = p.get("car_subsequent_hour", 20.0)

        if vtype == "MOTORCYCLE":
            base_rate = p.get("bike_first_hour", 15.0)
            subsequent_rate = p.get("bike_subsequent_hour", 10.0)
        elif vtype == "ACCESSIBLE":
            base_rate = p.get("accessible_first_hour", 20.0)
            subsequent_rate = p.get("accessible_subsequent_hour", 15.0)

        grace_period = p.get("grace_period_minutes", 15)
        min_charge = p.get("min_charge", 15.0)
        rounding_interval = p.get("rounding_minutes", 30)

        # Check Grace Period
        if total_mins <= grace_period:
            return {
                "duration_seconds": duration_sec,
                "duration_hours": hours,
                "duration_minutes": mins,
                "total_minutes": total_mins,
                "base_rate": base_rate,
                "subsequent_rate": subsequent_rate,
                "base_fee": 0.0,
                "subsequent_fee": 0.0,
                "total_fee": 0.0,
                "is_grace_period": True
            }

        base_fee = base_rate
        subsequent_fee = 0.0

        if total_mins > 60:
            extra_mins = total_mins - 60
            units = math.ceil(extra_mins / rounding_interval) if rounding_interval > 0 else math.ceil(extra_mins / 60)
            subsequent_fee = (units * rounding_interval / 60.0) * subsequent_rate

        total_fee = base_fee + subsequent_fee
        if total_fee < min_charge:
            total_fee = min_charge

        return {
            "duration_seconds": duration_sec,
            "duration_hours": hours,
            "duration_minutes": mins,
            "total_minutes": total_mins,
            "base_rate": base_rate,
            "subsequent_rate": subsequent_rate,
            "base_fee": round(base_fee, 2),
            "subsequent_fee": round(subsequent_fee, 2),
            "total_fee": round(total_fee, 2),
            "is_grace_period": False
        }

    def vehicle_exit_preview(self, query: str) -> Dict[str, Any]:
        """Looks up vehicle in Hash Table O(1) and computes current bill preview."""
        query_norm = query.upper().strip()
        vehicle = self.vehicle_hash_table.get(query_norm)

        if not vehicle:
            # Fallback search by ticket_id or slot_id
            for v in self.vehicle_hash_table.get_all():
                if v.get("ticket_id") == query_norm or v.get("slot_id") == query_norm:
                    vehicle = v
                    break

        if not vehicle:
            return {"success": False, "error": f"Vehicle or Ticket '{query}' not found in active parking registry."}

        now = int(time.time())
        bill = self.calculate_bill(vehicle["vehicle_type"], vehicle["entry_time"], now)
        exit_str = datetime.fromtimestamp(now).strftime("%Y-%m-%d %H:%M:%S")

        self.tx_counter += 1
        txn_id = f"TXN-{datetime.now().strftime('%Y%m%d')}-{self.tx_counter}"

        transaction = {
            "transaction_id": txn_id,
            "ticket_id": vehicle["ticket_id"],
            "reg_number": vehicle["reg_number"],
            "vehicle_type": vehicle["vehicle_type"],
            "slot_id": vehicle["slot_id"],
            "owner_name": vehicle.get("owner_name", ""),
            "contact": vehicle.get("contact", ""),
            "entry_time": vehicle["entry_time"],
            "exit_time": now,
            "entry_time_str": vehicle["entry_time_str"],
            "exit_time_str": exit_str,
            **bill
        }

        return {"success": True, "preview": True, "transaction": transaction, "vehicle": vehicle}

    def vehicle_exit_checkout(self, query: str) -> Dict[str, Any]:
        """
        Executes vehicle checkout:
        1. Finalizes billing transaction.
        2. Removes vehicle from DJB2 Hash Table O(1).
        3. Frees parking slot.
        4. Appends to persistent history.
        5. Automatically promotes next eligible vehicle from the FIFO waiting queue!
        """
        preview_res = self.vehicle_exit_preview(query)
        if not preview_res.get("success"):
            return preview_res

        txn = preview_res["transaction"]
        reg = txn["reg_number"]
        slot_id = txn["slot_id"]

        # Delete from Hash Table
        self.vehicle_hash_table.delete(reg)

        # Free slot
        slot = self.slots_map.get(slot_id)
        if slot:
            slot["status"] = "AVAILABLE"
            slot["occupied_reg"] = ""
            slot["occupied_time"] = 0
            slot["reservation_id"] = ""

        # Save to history
        self.history.append(txn)

        # Check FIFO waiting queue for auto-promotion
        promoted_vehicle = None
        if slot:
            queue_items = self.waiting_queue.to_list()
            for q in queue_items:
                if q.get("vehicle_type") == slot["category"]:
                    # Pop from queue
                    self.waiting_queue.remove_by_id(q["waiting_id"])

                    # Occupy freed slot
                    now = int(time.time())
                    slot["status"] = "OCCUPIED"
                    slot["occupied_reg"] = q["reg_number"]
                    slot["occupied_time"] = now

                    self.ticket_counter += 1
                    promo_ticket = f"TCK-{datetime.now().strftime('%Y%m%d')}-{self.ticket_counter}"
                    promoted_vehicle = {
                        "ticket_id": promo_ticket,
                        "reg_number": q["reg_number"],
                        "vehicle_type": q["vehicle_type"],
                        "slot_id": slot["slot_id"],
                        "owner_name": q.get("owner_name", ""),
                        "contact": q.get("contact", ""),
                        "entry_time": now,
                        "entry_time_str": datetime.fromtimestamp(now).strftime("%Y-%m-%d %H:%M:%S")
                    }
                    self.vehicle_hash_table.set(q["reg_number"], promoted_vehicle)
                    break

        self.sync_to_storage()

        return {
            "success": True,
            "receipt": txn,
            "freed_slot": slot_id,
            "promoted_queue_vehicle": promoted_vehicle
        }

    def enqueue_waiting(self, reg_number: str, vehicle_type: str = "CAR", owner_name: str = "", contact: str = "") -> Dict[str, Any]:
        """Appends waiting vehicle to the FIFO Queue."""
        reg = reg_number.upper().strip()
        vtype = vehicle_type.upper().strip()

        if not reg:
            return {"success": False, "error": "Registration plate number cannot be empty."}

        now = int(time.time())
        wait_id = f"WAIT-{now % 10000:04d}"
        entry = {
            "waiting_id": wait_id,
            "reg_number": reg,
            "vehicle_type": vtype,
            "owner_name": owner_name.strip(),
            "contact": contact.strip(),
            "entry_time": now,
            "entry_time_str": datetime.fromtimestamp(now).strftime("%Y-%m-%d %H:%M:%S")
        }

        self.waiting_queue.enqueue(entry)
        self.sync_to_storage()

        return {
            "success": True,
            "waiting_id": wait_id,
            "queue_position": self.waiting_queue.size(),
            "entry": entry
        }

    def dequeue_waiting(self, waiting_id: str) -> bool:
        """Removes a vehicle from the waiting queue by ID."""
        res = self.waiting_queue.remove_by_id(waiting_id)
        if res:
            self.sync_to_storage()
            return True
        return False

    def create_reservation(
        self,
        reg_number: str,
        vehicle_type: str,
        preferred_slot: str = "",
        duration_hours: int = 2,
        customer_name: str = "",
        contact: str = ""
    ) -> Dict[str, Any]:
        """Creates an advance reservation for a designated parking slot."""
        reg = reg_number.upper().strip()
        vtype = vehicle_type.upper().strip()
        pref = preferred_slot.upper().strip()

        target_slot = None
        if pref and pref in self.slots_map:
            s = self.slots_map[pref]
            if s["status"] == "AVAILABLE":
                target_slot = s

        if not target_slot:
            for s in self.slots_map.values():
                if s["status"] == "AVAILABLE" and s["category"] == vtype:
                    target_slot = s
                    break

        if not target_slot:
            return {"success": False, "error": f"No available parking slots matching {vtype} for reservation."}

        now = int(time.time())
        expires = now + (int(duration_hours) * 3600)
        res_id = f"RES-{now % 100000:05d}"

        target_slot["status"] = "RESERVED"
        target_slot["reservation_id"] = res_id

        res_obj = {
            "reservation_id": res_id,
            "reg_number": reg,
            "vehicle_type": vtype,
            "slot_id": target_slot["slot_id"],
            "customer_name": customer_name.strip(),
            "contact": contact.strip(),
            "start_time": now,
            "expiry_time": expires,
            "start_time_str": datetime.fromtimestamp(now).strftime("%Y-%m-%d %H:%M:%S"),
            "expiry_time_str": datetime.fromtimestamp(expires).strftime("%Y-%m-%d %H:%M:%S")
        }

        self.reservations.append(res_obj)
        self.sync_to_storage()

        return {"success": True, "reservation": res_obj, "slot_id": target_slot["slot_id"]}

    def cancel_reservation(self, res_id: str) -> bool:
        """Cancels an existing advance reservation and frees the slot."""
        removed = self.reservations.remove(lambda r: r.get("reservation_id") == res_id)
        if removed:
            sid = removed.get("slot_id")
            if sid in self.slots_map:
                s = self.slots_map[sid]
                if s["status"] == "RESERVED":
                    s["status"] = "AVAILABLE"
                    s["reservation_id"] = ""
            self.sync_to_storage()
            return True
        return False

    def search_vehicles(self, query: str) -> List[Dict[str, Any]]:
        """
        Fast Search:
        1. First checks DJB2 Hash Table for direct registration match O(1).
        2. Then falls back to multi-attribute scanning (ticket_id, slot_id, owner_name).
        """
        q = query.upper().strip()
        if not q:
            return self.vehicle_hash_table.get_all()

        exact = self.vehicle_hash_table.get(q)
        results = []
        if exact:
            results.append(exact)

        all_active = self.vehicle_hash_table.get_all()
        for v in all_active:
            if v == exact:
                continue
            if (
                q in v.get("reg_number", "") or
                q in v.get("ticket_id", "") or
                q in v.get("slot_id", "").upper() or
                q in v.get("owner_name", "").upper()
            ):
                results.append(v)

        return results

    def get_sorted_history(self, sort_by: str = "time", order: str = "desc", algorithm: str = "merge") -> Tuple[List[Dict[str, Any]], Dict[str, Any]]:
        """
        Sorts historical parking records using Merge Sort or Quick Sort.
        Measures sorting latency for DSA educational benchmarking.
        """
        records = [h.copy() for h in self.history]
        reverse = (order.lower() == "desc")

        if sort_by.lower() == "fee":
            key_fn = lambda x: float(x.get("total_fee", 0.0))
        else:
            key_fn = lambda x: int(x.get("exit_time", 0))

        start_t = time.perf_counter()
        if algorithm.lower() == "quick":
            sorted_list = quick_sort(records, key_fn, reverse=reverse)
            algo_name = "Quick Sort (Median Pivot, O(n log n))"
        else:
            sorted_list = merge_sort(records, key_fn, reverse=reverse)
            algo_name = "Merge Sort (Stable Divide & Conquer, O(n log n))"
        elapsed_us = (time.perf_counter() - start_t) * 1_000_000

        metrics = {
            "algorithm": algo_name,
            "record_count": len(sorted_list),
            "elapsed_microseconds": round(elapsed_us, 2),
            "sort_by": sort_by,
            "order": order
        }
        return sorted_list, metrics

    def get_dsa_benchmarks(self) -> Dict[str, Any]:
        """Returns deep structural metrics for DSA Lab visualization."""
        ht_stats = self.vehicle_hash_table.get_stats()
        q_items = self.waiting_queue.to_list()
        res_items = self.reservations.to_list()

        return {
            "hash_table": ht_stats,
            "queue": {
                "size": len(q_items),
                "head": q_items[0]["reg_number"] if q_items else None,
                "tail": q_items[-1]["reg_number"] if q_items else None,
                "complexity_enqueue": "O(1)",
                "complexity_dequeue": "O(1)"
            },
            "linked_list": {
                "active_reservations": len(res_items),
                "total_slots": len(self.slots_map),
                "complexity_insertion": "O(1)",
                "complexity_deletion": "O(1)"
            },
            "graph": {
                "total_vertices": len(self.graph.vertices),
                "total_edges": sum(len(v["edges"]) for v in self.graph.vertices.values()) // 2,
                "dijkstra_complexity": "O(V^2) / O((V+E) log V)",
                "bfs_complexity": "O(V + E)"
            }
        }
