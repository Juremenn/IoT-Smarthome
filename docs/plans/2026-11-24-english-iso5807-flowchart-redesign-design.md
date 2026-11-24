# Design Document: English ISO 5807 Anti-Slop Flowchart Redesign

**Date:** 2026-11-24
**Project:** Rumah Joglo Miniature Smart Home IoT System (ESP32)
**Status:** Approved by User
**Design Read (`antislop`):** Reading this as: ISO 5807 technical engineering flowcharts for academic and developer documentation, in a strict monochrome black-and-white engineering style, dial `ENERGY 1 / RHYTHM 1 / MOTION 1`.

---

## 1. Problem Statement

The previous flowchart deliverables in `IotSmarthome.drawio`, `PANDUAN_FLOWCHART_SMART_HOME.md`, `ALUR_FLOWCHART_SMART_HOME.txt`, and `README.md` exhibited several structural and copywriting flaws:
1. **Language Mismatch:** The diagrams were written in Indonesian, whereas the required target language is technical English.
2. **Topological & Routing Defects in `IotSmarthome.drawio`:**
   - **False Parallel Branching:** In Tab 1, node `M05` branched one outgoing line into four parallel columns sharing the exact same horizontal bus (`y=258`), and all four columns converged 11 arrows onto a single horizontal bus (`y=945`) into one port (`M06`).
   - **Multi-Branch Decision Vertex Collisions:** Nodes `C2_2` (Tab 1) and `P03` (Tab 3) routed two to three distinct branches out of the exact same diamond vertex (`exitX=1, exitY=0.5` or `exitX=0, exitY=0.5`), causing overlapping flowlines and unreadable branch labels.
   - **Excessive Zig-Zag Routing:** Edges `D06B -> D07` (Tab 4) and `W08A -> W09` (Tab 5) contained five orthogonal bends each due to unaligned branch merge nodes.
3. **Copywriting & AI Slop Patterns:** Presence of em dashes, all-caps section banners, inconsistent abbreviations (`Buka 2d`), and imprecise descriptions of the non-blocking firmware execution loop.

---

## 2. Design Decisions & Written Justifications (`antislop` R-31)

- **Monochrome ISO 5807 Palette (`#FFFFFF` fill, `#000000` stroke `1.5px`, `#000000` text):** Chosen to satisfy formal academic and engineering documentation standards with maximum 21:1 contrast (WCAG AAA) across digital and monochrome print media.
- **Typography (`Arial`, `11pt` node text, `10pt` branch labels, natural title/sentence casing):** Chosen to guarantee cross-platform bounding-box stability in Draw.io without text clipping or all-caps visual noise.
- **Strict Binary Decision Diamonds:** Every `rhombus` decision symbol has at most one incoming edge (top vertex) and two outgoing edges on distinct vertices (bottom, left, or right) labeled `Yes` or `No`. Multi-option dispatches (such as web dashboard commands) use cascaded binary decision diamonds.
- **Sequential Pipeline Topology (Tab 1):** Arranged left-to-right across four sequential execution columns matching `void loop()` in `src/main.cpp`, eliminating shared 11-edge collector buses and false multi-threaded branching.
- **ISO 5807 Shape Semantics:**
  - `Terminator` (`shape=mxgraph.flowchart.terminator`): System or sub-system `Start` and `End` points.
  - `Preparation` (`shape=hexagon`): Hardware GPIO, PWM timer, I2C, Wi-Fi AP, and HTTP server initialization in `setup()`.
  - `Input / Output` (`shape=parallelogram`): Physical sensor sampling (`D34`, `D4/D5`, `D27`), HTTP API requests/responses, and servo PWM output pulses (`D13/D12`).
  - `Process` (`rounded=0`): Internal state mutations, anti-flicker filtering, distance calculation, and motion curve computation (`easeInOutSine` and `easeSoftClose`).
  - `Decision` (`rhombus`): Binary condition checks with explicit `Yes` and `No` branch labels.
  - `Display` (`shape=display`): Visual telemetry rendered on the SSD1306 OLED screen or the Web Dashboard UI.
  - `Delay` (`shape=delay`): Explicit hold timers (`2000 ms` door hold) and polling intervals (`1000 ms` HTTP status poll).

