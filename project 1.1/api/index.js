/**
 * Smart Parking Management System - Cloud Serverless & Node.js DSA Engine
 * Replicates the pure C DSA backend for seamless Vercel Cloud Serverless execution.
 */

const fs = require('fs');
const path = require('path');
const http = require('http');

// ==========================================
// 1. PURE DATA STRUCTURES & ALGORITHMS (DSA)
// ==========================================

class DoublyLinkedListNode {
    constructor(data) {
        this.data = data;
        this.prev = null;
        this.next = null;
    }
}

class DoublyLinkedList {
    constructor() {
        this.head = null;
        this.tail = null;
        this.count = 0;
    }

    append(data) {
        const node = new DoublyLinkedListNode(data);
        if (!this.head) {
            this.head = node;
            this.tail = node;
        } else {
            this.tail.next = node;
            node.prev = this.tail;
            this.tail = node;
        }
        this.count++;
        return node;
    }

    remove(predicate) {
        let curr = this.head;
        while (curr) {
            if (predicate(curr.data)) {
                if (curr.prev) curr.prev.next = curr.next;
                else this.head = curr.next;

                if (curr.next) curr.next.prev = curr.prev;
                else this.tail = curr.prev;

                this.count--;
                return curr.data;
            }
            curr = curr.next;
        }
        return null;
    }

    find(predicate) {
        let curr = this.head;
        while (curr) {
            if (predicate(curr.data)) return curr.data;
            curr = curr.next;
        }
        return null;
    }

    toArray() {
        const arr = [];
        let curr = this.head;
        while (curr) {
            arr.push(curr.data);
            curr = curr.next;
        }
        return arr;
    }

    size() {
        return this.count;
    }
}

// DJB2 Polynomial Rolling Hash Table with Separate Chaining
class DJB2HashTable {
    constructor(capacity = 101) {
        this.capacity = capacity;
        this.buckets = new Array(capacity).fill(null).map(() => []);
        this.size = 0;
        this.collisions = 0;
    }

    hash(key) {
        let h = 5381;
        const str = String(key).toUpperCase().trim();
        for (let i = 0; i < str.length; i++) {
            h = (((h << 5) + h) + str.charCodeAt(i)) >>> 0;
        }
        return h % this.capacity;
    }

    set(key, value) {
        const idx = this.hash(key);
        const bucket = this.buckets[idx];
        const normalized = String(key).toUpperCase().trim();

        for (let i = 0; i < bucket.length; i++) {
            if (bucket[i].key === normalized) {
                bucket[i].value = value;
                return;
            }
        }

        if (bucket.length > 0) {
            this.collisions++;
        }

        bucket.push({ key: normalized, value });
        this.size++;
    }

    get(key) {
        const idx = this.hash(key);
        const bucket = this.buckets[idx];
        const normalized = String(key).toUpperCase().trim();

        for (let i = 0; i < bucket.length; i++) {
            if (bucket[i].key === normalized) return bucket[i].value;
        }
        return null;
    }

    delete(key) {
        const idx = this.hash(key);
        const bucket = this.buckets[idx];
        const normalized = String(key).toUpperCase().trim();

        for (let i = 0; i < bucket.length; i++) {
            if (bucket[i].key === normalized) {
                const item = bucket.splice(i, 1)[0];
                this.size--;
                return item.value;
            }
        }
        return null;
    }

    getAll() {
        const result = [];
        for (const bucket of this.buckets) {
            for (const item of bucket) {
                result.push(item.value);
            }
        }
        return result;
    }

    getStats() {
        return {
            buckets: this.capacity,
            active_keys: this.size,
            load_factor: +(this.size / this.capacity).toFixed(4),
            collisions: this.collisions,
            algorithm: 'DJB2 polynomial rolling hash with separate chaining'
        };
    }
}

// FIFO Queue for Waiting Vehicles
class FIFOQueue {
    constructor() {
        this.list = new DoublyLinkedList();
    }

    enqueue(item) {
        this.list.append(item);
    }

    dequeue() {
        if (this.list.head) {
            const data = this.list.head.data;
            this.list.remove((d) => d === data);
            return data;
        }
        return null;
    }

    removeById(id) {
        return this.list.remove((d) => d.waiting_id === id);
    }

    find(predicate) {
        return this.list.find(predicate);
    }

    toArray() {
        return this.list.toArray();
    }

    size() {
        return this.list.size();
    }
}

// Adjacency List Graph with Dijkstra's Shortest Path & BFS
class FacilityGraph {
    constructor() {
        this.vertices = new Map(); // id -> { id, label, floor, type, edges: [] }
    }

    addVertex(id, label, floor, type) {
        if (!this.vertices.has(id)) {
            this.vertices.set(id, { id, label, floor, type, edges: [] });
        }
        return this.vertices.get(id);
    }

