"""
Smart Parking Management System - Streamlit Web Application
College Data Structures & Algorithms (DSA) Capstone Project
Features: Doubly Linked Lists, DJB2 Hash Table O(1), FIFO Queue, Dijkstra Navigation, Merge & Quick Sort.
"""

import os
import json
import time
from datetime import datetime
import streamlit as st
import pandas as pd
import altair as alt
import streamlit.components.v1 as components

# Import the pure DSA core engine
from dsa_engine import SmartParkingEngine, DoublyLinkedList, DJB2HashTable, FIFOQueue, FacilityGraph

# ==============================================================================
# 1. PAGE SETUP & MODERN CSS DESIGN SYSTEM
# ==============================================================================

st.set_page_config(
    page_title="SmartPark Pro | DSA Capstone",
    page_icon="🅿️",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Custom Design System (Dark Glassmorphism, Google Fonts, Vibrant Badges)
st.markdown("""
<style>
    @import url('https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700;800&family=Inter:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500;600&display=swap');

    html, body, [class*="css"] {
        font-family: 'Inter', sans-serif;
    }

    h1, h2, h3, h4, h5, h6 {
        font-family: 'Outfit', sans-serif;
        font-weight: 700;
        letter-spacing: -0.02em;
    }

    /* Main Container Padding */
    .block-container {
        padding-top: 1.8rem;
        padding-bottom: 3rem;
        padding-left: 2.5rem;
        padding-right: 2.5rem;
    }

    /* Glassmorphic Cards */
    .glass-card {
        background: linear-gradient(135deg, rgba(30, 41, 59, 0.7) 0%, rgba(15, 23, 42, 0.8) 100%);
        border: 1px solid rgba(255, 255, 255, 0.08);
        border-radius: 16px;
        padding: 20px;
        backdrop-filter: blur(12px);
        box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.3), 0 8px 10px -6px rgba(0, 0, 0, 0.3);
        margin-bottom: 20px;
        transition: transform 0.2s ease, border-color 0.2s ease;
    }

    .glass-card:hover {
        border-color: rgba(59, 130, 246, 0.4);
    }

    /* Top Brand Hero Banner */
    .hero-banner {
        background: linear-gradient(135deg, #1e1b4b 0%, #0f172a 50%, #022c22 100%);
        border: 1px solid rgba(99, 102, 241, 0.25);
        border-radius: 20px;
        padding: 24px 30px;
        margin-bottom: 25px;
        box-shadow: 0 20px 30px -10px rgba(0, 0, 0, 0.5);
        display: flex;
        align-items: center;
        justify-content: space-between;
    }

    .brand-title {
        font-size: 28px;
        font-weight: 800;
        background: linear-gradient(135deg, #ffffff 0%, #cbd5e1 50%, #60a5fa 100%);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        margin: 0;
    }

    .brand-subtitle {
        color: #94a3b8;
        font-size: 14px;
        margin-top: 4px;
    }

    /* Status Pills */
    .status-pill {
        display: inline-flex;
        align-items: center;
        gap: 6px;
        padding: 6px 14px;
        border-radius: 9999px;
        font-size: 12px;
        font-weight: 600;
        text-transform: uppercase;
        letter-spacing: 0.05em;
    }

    .status-available {
        background: rgba(16, 185, 129, 0.15);
        color: #10b981;
        border: 1px solid rgba(16, 185, 129, 0.3);
    }

    .status-occupied {
        background: rgba(239, 68, 68, 0.15);
        color: #ef4444;
        border: 1px solid rgba(239, 68, 68, 0.3);
    }

    .status-reserved {
        background: rgba(139, 92, 246, 0.15);
        color: #a78bfa;
        border: 1px solid rgba(139, 92, 246, 0.3);
    }

    .status-maintenance {
        background: rgba(100, 116, 139, 0.15);
        color: #94a3b8;
        border: 1px solid rgba(100, 116, 139, 0.3);
    }

    /* Metric KPI Cards */
    .metric-kpi {
        background: linear-gradient(135deg, rgba(30, 41, 59, 0.8) 0%, rgba(15, 23, 42, 0.9) 100%);
        border: 1px solid rgba(255, 255, 255, 0.07);
        border-radius: 16px;
        padding: 20px 22px;
        display: flex;
        flex-direction: column;
        justify-content: space-between;
        min-height: 120px;
        box-shadow: 0 4px 20px rgba(0, 0, 0, 0.25);
    }

    .metric-kpi-label {
        font-size: 13px;
        font-weight: 600;
        color: #94a3b8;
        text-transform: uppercase;
        letter-spacing: 0.05em;
    }

    .metric-kpi-value {
        font-size: 32px;
        font-weight: 800;
        font-family: 'Outfit', sans-serif;
        color: #f8fafc;
        margin: 6px 0;
    }

    .metric-kpi-subtext {
        font-size: 12px;
        color: #64748b;
    }

    /* Parking Slot Visual Tile */
    .slot-tile {
        border-radius: 12px;
        padding: 12px;
        text-align: center;
        transition: all 0.2s ease;
        position: relative;
        overflow: hidden;
    }

    .slot-tile-avail {
        background: rgba(16, 185, 129, 0.08);
        border: 1.5px solid rgba(16, 185, 129, 0.35);
    }

    .slot-tile-avail:hover {
        background: rgba(16, 185, 129, 0.18);
        transform: translateY(-2px);
        box-shadow: 0 8px 16px rgba(16, 185, 129, 0.2);
    }

    .slot-tile-occ {
        background: rgba(239, 68, 68, 0.08);
        border: 1.5px solid rgba(239, 68, 68, 0.35);
    }

    .slot-tile-occ:hover {
        background: rgba(239, 68, 68, 0.18);
        transform: translateY(-2px);
        box-shadow: 0 8px 16px rgba(239, 68, 68, 0.2);
    }

    .slot-tile-res {
        background: rgba(139, 92, 246, 0.08);
        border: 1.5px solid rgba(139, 92, 246, 0.35);
    }

    .slot-id-badge {
        font-family: 'JetBrains Mono', monospace;
        font-size: 16px;
        font-weight: 700;
        color: #f8fafc;
    }

    /* Digital Parking Ticket */
    .ticket-container {
        background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%);
        border: 2px dashed rgba(59, 130, 246, 0.5);
        border-radius: 16px;
        padding: 24px;
        margin: 15px 0;
        box-shadow: 0 10px 30px rgba(0, 0, 0, 0.4);
    }

    .ticket-header {
        border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        padding-bottom: 12px;
        margin-bottom: 16px;
        display: flex;
        justify-content: space-between;
        align-items: center;
    }

    /* Turn-by-Turn Path Milestone */
    .path-milestone {
        display: flex;
        align-items: flex-start;
        gap: 14px;
        padding: 12px 16px;
        background: rgba(255, 255, 255, 0.03);
        border-left: 3px solid #3b82f6;
        border-radius: 0 8px 8px 0;
        margin-bottom: 10px;
    }

    .milestone-index {
        background: #3b82f6;
        color: #ffffff;
        font-size: 12px;
        font-weight: 700;
        width: 24px;
        height: 24px;
        border-radius: 50%;
        display: flex;
        align-items: center;
        justify-content: center;
        flex-shrink: 0;
    }

    /* Print Receipt */
    .receipt-box {
        background: #0f172a;
        border: 1px solid rgba(255, 255, 255, 0.15);
        border-radius: 14px;
        padding: 22px;
        font-family: 'JetBrains Mono', monospace;
    }
</style>
""", unsafe_allow_html=True)

# ==============================================================================
# 2. STATE INITIALIZATION & CACHE
# ==============================================================================

if "engine" not in st.session_state:
    st.session_state.engine = SmartParkingEngine()

engine: SmartParkingEngine = st.session_state.engine

# ==============================================================================
# 3. SIDEBAR NAVIGATION & QUICK CONTROLS
# ==============================================================================

with st.sidebar:
    st.markdown("""
    <div style="display: flex; align-items: center; gap: 12px; margin-bottom: 20px;">
        <div style="width: 44px; height: 44px; border-radius: 12px; background: linear-gradient(135deg, #3b82f6 0%, #1d4ed8 100%); display: flex; align-items: center; justify-content: center; font-size: 22px; font-weight: 800; color: white; box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4);">
            P
        </div>
        <div>
            <div style="font-size: 18px; font-weight: 800; color: #f8fafc; font-family: 'Outfit';">SmartPark Pro</div>
            <div style="font-size: 11px; color: #94a3b8;">DSA C & Python Engine</div>
        </div>
    </div>
    """, unsafe_allow_html=True)

    # Engine Status Pill
    st.markdown("""
    <div style="background: rgba(16, 185, 129, 0.1); border: 1px solid rgba(16, 185, 129, 0.25); border-radius: 10px; padding: 10px 14px; margin-bottom: 20px;">
        <div style="display: flex; align-items: center; gap: 8px;">
            <span style="display: inline-block; width: 8px; height: 8px; border-radius: 50%; background: #10b981; box-shadow: 0 0 8px #10b981;"></span>
            <span style="font-size: 12px; font-weight: 600; color: #10b981;">DSA Core Active</span>
        </div>
        <div style="font-size: 11px; color: #94a3b8; margin-top: 4px;">DJB2 Hash O(1) • Dijkstra Ready</div>
    </div>
    """, unsafe_allow_html=True)

    # Navigation Menu
    menu_selection = st.radio(
        "Navigation",
        [
            "📊 Executive Dashboard",
            "🅿️ Live Parking Slots",
            "🚗 Vehicle Entry (Allocation)",
            "💳 Exit & Automated Billing",
            "⏳ FIFO Waiting Queue",
            "📅 Slot Reservations",
            "🔍 Vehicle Search Engine",
            "📜 Parking History & DSA Sorting",
            "🗺️ Facility Dijkstra Navigation",
            "🧠 DSA Architecture Inspector",
            "🌐 Classic Web Dashboard (SPA)"
        ],
        index=0,
        label_visibility="collapsed"
    )

    st.markdown("---")

    # Quick Stats in Sidebar
    metrics = engine.get_dashboard_metrics()
    st.caption("FACILITY CAPACITY SNAPSHOT")
    c1, c2 = st.columns(2)
    with c1:
        st.metric("Available", metrics["available_slots"])
        st.metric("Parked", metrics["active_vehicles"])
    with c2:
        st.metric("Occupancy", f"{metrics['occupancy_percentage']}%")
        st.metric("Queue", metrics["waiting_queue_count"])

    st.markdown("---")

    # Quick Refresh & Reset
    col_ref, col_reset = st.columns(2)
    with col_ref:
        if st.button("🔄 Sync", use_container_width=True, help="Sync engine data with disk"):
            engine.initialize_engine()
            st.toast("Data synchronized successfully!", icon="✅")
            st.rerun()
    with col_reset:
        if st.button("⚡ Reset", use_container_width=True, help="Reinitialize default slots"):
            engine.initialize_engine()
            st.toast("Default configuration reloaded!", icon="⚡")
            st.rerun()

    st.caption(f"Last updated: {datetime.now().strftime('%H:%M:%S')}")

# ==============================================================================
# 4. VIEW: EXECUTIVE DASHBOARD
# ==============================================================================

if menu_selection == "📊 Executive Dashboard":
    # Hero Title Banner
    st.markdown("""
    <div class="hero-banner">
        <div>
            <h1 class="brand-title">Facility Operations Dashboard</h1>
            <div class="brand-subtitle">Real-time telemetry, space utilization, and algorithmic parking management</div>
        </div>
        <div style="text-align: right;">
            <span class="status-pill status-available">🟢 Pure DSA Engine Live</span>
            <div style="font-size: 11px; color: #64748b; margin-top: 6px;">Zero C-compiler dependency • Cloud-ready</div>
        </div>
    </div>
    """, unsafe_allow_html=True)

    metrics = engine.get_dashboard_metrics()

    # 4 Top KPI Cards
    col1, col2, col3, col4 = st.columns(4)

    with col1:
        st.markdown(f"""
        <div class="metric-kpi" style="border-left: 4px solid #3b82f6;">
            <div class="metric-kpi-label">Total Facility Slots</div>
            <div class="metric-kpi-value">{metrics['total_slots']}</div>
            <div class="metric-kpi-subtext">Floors 1, 2, & 3 combined</div>
        </div>
        """, unsafe_allow_html=True)

    with col2:
        st.markdown(f"""
        <div class="metric-kpi" style="border-left: 4px solid #10b981;">
            <div class="metric-kpi-label">Available Slots</div>
            <div class="metric-kpi-value" style="color: #10b981;">{metrics['available_slots']}</div>
            <div class="metric-kpi-subtext">{metrics['total_slots'] - metrics['available_slots']} slots currently occupied/reserved</div>
        </div>
        """, unsafe_allow_html=True)

    with col3:
        st.markdown(f"""
        <div class="metric-kpi" style="border-left: 4px solid #f59e0b;">
            <div class="metric-kpi-label">Active Parked Vehicles</div>
            <div class="metric-kpi-value" style="color: #f59e0b;">{metrics['active_vehicles']}</div>
            <div class="metric-kpi-subtext">Tracked in DJB2 Hash Table O(1)</div>
        </div>
        """, unsafe_allow_html=True)

    with col4:
        st.markdown(f"""
        <div class="metric-kpi" style="border-left: 4px solid #8b5cf6;">
            <div class="metric-kpi-label">Revenue Collected (Today)</div>
            <div class="metric-kpi-value" style="color: #a78bfa;">₹{metrics['today_revenue']:.2f}</div>
            <div class="metric-kpi-subtext">{metrics['today_checkouts']} transactions processed</div>
        </div>
        """, unsafe_allow_html=True)

    st.markdown("<div style='height: 15px;'></div>", unsafe_allow_html=True)

    # Charts Section
    col_chart1, col_chart2 = st.columns([1, 1])

    with col_chart1:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("🅿️ Slot Occupancy Distribution")

        status_df = pd.DataFrame([
            {"Status": "Available", "Count": metrics["available_slots"]},
            {"Status": "Occupied", "Count": metrics["occupied_slots"]},
            {"Status": "Reserved", "Count": metrics["reserved_slots"]},
            {"Status": "Maintenance", "Count": metrics["maintenance_slots"]},
        ])

        chart_status = alt.Chart(status_df).mark_arc(innerRadius=50).encode(
            theta=alt.Theta(field="Count", type="quantitative"),
            color=alt.Color(
                field="Status",
                type="nominal",
                scale=alt.Scale(
                    domain=["Available", "Occupied", "Reserved", "Maintenance"],
                    range=["#10b981", "#ef4444", "#8b5cf6", "#64748b"]
                ),
                legend=alt.Legend(title="Slot Status")
            ),
            tooltip=["Status", "Count"]
        ).properties(height=260)

        st.altair_chart(chart_status, use_container_width=True)
        st.markdown("</div>", unsafe_allow_html=True)

    with col_chart2:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("🚗 Vehicle Category Breakdown")

        cat_dist = metrics["distribution"]
        cat_df = pd.DataFrame([
            {"Category": "Cars", "Count": cat_dist["cars"]},
            {"Category": "Motorcycles", "Count": cat_dist["motorcycles"]},
            {"Category": "Accessible", "Count": cat_dist["accessible"]},
        ])

        chart_cat = alt.Chart(cat_df).mark_bar(cornerRadius=8).encode(
            x=alt.X("Category:N", title="Vehicle Category", axis=alt.Axis(labelAngle=0)),
            y=alt.Y("Count:Q", title="Active Count"),
            color=alt.Color(
                "Category:N",
                scale=alt.Scale(
                    domain=["Cars", "Motorcycles", "Accessible"],
                    range=["#3b82f6", "#f59e0b", "#ec4899"]
                ),
                legend=None
            ),
            tooltip=["Category", "Count"]
        ).properties(height=260)

        st.altair_chart(chart_cat, use_container_width=True)
        st.markdown("</div>", unsafe_allow_html=True)

    # Active Parked Vehicles Feed
    st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
    st.subheader("📋 Active Parked Vehicles (Tracked in DJB2 Hash Table)")

    active_vehicles = engine.vehicle_hash_table.get_all()
    if active_vehicles:
        df_active = pd.DataFrame(active_vehicles)[
            ["reg_number", "ticket_id", "vehicle_type", "slot_id", "owner_name", "entry_time_str"]
        ]
        df_active.columns = [
            "Registration Plate", "Ticket ID", "Category", "Assigned Slot", "Owner Name", "Check-in Timestamp"
        ]
        st.dataframe(df_active, use_container_width=True, hide_index=True)
    else:
        st.info("No vehicles currently parked. Use the 'Vehicle Entry' page to check in a vehicle.")

    st.markdown("</div>", unsafe_allow_html=True)

# ==============================================================================
# 5. VIEW: LIVE PARKING SLOTS FLOOR MAP
# ==============================================================================

elif menu_selection == "🅿️ Live Parking Slots":
    st.title("🅿️ Interactive Live Parking Slots Map")
    st.caption("Visual layout across all facility floors with real-time occupancy and category distribution.")

    # Filter Controls
    col_f1, col_f2, col_f3 = st.columns([1, 1, 1])
    with col_f1:
        floor_filter = st.selectbox("Floor Selection", ["All Floors", "Floor 1 (Ground)", "Floor 2", "Floor 3"])
    with col_f2:
        cat_filter = st.selectbox("Category Filter", ["All Categories", "CAR", "MOTORCYCLE", "ACCESSIBLE"])
    with col_f3:
        status_filter = st.selectbox("Status Filter", ["All Statuses", "AVAILABLE", "OCCUPIED", "RESERVED"])

    # Filter slots
    slots = list(engine.slots_map.values())
    filtered_slots = []
    for s in slots:
        if floor_filter == "Floor 1 (Ground)" and s["floor"] != 1:
            continue
        if floor_filter == "Floor 2" and s["floor"] != 2:
            continue
        if floor_filter == "Floor 3" and s["floor"] != 3:
            continue

        if cat_filter != "All Categories" and s["category"] != cat_filter:
            continue

        if status_filter != "All Statuses" and s["status"] != status_filter:
            continue

        filtered_slots.append(s)

    # Status Legend
    st.markdown("""
    <div style="display: flex; gap: 15px; margin: 15px 0 20px 0; align-items: center; flex-wrap: wrap;">
        <span class="status-pill status-available">🟢 Available ({})</span>
        <span class="status-pill status-occupied">🔴 Occupied ({})</span>
        <span class="status-pill status-reserved">🟣 Reserved ({})</span>
    </div>
    """.format(
        sum(1 for s in slots if s["status"] == "AVAILABLE"),
        sum(1 for s in slots if s["status"] == "OCCUPIED"),
        sum(1 for s in slots if s["status"] == "RESERVED")
    ), unsafe_allow_html=True)

    # 2D Grid Layout (4 columns)
    num_cols = 4
    cols = st.columns(num_cols)

    category_icons = {
        "CAR": "🚗",
        "MOTORCYCLE": "🏍️",
        "ACCESSIBLE": "♿"
    }

    for idx, slot in enumerate(filtered_slots):
        c = cols[idx % num_cols]
        status = slot["status"]

        if status == "AVAILABLE":
            card_class = "slot-tile-avail"
            status_text = "AVAILABLE"
            status_color = "#10b981"
            plate_display = "<span style='color: #64748b; font-size: 11px;'>Vacant</span>"
        elif status == "OCCUPIED":
            card_class = "slot-tile-occ"
            status_text = "OCCUPIED"
            status_color = "#ef4444"
            plate_display = f"<span style='color: #f87171; font-weight: 700; font-family: monospace;'>{slot.get('occupied_reg', 'Active')}</span>"
        else:
            card_class = "slot-tile-res"
            status_text = "RESERVED"
            status_color = "#a78bfa"
            plate_display = "<span style='color: #a78bfa; font-size: 11px;'>Reserved</span>"

        icon = category_icons.get(slot["category"], "🅿️")

        with c:
            st.markdown(f"""
            <div class="slot-tile {card_class}" style="margin-bottom: 14px;">
                <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px;">
                    <span class="slot-id-badge">{icon} {slot['slot_id']}</span>
                    <span style="font-size: 10px; font-weight: 700; color: {status_color}; background: rgba(0,0,0,0.2); padding: 2px 6px; border-radius: 4px;">{status_text}</span>
                </div>
                <div style="font-size: 11px; color: #94a3b8; text-align: left; margin-bottom: 4px;">{slot['zone']}</div>
                <div style="display: flex; justify-content: space-between; align-items: center; margin-top: 8px;">
                    <div>{plate_display}</div>
                    <div style="font-size: 10px; color: #64748b; font-family: monospace;">{slot['distance']}m away</div>
                </div>
            </div>
            """, unsafe_allow_html=True)

    # Detailed Slot Inspection Expander
    st.markdown("<div style='height: 20px;'></div>", unsafe_allow_html=True)
    with st.expander("🔍 Inspect Specific Slot Details & Dijkstra Route"):
        slot_options = [s["slot_id"] for s in slots]
        inspect_id = st.selectbox("Select Slot ID to inspect", slot_options)
        selected_slot = engine.slots_map.get(inspect_id)

        if selected_slot:
            i_col1, i_col2, i_col3 = st.columns(3)
            with i_col1:
                st.write(f"**Slot ID:** `{selected_slot['slot_id']}`")
                st.write(f"**Floor Level:** Floor {selected_slot['floor']}")
                st.write(f"**Category:** {selected_slot['category']}")
            with i_col2:
                st.write(f"**Status:** `{selected_slot['status']}`")
                st.write(f"**Zone:** {selected_slot['zone']}")
                st.write(f"**Distance from Gate:** {selected_slot['distance']} meters")
            with i_col3:
                st.write(f"**Occupied Vehicle:** `{selected_slot.get('occupied_reg') or 'None'}`")
                if selected_slot.get("occupied_time"):
                    st.write(f"**Parked Since:** {datetime.fromtimestamp(selected_slot['occupied_time']).strftime('%Y-%m-%d %H:%M:%S')}")

            # Route Guidance from North Entrance
            route_north = engine.graph.dijkstra("ENT_NORTH", inspect_id)
            if route_north["found"]:
                st.markdown(f"**🛣️ Turn-by-Turn Route from North Gate ({route_north['total_distance']}m):**")
                for s_idx, instr in enumerate(route_north["instructions"], 1):
                    st.markdown(f"- Step {s_idx}: {instr}")

# ==============================================================================
# 6. VIEW: VEHICLE ENTRY & ALLOCATION ENGINE
# ==============================================================================

elif menu_selection == "🚗 Vehicle Entry (Allocation)":
    st.title("🚗 Vehicle Entry & Optimal Slot Allocation")
    st.caption("Algorithmic slot selection based on category, floor level, and Dijkstra distance minimization.")

    col_form, col_result = st.columns([1, 1])

    with col_form:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("📝 Driver Registration Form")

        reg_num = st.text_input("Registration Plate *", placeholder="e.g. TN07HJ7876").strip().upper()
        veh_type = st.selectbox("Vehicle Category *", ["CAR", "MOTORCYCLE", "ACCESSIBLE"])
        owner_name = st.text_input("Driver / Owner Name", placeholder="e.g. John Doe")
        contact = st.text_input("Mobile Contact Number", placeholder="e.g. +91 98765 43210")
        entrance_gate = st.selectbox("Facility Entrance Gate", ["ENT_NORTH (North Main Entrance)", "ENT_SOUTH (South Gate Entrance)"])
        gate_id = "ENT_NORTH" if "NORTH" in entrance_gate else "ENT_SOUTH"

        submit_entry = st.button("🚀 Assign Optimal Slot & Generate Ticket", use_container_width=True, type="primary")
        st.markdown("</div>", unsafe_allow_html=True)

    with col_result:
        if submit_entry:
            if not reg_num:
                st.error("Please enter a valid vehicle registration plate.")
            else:
                res = engine.vehicle_entry(
                    reg_number=reg_num,
                    vehicle_type=veh_type,
                    owner_name=owner_name,
                    contact=contact,
                    entrance=gate_id
                )

                if res.get("success"):
                    tck = res["ticket"]
                    slot_id = res["assigned_slot"]
                    nav = res.get("navigation", {})

                    st.success(f"🎉 Slot **{slot_id}** successfully allocated to **{reg_num}**!")

                    # Render Digital Boarding Pass Ticket
                    st.markdown(f"""
                    <div class="ticket-container">
                        <div class="ticket-header">
                            <div>
                                <span style="font-size: 18px; font-weight: 800; color: #3b82f6;">SMARTPARK PRO</span>
                                <div style="font-size: 11px; color: #94a3b8;">OFFICIAL ENTRY PERMIT</div>
                            </div>
                            <div style="text-align: right;">
                                <span style="font-family: monospace; font-size: 13px; font-weight: 700; color: #f8fafc;">{tck['ticket_id']}</span>
                            </div>
                        </div>
                        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 16px;">
                            <div>
                                <div style="font-size: 11px; color: #94a3b8;">VEHICLE NUMBER</div>
                                <div style="font-size: 18px; font-weight: 800; font-family: monospace; color: #f8fafc;">{tck['reg_number']}</div>
                            </div>
                            <div>
                                <div style="font-size: 11px; color: #94a3b8;">ASSIGNED PARKING SLOT</div>
                                <div style="font-size: 20px; font-weight: 800; color: #10b981; font-family: monospace;">{slot_id}</div>
                            </div>
                            <div>
                                <div style="font-size: 11px; color: #94a3b8;">CATEGORY</div>
                                <div style="font-size: 13px; font-weight: 600; color: #f8fafc;">{tck['vehicle_type']}</div>
                            </div>
                            <div>
                                <div style="font-size: 11px; color: #94a3b8;">ENTRY TIME</div>
                                <div style="font-size: 12px; font-family: monospace; color: #f8fafc;">{tck['entry_time_str']}</div>
                            </div>
                        </div>
                        <div style="background: rgba(0,0,0,0.3); border-radius: 8px; padding: 10px; text-align: center; font-family: monospace; font-size: 11px; color: #64748b; letter-spacing: 2px;">
                            ||| | ||||| || |||||| | ||||| |||| | |||
                        </div>
                    </div>
                    """, unsafe_allow_html=True)

                    # Dijkstra Route Guidance
                    if nav.get("found"):
                        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
                        st.subheader(f"🗺️ Turn-by-Turn Driving Guidance ({nav['total_distance']}m)")
                        for m_idx, instruction in enumerate(nav["instructions"], 1):
                            st.markdown(f"""
                            <div class="path-milestone">
                                <div class="milestone-index">{m_idx}</div>
                                <div style="font-size: 13px; color: #e2e8f0;">{instruction}</div>
                            </div>
                            """, unsafe_allow_html=True)
                        st.markdown("</div>", unsafe_allow_html=True)

                elif res.get("full"):
                    st.warning(f"⚠️ {res['error']}")
                    if st.button("➕ Enqueue Vehicle to FIFO Waiting Queue", type="secondary"):
                        q_res = engine.enqueue_waiting(reg_num, veh_type, owner_name, contact)
                        if q_res.get("success"):
                            st.success(f"Vehicle added to waiting queue! Position: #{q_res['queue_position']}")
                            st.rerun()
                else:
                    st.error(res.get("error", "Failed to check in vehicle."))

# ==============================================================================
# 7. VIEW: EXIT & AUTOMATED BILLING
# ==============================================================================

elif menu_selection == "💳 Exit & Automated Billing":
    st.title("💳 Vehicle Exit, Billing & Automated Promotion")
    st.caption("Duration tracking, configurable rates, grace periods, instant checkout, and queue auto-promotion.")

    col_exit_left, col_exit_right = st.columns([1, 1])

    with col_exit_left:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("🔍 Locate Active Vehicle")

        search_query = st.text_input("Enter Plate Number or Ticket ID", placeholder="e.g. TN07HJ7876 or TCK-20261015-1002").strip().upper()

        col_b1, col_b2 = st.columns(2)
        with col_b1:
            btn_preview = st.button("🧾 Preview Bill", use_container_width=True)
        with col_b2:
            btn_checkout = st.button("💳 Settle & Checkout", type="primary", use_container_width=True)

        st.markdown("---")
        st.caption("ACTIVE VEHICLES QUICK SELECT")
        all_active = engine.vehicle_hash_table.get_all()
        if all_active:
            for v in all_active[:6]:
                if st.button(f"{v['reg_number']} ({v['slot_id']})", key=f"quick_{v['reg_number']}"):
                    st.session_state["exit_selected_plate"] = v["reg_number"]
                    st.rerun()

        st.markdown("</div>", unsafe_allow_html=True)

    # Handle query from quick select
    if "exit_selected_plate" in st.session_state and not search_query:
        search_query = st.session_state["exit_selected_plate"]

    with col_exit_right:
        if search_query and (btn_preview or btn_checkout):
            if btn_preview:
                preview = engine.vehicle_exit_preview(search_query)
                if preview.get("success"):
                    txn = preview["transaction"]
                    st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
                    st.subheader("🧾 Real-Time Billing Preview")

                    st.write(f"**Registration:** `{txn['reg_number']}` | **Category:** `{txn['vehicle_type']}`")
                    st.write(f"**Assigned Slot:** `{txn['slot_id']}` | **Ticket:** `{txn['ticket_id']}`")
                    st.write(f"**Entry Time:** {txn['entry_time_str']}")
                    st.write(f"**Current Time:** {txn['exit_time_str']}")
                    st.write(f"**Duration:** {txn['duration_hours']} hours {txn['duration_minutes']} minutes ({txn['total_minutes']} total mins)")

                    if txn["is_grace_period"]:
                        st.info("🎁 **Grace Period Active:** Stay under grace period. Total Fee: ₹0.00")
                    else:
                        st.markdown(f"""
                        <div style="background: rgba(59, 130, 246, 0.1); border: 1px solid rgba(59, 130, 246, 0.3); border-radius: 12px; padding: 16px; margin: 15px 0;">
                            <div style="display: flex; justify-content: space-between; margin-bottom: 6px;">
                                <span style="color: #94a3b8;">Base Fee (1st Hour):</span>
                                <span style="font-weight: 700; color: #f8fafc;">₹{txn['base_fee']:.2f}</span>
                            </div>
                            <div style="display: flex; justify-content: space-between; margin-bottom: 8px;">
                                <span style="color: #94a3b8;">Subsequent Fee:</span>
                                <span style="font-weight: 700; color: #f8fafc;">₹{txn['subsequent_fee']:.2f}</span>
                            </div>
                            <div style="border-top: 1px solid rgba(255,255,255,0.1); padding-top: 8px; display: flex; justify-content: space-between; align-items: center;">
                                <span style="font-weight: 800; font-size: 16px; color: #f8fafc;">Total Amount Due:</span>
                                <span style="font-weight: 800; font-size: 24px; color: #10b981;">₹{txn['total_fee']:.2f}</span>
                            </div>
                        </div>
                        """, unsafe_allow_html=True)
                    st.markdown("</div>", unsafe_allow_html=True)
                else:
                    st.error(preview.get("error", "Vehicle not found."))

            elif btn_checkout:
                checkout = engine.vehicle_exit_checkout(search_query)
                if checkout.get("success"):
                    rcpt = checkout["receipt"]
                    st.success(f"✅ Vehicle **{rcpt['reg_number']}** checked out successfully! Slot **{checkout['freed_slot']}** is now free.")

                    # Check if auto-promotion happened
                    promoted = checkout.get("promoted_queue_vehicle")
                    if promoted:
                        st.balloons()
                        st.markdown(f"""
                        <div style="background: rgba(139, 92, 246, 0.15); border: 1px solid rgba(139, 92, 246, 0.4); border-radius: 12px; padding: 14px; margin-bottom: 15px;">
                            <span style="font-weight: 800; color: #a78bfa;">🎉 FIFO Queue Auto-Promotion:</span>
                            <div style="font-size: 13px; color: #e2e8f0; margin-top: 4px;">
                                Waiting vehicle <strong>{promoted['reg_number']}</strong> has been automatically promoted into the freed slot <strong>{promoted['slot_id']}</strong>!
                            </div>
                        </div>
                        """, unsafe_allow_html=True)

                    # Official Receipt Display
                    st.markdown(f"""
                    <div class="receipt-box">
                        <div style="text-align: center; border-bottom: 1px dashed rgba(255,255,255,0.2); padding-bottom: 10px; margin-bottom: 12px;">
                            <div style="font-weight: 800; font-size: 16px; color: #f8fafc;">SMARTPARK PRO RECEIPT</div>
                            <div style="font-size: 11px; color: #94a3b8;">TRANSACTION SETTLED</div>
                        </div>
                        <div style="font-size: 12px; line-height: 1.8; color: #cbd5e1;">
                            <div>TXN ID: {rcpt['transaction_id']}</div>
                            <div>PLATE:  {rcpt['reg_number']} ({rcpt['vehicle_type']})</div>
                            <div>SLOT:   {rcpt['slot_id']}</div>
                            <div>IN:     {rcpt['entry_time_str']}</div>
                            <div>OUT:    {rcpt['exit_time_str']}</div>
                            <div>TIME:   {rcpt['duration_hours']}h {rcpt['duration_minutes']}m</div>
                            <div style="border-top: 1px dashed rgba(255,255,255,0.2); margin: 8px 0; padding-top: 8px; font-weight: 800; font-size: 15px; color: #10b981; display: flex; justify-content: space-between;">
                                <span>PAID:</span>
                                <span>₹{rcpt['total_fee']:.2f}</span>
                            </div>
                        </div>
                    </div>
                    """, unsafe_allow_html=True)
                else:
                    st.error(checkout.get("error", "Checkout failed."))

# ==============================================================================
# 8. VIEW: FIFO WAITING QUEUE
# ==============================================================================

elif menu_selection == "⏳ FIFO Waiting Queue":
    st.title("⏳ FIFO Waiting Queue Management")
    st.caption("First-In-First-Out linked-list waiting pipeline with automatic slot reallocation on checkout.")

    col_q1, col_q2 = st.columns([1, 2])

    with col_q1:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("➕ Add to Waiting Queue")
        q_reg = st.text_input("Registration Plate *", placeholder="e.g. KA05MN9999").strip().upper()
        q_type = st.selectbox("Vehicle Category *", ["CAR", "MOTORCYCLE", "ACCESSIBLE"])
        q_owner = st.text_input("Driver Name", placeholder="e.g. Alice Smith")
        q_contact = st.text_input("Contact", placeholder="e.g. +91 99887 76655")

        if st.button("Enqueue Vehicle", use_container_width=True, type="primary"):
            if not q_reg:
                st.error("Please enter a valid plate number.")
            else:
                q_res = engine.enqueue_waiting(q_reg, q_type, q_owner, q_contact)
                if q_res.get("success"):
                    st.success(f"Vehicle added at position #{q_res['queue_position']}")
                    st.rerun()
                else:
                    st.error(q_res.get("error"))
        st.markdown("</div>", unsafe_allow_html=True)

    with col_q2:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("📋 Current Queue Pipeline (Linked List Traversal)")

        queue_items = engine.waiting_queue.to_list()
        if queue_items:
            # Linked List Visual Header
            st.markdown(f"**Queue Size:** `{len(queue_items)} drivers waiting` | **Head:** `{queue_items[0]['reg_number']}` | **Tail:** `{queue_items[-1]['reg_number']}`")

            for pos, item in enumerate(queue_items, 1):
                badge_type = "🔴 Head" if pos == 1 else f"#{pos}"
                c_item1, c_item2 = st.columns([4, 1])
                with c_item1:
                    st.markdown(f"""
                    <div style="background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.08); border-radius: 10px; padding: 12px; margin-bottom: 8px;">
                        <span style="font-weight: 700; font-family: monospace; font-size: 14px; color: #f8fafc;">{item['reg_number']}</span>
                        <span style="font-size: 11px; color: #3b82f6; background: rgba(59,130,246,0.15); padding: 2px 8px; border-radius: 4px; margin-left: 8px;">{item['vehicle_type']}</span>
                        <div style="font-size: 11px; color: #94a3b8; margin-top: 4px;">Queue ID: {item['waiting_id']} • Queued at: {item['entry_time_str']}</div>
                    </div>
                    """, unsafe_allow_html=True)
                with c_item2:
                    if st.button("Remove", key=f"del_{item['waiting_id']}"):
                        engine.dequeue_waiting(item["waiting_id"])
                        st.toast("Vehicle dequeued.", icon="🗑️")
                        st.rerun()
        else:
            st.info("The waiting queue is currently empty. All arriving drivers get assigned slots directly!")

        st.markdown("</div>", unsafe_allow_html=True)

# ==============================================================================
# 9. VIEW: ADVANCE SLOT RESERVATIONS
# ==============================================================================

elif menu_selection == "📅 Slot Reservations":
    st.title("📅 Advance Slot Reservations")
    st.caption("Reserve designated slots in advance with conflict detection and automated expiration.")

    col_res1, col_res2 = st.columns([1, 1])

    with col_res1:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("📝 New Slot Reservation")

        r_reg = st.text_input("Vehicle Plate *", placeholder="e.g. DL01AB5555").strip().upper()
        r_type = st.selectbox("Category *", ["CAR", "MOTORCYCLE", "ACCESSIBLE"])

        avail_slots = [s["slot_id"] for s in engine.slots_map.values() if s["status"] == "AVAILABLE" and s["category"] == r_type]
        r_slot = st.selectbox("Preferred Available Slot (Optional)", ["Auto-Allocate Closest"] + avail_slots)
        pref_slot_arg = "" if r_slot == "Auto-Allocate Closest" else r_slot

        r_hours = st.slider("Duration (Hours)", 1, 12, 2)
        r_name = st.text_input("Customer Name", placeholder="e.g. Sarah Connor")
        r_contact = st.text_input("Contact Number", placeholder="e.g. +91 91234 56789")

        if st.button("Confirm Reservation", use_container_width=True, type="primary"):
            if not r_reg:
                st.error("Please enter a vehicle registration plate.")
            else:
                res_out = engine.create_reservation(r_reg, r_type, pref_slot_arg, r_hours, r_name, r_contact)
                if res_out.get("success"):
                    st.success(f"Reservation confirmed for Slot {res_out['slot_id']}! ID: {res_out['reservation']['reservation_id']}")
                    st.rerun()
                else:
                    st.error(res_out.get("error", "Reservation failed."))
        st.markdown("</div>", unsafe_allow_html=True)

    with col_res2:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("📋 Active Reservations")

        res_list = engine.reservations.to_list()
        if res_list:
            for r in res_list:
                c_r1, c_r2 = st.columns([4, 1])
                with c_r1:
                    st.markdown(f"""
                    <div style="background: rgba(139,92,246,0.08); border: 1px solid rgba(139,92,246,0.3); border-radius: 10px; padding: 12px; margin-bottom: 8px;">
                        <span style="font-weight: 700; color: #a78bfa; font-family: monospace;">{r['slot_id']}</span> —
                        <span style="font-weight: 600; color: #f8fafc;">{r['reg_number']}</span> ({r['vehicle_type']})
                        <div style="font-size: 11px; color: #94a3b8; margin-top: 4px;">Expires at: {r['expiry_time_str']} • Res ID: {r['reservation_id']}</div>
                    </div>
                    """, unsafe_allow_html=True)
                with c_r2:
                    if st.button("Cancel", key=f"cancel_{r['reservation_id']}"):
                        engine.cancel_reservation(r["reservation_id"])
                        st.toast("Reservation cancelled. Slot freed.", icon="✅")
                        st.rerun()
        else:
            st.info("No active reservations. You can reserve a slot using the form on the left.")

        st.markdown("</div>", unsafe_allow_html=True)

# ==============================================================================
# 10. VIEW: VEHICLE SEARCH ENGINE
# ==============================================================================

elif menu_selection == "🔍 Vehicle Search Engine":
    st.title("🔍 Vehicle Search Engine")
    st.caption("Demonstrating O(1) DJB2 Hash Table lookup and multi-attribute secondary search.")

    search_term = st.text_input("Enter Plate Number, Ticket ID, Slot ID, or Owner Name", placeholder="e.g. TN30BF3333 or C-01").strip()

    if search_term:
        start_t = time.perf_counter()
        results = engine.search_vehicles(search_term)
        elapsed_us = (time.perf_counter() - start_t) * 1_000_000

        st.markdown(f"""
        <div style="background: rgba(59,130,246,0.1); border: 1px solid rgba(59,130,246,0.3); border-radius: 10px; padding: 10px 16px; margin: 15px 0;">
            <span style="font-weight: 700; color: #60a5fa;">⚡ Search Completed:</span>
            <span style="font-size: 13px; color: #cbd5e1;">Found {len(results)} match(es) in <strong>{elapsed_us:.2f} microseconds</strong> (DJB2 polynomial hash key lookup).</span>
        </div>
        """, unsafe_allow_html=True)

        if results:
            df_search = pd.DataFrame(results)[["reg_number", "ticket_id", "vehicle_type", "slot_id", "owner_name", "entry_time_str"]]
            df_search.columns = ["Registration Plate", "Ticket ID", "Category", "Slot ID", "Owner", "Entry Timestamp"]
            st.dataframe(df_search, use_container_width=True, hide_index=True)
        else:
            st.warning(f"No active vehicle matching '{search_term}' was found.")
    else:
        st.info("Type any character into the search box to query the active registry.")

# ==============================================================================
# 11. VIEW: PARKING HISTORY & DSA SORTING LAB
# ==============================================================================

elif menu_selection == "📜 Parking History & DSA Sorting":
    st.title("📜 Parking History & DSA Sorting Lab")
    st.caption("Benchmark stable Merge Sort O(n log n) versus in-place Quick Sort O(n log n).")

    # Sorting Controls
    col_s1, col_s2, col_s3, col_s4 = st.columns([1, 1, 1, 1])

    with col_s1:
        algo_choice = st.selectbox("Sorting Algorithm", ["Merge Sort (Stable O(n log n))", "Quick Sort (Median Pivot)"])
    with col_s2:
        sort_field = st.selectbox("Sort Key", ["Total Fee", "Exit Time"])
    with col_s3:
        sort_dir = st.selectbox("Direction", ["Descending (High to Low)", "Ascending (Low to High)"])
    with col_s4:
        st.write("")
        st.write("")
        # Trigger sorting
        algo_code = "quick" if "Quick" in algo_choice else "merge"
        field_code = "fee" if "Fee" in sort_field else "time"
        order_code = "asc" if "Ascending" in sort_dir else "desc"

    sorted_history, sort_metrics = engine.get_sorted_history(sort_by=field_code, order=order_code, algorithm=algo_code)

    st.markdown(f"""
    <div style="background: rgba(16,185,129,0.1); border: 1px solid rgba(16,185,129,0.3); border-radius: 10px; padding: 12px 18px; margin: 15px 0;">
        <span style="font-weight: 700; color: #10b981;">⚡ DSA Benchmark:</span>
        <span style="font-size: 13px; color: #e2e8f0;">
            Sorted <strong>{sort_metrics['record_count']} records</strong> in <strong>{sort_metrics['elapsed_microseconds']} microseconds</strong> using <strong>{sort_metrics['algorithm']}</strong>.
        </span>
    </div>
    """, unsafe_allow_html=True)

    if sorted_history:
        df_hist = pd.DataFrame(sorted_history)[
            ["transaction_id", "reg_number", "vehicle_type", "slot_id", "entry_time_str", "exit_time_str", "total_fee"]
        ]
        df_hist.columns = ["Transaction ID", "Plate Number", "Category", "Slot", "Check-in Time", "Check-out Time", "Fee Paid (₹)"]
        st.dataframe(df_hist, use_container_width=True, hide_index=True)

        # CSV Download Button
        csv_data = df_hist.to_csv(index=False).encode("utf-8")
        st.download_button(
            label="📥 Download Audited Parking History (CSV)",
            data=csv_data,
            file_name="parking_history_audit.csv",
            mime="text/csv"
        )
    else:
        st.info("No historical transactions recorded yet.")

# ==============================================================================
# 12. VIEW: FACILITY DIJKSTRA NAVIGATION
# ==============================================================================

elif menu_selection == "🗺️ Facility Dijkstra Navigation":
    st.title("🗺️ Facility Dijkstra Shortest Path Navigation")
    st.caption("Interactive graph pathfinding computing shortest driving distances from gate entrances to target slots.")

    col_nav1, col_nav2 = st.columns([1, 2])

    with col_nav1:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("📍 Route Selection")

        nav_entrance = st.selectbox("Select Entrance Gate", ["ENT_NORTH (North Main Entrance)", "ENT_SOUTH (South Gate Entrance)"])
        gate_code = "ENT_NORTH" if "NORTH" in nav_entrance else "ENT_SOUTH"

        all_slot_ids = sorted(list(engine.slots_map.keys()))
        nav_target = st.selectbox("Select Target Parking Slot", all_slot_ids)

        st.markdown("---")
        st.markdown(f"**Target Zone:** `{engine.slots_map[nav_target]['zone']}`")
        st.markdown(f"**Target Category:** `{engine.slots_map[nav_target]['category']}`")
        st.markdown(f"**Floor Level:** Floor {engine.slots_map[nav_target]['floor']}")

        st.markdown("</div>", unsafe_allow_html=True)

    with col_nav2:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("🛣️ Computed Driving Route")

        route = engine.graph.dijkstra(gate_code, nav_target)

        if route["found"]:
            st.markdown(f"""
            <div style="background: rgba(59,130,246,0.15); border: 1px solid rgba(59,130,246,0.35); border-radius: 12px; padding: 14px; margin-bottom: 18px; display: flex; justify-content: space-between; align-items: center;">
                <div>
                    <div style="font-size: 11px; color: #94a3b8;">TOTAL DISTANCE</div>
                    <div style="font-size: 22px; font-weight: 800; color: #60a5fa;">{route['total_distance']} meters</div>
                </div>
                <div style="text-align: right;">
                    <div style="font-size: 11px; color: #94a3b8;">GRAPH WAYPOINTS</div>
                    <div style="font-size: 18px; font-weight: 700; color: #f8fafc;">{len(route['path_nodes'])} vertices</div>
                </div>
            </div>
            """, unsafe_allow_html=True)

            # Node trail breadcrumbs
            st.markdown(f"**Path Trail:** `{' ➔ '.join(route['path_nodes'])}`")
            st.markdown("<div style='height: 10px;'></div>", unsafe_allow_html=True)

            # Step-by-step guidance cards
            for s_idx, instr in enumerate(route["instructions"], 1):
                st.markdown(f"""
                <div class="path-milestone">
                    <div class="milestone-index">{s_idx}</div>
                    <div style="font-size: 13px; color: #e2e8f0;">{instr}</div>
                </div>
                """, unsafe_allow_html=True)
        else:
            st.error("No valid driving route could be computed between these points.")

        st.markdown("</div>", unsafe_allow_html=True)

# ==============================================================================
# 13. VIEW: DSA ARCHITECTURE INSPECTOR
# ==============================================================================

elif menu_selection == "🧠 DSA Architecture Inspector":
    st.title("🧠 DSA Architecture & Complexity Inspector")
    st.caption("Deep inspection of hash table buckets, collision counts, queue pointers, and algorithmic proofs.")

    bench = engine.get_dsa_benchmarks()

    c_b1, c_b2 = st.columns(2)

    with c_b1:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("🔑 DJB2 Hash Table (Separate Chaining)")
        ht = bench["hash_table"]
        st.write(f"- **Algorithm:** `{ht['algorithm']}`")
        st.write(f"- **Bucket Capacity:** `{ht['capacity']}`")
        st.write(f"- **Active Keys:** `{ht['active_keys']}`")
        st.write(f"- **Load Factor (α):** `{ht['load_factor']}`")
        st.write(f"- **Collisions Encountered:** `{ht['collisions']}`")
        st.write(f"- **Occupied Buckets:** `{ht['occupied_buckets']}`")
        st.markdown(r"""
        > **DJB2 Formula:** $h = ((h \ll 5) + h) + c$
        >
        > **Search Complexity:** Average $O(1)$, Worst-case $O(n)$ with chaining.
        """)
        st.markdown("</div>", unsafe_allow_html=True)

    with c_b2:
        st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
        st.subheader("⏳ FIFO Linked-List Waiting Queue")
        q = bench["queue"]
        st.write(f"- **Queue Length:** `{q['size']}`")
        st.write(f"- **Head Vehicle:** `{q['head'] or 'None'}`")
        st.write(f"- **Tail Vehicle:** `{q['tail'] or 'None'}`")
        st.write(f"- **Enqueue Time Complexity:** `{q['complexity_enqueue']}`")
        st.write(f"- **Dequeue Time Complexity:** `{q['complexity_dequeue']}`")
        st.markdown("""
        > **FIFO Invariant:** Drivers are queued in strictly chronological arrival order.
        > Upon vehicle checkout, the queue is scanned in-order for category eligibility.
        """)
        st.markdown("</div>", unsafe_allow_html=True)

    st.markdown("<div class='glass-card'>", unsafe_allow_html=True)
    st.subheader("🗺️ Facility Graph & Dijkstra Analysis")
    g_info = bench["graph"]
    st.write(f"- **Total Facility Vertices (V):** `{g_info['total_vertices']}`")
    st.write(f"- **Total Directed/Bidirectional Edges (E):** `{g_info['total_edges']}`")
    st.write(f"- **Dijkstra Complexity:** `{g_info['dijkstra_complexity']}`")
    st.write(f"- **BFS Traversal Complexity:** `{g_info['bfs_complexity']}`")

    complexity_df = pd.DataFrame([
        {"Data Structure / Algorithm": "Doubly Linked List", "Operation": "Insert at Head/Tail", "Time Complexity": "O(1)", "Space Complexity": "O(n)"},
        {"Data Structure / Algorithm": "DJB2 Hash Table", "Operation": "Search / Insert by Plate", "Time Complexity": "O(1) avg", "Space Complexity": "O(n)"},
        {"Data Structure / Algorithm": "FIFO Queue", "Operation": "Enqueue / Dequeue", "Time Complexity": "O(1)", "Space Complexity": "O(n)"},
        {"Data Structure / Algorithm": "Dijkstra's Algorithm", "Operation": "Shortest Driving Path", "Time Complexity": "O(V^2)", "Space Complexity": "O(V)"},
        {"Data Structure / Algorithm": "Merge Sort", "Operation": "Sort Parking History (Stable)", "Time Complexity": "O(n log n)", "Space Complexity": "O(n)"},
        {"Data Structure / Algorithm": "Quick Sort", "Operation": "In-Place Quick Sort", "Time Complexity": "O(n log n) avg", "Space Complexity": "O(log n)"},
    ])
    st.dataframe(complexity_df, use_container_width=True, hide_index=True)
    st.markdown("</div>", unsafe_allow_html=True)

# ==============================================================================
# 14. VIEW: CLASSIC WEB DASHBOARD (EMBEDDED)
# ==============================================================================

elif menu_selection == "🌐 Classic Web Dashboard (SPA)":
    st.title("🌐 Classic Web Dashboard (Embedded Prototype)")
    st.caption("The original single-page HTML5/CSS3 prototype embedded directly inside your Streamlit deployment.")

    index_html_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "index.html")

    if os.path.exists(index_html_path):
        try:
            with open(index_html_path, "r", encoding="utf-8") as f:
                html_content = f.read()

            # Render full component
            components.html(html_content, height=850, scrolling=True)
        except Exception as e:
            st.error(f"Error loading index.html: {e}")
    else:
        st.warning("index.html not found in the root directory.")