---

## 3. Architecture of the 5 Draw.io Diagrams (`IotSmarthome.drawio`)

### 3.1 Tab 1: `1. Complete System Flowchart` (`pg_main`)
- **Top Row (Boot & Initialization, Left to Right):**
  - `M01 [Terminator]`: `Start (ESP32 Power ON)`
  - `M02 [Preparation]`: `Initialize ESP32 Hardware: Servo D13/D12 (Closed 180°), Fan D23 (OFF), Flame D34, Ultrasonic D4/D5, DHT11 D27, OLED D21/D22`
  - `M03 [Preparation]`: `Start Standalone Wi-Fi AP ("RumahJoglo-SmartHome", 192.168.4.1) & HTTP Web Server (Port 80)`
  - `M04 [Display]`: `Display "System Ready" Splash & IP Address on OLED`
  - `M05 [Input/Output]`: `Sample Sensors (Flame D34, HC-SR04 D4/D5, DHT11 D27) & Poll HTTP Requests (/api/*)`
- **Column 1 (Stage 1: Fire Detection & Emergency Evacuation, Priority 1):**
  - `C1_1 [Decision]`: `Flame Detected on D34? (Active >= 15 ms)`
    - `No (Safe)` -> advances sequentially to top of Column 2 (`C2_1`).
    - `Yes` -> `C1_2 [Process]`: `Activate Emergency Evacuation: Force Door Open to 0° & Lock Web Close Button` -> `C1_3 [Input/Output]`: `Drive Servo PWM (D13/D12) Open to 0°` -> `C1_4 [Display]`: `Flash OLED "FIRE WARNING!" & Trigger Web Alarm (/alarm.mp3)` -> `C1_5 [Decision]`: `Flame Extinguished & Quiet >= 1.2 s?`
      - `No (Still Active)` -> loops back to `C1_3`.
      - `Yes` -> `C1_6 [Process]`: `Clear Evacuation State, Stop Alarm, & Schedule Soft-Close Timer (+2 s)` -> advances to top of Column 2 (`C2_1`).
- **Column 2 (Stage 2: Web Dashboard Command Dispatch):**
  - `C2_1 [Decision]`: `Web Command Received?` (`No` -> advances to Column 3 `C3_1`; `Yes` -> `C2_2`).
  - `C2_2 [Decision]`: `Action == "Open 2s"?` (`Yes` -> `C2_5 [Process]`: `Start Auto Cycle: Open to 0°, Hold 2 s, Soft-Close to 180°`; `No` -> `C2_3`).
  - `C2_3 [Decision]`: `Action == "Keep Open"?` (`Yes` -> `C2_6 [Process]`: `Latch Manual Open at 0° (Bypass Ultrasonic Sensor)`; `No` -> `C2_4`).
  - `C2_4 [Decision]`: `Action == "Close"?` (`Yes` -> `C2_7 [Process]`: `Soft-Close Door to 180° (If No Fire Active)`; `No` -> `C2_8 [Process]`: `Invert Flame Sensor Logic (Active-HIGH / Active-LOW)`).
  - All Column 2 action outputs merge cleanly on a dedicated collector line and advance to top of Column 3 (`C3_1`).
- **Column 3 (Stage 3: Ultrasonic Automatic Door Logic):**
  - `C3_1 [Decision]`: `Manual Open or Evacuation Active?` (`Yes (Bypass)` -> advances to Column 4 `C4_1`; `No` -> `C3_2`).
  - `C3_2 [Input/Output]`: `Measure HC-SR04 Distance (D4/D5): (Duration * 0.034) / 2 cm` -> `C3_3 [Decision]`: `Object Detected? (0 < Distance <= Threshold)`
    - `Yes` -> `C3_4 [Process]`: `Open Door to 0° / Extend 2 s Hold / Anti-Pinch Reverse` -> advances to Column 4 `C4_1`.
    - `No` -> `C3_5 [Decision]`: `Auto-Open Hold Timer (2 s) Expired?`
      - `Yes` -> `C3_6 [Process]`: `Soft-Close Door to 180° (7000 ms Curve)` -> advances to Column 4 `C4_1`.
      - `No` -> advances to Column 4 `C4_1`.