    addEdge(fromId, toId, weight, instruction) {
        const u = this.vertices.get(fromId);
        const v = this.vertices.get(toId);
        if (!u || !v) return;
        u.edges.push({ target: toId, weight, instruction });
    }

    addBidirectional(u, v, weight, forwardHint, backwardHint) {
        this.addEdge(u, v, weight, forwardHint);
        this.addEdge(v, u, weight, backwardHint);
    }

    dijkstra(startId, endId) {
        if (!this.vertices.has(startId) || !this.vertices.has(endId)) {
            return { found: false, total_distance: 0, path_nodes: [], instructions: [] };
        }

        const distances = new Map();
        const previous = new Map();
        const edgeInstr = new Map();
        const unvisited = new Set(this.vertices.keys());

        for (const id of this.vertices.keys()) {
            distances.set(id, Infinity);
        }
        distances.set(startId, 0);

        while (unvisited.size > 0) {
            let curr = null;
            let minDist = Infinity;
            for (const node of unvisited) {
                const dist = distances.get(node);
                if (dist < minDist) {
                    minDist = dist;
                    curr = node;
                }
            }

            if (!curr || minDist === Infinity || curr === endId) break;
            unvisited.delete(curr);

            const vObj = this.vertices.get(curr);
            for (const edge of vObj.edges) {
                if (!unvisited.has(edge.target)) continue;
                const alt = minDist + edge.weight;
                if (alt < distances.get(edge.target)) {
                    distances.set(edge.target, alt);
                    previous.set(edge.target, curr);
                    edgeInstr.set(edge.target, edge.instruction);
                }
            }
        }

        if (distances.get(endId) === Infinity) {
            return { found: false, total_distance: 0, path_nodes: [], instructions: [] };
        }

        const pathNodes = [];
        const instructions = [];
        let curr = endId;
        while (curr) {
            pathNodes.unshift(curr);
            const prev = previous.get(curr);
            if (prev) {
                instructions.unshift(edgeInstr.get(curr) || `Proceed toward ${curr}`);
            }
            curr = prev;
        }

        return {
            found: true,
            total_distance: distances.get(endId),
            path_nodes: pathNodes,
            instructions
        };
    }
}

// Sorting Algorithms: Merge Sort and Quick Sort
function mergeSort(arr, compareFn) {
    if (arr.length <= 1) return arr;
    const mid = Math.floor(arr.length / 2);
    const left = mergeSort(arr.slice(0, mid), compareFn);
    const right = mergeSort(arr.slice(mid), compareFn);

    const merged = [];
    let i = 0, j = 0;
    while (i < left.length && j < right.length) {
        if (compareFn(left[i], right[j]) <= 0) {
            merged.push(left[i++]);
        } else {
            merged.push(right[j++]);
        }
    }
    return merged.concat(left.slice(i)).concat(right.slice(j));
}

function quickSort(arr, compareFn) {
    if (arr.length <= 1) return arr.slice();
    const pivot = arr[Math.floor(arr.length / 2)];
    const less = [];
    const equal = [];
    const greater = [];

    for (const item of arr) {
        const cmp = compareFn(item, pivot);
        if (cmp < 0) less.push(item);
        else if (cmp > 0) greater.push(item);
        else equal.push(item);
    }

    return quickSort(less, compareFn).concat(equal).concat(quickSort(greater, compareFn));
}

// ==========================================
// 2. PARKING LOT ENGINE & STATE
// ==========================================

let g_lot = null;

