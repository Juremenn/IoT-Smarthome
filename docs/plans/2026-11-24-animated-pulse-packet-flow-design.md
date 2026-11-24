# Design Document: Animated Pulse Packet Flow & Minimalist Text Overhaul

**Date:** 2026-11-24  
**Topic:** 9Router-inspired Animated Pulse Packet Flow on Automation Cards & Text Cleanliness Overhaul  
**Status:** Approved  

---

## 1. Overview & Objective
Transform the static Automation Flow diagram in the "Info Pinout" view into a dynamic, 9Router-inspired interactive signal packet routing visualization. Concurrently, strip all verbose narrative paragraphs across the dashboard to deliver a crisp, high-density, minimal light-theme IoT dashboard.

---

## 2. Visual Architecture & Component Design

### 2.1 Animated Wire Conduit (Connectors between Nodes)
- Replace static chevron icons with an interactive conduit line connecting Node 1 &rarr; Node 2 &rarr; Node 3.
- Conduit width: responsive flex layout (40px to 64px width on desktop, vertical on mobile).
- SVG Structure:
  - Base conduit line (`stroke: #E5E7EB; stroke-width: 2px; stroke-linecap: round;`).
  - Signal packet element: An animated SVG `<circle>` or CSS keyframe pseudo-element traveling along the conduit.
  - Direction indicator: Minimalist chevron arrowhead (`#9CA3AF`).

### 2.2 Flow 1: Akses Otomatis (Ultrasonic to Servo)
- **Nodes:**
  1. `Sensor Ultrasonik` (Input token: `≤ 15 cm`)
  2. `ESP32 DevKit V1` (Processing token: `Non-blocking Loop`)
  3. `Servo Buka Pintu` (Action token: `5s`)
- **Animation States:**
  - *Standby Mode:* Smooth traveling packet dot (warm gold / celadon `#4A6B5D`) gliding across every 2 seconds with `cubic-bezier(0.4, 0, 0.2, 1)`.
  - *Triggered Mode (Distance ≤ 15 cm):* Signal packet turns terracotta `#A84A28`, travel speed accelerates to 0.75 seconds, and Servo node border gently pulses to show active actuation.

### 2.3 Flow 2: Keselamatan Api (Flame to Evacuation)
- **Nodes:**
  1. `Sensor Api` (Input token: `D34`)
  2. `Konfirmasi Filter` (Processing token: `15ms`)
  3. `Evakuasi` (Action token: `Pintu Buka Penuh`)
- **Animation States:**
  - *Normal Mode:* Calm, dim trace indicator gliding at low frequency.
  - *Danger Mode (Flame Detected):* Fast crimson packet (`#DC2626`) firing continuously across the conduit, paired with an emergency warning alert ring on the Evakuasi node.

---

## 3. Minimalist Text Cleanliness Overhaul

### 3.1 View 2 (Info Pinout)
- Remove verbose hero subtitle paragraph (`<p class="hero-desc">`).
- Hero chip format: `ESP32 DevKit V1 · 240 MHz`, `Wi-Fi AP: RumahJoglo`, `Suplai: 5V DC`, `OLED I2C: D21/D22`.
- Remove all long descriptive `<p>` narrative paragraphs from the 4 sensor cards (`pinout-card-body`).
- Keep only: Card title, GPIO badges, and technical spec chips (`PWM 50 Hz`, `Sudut 0° - 180°`, `Siklus 5s`, etc.).

### 3.2 View 1 (Perangkat / Dashboard)
- Door sub-label: simplify `Standby (Gerak Halus Siap)` &rarr; `Standby`.
- Flame sub-label: simplify `Kondisi Normal Terpantau` &rarr; `Normal`.
- Trigger footnote: simplify `Pintu terbuka saat terdeteksi jarak ≤ 15 cm` &rarr; `Pemicu ≤ 15 cm`.
- Distance sub-label: simplify `Status: Standby` &rarr; `Standby`.
- Synchronize all dynamic status strings in `app.js` to match these compact labels.

---

## 4. Technical Implementation & Performance Constraints
- Pure CSS keyframes and SVG stroke animations.
- Zero external heavyweight dependencies (Lottie or Canvas not needed).
- Hardware-accelerated transforms (`transform: translateX(...)` and `opacity`) for 60fps rendering without CPU overhead on client devices.
- Single source of truth sync via `tools/sync_to_esp.py`.
- Validation via `tools/test_server.py`.
