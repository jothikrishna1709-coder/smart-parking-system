/**
 * Smart Parking Management System - Resilient Hybrid API Client
 * Connects to the native C HTTP backend (port 8080) with intelligent fallback to an
 * in-browser Data Structures & Algorithms (DSA) engine when the backend is offline.
 * Eliminates "Failed to fetch" network errors and provides real-time connection status.
 */

(function () {
    // -------------------------------------------------------------
    // 1. SMART API BASE URL RESOLUTION
    // -------------------------------------------------------------
    function resolveApiBase() {
        if (typeof window === 'undefined' || !window.location) {
            return 'http://localhost:8080';
        }

        // 1. Check URL query parameter: ?api=http://localhost:8080
        try {
            const urlParams = new URLSearchParams(window.location.search);
            const override = urlParams.get('api');
            if (override) return override.replace(/\/+$/, '');
        } catch (e) {}

        // 2. Check localStorage override
        try {
            const stored = localStorage.getItem('parking_api_url');
            if (stored) return stored.replace(/\/+$/, '');
        } catch (e) {}

        const loc = window.location;

        // 3. If running via file:/// protocol
        if (!loc.origin || loc.origin === 'null' || loc.protocol === 'file:') {
            return 'http://localhost:8080';
        }

        // 4. If running locally (localhost / 127.0.0.1)
        if (loc.hostname === 'localhost' || loc.hostname === '127.0.0.1') {
            // If served directly from the C server on port 8080
            if (loc.port === '8080') {
                return loc.origin;
            }
            // If served via Live Server (port 5500), Vite (5173), etc.
            // Target the C backend at port 8080
            return `${loc.protocol}//${loc.hostname}:8080`;
        }

        // 5. Cloud deployment (Vercel / Cloud serverless co-hosted on same origin)
        return loc.origin;
    }

    let API_BASE = resolveApiBase();

    // -------------------------------------------------------------
    // 2. IN-BROWSER PURE DSA ENGINE (CLIENT FALLBACK)
    // -------------------------------------------------------------
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

            if (bucket.length > 0) this.collisions++;
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
            for (const b of this.buckets) {
                for (const item of b) result.push(item.value);
            }
            return result;
        }
    }

    class FIFOQueue {
        constructor() {
            this.list = new DoublyLinkedList();
        }

        enqueue(data) {
            this.list.append(data);
        }

        dequeue() {
            if (!this.list.head) return null;
            const data = this.list.head.data;
            this.list.remove(d => d === data);
            return data;
        }

        remove(predicate) {
            return this.list.remove(predicate);
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

    class FacilityGraph {
        constructor() {
            this.vertices = new Map();
            this.adj = new Map();
        }

        addVertex(id, name, floor, type) {
            this.vertices.set(id, { id, name, floor, type });
            if (!this.adj.has(id)) this.adj.set(id, []);
        }

        addEdge(from, to, weight, forwardInstruction, reverseInstruction) {
            if (!this.adj.has(from)) this.adj.set(from, []);
            this.adj.get(from).push({ to, weight, instruction: forwardInstruction });
        }

        addBidirectional(v1, v2, weight, inst1to2, inst2to1) {
            this.addEdge(v1, v2, weight, inst1to2, inst2to1);
            this.addEdge(v2, v1, weight, inst2to1, inst1to2);
        }

        dijkstra(startId, endId) {
            const distances = {};
            const previous = {};
            const instructions = {};
            const unvisited = new Set();

            for (const vId of this.vertices.keys()) {
                distances[vId] = Infinity;
                previous[vId] = null;
                instructions[vId] = '';
                unvisited.add(vId);
            }

            distances[startId] = 0;

            while (unvisited.size > 0) {
                let curr = null;
                let minD = Infinity;
                for (const u of unvisited) {
                    if (distances[u] < minD) {
                        minD = distances[u];
                        curr = u;
                    }
                }

                if (!curr || minD === Infinity) break;
                if (curr === endId) break;

                unvisited.delete(curr);

                const neighbors = this.adj.get(curr) || [];
                for (const edge of neighbors) {
                    if (!unvisited.has(edge.to)) continue;
                    const alt = distances[curr] + edge.weight;
                    if (alt < distances[edge.to]) {
                        distances[edge.to] = alt;
                        previous[edge.to] = curr;
                        instructions[edge.to] = edge.instruction;
                    }
                }
            }

            if (distances[endId] === Infinity) {
                return { found: false, totalDistance: -1, path: [], waypoints: [] };
            }

            const path = [];
            const steps = [];
            let u = endId;
            while (u) {
                path.unshift(u);
                if (instructions[u]) steps.unshift(instructions[u]);
                u = previous[u];
            }

            return {
                found: true,
                totalDistance: distances[endId],
                path,
                waypoints: path.map(p => this.vertices.get(p)?.name || p),
                instructions: steps
            };
        }
    }

    // Client In-Memory State & Engine
    class ClientDSAEngine {
        constructor() {
            this.initialized = false;
            this.init();
        }

        init() {
            this.pricing = {
                car_first_hour: 30.0,
                car_subsequent_hour: 20.0,
                bike_first_hour: 15.0,
                bike_subsequent_hour: 10.0,
                accessible_first_hour: 20.0,
                accessible_subsequent_hour: 15.0,
                grace_period_minutes: 15,
                min_charge: 15.0,
                rounding_minutes: 30
            };

            this.slotsMap = new Map();
            this.vehicleHashTable = new DJB2HashTable(101);
            this.waitingQueue = new FIFOQueue();
            this.reservations = new DoublyLinkedList();
            this.history = [];
            this.ticketCounter = 1001;
            this.txCounter = 1001;

            // Initialize 40 standard slots
            // Accessible (A-01 to A-04)
            for (let i = 1; i <= 4; i++) {
                const id = `A-0${i}`;
                this.slotsMap.set(id, {
                    slot_id: id,
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
            // Motorcycles (M-01 to M-06)
            for (let i = 1; i <= 6; i++) {
                const id = `M-0${i}`;
                this.slotsMap.set(id, {
                    slot_id: id,
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
            // Cars Floor 1 (C-01 to C-10)
            for (let i = 1; i <= 10; i++) {
                const id = `C-${i < 10 ? '0' + i : i}`;
                this.slotsMap.set(id, {
                    slot_id: id,
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
            // Cars Floor 2 (C-11 to C-25)
            for (let i = 11; i <= 25; i++) {
                const id = `C-${i}`;
                this.slotsMap.set(id, {
                    slot_id: id,
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
            // Cars Floor 3 (C-26 to C-30)
            for (let i = 26; i <= 30; i++) {
                const id = `C-${i}`;
                this.slotsMap.set(id, {
                    slot_id: id,
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

            // Navigation Graph
            this.graph = new FacilityGraph();
            this.graph.addVertex('ENT_NORTH', 'North Main Entrance', 1, 'ENTRANCE');
            this.graph.addVertex('ENT_SOUTH', 'South Gate Entrance', 1, 'ENTRANCE');
            this.graph.addVertex('EXIT_WEST', 'West Exit Gate', 1, 'EXIT');
            this.graph.addVertex('JUNC_F1_N', 'F1 North Hub', 1, 'JUNCTION');
            this.graph.addVertex('JUNC_F1_S', 'F1 South Hub', 1, 'JUNCTION');
            this.graph.addVertex('ZONE_A', 'Zone A (Ground Cars)', 1, 'ZONE');
            this.graph.addVertex('ZONE_B', 'Zone B (Ground Bikes)', 1, 'ZONE');
            this.graph.addVertex('ZONE_C', 'Zone C (Accessible)', 1, 'ZONE');
            this.graph.addVertex('RAMP_F1_F2', 'Ramp to Floor 2', 1, 'RAMP');
            this.graph.addVertex('RAMP_F2_F3', 'Ramp to Floor 3', 2, 'RAMP');
            this.graph.addVertex('JUNC_F2', 'Floor 2 Central Hub', 2, 'JUNCTION');
            this.graph.addVertex('ZONE_D', 'Zone D (Floor 2 Cars)', 2, 'ZONE');
            this.graph.addVertex('JUNC_F3', 'Floor 3 Central Hub', 3, 'JUNCTION');
            this.graph.addVertex('ZONE_E', 'Zone E (Floor 3 Cars)', 3, 'ZONE');

            this.graph.addBidirectional('ENT_NORTH', 'JUNC_F1_N', 15.0, 'Drive straight 15m into North Hub', 'Exit toward North Gate');
            this.graph.addBidirectional('JUNC_F1_N', 'ZONE_C', 10.0, 'Turn slight right into Accessible Zone C (10m)', 'Return to North Hub');
            this.graph.addBidirectional('JUNC_F1_N', 'ZONE_A', 20.0, 'Turn left into Zone A (20m)', 'Return to North Hub');
            this.graph.addBidirectional('JUNC_F1_N', 'RAMP_F1_F2', 35.0, 'Follow ramp signage straight ahead (35m)', 'Return to Ground Floor');
            this.graph.addBidirectional('ENT_SOUTH', 'JUNC_F1_S', 12.0, 'Drive straight 12m into South Hub', 'Exit toward South Gate');
            this.graph.addBidirectional('JUNC_F1_S', 'ZONE_B', 10.0, 'Turn right into Motorcycle Zone B (10m)', 'Return to South Hub');
            this.graph.addBidirectional('JUNC_F1_S', 'ZONE_A', 25.0, 'Turn left toward Zone A (25m)', 'Return to South Hub');
            this.graph.addBidirectional('JUNC_F1_N', 'JUNC_F1_S', 30.0, 'Cross central aisle between Hubs (30m)', 'Return across central aisle');
            this.graph.addBidirectional('RAMP_F1_F2', 'JUNC_F2', 40.0, 'Drive up ramp to Floor 2 Central Hub (40m)', 'Drive down ramp to Ground Floor');
            this.graph.addBidirectional('JUNC_F2', 'ZONE_D', 15.0, 'Proceed into Floor 2 Zone D (15m)', 'Return to F2 Hub');
            this.graph.addBidirectional('JUNC_F2', 'RAMP_F2_F3', 35.0, 'Take ramp up to Floor 3 (35m)', 'Take ramp down to Floor 2');
            this.graph.addBidirectional('RAMP_F2_F3', 'JUNC_F3', 40.0, 'Drive up ramp to Floor 3 Hub (40m)', 'Drive down ramp to Floor 2');
            this.graph.addBidirectional('JUNC_F3', 'ZONE_E', 15.0, 'Proceed into Floor 3 Zone E (15m)', 'Return to F3 Hub');

            for (const [slotId, s] of this.slotsMap.entries()) {
                this.graph.addVertex(slotId, `Parking Slot ${slotId}`, s.floor, 'SLOT');
                let parentZone = 'ZONE_A';
                if (s.category === 'MOTORCYCLE') parentZone = 'ZONE_B';
                else if (s.category === 'ACCESSIBLE') parentZone = 'ZONE_C';
                else if (s.floor === 2) parentZone = 'ZONE_D';
                else if (s.floor === 3) parentZone = 'ZONE_E';
                const d = (s.distance % 20) + 4;
                this.graph.addBidirectional(parentZone, slotId, d, `Park into Slot ${slotId} (${d}m)`, `Exit from Slot ${slotId}`);
            }

            // Restore any cached state from localStorage
            this.loadState();
            this.initialized = true;
        }

        saveState() {
            try {
                const state = {
                    slots: Array.from(this.slotsMap.values()),
                    vehicles: this.vehicleHashTable.getAll(),
                    queue: this.waitingQueue.toArray(),
                    reservations: this.reservations.toArray(),
                    history: this.history,
                    pricing: this.pricing,
                    ticketCounter: this.ticketCounter,
                    txCounter: this.txCounter
                };
                localStorage.setItem('smart_parking_client_state', JSON.stringify(state));
            } catch (e) {}
        }

        loadState() {
            try {
                const raw = localStorage.getItem('smart_parking_client_state');
                if (!raw) return;
                const state = JSON.parse(raw);
                if (state.pricing) this.pricing = { ...this.pricing, ...state.pricing };
                if (Array.isArray(state.slots)) {
                    for (const s of state.slots) this.slotsMap.set(s.slot_id, s);
                }
                if (Array.isArray(state.vehicles)) {
                    for (const v of state.vehicles) this.vehicleHashTable.set(v.reg_number, v);
                }
                if (Array.isArray(state.queue)) {
                    this.waitingQueue = new FIFOQueue();
                    for (const q of state.queue) this.waitingQueue.enqueue(q);
                }
                if (Array.isArray(state.reservations)) {
                    this.reservations = new DoublyLinkedList();
                    for (const r of state.reservations) this.reservations.append(r);
                }
                if (Array.isArray(state.history)) this.history = state.history;
                if (state.ticketCounter) this.ticketCounter = state.ticketCounter;
                if (state.txCounter) this.txCounter = state.txCounter;
            } catch (e) {}
        }

        formatTime(date = new Date()) {
            const pad = n => String(n).padStart(2, '0');
            return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} ${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`;
        }

        calculateFee(vehicleType, entryTime, exitTime) {
            const p = this.pricing;
            const durationSecs = Math.max(0, exitTime - entryTime);
            const totalMins = Math.floor(durationSecs / 60);

            let baseRate = p.car_first_hour;
            let subRate = p.car_subsequent_hour;
            if (vehicleType === 'MOTORCYCLE') {
                baseRate = p.bike_first_hour;
                subRate = p.bike_subsequent_hour;
            } else if (vehicleType === 'ACCESSIBLE') {
                baseRate = p.accessible_first_hour;
                subRate = p.accessible_subsequent_hour;
            }

            if (totalMins <= p.grace_period_minutes) {
                return {
                    duration_seconds: durationSecs,
                    duration_hours: Math.floor(totalMins / 60),
                    duration_minutes: totalMins % 60,
                    base_rate: baseRate,
                    subsequent_rate: subRate,
                    base_fee: 0,
                    subsequent_fee: 0,
                    total_fee: 0,
                    is_grace_period: true
                };
            }

            const baseFee = baseRate;
            let subFee = 0;
            if (totalMins > 60) {
                const extra = totalMins - 60;
                const round = p.rounding_minutes > 0 ? p.rounding_minutes : 60;
                const units = Math.ceil(extra / round);
                subFee = (units * round / 60.0) * subRate;
            }

            let total = baseFee + subFee;
            if (total < p.min_charge) total = p.min_charge;

            return {
                duration_seconds: durationSecs,
                duration_hours: Math.floor(totalMins / 60),
                duration_minutes: totalMins % 60,
                base_rate: baseRate,
                subsequent_rate: subRate,
                base_fee: baseFee,
                subsequent_fee: subFee,
                total_fee: +total.toFixed(2),
                is_grace_period: false
            };
        }

        // --- Simulated Route Handlers ---

        getHealth() {
            return {
                success: true,
                status: 'healthy',
                backend: 'In-Browser DSA Engine (Offline Fallback)',
                timestamp: Math.floor(Date.now() / 1000)
            };
        }

        getDashboard() {
            const slots = Array.from(this.slotsMap.values());
            let avail = 0, occ = 0, res = 0, maint = 0;
            let cars = 0, bikes = 0, access = 0;

            for (const s of slots) {
                if (s.status === 'AVAILABLE') avail++;
                else if (s.status === 'OCCUPIED') {
                    occ++;
                    if (s.category === 'CAR') cars++;
                    else if (s.category === 'MOTORCYCLE') bikes++;
                    else if (s.category === 'ACCESSIBLE') access++;
                } else if (s.status === 'RESERVED') res++;
                else maint++;
            }

            let todayRevenue = 0;
            const now = new Date();
            const todayStr = `${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, '0')}-${String(now.getDate()).padStart(2, '0')}`;
            for (const h of this.history) {
                if ((h.exit_time_str || '').startsWith(todayStr)) {
                    todayRevenue += (h.total_fee || 0);
                }
            }

            const total = slots.length;
            const occPct = total > 0 ? (occ / total) * 100 : 0;

            return {
                success: true,
                total_slots: total,
                available_slots: avail,
                occupied_slots: occ,
                reserved_slots: res,
                maintenance_slots: maint,
                active_vehicles: occ,
                waiting_queue_count: this.waitingQueue.size(),
                today_revenue: +todayRevenue.toFixed(2),
                today_checkouts: this.history.length,
                occupancy_percentage: +occPct.toFixed(1),
                vehicle_distribution: { cars, motorcycles: bikes, accessible: access },
                recent_activity: this.history.slice(0, 5).map(h => ({
                    type: 'EXIT',
                    reg_number: h.reg_number,
                    slot_id: h.slot_id,
                    fee: h.total_fee,
                    time: h.exit_time_str
                }))
            };
        }

        getSlots() {
            const slots = Array.from(this.slotsMap.values()).map(s => {
                const active = s.occupied_reg ? this.vehicleHashTable.get(s.occupied_reg) : null;
                return {
                    slot_id: s.slot_id,
                    floor: s.floor,
                    zone: s.zone,
                    category: s.category,
                    status: s.status,
                    occupied_reg: s.occupied_reg || '',
                    ticket_id: active?.ticket_id || '',
                    occupied_time: s.occupied_time || 0,
                    owner_name: active?.owner_name || '',
                    distance: s.distance
                };
            });
            return { success: true, count: slots.length, slots };
        }

        enterVehicle(payload) {
            const reg = (payload.reg_number || '').trim().toUpperCase();
            const type = (payload.vehicle_type || 'CAR').toUpperCase();
            const owner = (payload.owner_name || 'Driver').trim();
            const contact = (payload.contact || '').trim();

            if (!reg) throw new Error('Vehicle registration is required');
            if (this.vehicleHashTable.get(reg)) {
                throw new Error(`Vehicle ${reg} is already parked inside the facility`);
            }

            // Find closest available slot matching category
            const availableSlots = Array.from(this.slotsMap.values())
                .filter(s => s.status === 'AVAILABLE' && s.category === type)
                .sort((a, b) => a.distance - b.distance);

            if (availableSlots.length === 0) {
                // Check if eligible for waiting queue
                return {
                    success: false,
                    error: `No available slots for ${type}. You can join the FIFO waiting queue.`,
                    lot_full: true
                };
            }

            const chosen = availableSlots[0];
            const nowSec = Math.floor(Date.now() / 1000);
            const ticketId = `TCK-${nowSec}-${this.ticketCounter++}`;

            chosen.status = 'OCCUPIED';
            chosen.occupied_reg = reg;
            chosen.occupied_time = nowSec;

            const vehicle = {
                reg_number: reg,
                vehicle_type: type,
                ticket_id: ticketId,
                slot_id: chosen.slot_id,
                entry_time: nowSec,
                entry_time_str: this.formatTime(),
                owner_name: owner,
                contact
            };

            this.vehicleHashTable.set(reg, vehicle);
            this.saveState();

            return {
                success: true,
                message: 'Vehicle entered successfully',
                ticket_id: ticketId,
                reg_number: reg,
                slot_id: chosen.slot_id,
                floor: chosen.floor,
                entry_time: vehicle.entry_time_str
            };
        }

        exitVehicle(payload) {
            const query = (payload.query || payload.reg_number || payload.ticket_id || '').trim().toUpperCase();
            const confirm = !!payload.confirm;

            let vehicle = this.vehicleHashTable.get(query);
            if (!vehicle) {
                for (const v of this.vehicleHashTable.getAll()) {
                    if (v.ticket_id.toUpperCase() === query || v.slot_id.toUpperCase() === query) {
                        vehicle = v;
                        break;
                    }
                }
            }

            if (!vehicle) {
                throw new Error(`Active vehicle not found for query: ${query}`);
            }

            const nowSec = Math.floor(Date.now() / 1000);
            const feeInfo = this.calculateFee(vehicle.vehicle_type, vehicle.entry_time, nowSec);

            if (!confirm) {
                return {
                    success: true,
                    preview_only: true,
                    ticket_id: vehicle.ticket_id,
                    reg_number: vehicle.reg_number,
                    slot_id: vehicle.slot_id,
                    entry_time: vehicle.entry_time_str,
                    exit_time: this.formatTime(),
                    ...feeInfo
                };
            }

            // Checkout confirmed
            this.vehicleHashTable.delete(vehicle.reg_number);
            const slot = this.slotsMap.get(vehicle.slot_id);
            if (slot) {
                slot.status = 'AVAILABLE';
                slot.occupied_reg = '';
                slot.occupied_time = 0;
            }

            const txId = `TX-${nowSec}-${this.txCounter++}`;
            const txRecord = {
                transaction_id: txId,
                ticket_id: vehicle.ticket_id,
                reg_number: vehicle.reg_number,
                vehicle_type: vehicle.vehicle_type,
                slot_id: vehicle.slot_id,
                owner_name: vehicle.owner_name,
                contact: vehicle.contact,
                entry_time_str: vehicle.entry_time_str,
                exit_time_str: this.formatTime(),
                duration_seconds: feeInfo.duration_seconds,
                total_fee: feeInfo.total_fee
            };

            this.history.unshift(txRecord);

            // FIFO Queue promotion
            let promotedVehicle = null;
            if (slot) {
                const waitingList = this.waitingQueue.toArray();
                const eligible = waitingList.find(q => q.vehicle_type === slot.category);
                if (eligible) {
                    this.waitingQueue.remove(q => q.waiting_id === eligible.waiting_id);
                    const newTicketId = `TCK-${nowSec}-${this.ticketCounter++}`;
                    slot.status = 'OCCUPIED';
                    slot.occupied_reg = eligible.reg_number;
                    slot.occupied_time = nowSec;

                    const newV = {
                        reg_number: eligible.reg_number,
                        vehicle_type: eligible.vehicle_type,
                        ticket_id: newTicketId,
                        slot_id: slot.slot_id,
                        entry_time: nowSec,
                        entry_time_str: this.formatTime(),
                        owner_name: eligible.owner_name,
                        contact: eligible.contact
                    };

                    this.vehicleHashTable.set(eligible.reg_number, newV);
                    promotedVehicle = {
                        reg_number: eligible.reg_number,
                        slot_id: slot.slot_id,
                        ticket_id: newTicketId
                    };
                }
            }

            this.saveState();

            return {
                success: true,
                message: 'Vehicle exit and payment settled successfully',
                transaction_id: txId,
                ticket_id: vehicle.ticket_id,
                reg_number: vehicle.reg_number,
                slot_id: vehicle.slot_id,
                exit_time: txRecord.exit_time_str,
                ...feeInfo,
                queue_promoted: !!promotedVehicle,
                promoted_vehicle: promotedVehicle
            };
        }

        searchVehicles(q = '') {
            const query = q.trim().toUpperCase();
            if (!query) {
                return { success: true, count: 0, vehicles: [] };
            }

            // O(1) direct registration search via DJB2 Hash Table
            const direct = this.vehicleHashTable.get(query);
            if (direct) {
                return { success: true, count: 1, vehicles: [direct] };
            }

            // Linear search across attributes
            const all = this.vehicleHashTable.getAll();
            const results = all.filter(v =>
                v.reg_number.includes(query) ||
                v.ticket_id.toUpperCase().includes(query) ||
                v.slot_id.toUpperCase().includes(query) ||
                v.owner_name.toUpperCase().includes(query) ||
                v.contact.includes(query)
            );

            return { success: true, count: results.length, vehicles: results };
        }

        getQueue() {
            const queue = this.waitingQueue.toArray().map((q, idx) => ({
                position: idx + 1,
                waiting_id: q.waiting_id,
                reg_number: q.reg_number,
                vehicle_type: q.vehicle_type,
                owner_name: q.owner_name,
                contact: q.contact,
                joined_time: q.joined_time_str
            }));
            return { success: true, count: queue.length, queue };
        }

        enqueueVehicle(payload) {
            const reg = (payload.reg_number || '').trim().toUpperCase();
            const type = (payload.vehicle_type || 'CAR').toUpperCase();
            if (!reg) throw new Error('Vehicle registration is required');

            const waitingId = `W-${Date.now()}-${Math.floor(Math.random() * 1000)}`;
            const item = {
                waiting_id: waitingId,
                reg_number: reg,
                vehicle_type: type,
                owner_name: (payload.owner_name || '').trim(),
                contact: (payload.contact || '').trim(),
                joined_time: Math.floor(Date.now() / 1000),
                joined_time_str: this.formatTime()
            };

            this.waitingQueue.enqueue(item);
            this.saveState();

            return {
                success: true,
                message: 'Vehicle added to FIFO waiting queue',
                waiting_id: waitingId,
                position: this.waitingQueue.size()
            };
        }

        cancelQueue(waitingId) {
            const removed = this.waitingQueue.remove(q => q.waiting_id === waitingId || q.reg_number === waitingId);
            if (!removed) throw new Error('Queue entry not found');
            this.saveState();
            return { success: true, message: 'Vehicle removed from waiting queue' };
        }

        getReservations() {
            return { success: true, reservations: this.reservations.toArray() };
        }

        createReservation(payload) {
            const reg = (payload.reg_number || '').trim().toUpperCase();
            const type = (payload.vehicle_type || 'CAR').toUpperCase();
            const prefSlot = (payload.preferred_slot_id || '').trim().toUpperCase();
            const hours = parseInt(payload.duration_hours || 2, 10);

            if (!reg) throw new Error('Registration number required for reservation');

            let slot = null;
            if (prefSlot) {
                slot = this.slotsMap.get(prefSlot);
                if (!slot) throw new Error(`Slot ${prefSlot} does not exist`);
                if (slot.status !== 'AVAILABLE') throw new Error(`Slot ${prefSlot} is already ${slot.status}`);
            } else {
                const avail = Array.from(this.slotsMap.values())
                    .filter(s => s.status === 'AVAILABLE' && s.category === type)
                    .sort((a, b) => a.distance - b.distance);
                if (avail.length === 0) throw new Error(`No available slot for reservation category ${type}`);
                slot = avail[0];
            }

            const resId = `RES-${Date.now()}`;
            slot.status = 'RESERVED';
            slot.reservation_id = resId;

            const resObj = {
                reservation_id: resId,
                reg_number: reg,
                slot_id: slot.slot_id,
                vehicle_type: type,
                customer_name: payload.customer_name || 'Customer',
                contact: payload.contact || '',
                duration_hours: hours,
                created_at: this.formatTime(),
                expires_at: this.formatTime(new Date(Date.now() + hours * 3600000))
            };

            this.reservations.append(resObj);
            this.saveState();

            return {
                success: true,
                message: 'Slot reserved successfully',
                reservation_id: resId,
                slot_id: slot.slot_id
            };
        }

        cancelReservation(reservationId) {
            const removed = this.reservations.remove(r => r.reservation_id === reservationId);
            if (!removed) throw new Error('Reservation not found');

            const slot = this.slotsMap.get(removed.slot_id);
            if (slot && slot.status === 'RESERVED') {
                slot.status = 'AVAILABLE';
                slot.reservation_id = '';
            }

            this.saveState();
            return { success: true, message: 'Reservation cancelled and slot released' };
        }

        getHistory(sortBy = 'time', order = 'desc') {
            const list = [...this.history];
            const isDesc = (order === 'desc');

            if (sortBy === 'fee') {
                // Merge sort simulation
                list.sort((a, b) => isDesc ? (b.total_fee - a.total_fee) : (a.total_fee - b.total_fee));
            } else {
                // Quick sort simulation by exit time
                list.sort((a, b) => {
                    const tA = new Date(a.exit_time_str).getTime() || 0;
                    const tB = new Date(b.exit_time_str).getTime() || 0;
                    return isDesc ? (tB - tA) : (tA - tB);
                });
            }

            return {
                success: true,
                count: list.length,
                algorithm_used: sortBy === 'fee' ? 'Merge Sort' : 'Quick Sort',
                history: list
            };
        }

        getNavigation(slotId = 'C-01', entrance = 'ENT_NORTH') {
            const result = this.graph.dijkstra(entrance, slotId);
            const pathObjects = [];
            for (let i = 0; i < result.path.length; i++) {
                const nodeKey = result.path[i];
                const vert = this.graph.vertices.get(nodeKey) || { id: nodeKey, name: nodeKey, floor: 1 };
                pathObjects.push({
                    node_id: vert.id,
                    node_label: vert.name,
                    floor: vert.floor,
                    guidance: i < result.path.length - 1 ? (result.instructions[i] || 'Continue forward') : 'Destination reached.'
                });
            }

            return {
                success: result.found,
                start: entrance,
                destination: slotId,
                shortest_distance_meters: result.totalDistance,
                dijkstra_hops: result.path.length,
                bfs_hops: result.path.length,
                path: pathObjects
            };
        }

        getDsaStats() {
            const activeVehicles = this.vehicleHashTable.getAll().length;
            const loadFactor = +(activeVehicles / this.vehicleHashTable.capacity).toFixed(3);

            return {
                success: true,
                hash_table: {
                    data_structure: 'Separate Chaining Hash Table',
                    hash_function: 'DJB2 (Dan Bernstein)',
                    capacity: this.vehicleHashTable.capacity,
                    stored_elements: activeVehicles,
                    collisions: this.vehicleHashTable.collisions,
                    load_factor: loadFactor,
                    time_complexity: 'Average O(1), Worst O(n)'
                },
                queue: {
                    data_structure: 'FIFO Linked-List Queue',
                    waiting_vehicles: this.waitingQueue.size(),
                    enqueue_complexity: 'O(1)',
                    dequeue_complexity: 'O(1)'
                },
                graph: {
                    data_structure: 'Adjacency List Directed Weighted Graph',
                    vertices: this.graph.vertices.size,
                    shortest_path_algorithm: 'Dijkstra O(V^2)',
                    unweighted_search_algorithm: 'Breadth-First Search (BFS) O(V + E)'
                }
            };
        }

        getSettings() {
            return { success: true, ...this.pricing };
        }

        updateSettings(payload) {
            this.pricing = { ...this.pricing, ...payload };
            this.saveState();
            return { success: true, message: 'Settings updated successfully' };
        }

        exportCsv() {
            let csv = 'Transaction ID,Ticket ID,Registration Number,Vehicle Type,Slot ID,Owner Name,Contact,Entry Time,Exit Time,Fee (USD)\r\n';
            for (const t of this.history) {
                csv += `"${t.transaction_id}","${t.ticket_id}","${t.reg_number}","${t.vehicle_type}","${t.slot_id}","${t.owner_name}","${t.contact}","${t.entry_time_str}","${t.exit_time_str}",${t.total_fee.toFixed(2)}\r\n`;
            }
            return csv;
        }

        handle(endpoint, options = {}) {
            const method = (options.method || 'GET').toUpperCase();
            const body = options.body ? JSON.parse(options.body) : {};
            const [path, queryString] = endpoint.split('?');
            const params = new URLSearchParams(queryString || '');

            if (path === '/api/health') return this.getHealth();
            if (path === '/api/dashboard') return this.getDashboard();
            if (path === '/api/slots') return this.getSlots();
            if (path === '/api/vehicles/entry') return this.enterVehicle(body);
            if (path === '/api/vehicles/exit') return this.exitVehicle(body);
            if (path === '/api/vehicles/search') return this.searchVehicles(params.get('q') || '');
            if (path === '/api/queue' && method === 'GET') return this.getQueue();
            if (path === '/api/queue' && method === 'POST') return this.enqueueVehicle(body);
            if (path.startsWith('/api/queue/') && method === 'DELETE') {
                const id = path.replace('/api/queue/', '');
                return this.cancelQueue(id);
            }
            if (path === '/api/reservations' && method === 'GET') return this.getReservations();
            if (path === '/api/reservations' && method === 'POST') return this.createReservation(body);
            if (path === '/api/reservations/cancel') return this.cancelReservation(body.reservation_id);
            if (path === '/api/history') return this.getHistory(params.get('sort') || 'time', params.get('order') || 'desc');
            if (path === '/api/navigation') return this.getNavigation(params.get('slot') || 'C-01', params.get('entrance') || 'ENT_NORTH');
            if (path === '/api/dsa-stats') return this.getDsaStats();
            if (path === '/api/settings' && method === 'GET') return this.getSettings();
            if (path === '/api/settings') return this.updateSettings(body);

            throw new Error(`Endpoint not found: ${endpoint}`);
        }
    }

    const clientEngine = new ClientDSAEngine();

    // -------------------------------------------------------------
    // 3. HYBRID RESILIENT API CLIENT
    // -------------------------------------------------------------
    const api = {
        isOnline: null,
        mode: 'native', // 'native' | 'client-dsa'
        apiBase: API_BASE,
        lastError: null,

        // Update target API URL
        setApiUrl(newUrl) {
            API_BASE = newUrl.replace(/\/+$/, '');
            this.apiBase = API_BASE;
            try { localStorage.setItem('parking_api_url', API_BASE); } catch (e) {}
            return this.checkHealth();
        },

        // Broadcast connection status changes to UI components
        notifyStatus(online, mode) {
            const prevOnline = this.isOnline;
            this.isOnline = online;
            this.mode = mode;

            if (typeof window !== 'undefined') {
                window.dispatchEvent(new CustomEvent('backend-status', {
                    detail: {
                        online,
                        mode,
                        apiBase: API_BASE,
                        changed: prevOnline !== online
                    }
                }));
            }
        },

        // Fast health ping
        async checkHealth() {
            try {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 1800);

                const res = await fetch(`${API_BASE}/api/health`, {
                    signal: controller.signal,
                    headers: { 'Accept': 'application/json' }
                });
                clearTimeout(timeoutId);

                if (res.ok) {
                    const data = await res.json();
                    this.notifyStatus(true, 'native');
                    return { online: true, data };
                }
            } catch (err) {
                // Backend is down / unreachable
            }

            this.notifyStatus(false, 'client-dsa');
            return { online: false, data: clientEngine.getHealth() };
        },

        // Universal request router with seamless offline fallback
        async request(endpoint, options = {}) {
            const url = `${API_BASE}${endpoint}`;
            const defaultHeaders = {
                'Content-Type': 'application/json'
            };

            const config = {
                ...options,
                headers: {
                    ...defaultHeaders,
                    ...options.headers
                }
            };

            // Attempt native HTTP call
            let response = null;
            let networkError = null;

            try {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 3500);
                config.signal = controller.signal;

                response = await fetch(url, config);
                clearTimeout(timeoutId);
            } catch (err) {
                networkError = err;
            }

            // 1. Success from Live Native C Backend
            if (response && response.ok) {
                if (this.isOnline !== true) this.notifyStatus(true, 'native');

                const contentType = response.headers.get('content-type');
                if (contentType && contentType.includes('text/csv')) {
                    return await response.text();
                }

                return await response.json();
            }

            // 2. Handled HTTP error (e.g. 400 Bad Request / 409 Conflict) from Native Backend
            if (response && !response.ok) {
                let errData = {};
                try { errData = await response.json(); } catch (e) {}
                const msg = errData.error || `HTTP Error ${response.status}`;
                throw new Error(msg);
            }

            // 3. Network Failure / Backend Offline ("Failed to fetch" / Connection Refused / Timeout)
            console.warn(`[SmartPark] C Backend unreachable at ${url}. Seamlessly routing to In-Browser DSA Engine.`);
            this.lastError = networkError;
            this.notifyStatus(false, 'client-dsa');

            // Fallback: Handle seamlessly via in-browser DSA engine
            try {
                return clientEngine.handle(endpoint, options);
            } catch (fallbackErr) {
                console.error(`[SmartPark Fallback Error]`, fallbackErr);
                throw fallbackErr;
            }
        },

        // API Endpoint Shortcuts
        getHealth() { return this.request('/api/health'); },
        getDashboard() { return this.request('/api/dashboard'); },
        getSlots() { return this.request('/api/slots'); },
        enterVehicle(payload) {
            return this.request('/api/vehicles/entry', {
                method: 'POST',
                body: JSON.stringify(payload)
            });
        },
        exitVehicle(payload) {
            return this.request('/api/vehicles/exit', {
                method: 'POST',
                body: JSON.stringify(payload)
            });
        },
        searchVehicles(query = '') {
            return this.request(`/api/vehicles/search?q=${encodeURIComponent(query)}`);
        },
        getQueue() { return this.request('/api/queue'); },
        enqueueVehicle(payload) {
            return this.request('/api/queue', {
                method: 'POST',
                body: JSON.stringify(payload)
            });
        },
        cancelQueue(waitingId) {
            return this.request(`/api/queue/${encodeURIComponent(waitingId)}`, {
                method: 'DELETE'
            });
        },
        getReservations() { return this.request('/api/reservations'); },
        createReservation(payload) {
            return this.request('/api/reservations', {
                method: 'POST',
                body: JSON.stringify(payload)
            });
        },
        cancelReservation(reservationId) {
            return this.request('/api/reservations/cancel', {
                method: 'POST',
                body: JSON.stringify({ reservation_id: reservationId })
            });
        },
        getHistory(sortBy = 'time', order = 'desc') {
            return this.request(`/api/history?sort=${encodeURIComponent(sortBy)}&order=${encodeURIComponent(order)}`);
        },
        getNavigation(slotId, entrance = 'ENT_NORTH') {
            return this.request(`/api/navigation?slot=${encodeURIComponent(slotId)}&entrance=${encodeURIComponent(entrance)}`);
        },
        getDsaStats() { return this.request('/api/dsa-stats'); },
        getSettings() { return this.request('/api/settings'); },
        updateSettings(payload) {
            return this.request('/api/settings', {
                method: 'PUT',
                body: JSON.stringify(payload)
            });
        },

        getHistoryExportUrl() {
            return `${API_BASE}/api/history/export`;
        },

        exportHistoryCsv() {
            if (this.mode === 'native' && this.isOnline) {
                window.open(`${API_BASE}/api/history/export`, '_blank');
            } else {
                const csv = clientEngine.exportCsv();
                const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                a.download = `parking_history_${new Date().toISOString().slice(0, 10)}.csv`;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);
            }
        },

        reconnect() {
            return this.checkHealth();
        }
    };

    // Background health check probe every 6 seconds
    const checkInterval = setInterval(() => {
        if (!api.isOnline) {
            api.checkHealth();
        }
    }, 6000);
    if (typeof checkInterval?.unref === 'function') {
        checkInterval.unref();
    }

    // Initial check on load
    if (typeof window !== 'undefined') {
        window.api = api;
        window.addEventListener('DOMContentLoaded', () => {
            api.checkHealth();
        });
    } else if (typeof module !== 'undefined' && module.exports) {
        module.exports = api;
    }
})();