function initializeParkingEngine() {
    if (g_lot) return g_lot;

    // Load data from files or fallback
    let slotsData = [];
    let activeVehiclesData = [];
    let historyData = [];
    let queueData = [];
    let settingsData = {
        car_first_hour: 30.0,
        car_subsequent_hour: 20.0,
        bike_first_hour: 15.0,
        bike_subsequent_hour: 10.0,
        accessible_first_hour: 20.0,
        accessible_subsequent_hour: 15.0,
        grace_period_minutes: 15,
        min_charge: 10.0,
        rounding_minutes: 30
    };

    const tryLoad = (relPath) => {
        try {
            const p = path.resolve(__dirname, '..', relPath);
            if (fs.existsSync(p)) {
                return JSON.parse(fs.readFileSync(p, 'utf8'));
            }
        } catch (e) {
            // ignore
        }
        return null;
    };

    slotsData = tryLoad('data/slots.json') || [];
    activeVehiclesData = tryLoad('data/active_vehicles.json') || [];
    historyData = tryLoad('data/history.json') || [];
    queueData = tryLoad('data/queue.json') || [];
    settingsData = tryLoad('data/settings.json') || settingsData;

    // Build default slots if empty
    if (!slotsData.length) {
        // Floor 1 Accessible
        for (let i = 1; i <= 4; i++) {
            slotsData.push({
                slot_id: `A-0${i}`,
                floor: 1,
                zone: 'Zone C (Accessible)',
                category: 'ACCESSIBLE',
                status: 'AVAILABLE',
                occupied_reg: '',
                occupied_time: 0,
                reservation_id: '',
                distance: 10 + i * 2
            });
        }
        // Floor 1 Bikes
        for (let i = 1; i <= 6; i++) {
            slotsData.push({
                slot_id: `M-0${i}`,
                floor: 1,
                zone: 'Zone B (Ground Bikes)',
                category: 'MOTORCYCLE',
                status: 'AVAILABLE',
                occupied_reg: '',
                occupied_time: 0,
                reservation_id: '',
                distance: 15 + i * 3
            });
        }
        // Floor 1 Cars
        for (let i = 1; i <= 10; i++) {
            slotsData.push({
                slot_id: `C-${i < 10 ? '0' + i : i}`,
                floor: 1,
                zone: 'Zone A (Ground Cars)',
                category: 'CAR',
                status: 'AVAILABLE',
                occupied_reg: '',
                occupied_time: 0,
                reservation_id: '',
                distance: 20 + i * 3
            });
        }
        // Floor 2 Cars
        for (let i = 11; i <= 25; i++) {
            slotsData.push({
                slot_id: `C-${i}`,
                floor: 2,
                zone: 'Zone D (Floor 2 Cars)',
                category: 'CAR',
                status: 'AVAILABLE',
                occupied_reg: '',
                occupied_time: 0,
                reservation_id: '',
                distance: 55 + (i - 10) * 3
            });
        }
        // Floor 3 Cars
        for (let i = 26; i <= 30; i++) {
            slotsData.push({
                slot_id: `C-${i}`,
                floor: 3,
                zone: 'Zone E (Floor 3 Cars)',
                category: 'CAR',
                status: 'AVAILABLE',
                occupied_reg: '',
                occupied_time: 0,
                reservation_id: '',
                distance: 95 + (i - 25) * 3
            });
        }
    }

    // Initialize Structures
    const slotsMap = new Map();
    for (const s of slotsData) {
        slotsMap.set(s.slot_id, { ...s });
    }

    const vehicleHashTable = new DJB2HashTable(101);
    for (const v of activeVehiclesData) {
        vehicleHashTable.set(v.reg_number, { ...v });
        const sl = slotsMap.get(v.slot_id);
        if (sl) {
            sl.status = 'OCCUPIED';
            sl.occupied_reg = v.reg_number;
            sl.occupied_time = v.entry_time;
        }
    }

    const waitingQueue = new FIFOQueue();
    for (const q of queueData) {
        waitingQueue.enqueue({ ...q });
    }

    const reservations = new DoublyLinkedList();

    // Facility Graph Layout
    const graph = new FacilityGraph();
    graph.addVertex('ENT_NORTH', 'North Main Entrance', 1, 'ENTRANCE');
    graph.addVertex('ENT_SOUTH', 'South Gate Entrance', 1, 'ENTRANCE');
    graph.addVertex('EXIT_WEST', 'West Exit Gate', 1, 'EXIT');

    graph.addVertex('JUNC_F1_N', 'F1 North Hub', 1, 'JUNCTION');
    graph.addVertex('JUNC_F1_S', 'F1 South Hub', 1, 'JUNCTION');
    graph.addVertex('ZONE_A', 'Zone A (Ground Cars)', 1, 'ZONE');
    graph.addVertex('ZONE_B', 'Zone B (Ground Bikes)', 1, 'ZONE');
    graph.addVertex('ZONE_C', 'Zone C (Accessible)', 1, 'ZONE');

    graph.addVertex('RAMP_F1_F2', 'Ramp to Floor 2', 1, 'RAMP');
    graph.addVertex('RAMP_F2_F3', 'Ramp to Floor 3', 2, 'RAMP');
    graph.addVertex('JUNC_F2', 'Floor 2 Central Hub', 2, 'JUNCTION');
    graph.addVertex('ZONE_D', 'Zone D (Floor 2 Cars)', 2, 'ZONE');
    graph.addVertex('JUNC_F3', 'Floor 3 Central Hub', 3, 'JUNCTION');
    graph.addVertex('ZONE_E', 'Zone E (Floor 3 Cars)', 3, 'ZONE');

    // Add slot vertices
    for (const [slotId, s] of slotsMap.entries()) {
        graph.addVertex(slotId, `Slot ${slotId} (${s.category})`, s.floor, 'SLOT');
    }

    // Graph Edges
    graph.addBidirectional('ENT_NORTH', 'JUNC_F1_N', 15.0, 'Drive straight 15m into North Hub', 'Exit toward North Gate');
    graph.addBidirectional('JUNC_F1_N', 'ZONE_C', 10.0, 'Turn slight right into Accessible Zone C (10m)', 'Return to North Hub');
    graph.addBidirectional('JUNC_F1_N', 'ZONE_A', 20.0, 'Turn left into Zone A (20m)', 'Return to North Hub');
    graph.addBidirectional('JUNC_F1_N', 'RAMP_F1_F2', 35.0, 'Follow ramp signage straight ahead (35m)', 'Return to Ground Floor');

    graph.addBidirectional('ENT_SOUTH', 'JUNC_F1_S', 12.0, 'Drive straight 12m into South Hub', 'Exit toward South Gate');
    graph.addBidirectional('JUNC_F1_S', 'ZONE_B', 10.0, 'Turn right into Motorcycle Zone B (10m)', 'Return to South Hub');
    graph.addBidirectional('JUNC_F1_S', 'ZONE_A', 25.0, 'Turn left toward Zone A (25m)', 'Return to South Hub');
    graph.addBidirectional('JUNC_F1_N', 'JUNC_F1_S', 30.0, 'Cross central aisle between North and South Hubs (30m)', 'Return across central aisle');

    graph.addBidirectional('RAMP_F1_F2', 'JUNC_F2', 40.0, 'Drive up ramp to Floor 2 Central Hub (40m)', 'Drive down ramp to Ground Floor');
    graph.addBidirectional('JUNC_F2', 'ZONE_D', 15.0, 'Proceed into Floor 2 Zone D (15m)', 'Return to F2 Hub');
    graph.addBidirectional('JUNC_F2', 'RAMP_F2_F3', 35.0, 'Take ramp up to Floor 3 (35m)', 'Take ramp down to Floor 2');
    graph.addBidirectional('RAMP_F2_F3', 'JUNC_F3', 40.0, 'Drive up ramp to Floor 3 Central Hub (40m)', 'Drive down ramp to Floor 2');
    graph.addBidirectional('JUNC_F3', 'ZONE_E', 15.0, 'Proceed into Floor 3 Zone E (15m)', 'Return to F3 Hub');

    // Connect slots to zones
    for (const [slotId, s] of slotsMap.entries()) {
        let parentZone = 'ZONE_A';
        if (s.category === 'MOTORCYCLE') parentZone = 'ZONE_B';
        else if (s.category === 'ACCESSIBLE') parentZone = 'ZONE_C';
        else if (s.floor === 2) parentZone = 'ZONE_D';
        else if (s.floor === 3) parentZone = 'ZONE_E';

        const d = (s.distance % 20) + 4;
        graph.addBidirectional(parentZone, slotId, d, `Park into Slot ${slotId} (${d}m)`, `Exit from Slot ${slotId}`);
    }

    g_lot = {
        slotsMap,
        vehicleHashTable,
        waitingQueue,
        reservations,
        history: historyData,
        pricing: settingsData,
        graph,
        ticketCounter: 1000 + activeVehiclesData.length,
        txCounter: 1000 + historyData.length
    };

    return g_lot;
}