- **Column 4 (Stage 4: Non-Blocking DHT11 Telemetry & Hot-Plug OLED):**
  - `C4_1 [Decision]`: `Door Stationary & DHT11 Interval >= 2.1 s?`
    - `No (Defer While Servo Moves)` -> routes cleanly to `C4_6`.
    - `Yes` -> `C4_2 [Input/Output]`: `Read Temperature (°C) & Humidity (%) from DHT11 (Pin D27)` -> `C4_3 [Decision]`: `Reading Valid? (-10..80 °C & 0..100%)`
      - `Yes` -> `C4_4 [Process]`: `Store New Temperature & Humidity (Reset Failure Counter)` -> `C4_6`.
      - `No` -> `C4_5 [Process]`: `Retain Last Valid Reading (Max 5 Consecutive Failures)` -> `C4_6`.
  - `C4_6 [Display]`: `Update SSD1306 OLED (D21/D22 Hot-Plug): IP, Wayang Animation, Temp, & "FIRE: SAFE"` -> advances to Bottom Row `M06`.
- **Bottom Row (Actuator Update, Web Sync & Loop Termination):**
  - `M06 [Input/Output]`: `Drive Door Servo PWM (D13 & D12: 50 Hz, 500 to 2400 us, 20 ms Tick)` -> `M07 [Display]`: `Sync Web Dashboard UI (Devices Tab: 5 Cards & Distance Gauge, Pinout Info Tab)` -> `M08 [Decision]`: `Is System Powered OFF?`
    - `No (Continue Loop)` -> loops back along outer right margin to `M05`.
    - `Yes` -> `M09 [Terminator]`: `End (System Halted)`.

### 3.2 Tab 2: `2. Fire Detection & Evacuation` (`pg_f1`)
- Nodes `A01` to `A13` with clean orthogonal loop-back from `A10` (`Flame Extinguished & Quiet >= 1.2 s?` -> `No`) to `A04` (`Read Digital Flame Signal on D34 (Every 5 ms)`), and clean convergence of `A11` (post-evacuation recovery) and `A12` (normal safe state) into `A13` (`End`).

### 3.3 Tab 3: `3. Door Servo Control` (`pg_f2`)
- Nodes `P01` to `P14` using three cascaded binary decision diamonds (`P03A`: `Command == "Keep Open"?`, `P03B`: `Command == "Open 2s"?`, `P03C`: `Command == "Close"?`) so every diamond vertex carries at most one outgoing edge.

### 3.4 Tab 4: `4. DHT11 & Hot-Plug OLED` (`pg_f3`)
- Nodes `D01` to `D10` arranged without multi-bend cross-overs: validation outcomes `D06A` (valid) and `D06B` (invalid fallback) feed directly downward into `D07` (`Is OLED Active on I2C (0x3C / 0x3D)?`), followed by `D08A` / `D08B` -> `D09` -> `D10` (`End`).

### 3.5 Tab 5: `5. Web Dashboard Workflow` (`pg_f4`)
- Nodes `W01` to `W11` arranged in a top-down spine so `W08` (`User Clicked Control Button?`) and `W08A` (`Send HTTP GET /api/door or /api/flame`) merge cleanly into `W09` (`Wait Polling Interval (1000 ms)`) -> `W10` (`Is Dashboard Closed?`) -> `W11` (`End`).

---

## 4. Documentation Synchronization & Verification Criteria

1. **`IotSmarthome.drawio`:** Rebuild all 5 tabs in English with exact coordinates, zero overlapping edges, zero multi-branch diamond ports, and validate via `drawio:validate_diagram` and `drawio:analyze_diagram`.
2. **`PANDUAN_FLOWCHART_SMART_HOME.md` & `ALUR_FLOWCHART_SMART_HOME.txt`:** Rewrite in English to document the exact node IDs, ISO 5807 shapes, text labels, and transitions of the 5 diagrams.
3. **`README.md`:** Translate Section 3 (Master System Flowchart in Mermaid) and Section 4 (ISO 5807 Symbol Legend) into English, removing all em dashes and AI slop patterns.
4. **`antislop` Delivery Gate:** Run the full 4-block Delivery Gate + Copywriting Checklist and report concrete verification evidence before completion.
