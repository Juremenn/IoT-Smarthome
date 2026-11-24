# English ISO 5807 Anti-Slop Flowchart Redesign Implementation Plan

> **For Antigravity:** REQUIRED SUB-SKILL: Load executing-plans to implement this plan task-by-task.

**Goal:** Rebuild all 5 tabs of `IotSmarthome.drawio` and synchronize `PANDUAN_FLOWCHART_SMART_HOME.md`, `ALUR_FLOWCHART_SMART_HOME.txt`, and `README.md` (Section 3 & 4) into crisp, accurate, anti-slop English adhering to ISO 5807 monochrome engineering standards.

**Architecture:** The Master System Flowchart (Tab 1) uses a sequential 4-stage pipeline layout (`Start` -> Boot Row -> Stage 1 Fire Evacuation -> Stage 2 Web Command Dispatch -> Stage 3 Ultrasonic Automation -> Stage 4 Non-Blocking DHT11 & OLED -> Bottom Output Row -> `End`), while Tabs 2 to 5 provide self-contained `Start` to `End` sub-system diagrams. Every decision diamond enforces strict binary branching (at most one outgoing edge per diamond vertex), and all copywriting complies with `antislop` and `antislop-copywriting` rules (zero em dashes, zero buzzwords, 100% fidelity to `src/*.cpp` and `web_src/*`).

**Tech Stack:** Draw.io mxGraph XML (`IotSmarthome.drawio`), Draw.io MCP Validation Tools (`validate_diagram`, `analyze_diagram`), GitHub Mermaid (`README.md`), Python 3 verification scripts.

---

### Task 1: Write Automated Flowchart Quality & Anti-Slop Audit Script

**Files:**
- Create: `tools/test_flowchart.py`

**Step 1: Write the verification test script**

Create `tools/test_flowchart.py` to audit `IotSmarthome.drawio`, `PANDUAN_FLOWCHART_SMART_HOME.md`, `ALUR_FLOWCHART_SMART_HOME.txt`, and `README.md` for:
1. Valid XML structure and presence of all 5 diagram tabs in English.
2. Zero em dashes (`—` / `&mdash;`) across all flowchart files (`antislop` R-02).
3. Zero Indonesian leftover words (`Inisialisasi`, `Apakah`, `Tidak`, `Pintu`, `Suhu`, `Layar`, `Kirim`, `Baca`, `Tombol`, `Mulai`, `Selesai`) inside `IotSmarthome.drawio`, `PANDUAN_FLOWCHART_SMART_HOME.md`, and `ALUR_FLOWCHART_SMART_HOME.txt`.
4. Strict binary decision diamond rule in `IotSmarthome.drawio`: every `rhombus` vertex has at most 2 outgoing edges, and never two outgoing edges sharing the same `(exitX, exitY)` port on any decision node.

**Step 2: Run the test to confirm it fails on current files**

Run: `python tools/test_flowchart.py`
Expected: FAIL (detects Indonesian labels, em dashes, and duplicate diamond exit ports in `IotSmarthome.drawio`).

---

### Task 2: Rebuild All 5 Tabs of `IotSmarthome.drawio` in English ISO 5807 Monochrome

**Files:**
- Modify: `IotSmarthome.drawio`

**Step 1: Generate clean English 5-tab mxGraph XML for `IotSmarthome.drawio`**

Implement all 5 diagrams in `IotSmarthome.drawio`:
- `pg_main`: `1. Complete System Flowchart (Start-End)` — Sequential 4-column pipeline with cascaded binary decisions for Web Commands (`C2_2`, `C2_3`, `C2_4`) and distinct orthogonal channels with zero shared bus collisions.
- `pg_f1`: `2. Fire Detection & Evacuation` — Nodes `A01` to `A13` with clean 5 ms sampling, 15 ms / 2-streak anti-flicker confirmation, 0 deg evacuation override, and 1.2 s release hold.
- `pg_f2`: `3. Door Servo Control` — Nodes `P01` to `P14` with cascaded binary decisions (`P03A`, `P03B`, `P03C`), 5000 ms `easeInOutSine` opening, 2 s hold delay, 7000 ms `easeSoftClose` closing, and anti-pinch reversal.
- `pg_f3`: `4. DHT11 & Hot-Plug OLED` — Nodes `D01` to `D10` with non-blocking servo guard (`>= 2.1 s`), `-10..80 °C` & `0..100%` validation, 5-failure retention, and 1500 ms I2C hot-plug recovery.
- `pg_f4`: `5. Web Dashboard Workflow` — Nodes `W01` to `W11` covering AP connection (`192.168.4.1`), 1000 ms status polling, Devices & Pinout Info tabs, fire alarm audio playback (`/alarm.mp3`), and control actions.

**Step 2: Validate geometry and topology using Draw.io MCP tools**

Run MCP tools:
- `drawio:validate_diagram` on `C:\Users\Nirvana\Documents\Script\IoT-Smarthome\IotSmarthome.drawio`
- `drawio:analyze_diagram` on `C:\Users\Nirvana\Documents\Script\IoT-Smarthome\IotSmarthome.drawio`
Expected: 0 node collisions, 0 text boundary overflows, clean entry (`Start`) and sink (`End`) topology on every page.

---

### Task 3: Rewrite `PANDUAN_FLOWCHART_SMART_HOME.md` and `ALUR_FLOWCHART_SMART_HOME.txt` in English

**Files:**
- Modify: `PANDUAN_FLOWCHART_SMART_HOME.md`
- Modify: `ALUR_FLOWCHART_SMART_HOME.txt`

**Step 1: Update `PANDUAN_FLOWCHART_SMART_HOME.md` in English**

Rewrite `PANDUAN_FLOWCHART_SMART_HOME.md` in English so every section, ISO 5807 symbol dictionary, node ID table (`M01..M09`, `C1_1..C4_6`, `A01..A13`, `P01..P14`, `D01..D10`, `W01..W11`), and GPIO reference table matches `IotSmarthome.drawio` 100%, with zero em dashes.

**Step 2: Update `ALUR_FLOWCHART_SMART_HOME.txt` in English**

Rewrite `ALUR_FLOWCHART_SMART_HOME.txt` in English as a concise plain-text step-by-step reference synchronized with the 5 diagrams and firmware constants, with zero em dashes.

---

### Task 4: Update Master Flowchart & ISO 5807 Legend in `README.md` to English

**Files:**
- Modify: `README.md:50-205`

**Step 1: Translate Mermaid Master Flowchart and Legend in `README.md` to English and purge em dashes**

Update the Mermaid `flowchart LR` diagram (Section 3) and ISO 5807 Symbol Legend (Section 4) in `README.md` so all subgraph titles, node labels, and branch labels are in crisp technical English (`Yes` / `No`, accurate firmware constants, zero em dashes `—` across `README.md`).

**Step 2: Run automated audit script and `antislop` Delivery Gate**

Run: `python tools/test_flowchart.py`
Expected: PASS on all checks.