// Helpers
function formatIsoTime(timestamp) {
    const d = new Date(timestamp * 1000);
    const pad = (n) => String(n).padStart(2, '0');
    return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
}

function calculateFee(lot, vehicleType, entryTime, exitTime) {
    const p = lot.pricing;
    const durationSeconds = Math.max(0, exitTime - entryTime);
    const totalMinutes = Math.floor(durationSeconds / 60);
    const durationHours = Math.floor(totalMinutes / 60);
    const durationMins = totalMinutes % 60;

    let baseRate = p.car_first_hour;
    let subsequentRate = p.car_subsequent_hour;
    if (vehicleType === 'MOTORCYCLE') {
        baseRate = p.bike_first_hour;
        subsequentRate = p.bike_subsequent_hour;
    } else if (vehicleType === 'ACCESSIBLE') {
        baseRate = p.accessible_first_hour;
        subsequentRate = p.accessible_subsequent_hour;
    }

    if (totalMinutes <= p.grace_period_minutes) {
        return {
            durationSeconds,
            durationHours,
            durationMinutes: durationMins,
            baseRate,
            subsequentRate,
            baseFee: 0,
            subsequentFee: 0,
            totalFee: 0,
            isGracePeriod: true
        };
    }

    const baseFee = baseRate;
    let subsequentFee = 0;
    if (totalMinutes > 60) {
        const extraMinutes = totalMinutes - 60;
        const rounding = p.rounding_minutes > 0 ? p.rounding_minutes : 60;
        const extraUnits = Math.ceil(extraMinutes / rounding);
        subsequentFee = (extraUnits * rounding / 60.0) * subsequentRate;
    }

    let totalFee = baseFee + subsequentFee;
    if (totalFee < p.min_charge) {
        totalFee = p.min_charge;
    }

    return {
        durationSeconds,
        durationHours,
        durationMinutes: durationMins,
        baseRate,
        subsequentRate,
        baseFee,
        subsequentFee,
        totalFee: +totalFee.toFixed(2),
        isGracePeriod: false
    };
}

// ==========================================
// 3. HTTP / SERVERLESS REQUEST ROUTER
// ==========================================

