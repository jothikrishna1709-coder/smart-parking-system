/**
 * Smart Parking Management System - Application Controller
 * Manages view switching, reactive state, UI rendering, and user interactions.
 */

document.addEventListener('DOMContentLoaded', () => {
    // Current state
    const state = {
        currentView: 'dashboard',
        slots: [],
        queue: [],
        reservations: [],
        history: [],
        pendingExitQuery: null,
        selectedFloor: 'all',
        selectedCategory: 'all',
        selectedStatus: 'all'
    };

    // Navigation and View Management
    const navItems = document.querySelectorAll('.nav-item');
    const viewPanels = document.querySelectorAll('.view-panel');
    const topbarTitle = document.getElementById('topbar-title');
    const topbarDesc = document.getElementById('topbar-desc');

    const viewTitles = {
        dashboard: { title: 'Facility Dashboard', desc: 'Real-time status of parking capacity and DSA core operations' },
        slots: { title: 'Live Parking Slots', desc: 'Visual floor map of slots with real-time occupancy and category distribution' },
        entry: { title: 'Vehicle Entry Registration', desc: 'Validate vehicle plate and assign optimal slot using C allocation algorithm' },
        exit: { title: 'Vehicle Exit & Billing', desc: 'Duration calculation, configurable fee settlement, and automated queue promotion' },
        queue: { title: 'FIFO Waiting Queue', desc: 'Custom linked-list waiting queue with automatic slot allocation' },
        reservations: { title: 'Slot Reservations', desc: 'Advance slot booking with conflict detection and expiration timers' },
        search: { title: 'Vehicle Search Engine', desc: 'O(1) hash table registration lookup and multi-attribute linear search' },
        history: { title: 'Parking History & Reports', desc: 'Audited log with C Merge Sort and Quick Sort ordering' },
        analytics: { title: 'DSA Analytics & Navigation', desc: 'Live DSA complexity benchmarks and Dijkstra shortest-path navigation' },
        settings: { title: 'Pricing & System Settings', desc: 'Configure category rates, grace period, and minimum charges' }
    };

    function switchView(viewName) {
        state.currentView = viewName;

        navItems.forEach(item => {
            if (item.dataset.view === viewName) {
                item.classList.add('active');
            } else {
                item.classList.remove('active');
            }
        });

        viewPanels.forEach(panel => {
            if (panel.id === `view-${viewName}`) {
                panel.classList.add('active');
            } else {
                panel.classList.remove('active');
            }
        });

        if (viewTitles[viewName]) {
            topbarTitle.textContent = viewTitles[viewName].title;
            topbarDesc.textContent = viewTitles[viewName].desc;
        }

        // Trigger view-specific data refresh
        refreshCurrentView();
    }

    navItems.forEach(item => {
        item.addEventListener('click', () => {
            const view = item.dataset.view;
            if (view) switchView(view);
        });
    });

    document.getElementById('btn-quick-entry')?.addEventListener('click', () => switchView('entry'));
    document.getElementById('btn-quick-refresh')?.addEventListener('click', () => refreshCurrentView(true));

    // Toast Notification System
    function showToast(message, type = 'success') {
        const container = document.getElementById('toast-container');
        if (!container) return;

        let displayMsg = message || '';
        if (typeof displayMsg === 'string' && (displayMsg.includes('Failed to fetch') || displayMsg.includes('NetworkError'))) {
            displayMsg = 'Backend server offline. Running on in-browser DSA simulation.';
            type = 'warning';
        }

        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        toast.innerHTML = `
            <span>${displayMsg}</span>
        `;
        container.appendChild(toast);

        setTimeout(() => {
            toast.style.opacity = '0';
            toast.style.transform = 'translateY(10px)';
            toast.style.transition = 'all 0.3s ease';
            setTimeout(() => toast.remove(), 300);
        }, 4000);
    }

    // Backend Connection Status Synchronization
    const statusDot = document.getElementById('status-dot');
    const statusText = document.getElementById('backend-status-text');
    const offlineBanner = document.getElementById('offline-banner');
    const btnReconnect = document.getElementById('btn-reconnect-backend');

    function updateBackendUI(online, mode) {
        if (online) {
            if (statusDot) statusDot.classList.remove('offline');
            if (statusText) statusText.textContent = 'C Backend Online (8080)';
            if (offlineBanner) offlineBanner.style.display = 'none';
        } else {
            if (statusDot) statusDot.classList.add('offline');
            if (statusText) statusText.textContent = 'Client DSA Engine (Demo)';
            if (offlineBanner) offlineBanner.style.display = 'flex';
        }
    }

    window.addEventListener('backend-status', (e) => {
        const { online, mode, changed } = e.detail;
        updateBackendUI(online, mode);
        if (changed && online) {
            showToast('Connected to Native C Backend!', 'success');
            refreshCurrentView(false);
        }
    });

    btnReconnect?.addEventListener('click', async () => {
        btnReconnect.textContent = '⏳ Checking...';
        btnReconnect.disabled = true;
        const res = await api.reconnect();
        btnReconnect.textContent = '🔄 Reconnect C Backend';
        btnReconnect.disabled = false;
        if (res.online) {
            showToast('Connected to C Backend on port 8080!', 'success');
            refreshCurrentView(true);
        } else {
            showToast('C server not found on port 8080. Operating via client DSA engine.', 'warning');
        }
    });

    document.getElementById('backend-status-indicator')?.addEventListener('click', () => {
        api.reconnect().then(res => {
            if (res.online) {
                showToast('C Backend is online and responding!', 'success');
            } else {
                showToast('C Backend offline on port 8080. In-browser DSA engine active.', 'warning');
            }
        });
    });

    document.getElementById('btn-export-csv')?.addEventListener('click', (e) => {
        e.preventDefault();
        api.exportHistoryCsv();
    });

    // Modal Helpers
    function openModal(id) {
        const modal = document.getElementById(id);
        if (modal) modal.classList.add('active');
    }

    function closeModal(id) {
        const modal = document.getElementById(id);
        if (modal) modal.classList.remove('active');
    }

    document.querySelectorAll('.close-btn, [id^="btn-close-"]').forEach(btn => {
        btn.addEventListener('click', (e) => {
            const modal = e.target.closest('.modal-overlay');
            if (modal) modal.classList.remove('active');
        });
    });

    // 1. DASHBOARD CONTROLLER
    async function loadDashboard() {
        try {
            const data = await api.getDashboard();
            if (!data.success) return;

            document.getElementById('stat-total-slots').textContent = data.total_slots;
            document.getElementById('stat-available-slots').textContent = data.available_slots;
            document.getElementById('stat-occupied-slots').textContent = data.occupied_slots;
            document.getElementById('stat-reserved-slots').textContent = data.reserved_slots;
            document.getElementById('stat-today-revenue').textContent = `$${data.today_revenue.toFixed(2)}`;

            document.getElementById('badge-avail-slots').textContent = data.available_slots;
            document.getElementById('badge-queue-count').textContent = data.waiting_queue_count;

            const occPct = data.occupancy_percentage.toFixed(1);
            document.getElementById('badge-occupancy-rate').textContent = `${occPct}% Occupied`;

            const total = data.total_slots || 1;
            const occWidth = (data.occupied_slots / total) * 100;
            const resWidth = (data.reserved_slots / total) * 100;
            const availWidth = 100 - occWidth - resWidth;

            document.getElementById('bar-occ-fill').style.width = `${occWidth}%`;
            document.getElementById('bar-res-fill').style.width = `${resWidth}%`;
            document.getElementById('bar-avail-fill').style.width = `${availWidth}%`;

            document.getElementById('txt-occ-count').textContent = data.occupied_slots;
            document.getElementById('txt-res-count').textContent = data.reserved_slots;
            document.getElementById('txt-avail-count').textContent = data.available_slots;

            if (data.vehicle_distribution) {
                document.getElementById('dist-cars').textContent = data.vehicle_distribution.cars;
                document.getElementById('dist-bikes').textContent = data.vehicle_distribution.motorcycles;
                document.getElementById('dist-accessible').textContent = data.vehicle_distribution.accessible;
            }

            // Recent activity
            const list = document.getElementById('recent-activity-list');
            if (list) {
                list.innerHTML = '';
                if (!data.recent_activity || data.recent_activity.length === 0) {
                    list.innerHTML = '<p style="color: var(--text-muted); font-size: 13px; text-align: center; padding: 20px;">No recent transactions.</p>';
                } else {
                    data.recent_activity.forEach(act => {
                        const item = document.createElement('div');
                        item.style.cssText = 'display: flex; justify-content: space-between; align-items: center; padding: 10px 14px; background: #f8fafc; border-radius: var(--radius-md); border: 1px solid var(--border-color); font-size: 13px;';
                        item.innerHTML = `
                            <div>
                                <span style="font-weight: 700; font-family: var(--font-mono);">${act.reg_number}</span>
                                <span style="font-size: 11px; color: var(--text-secondary); margin-left: 6px;">Slot ${act.slot_id}</span>
                            </div>
                            <div style="text-align: right;">
                                <span style="color: #10b981; font-weight: 700;">+$${act.fee.toFixed(2)}</span>
                                <div style="font-size: 10px; color: var(--text-muted);">${act.time}</div>
                            </div>
                        `;
                        list.appendChild(item);
                    });
                }
            }
        } catch (err) {
            console.error('Failed to load dashboard:', err);
        }
    }

    // 2. LIVE SLOTS CONTROLLER
    async function loadSlots() {
        try {
            const data = await api.getSlots();
            if (!data.success) return;
            state.slots = data.slots;
            renderSlotsGrid();

            // Populate destination slot dropdown in analytics view
            const select = document.getElementById('nav-slot-select');
            if (select) {
                select.innerHTML = '';
                data.slots.forEach(s => {
                    const opt = document.createElement('option');
                    opt.value = s.slot_id;
                    opt.textContent = `${s.slot_id} (${s.category} - Floor ${s.floor})`;
                    select.appendChild(opt);
                });
            }
        } catch (err) {
            showToast('Failed to load parking slots', 'error');
        }
    }

    function renderSlotsGrid() {
        const container = document.getElementById('slots-container');
        if (!container) return;
        container.innerHTML = '';

        const filtered = state.slots.filter(s => {
            if (state.selectedFloor !== 'all' && s.floor.toString() !== state.selectedFloor) return false;
            if (state.selectedCategory !== 'all' && s.category !== state.selectedCategory) return false;
            if (state.selectedStatus !== 'all' && s.status !== state.selectedStatus) return false;
            return true;
        });

        if (filtered.length === 0) {
            container.innerHTML = '<div style="grid-column: 1 / -1; text-align: center; padding: 40px; color: var(--text-muted);">No slots match the selected filters.</div>';
            return;
        }

        filtered.forEach(slot => {
            const card = document.createElement('div');
            card.className = `slot-card ${slot.status.toLowerCase()}`;
            card.dataset.slotId = slot.slot_id;

            let icon = '🚗';
            if (slot.category === 'MOTORCYCLE') icon = '🏍️';
            else if (slot.category === 'ACCESSIBLE') icon = '♿';

            card.innerHTML = `
                <div class="slot-header">
                    <span class="slot-id-badge">${slot.slot_id}</span>
                    <span class="slot-type-icon" title="${slot.category}">${icon}</span>
                </div>
                <div class="slot-status-pill">${slot.status}</div>
                ${slot.occupied_reg ? `<div class="slot-plate-tag">${slot.occupied_reg}</div>` : ''}
                <div class="slot-meta">
                    <span>Floor ${slot.floor}</span>
                    <span>${slot.distance}m</span>
                </div>
            `;

            card.addEventListener('click', () => openSlotDetails(slot));
            container.appendChild(card);
        });
    }

    // Filter controls for slots
    document.querySelectorAll('#floor-tabs .tab-btn').forEach(btn => {
        btn.addEventListener('click', (e) => {
            document.querySelectorAll('#floor-tabs .tab-btn').forEach(b => b.classList.remove('active'));
            e.target.classList.add('active');
            state.selectedFloor = e.target.dataset.floor;
            renderSlotsGrid();
        });
    });

    document.getElementById('slot-category-filter')?.addEventListener('change', (e) => {
        state.selectedCategory = e.target.value;
        renderSlotsGrid();
    });

    document.getElementById('slot-status-filter')?.addEventListener('change', (e) => {
        state.selectedStatus = e.target.value;
        renderSlotsGrid();
    });

    async function openSlotDetails(slot) {
        document.getElementById('modal-slot-title').textContent = `Parking Slot ${slot.slot_id}`;
        const st = document.getElementById('modal-slot-status');
        st.textContent = slot.status;
        st.className = `badge slot-status-pill`;

        document.getElementById('modal-slot-category').textContent = slot.category;
        document.getElementById('modal-slot-floor').textContent = `Floor ${slot.floor}`;
        document.getElementById('modal-slot-zone').textContent = slot.zone;
        document.getElementById('modal-slot-plate').textContent = slot.occupied_reg || 'None (Unoccupied)';

        openModal('modal-slot-detail');

        // Fetch Dijkstra path
        const navContainer = document.getElementById('modal-slot-nav-steps');
        navContainer.innerHTML = '<span style="color: var(--text-muted);">Calculating shortest path via Dijkstra...</span>';
        try {
            const nav = await api.getNavigation(slot.slot_id, 'ENT_NORTH');
            if (nav.success && nav.path) {
                let html = `<div style="font-weight: 700; margin-bottom: 8px; color: var(--accent-blue);">Distance: ${nav.shortest_distance_meters}m (${nav.dijkstra_hops} waypoints)</div>`;
                nav.path.forEach((step, idx) => {
                    if (step.guidance) {
                        html += `<div style="padding: 4px 0; border-bottom: 1px dashed #e2e8f0;">${idx + 1}. ${step.guidance}</div>`;
                    }
                });
                navContainer.innerHTML = html;
            } else {
                navContainer.innerHTML = 'Direct path not registered in graph.';
            }
        } catch (e) {
            navContainer.innerHTML = 'Failed to load navigation route.';
        }
    }

    // 3. VEHICLE ENTRY CONTROLLER
    const entryForm = document.getElementById('entry-form');
    if (entryForm) {
        entryForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const regNumber = document.getElementById('entry-reg').value.trim();
            const vehicleType = document.getElementById('entry-type').value;
            const ownerName = document.getElementById('entry-owner').value.trim();
            const contact = document.getElementById('entry-contact').value.trim();
            const reservationId = document.getElementById('entry-reservation').value.trim();

            try {
                const res = await api.enterVehicle({
                    reg_number: regNumber,
                    vehicle_type: vehicleType,
                    owner_name: ownerName,
                    contact: contact,
                    reservation_id: reservationId
                });

                if (res.success) {
                    showToast(`Vehicle ${res.reg_number} allocated to Slot ${res.slot_id}!`, 'success');
                    entryForm.reset();

                    // Show ticket modal
                    document.getElementById('modal-ticket-id').textContent = res.ticket_id;
                    document.getElementById('modal-ticket-plate').textContent = res.reg_number;
                    document.getElementById('modal-ticket-slot').textContent = res.slot_id;
                    document.getElementById('modal-ticket-time').textContent = res.entry_time;
                    openModal('modal-ticket');

                    loadDashboard();
                }
            } catch (err) {
                if (err.message && err.message.includes('waiting queue')) {
                    const enroll = confirm(`${err.message}\n\nWould you like to enqueue this vehicle now?`);
                    if (enroll) {
                        try {
                            const qRes = await api.enqueueVehicle({
                                reg_number: regNumber,
                                vehicle_type: vehicleType,
                                owner_name: ownerName,
                                contact: contact
                            });
                            showToast(`Enqueued successfully! Position #${qRes.position}`, 'success');
                            switchView('queue');
                        } catch (qErr) {
                            showToast(qErr.message, 'error');
                        }
                    }
                } else {
                    showToast(err.message, 'error');
                }
            }
        });
    }

    document.getElementById('btn-done-ticket')?.addEventListener('click', () => {
        closeModal('modal-ticket');
        switchView('slots');
    });

    // 4. VEHICLE EXIT & BILLING CONTROLLER
    const exitLookupForm = document.getElementById('exit-lookup-form');
    if (exitLookupForm) {
        exitLookupForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const query = document.getElementById('exit-query').value.trim();
            if (!query) return;

            try {
                const res = await api.exitVehicle({ query: query, confirm: false });
                if (res.success) {
                    state.pendingExitQuery = query;
                    document.getElementById('bill-ticket-id').textContent = res.ticket_id;
                    document.getElementById('bill-plate').textContent = res.reg_number;
                    document.getElementById('bill-slot').textContent = res.slot_id;
                    document.getElementById('bill-entry-time').textContent = res.entry_time;
                    document.getElementById('bill-exit-time').textContent = res.exit_time;
                    document.getElementById('bill-duration').textContent = `${res.duration_hours}h ${res.duration_minutes}m`;
                    document.getElementById('bill-category').textContent = res.vehicle_type;

                    document.getElementById('bill-base-fee').textContent = `$${res.base_fee.toFixed(2)}`;
                    document.getElementById('bill-subsequent-fee').textContent = `$${res.subsequent_fee.toFixed(2)}`;
                    document.getElementById('bill-total-fee').textContent = `$${res.total_fee.toFixed(2)}`;

                    const graceNotice = document.getElementById('bill-grace-notice');
                    graceNotice.style.display = res.is_grace_period ? 'block' : 'none';

                    document.getElementById('exit-billing-result').style.display = 'block';
                }
            } catch (err) {
                showToast(err.message, 'error');
                document.getElementById('exit-billing-result').style.display = 'none';
            }
        });
    }

    document.getElementById('btn-cancel-checkout')?.addEventListener('click', () => {
        document.getElementById('exit-billing-result').style.display = 'none';
        state.pendingExitQuery = null;
    });

    document.getElementById('btn-confirm-checkout')?.addEventListener('click', async () => {
        if (!state.pendingExitQuery) return;
        try {
            const res = await api.exitVehicle({ query: state.pendingExitQuery, confirm: true });
            if (res.success) {
                let msg = `Checkout confirmed! Fee settled: $${res.total_fee.toFixed(2)}. Slot ${res.slot_id} is now released.`;
                if (res.queue_promoted) {
                    msg += ` Notice: Waiting vehicle ${res.promoted_vehicle} was automatically assigned to Slot ${res.slot_id}!`;
                }
                showToast(msg, 'success');
                document.getElementById('exit-billing-result').style.display = 'none';
                document.getElementById('exit-lookup-form').reset();
                state.pendingExitQuery = null;
                loadDashboard();
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    });

    // 5. WAITING QUEUE CONTROLLER
    async function loadQueue() {
        try {
            const data = await api.getQueue();
            if (!data.success) return;
            state.queue = data.queue;

            const tbody = document.getElementById('queue-table-body');
            if (!tbody) return;
            tbody.innerHTML = '';

            if (data.queue.length === 0) {
                tbody.innerHTML = '<tr><td colspan="8" style="text-align: center; color: var(--text-muted); padding: 30px;">Queue is currently empty.</td></tr>';
                return;
            }

            data.queue.forEach(item => {
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td><span class="step-number">${item.position}</span></td>
                    <td style="font-family: var(--font-mono); font-weight: 600;">${item.waiting_id}</td>
                    <td style="font-family: var(--font-mono); font-weight: 700;">${item.reg_number}</td>
                    <td>${item.vehicle_type}</td>
                    <td>${item.owner_name || '--'}</td>
                    <td>${item.contact || '--'}</td>
                    <td>${item.request_time}</td>
                    <td>
                        <button class="btn btn-danger" style="padding: 4px 10px; font-size: 11px;" onclick="cancelQueueItem('${item.waiting_id}')">Cancel</button>
                    </td>
                `;
                tbody.appendChild(tr);
            });
        } catch (err) {
            console.error('Failed to load queue:', err);
        }
    }

    window.cancelQueueItem = async function(id) {
        if (!confirm(`Cancel waiting record ${id}?`)) return;
        try {
            const res = await api.cancelQueue(id);
            if (res.success) {
                showToast('Waiting queue record removed', 'success');
                loadQueue();
                loadDashboard();
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    };

    document.getElementById('btn-open-queue-modal')?.addEventListener('click', () => openModal('modal-queue'));

    document.getElementById('queue-form')?.addEventListener('submit', async (e) => {
        e.preventDefault();
        const reg = document.getElementById('queue-reg').value.trim();
        const type = document.getElementById('queue-type').value;
        const owner = document.getElementById('queue-owner').value.trim();
        const contact = document.getElementById('queue-contact').value.trim();

        try {
            const res = await api.enqueueVehicle({
                reg_number: reg,
                vehicle_type: type,
                owner_name: owner,
                contact: contact
            });
            if (res.success) {
                showToast(`Enqueued successfully as #${res.position}`, 'success');
                closeModal('modal-queue');
                document.getElementById('queue-form').reset();
                loadQueue();
                loadDashboard();
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    });

    // 6. RESERVATIONS CONTROLLER
    async function loadReservations() {
        try {
            const data = await api.getReservations();
            if (!data.success) return;
            state.reservations = data.reservations;

            const tbody = document.getElementById('reservations-table-body');
            if (!tbody) return;
            tbody.innerHTML = '';

            if (data.reservations.length === 0) {
                tbody.innerHTML = '<tr><td colspan="10" style="text-align: center; color: var(--text-muted); padding: 30px;">No active reservations found.</td></tr>';
                return;
            }

            data.reservations.forEach(r => {
                const tr = document.createElement('tr');
                let statusColor = '#10b981';
                if (r.status === 'CANCELLED') statusColor = '#ef4444';
                else if (r.status === 'EXPIRED') statusColor = '#94a3b8';
                else if (r.status === 'FULFILLED') statusColor = '#2563eb';

                tr.innerHTML = `
                    <td style="font-family: var(--font-mono); font-weight: 600;">${r.reservation_id}</td>
                    <td style="font-family: var(--font-mono); font-weight: 700;">${r.reg_number}</td>
                    <td>${r.vehicle_type}</td>
                    <td><strong>${r.slot_id}</strong></td>
                    <td>${r.customer_name || '--'}</td>
                    <td>${r.contact || '--'}</td>
                    <td>${r.reservation_time}</td>
                    <td>${r.expiry_time}</td>
                    <td><span style="font-weight: 700; color: ${statusColor};">${r.status}</span></td>
                    <td>
                        ${r.status === 'ACTIVE' ? `<button class="btn btn-secondary" style="padding: 4px 10px; font-size: 11px;" onclick="cancelReservation('${r.reservation_id}')">Cancel</button>` : '--'}
                    </td>
                `;
                tbody.appendChild(tr);
            });
        } catch (err) {
            console.error('Failed to load reservations:', err);
        }
    }

    window.cancelReservation = async function(id) {
        if (!confirm(`Cancel reservation ${id}? This will release the reserved slot.`)) return;
        try {
            const res = await api.cancelReservation(id);
            if (res.success) {
                showToast('Reservation cancelled and slot released', 'success');
                loadReservations();
                loadDashboard();
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    };

    document.getElementById('btn-open-reservation-modal')?.addEventListener('click', () => openModal('modal-reservation'));

    document.getElementById('res-form')?.addEventListener('submit', async (e) => {
        e.preventDefault();
        const reg = document.getElementById('res-reg').value.trim();
        const type = document.getElementById('res-type').value;
        const slotPref = document.getElementById('res-slot-pref').value.trim();
        const name = document.getElementById('res-name').value.trim();
        const phone = document.getElementById('res-phone').value.trim();
        const hours = parseInt(document.getElementById('res-hours').value, 10);

        try {
            const res = await api.createReservation({
                reg_number: reg,
                vehicle_type: type,
                slot_id: slotPref,
                customer_name: name,
                contact: phone,
                valid_hours: hours
            });
            if (res.success) {
                showToast(`Reserved Slot ${res.slot_id}! Code: ${res.reservation_id}`, 'success');
                closeModal('modal-reservation');
                document.getElementById('res-form').reset();
                loadReservations();
                loadDashboard();
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    });

    // 7. VEHICLE SEARCH CONTROLLER
    async function performSearch() {
        const query = document.getElementById('search-input')?.value.trim() || '';
        try {
            const data = await api.searchVehicles(query);
            if (!data.success) return;

            const tbody = document.getElementById('search-results-body');
            if (!tbody) return;
            tbody.innerHTML = '';

            if (data.results.length === 0) {
                tbody.innerHTML = '<tr><td colspan="8" style="text-align: center; color: var(--text-muted); padding: 30px;">No vehicles found matching query.</td></tr>';
                return;
            }

            data.results.forEach(v => {
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td><span class="slot-status-pill" style="background:#fee2e2; color:#991b1b;">${v.status}</span></td>
                    <td style="font-family: var(--font-mono);">${v.ticket_id}</td>
                    <td style="font-family: var(--font-mono); font-weight: 700;">${v.reg_number}</td>
                    <td><strong>${v.slot_id}</strong></td>
                    <td>${v.vehicle_type}</td>
                    <td>${v.owner_name || '--'}</td>
                    <td>${v.contact || '--'}</td>
                    <td>${v.entry_time}</td>
                `;
                tbody.appendChild(tr);
            });
        } catch (err) {
            showToast('Search query failed', 'error');
        }
    }

    document.getElementById('btn-perform-search')?.addEventListener('click', performSearch);
    document.getElementById('search-input')?.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') performSearch();
    });

    // 8. PARKING HISTORY CONTROLLER
    async function loadHistory() {
        const sortBy = document.getElementById('history-sort-by')?.value || 'time';
        const order = document.getElementById('history-order')?.value || 'desc';

        try {
            const data = await api.getHistory(sortBy, order);
            if (!data.success) return;
            state.history = data.history;

            const tbody = document.getElementById('history-table-body');
            if (!tbody) return;
            tbody.innerHTML = '';

            if (data.history.length === 0) {
                tbody.innerHTML = '<tr><td colspan="10" style="text-align: center; color: var(--text-muted); padding: 30px;">No historical checkout records yet.</td></tr>';
                return;
            }

            data.history.forEach(t => {
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td style="font-family: var(--font-mono); font-size: 11px;">${t.transaction_id}</td>
                    <td style="font-family: var(--font-mono); font-size: 11px;">${t.ticket_id}</td>
                    <td style="font-family: var(--font-mono); font-weight: 700;">${t.reg_number}</td>
                    <td>${t.vehicle_type}</td>
                    <td>${t.slot_id}</td>
                    <td>${t.owner_name || '--'}</td>
                    <td>${t.entry_time || t.entry_time_str || '--'}</td>
                    <td>${t.exit_time || t.exit_time_str || '--'}</td>
                    <td>${t.duration_hours !== undefined ? `${t.duration_hours}h ${t.duration_minutes}m` : `${Math.floor((t.duration_seconds || 0) / 60)} mins`}</td>
                    <td><strong style="color: #10b981;">$${((t.fee !== undefined ? t.fee : t.total_fee) || 0).toFixed(2)}</strong></td>
                `;
                tbody.appendChild(tr);
            });
        } catch (err) {
            console.error('Failed to load history:', err);
        }
    }

    document.getElementById('history-sort-by')?.addEventListener('change', loadHistory);
    document.getElementById('history-order')?.addEventListener('change', loadHistory);

    // 9. ANALYTICS & GRAPH CONTROLLER
    async function loadAnalytics() {
        try {
            const data = await api.getDsaStats();
            if (!data.success) return;

            document.getElementById('dsa-ht-cap').textContent = data.hash_table.capacity;
            document.getElementById('dsa-ht-count').textContent = data.hash_table.stored_elements;
            document.getElementById('dsa-ht-coll').textContent = data.hash_table.collisions;
            document.getElementById('dsa-ht-load').textContent = `${(data.hash_table.load_factor * 100).toFixed(1)}%`;

            document.getElementById('dsa-q-count').textContent = data.queue.waiting_vehicles;
            document.getElementById('dsa-graph-nodes').textContent = data.graph.vertices;
        } catch (err) {
            console.error('Failed to load DSA analytics:', err);
        }
    }

    document.getElementById('btn-calculate-nav')?.addEventListener('click', async () => {
        const entrance = document.getElementById('nav-entrance-select').value;
        const slot = document.getElementById('nav-slot-select').value;
        if (!slot) return;

        try {
            const nav = await api.getNavigation(slot, entrance);
            if (nav.success && nav.path) {
                document.getElementById('nav-total-dist').textContent = `${nav.shortest_distance_meters} meters`;
                document.getElementById('nav-total-hops').textContent = `${nav.dijkstra_hops} steps`;

                const stepsList = document.getElementById('nav-steps-list');
                stepsList.innerHTML = '';

                nav.path.forEach((step, idx) => {
                    const div = document.createElement('div');
                    div.className = 'nav-step';
                    div.innerHTML = `
                        <div class="step-number">${idx + 1}</div>
                        <div>
                            <strong>${step.node_label} (Floor ${step.floor})</strong>
                            <div style="font-size: 12px; color: var(--text-secondary); margin-top: 2px;">
                                ${step.guidance ? step.guidance : 'Destination reached.'}
                            </div>
                        </div>
                    `;
                    stepsList.appendChild(div);
                });

                document.getElementById('nav-path-result').style.display = 'block';
            }
        } catch (err) {
            showToast('Failed to calculate navigation route', 'error');
        }
    });

    // 10. SETTINGS CONTROLLER
    async function loadSettings() {
        try {
            const data = await api.getSettings();
            if (!data.success) return;

            document.getElementById('set-car-first').value = data.car_first_hour;
            document.getElementById('set-car-sub').value = data.car_subsequent_hour;
            document.getElementById('set-bike-first').value = data.bike_first_hour;
            document.getElementById('set-bike-sub').value = data.bike_subsequent_hour;
            document.getElementById('set-acc-first').value = data.accessible_first_hour;
            document.getElementById('set-acc-sub').value = data.accessible_subsequent_hour;
            document.getElementById('set-grace').value = data.grace_period_minutes;
            document.getElementById('set-min-charge').value = data.min_charge;
        } catch (err) {
            console.error('Failed to load settings:', err);
        }
    }

    document.getElementById('settings-form')?.addEventListener('submit', async (e) => {
        e.preventDefault();
        const payload = {
            car_first_hour: parseFloat(document.getElementById('set-car-first').value),
            car_subsequent_hour: parseFloat(document.getElementById('set-car-sub').value),
            bike_first_hour: parseFloat(document.getElementById('set-bike-first').value),
            bike_subsequent_hour: parseFloat(document.getElementById('set-bike-sub').value),
            accessible_first_hour: parseFloat(document.getElementById('set-acc-first').value),
            accessible_subsequent_hour: parseFloat(document.getElementById('set-acc-sub').value),
            grace_period_minutes: parseInt(document.getElementById('set-grace').value, 10),
            min_charge: parseFloat(document.getElementById('set-min-charge').value),
            rounding_minutes: 30
        };

        try {
            const res = await api.updateSettings(payload);
            if (res.success) {
                showToast('Pricing settings saved and applied to C backend!', 'success');
            }
        } catch (err) {
            showToast(err.message, 'error');
        }
    });

    function refreshCurrentView(manual = false) {
        if (state.currentView === 'dashboard') loadDashboard();
        else if (state.currentView === 'slots') loadSlots();
        else if (state.currentView === 'queue') loadQueue();
        else if (state.currentView === 'reservations') loadReservations();
        else if (state.currentView === 'search') performSearch();
        else if (state.currentView === 'history') loadHistory();
        else if (state.currentView === 'analytics') loadAnalytics();
        else if (state.currentView === 'settings') loadSettings();

        if (manual) showToast('Data refreshed', 'success');
    }

    // Initial Load
    loadDashboard();
    loadSlots();

    // Auto-refresh every 8 seconds
    setInterval(() => {
        if (state.currentView === 'dashboard' || state.currentView === 'slots') {
            refreshCurrentView(false);
        }
    }, 8000);
});