function handleApiRequest(req, res, parsedUrl, body) {
    const lot = initializeParkingEngine();
    const pathname = parsedUrl.pathname;
    const method = req.method;

    // Response helper
    const sendJson = (status, data) => {
        if (typeof res.status === 'function' && typeof res.json === 'function') {
            res.setHeader('Access-Control-Allow-Origin', '*');
            res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
            res.setHeader('Access-Control-Allow-Headers', '*');
            return res.status(status).json(data);
        }
        res.writeHead(status, {
            'Content-Type': 'application/json',
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, POST, PUT, DELETE, OPTIONS',
            'Access-Control-Allow-Headers': '*'
        });
        res.end(JSON.stringify(data));
    };

    if (method === 'OPTIONS') {
        if (typeof res.status === 'function') {
            res.setHeader('Access-Control-Allow-Origin', '*');
            res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
            res.setHeader('Access-Control-Allow-Headers', '*');
            return res.status(200).end();
        }
        res.writeHead(200, {
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, POST, PUT, DELETE, OPTIONS',
            'Access-Control-Allow-Headers': '*'
        });
        return res.end();
    }

    // 1. Health
    if (pathname === '/api/health' && method === 'GET') {
        return sendJson(200, {
            success: true,
            status: 'healthy',
            backend: 'C Native Server (DSA Engine) & Vercel Cloud Serverless',
            timestamp: Math.floor(Date.now() / 1000)
        });
    }

    // 2. Dashboard
    if (pathname === '/api/dashboard' && method === 'GET') {
        let total = 0, avail = 0, occ = 0, reserved = 0, maint = 0;
        let cars = 0, bikes = 0, accessible = 0;

        for (const s of lot.slotsMap.values()) {
            total++;
            if (s.status === 'AVAILABLE') avail++;
            else if (s.status === 'OCCUPIED') occ++;
            else if (s.status === 'RESERVED') reserved++;
            else if (s.status === 'MAINTENANCE') maint++;
        }

        const activeList = lot.vehicleHashTable.getAll();
        for (const v of activeList) {
            if (v.vehicle_type === 'MOTORCYCLE') bikes++;
            else if (v.vehicle_type === 'ACCESSIBLE') accessible++;
            else cars++;
        }

        // Revenue today
        const todayStr = new Date().toISOString().slice(0, 10);
        let todayRevenue = 0;
        let todayCheckouts = 0;
        const recentActivity = [];

        for (let i = lot.history.length - 1; i >= 0; i--) {
            const h = lot.history[i];
            if (h.exit_time_str && h.exit_time_str.startsWith(todayStr)) {
                todayRevenue += h.total_fee || 0;
                todayCheckouts++;
            }
            if (recentActivity.length < 5) {
                recentActivity.push({
                    type: 'EXIT',
                    reg_number: h.reg_number,
                    slot_id: h.slot_id,
                    fee: h.total_fee,
                    time: h.exit_time_str
                });
            }
        }

        const occupancyPct = total > 0 ? +(((occ + reserved) / total) * 100).toFixed(1) : 0;

        return sendJson(200, {
            success: true,
            total_slots: total,
            available_slots: avail,
            occupied_slots: occ,
            reserved_slots: reserved,
            maintenance_slots: maint,
            active_vehicles: activeList.length,
            waiting_queue_count: lot.waitingQueue.size(),
            today_revenue: +todayRevenue.toFixed(2),
            today_checkouts: todayCheckouts,
            occupancy_percentage: occupancyPct,
            vehicle_distribution: { cars, motorcycles: bikes, accessible },
            recent_activity: recentActivity
        });
    }

    // 3. Slots
    if (pathname === '/api/slots' && method === 'GET') {
        const slots = Array.from(lot.slotsMap.values());
        return sendJson(200, { success: true, slots });
    }

    // 4. Vehicle Entry
    if (pathname === '/api/vehicles/entry' && method === 'POST') {
        const reg = (body.reg_number || '').trim().toUpperCase();
        const type = (body.vehicle_type || 'CAR').toUpperCase();
        const name = (body.owner_name || '').trim();
        const contact = (body.contact || '').trim();

        if (!reg) return sendJson(400, { success: false, error: 'Registration number is required' });

        // Check already parked
        if (lot.vehicleHashTable.get(reg)) {
            return sendJson(409, { success: false, error: `Vehicle ${reg} is already parked inside the facility` });
        }

        // Find optimal available slot
        let chosen = null;
        let minDistance = Infinity;

        for (const s of lot.slotsMap.values()) {
            if (s.status === 'AVAILABLE') {
                const match = (s.category === type) || (type === 'ACCESSIBLE' && s.category === 'CAR');
                if (match && s.distance < minDistance) {
                    minDistance = s.distance;
                    chosen = s;
                }
            }
        }

        if (!chosen) {
            return sendJson(200, {
                success: false,
                full: true,
                error: `No parking slots available for category ${type}`
            });
        }

        const now = Math.floor(Date.now() / 1000);
        chosen.status = 'OCCUPIED';
        chosen.occupied_reg = reg;
        chosen.occupied_time = now;

        lot.ticketCounter++;
        const ticketId = `TCK-${new Date().toISOString().slice(0, 10).replace(/-/g, '')}-${lot.ticketCounter}`;
        const activeVeh = {
            ticket_id: ticketId,
            reg_number: reg,
            vehicle_type: type,
            slot_id: chosen.slot_id,
            owner_name: name,
            contact,
            entry_time: now,
            entry_time_str: formatIsoTime(now)
        };

        lot.vehicleHashTable.set(reg, activeVeh);

        return sendJson(200, {
            success: true,
            ticket: activeVeh,
            assigned_slot: chosen.slot_id
        });
    }

    // 5. Vehicle Exit & Billing
    if (pathname === '/api/vehicles/exit' && method === 'POST') {
        const query = (body.reg_number || body.ticket_id || '').trim().toUpperCase();
        const action = (body.action || 'preview').toLowerCase();

        if (!query) return sendJson(400, { success: false, error: 'Registration number or Ticket ID is required' });

        // Locate active vehicle
        let vehicle = lot.vehicleHashTable.get(query);
        if (!vehicle) {
            const all = lot.vehicleHashTable.getAll();
            vehicle = all.find(v => v.ticket_id === query || v.reg_number === query);
        }

        if (!vehicle) {
            return sendJson(404, { success: false, error: `Vehicle ${query} not found in active parking registry` });
        }

        const now = Math.floor(Date.now() / 1000);
        const feeCalc = calculateFee(lot, vehicle.vehicle_type, vehicle.entry_time, now);

        lot.txCounter++;
        const txnId = `TXN-${new Date().toISOString().slice(0, 10).replace(/-/g, '')}-${lot.txCounter}`;
        const transaction = {
            transaction_id: txnId,
            ticket_id: vehicle.ticket_id,
            reg_number: vehicle.reg_number,
            vehicle_type: vehicle.vehicle_type,
            slot_id: vehicle.slot_id,
            owner_name: vehicle.owner_name,
            contact: vehicle.contact,
            entry_time: vehicle.entry_time,
            exit_time: now,
            entry_time_str: vehicle.entry_time_str,
            exit_time_str: formatIsoTime(now),
            duration_seconds: feeCalc.durationSeconds,
            duration_hours: feeCalc.durationHours,
            duration_minutes: feeCalc.durationMinutes,
            base_rate: feeCalc.baseRate,
            subsequent_rate: feeCalc.subsequentRate,
            base_fee: feeCalc.baseFee,
            subsequent_fee: feeCalc.subsequentFee,
            total_fee: feeCalc.totalFee,
            is_grace_period: feeCalc.isGracePeriod
        };

        if (action === 'preview') {
            return sendJson(200, {
                success: true,
                preview: true,
                transaction
            });
        }

        // Checkout Action
        lot.vehicleHashTable.delete(vehicle.reg_number);
        const slot = lot.slotsMap.get(vehicle.slot_id);
        if (slot) {
            slot.status = 'AVAILABLE';
            slot.occupied_reg = '';
            slot.occupied_time = 0;
            slot.reservation_id = '';
        }

        lot.history.push(transaction);

        // Auto-promote waiting queue if match
        let promoted = null;
        if (slot) {
            const queueList = lot.waitingQueue.toArray();
            for (const q of queueList) {
                if (q.vehicle_type === slot.category) {
                    lot.waitingQueue.removeById(q.waiting_id);
                    slot.status = 'OCCUPIED';
                    slot.occupied_reg = q.reg_number;
                    slot.occupied_time = now;

                    lot.ticketCounter++;
                    const autoTicket = `TCK-${new Date().toISOString().slice(0, 10).replace(/-/g, '')}-${lot.ticketCounter}`;
                    const autoActive = {
                        ticket_id: autoTicket,
                        reg_number: q.reg_number,
                        vehicle_type: q.vehicle_type,
                        slot_id: slot.slot_id,
                        owner_name: q.owner_name,
                        contact: q.contact,
                        entry_time: now,
                        entry_time_str: formatIsoTime(now)
                    };
                    lot.vehicleHashTable.set(q.reg_number, autoActive);
                    promoted = autoActive;
                    break;
                }
            }
        }

        return sendJson(200, {
            success: true,
            checkout: true,
            receipt: transaction,
            queue_promoted: promoted
        });
    }

    // 6. Search Vehicles
    if (pathname === '/api/vehicles/search' && method === 'GET') {
        const q = (parsedUrl.searchParams.get('q') || '').trim().toUpperCase();
        const all = lot.vehicleHashTable.getAll();

        if (!q) {
            return sendJson(200, { success: true, count: all.length, vehicles: all });
        }

        const filtered = all.filter(v =>
            v.reg_number.includes(q) ||
            v.ticket_id.includes(q) ||
            v.slot_id.toUpperCase().includes(q) ||
            (v.owner_name && v.owner_name.toUpperCase().includes(q))
        );

        return sendJson(200, { success: true, count: filtered.length, vehicles: filtered });
    }

    // 7. Queue GET, POST, DELETE
    if (pathname === '/api/queue' && method === 'GET') {
        return sendJson(200, { success: true, queue: lot.waitingQueue.toArray() });
    }

    if (pathname === '/api/queue' && method === 'POST') {
        const reg = (body.reg_number || '').trim().toUpperCase();
        const type = (body.vehicle_type || 'CAR').toUpperCase();
        if (!reg) return sendJson(400, { success: false, error: 'Registration number is required' });

        const waitId = `WAIT-${Date.now().toString().slice(-4)}`;
        const entry = {
            waiting_id: waitId,
            reg_number: reg,
            vehicle_type: type,
            owner_name: body.owner_name || '',
            contact: body.contact || '',
            entry_time: Math.floor(Date.now() / 1000),
            entry_time_str: formatIsoTime(Math.floor(Date.now() / 1000))
        };
        lot.waitingQueue.enqueue(entry);

        return sendJson(200, {
            success: true,
            waiting_id: waitId,
            queue_position: lot.waitingQueue.size(),
            message: 'Vehicle added to waiting queue'
        });
    }

    if (pathname.startsWith('/api/queue/') && method === 'DELETE') {
        const id = pathname.replace('/api/queue/', '');
        const removed = lot.waitingQueue.removeById(id);
        if (removed) {
            return sendJson(200, { success: true, message: 'Queue item removed successfully' });
        }
        return sendJson(404, { success: false, error: 'Queue item not found' });
    }

    // 8. History & Export
    if (pathname === '/api/history' && method === 'GET') {
        const sortBy = parsedUrl.searchParams.get('sort') || 'time';
        const order = parsedUrl.searchParams.get('order') || 'desc';

        let sorted = [...lot.history];
        const compareFn = (a, b) => {
            let valA = sortBy === 'fee' ? a.total_fee : a.exit_time;
            let valB = sortBy === 'fee' ? b.total_fee : b.exit_time;
            return order === 'asc' ? valA - valB : valB - valA;
        };

        sorted = mergeSort(sorted, compareFn);

        return sendJson(200, {
            success: true,
            algorithm_used: 'Merge Sort (Stable O(n log n))',
            count: sorted.length,
            history: sorted
        });
    }

    if (pathname === '/api/history/export' && method === 'GET') {
        let csv = 'Transaction ID,Ticket ID,Registration Number,Vehicle Type,Slot ID,Entry Time,Exit Time,Duration (Mins),Total Fee\n';
        for (const h of lot.history) {
            const mins = Math.floor((h.duration_seconds || 0) / 60);
            csv += `"${h.transaction_id}","${h.ticket_id}","${h.reg_number}","${h.vehicle_type}","${h.slot_id}","${h.entry_time_str}","${h.exit_time_str}",${mins},${h.total_fee}\n`;
        }
        res.writeHead(200, {
            'Content-Type': 'text/csv; charset=utf-8',
            'Content-Disposition': 'attachment; filename="parking_history.csv"',
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, OPTIONS',
            'Access-Control-Allow-Headers': '*'
        });
        return res.end(csv);
    }

    // 9. Reservations
    if (pathname === '/api/reservations' && method === 'GET') {
        return sendJson(200, { success: true, reservations: lot.reservations.toArray() });
    }

    if (pathname === '/api/reservations' && method === 'POST') {
        const reg = (body.reg_number || '').trim().toUpperCase();
        const type = (body.vehicle_type || 'CAR').toUpperCase();
        const prefSlot = (body.preferred_slot || '').trim().toUpperCase();
        const hours = parseInt(body.duration_hours, 10) || 2;

        if (!reg) return sendJson(400, { success: false, error: 'Registration number is required' });

        let targetSlot = null;
        if (prefSlot && lot.slotsMap.has(prefSlot)) {
            const s = lot.slotsMap.get(prefSlot);
            if (s.status === 'AVAILABLE') targetSlot = s;
        }

        if (!targetSlot) {
            for (const s of lot.slotsMap.values()) {
                if (s.status === 'AVAILABLE' && s.category === type) {
                    targetSlot = s;
                    break;
                }
            }
        }

        if (!targetSlot) {
            return sendJson(200, { success: false, error: 'No available slots for reservation' });
        }

        const now = Math.floor(Date.now() / 1000);
        const expires = now + (hours * 3600);
        const resId = `RES-${Date.now().toString().slice(-5)}`;

        targetSlot.status = 'RESERVED';
        targetSlot.reservation_id = resId;

        const resObj = {
            reservation_id: resId,
            reg_number: reg,
            vehicle_type: type,
            slot_id: targetSlot.slot_id,
            customer_name: body.customer_name || '',
            contact: body.contact || '',
            start_time: now,
            expiry_time: expires,
            start_time_str: formatIsoTime(now),
            expiry_time_str: formatIsoTime(expires)
        };

        lot.reservations.append(resObj);

        return sendJson(200, {
            success: true,
            reservation: resObj
        });
    }

    if (pathname === '/api/reservations/cancel' && method === 'POST') {
        const id = body.reservation_id;
        const removed = lot.reservations.remove(r => r.reservation_id === id);
        if (removed) {
            const slot = lot.slotsMap.get(removed.slot_id);
            if (slot && slot.status === 'RESERVED') {
                slot.status = 'AVAILABLE';
                slot.reservation_id = '';
            }
            return sendJson(200, { success: true, message: 'Reservation cancelled successfully' });
        }
        return sendJson(404, { success: false, error: 'Reservation not found' });
    }

    // 10. Navigation (Dijkstra)
    if (pathname === '/api/navigation' && method === 'GET') {
        const slot = parsedUrl.searchParams.get('slot') || 'C-01';
        const entrance = parsedUrl.searchParams.get('entrance') || 'ENT_NORTH';

        const result = lot.graph.dijkstra(entrance, slot);
        return sendJson(200, {
            success: true,
            algorithm: "Dijkstra's Shortest Path",
            entrance,
            target_slot: slot,
            ...result
        });
    }

    // 11. DSA Stats
    if (pathname === '/api/dsa-stats' && method === 'GET') {
        return sendJson(200, {
            success: true,
            hash_table: lot.vehicleHashTable.getStats(),
            queue: {
                type: 'FIFO Linked List Queue',
                count: lot.waitingQueue.size(),
                head_item: lot.waitingQueue.list.head ? lot.waitingQueue.list.head.data.reg_number : null,
                tail_item: lot.waitingQueue.list.tail ? lot.waitingQueue.list.tail.data.reg_number : null
            },
            graph: {
                algorithm: "Dijkstra's Shortest Path & BFS",
                total_vertices: lot.graph.vertices.size
            },
            slots_structure: {
                type: 'Doubly Linked List + Direct Index Map',
                total: lot.slotsMap.size
            }
        });
    }

    // 12. Settings
    if (pathname === '/api/settings' && method === 'GET') {
        return sendJson(200, { success: true, settings: lot.pricing });
    }

    if (pathname === '/api/settings' && (method === 'PUT' || method === 'POST')) {
        Object.assign(lot.pricing, body);
        return sendJson(200, { success: true, message: 'Settings updated successfully', settings: lot.pricing });
    }

    return sendJson(404, { success: false, error: `Endpoint ${pathname} not found` });
}

// ==========================================
// 4. VERCEL SERVERLESS EXPORT & STANDALONE
// ==========================================

module.exports = async function handler(req, res) {
    const parsedUrl = new URL(req.url, `http://${req.headers && req.headers.host ? req.headers.host : 'localhost'}`);
    const rewrittenRoute = parsedUrl.searchParams.get('__route');
    if (rewrittenRoute !== null) {
        parsedUrl.pathname = rewrittenRoute ? `/api/${rewrittenRoute.replace(/^\/+/, '')}` : '/api';
    }

    if (req.body && typeof req.body === 'object') {
        return handleApiRequest(req, res, parsedUrl, req.body);
    }

    if (typeof req.on === 'function') {
        let rawBody = '';
        req.on('data', chunk => { rawBody += chunk; });
        req.on('end', () => {
            let body = {};
            if (rawBody) {
                try { body = JSON.parse(rawBody); }
                catch (e) { body = {}; }
            }
            handleApiRequest(req, res, parsedUrl, body);
        });
    } else {
        let body = {};
        if (typeof req.body === 'string') {
            try { body = JSON.parse(req.body); }
            catch (e) { body = {}; }
        }
        handleApiRequest(req, res, parsedUrl, body);
    }
};

// If run directly via "node api/index.js", serve as standalone dev server
if (require.main === module) {
    const port = process.env.PORT || 8080;
    const server = http.createServer((req, res) => {
        const parsedUrl = new URL(req.url, `http://localhost:${port}`);
        if (parsedUrl.pathname.startsWith('/api/')) {
            module.exports(req, res);
        } else {
            // Serve static files
            let filePath = path.join(__dirname, '..', 'frontend', parsedUrl.pathname === '/' ? 'index.html' : parsedUrl.pathname);
            if (!fs.existsSync(filePath)) {
                filePath = path.join(__dirname, '..', 'frontend', 'index.html');
            }
            const ext = path.extname(filePath);
            const mimes = {
                '.html': 'text/html',
                '.css': 'text/css',
                '.js': 'application/javascript',
                '.json': 'application/json',
                '.svg': 'image/svg+xml'
            };
            res.writeHead(200, { 'Content-Type': mimes[ext] || 'text/plain' });
            fs.createReadStream(filePath).pipe(res);
        }
    });
    server.listen(port, () => {
        console.log(`Server running at http://localhost:${port}`);
    });
}
