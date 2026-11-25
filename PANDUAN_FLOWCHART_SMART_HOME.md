# Complete ISO 5807 Flowchart Guide: Rumah Joglo Miniature Smart Home IoT (ESP32)

This document specifies the monochrome **ISO 5807** engineering flowcharts for the ESP32 firmware (`src/*.cpp`, `include/config.h`) and Web Dashboard (`web_src/*`) of the Rumah Joglo Miniature Smart Home IoT system. All five diagrams follow a sequential layout from `Start` to `End` with strict binary decision diamonds (at most one outgoing flowline per diamond vertex).

System behavior covered across all diagrams:
- Dual door servo motion on `GPIO 13` and `GPIO 12`: opens to `0°` (`5000 ms` `easeInOutSine` curve), holds open for `2000 ms`, and soft-closes to `180°` (`7000 ms` `easeSoftClose` curve).
- Global fire evacuation priority on `GPIO 34`: forces the door open to `0°`, locks the web Close command, flashes the OLED display, and plays `/alarm.mp3` until the flame clears for `1200 ms`.
- Fan relay on `GPIO 23`: initialized safely OFF at boot; web dashboard focuses on door control, fire monitoring, ultrasonic distance gauge, and DHT11 telemetry across the **Devices** and **Pinout Info** tabs.

Every node ID, ISO 5807 symbol, text label, and branch transition in this guide matches the 5-tab Draw.io file [`IotSmarthome.drawio`](IotSmarthome.drawio) and plain-text specification [`ALUR_FLOWCHART_SMART_HOME.txt`](ALUR_FLOWCHART_SMART_HOME.txt).

---

## Table of Contents

1. [ISO 5807 Standard Symbol Dictionary (Monochrome)](#1-iso-5807-standard-symbol-dictionary-monochrome)
2. [Diagram 1: Complete System Flowchart (Sequential 4-Stage Pipeline)](#2-diagram-1-complete-system-flowchart-sequential-4-stage-pipeline)
3. [Diagram 2: Subsystem 1 - Fire Detection & Emergency Evacuation](#3-diagram-2-subsystem-1---fire-detection--emergency-evacuation)
4. [Diagram 3: Subsystem 2 - Door Servo Control (Open & Soft-Close)](#4-diagram-3-subsystem-2---door-servo-control-open--soft-close)
5. [Diagram 4: Subsystem 3 - DHT11 Telemetry & Hot-Plug OLED Display](#5-diagram-4-subsystem-3---dht11-telemetry--hot-plug-oled-display)
6. [Diagram 5: Subsystem 4 - Web Dashboard Workflow](#6-diagram-5-subsystem-4---web-dashboard-workflow)
7. [Quick Reference: GPIO Pinout & Firmware Parameters](#7-quick-reference-gpio-pinout--firmware-parameters)

---

## 1. ISO 5807 Standard Symbol Dictionary (Monochrome)

All diagrams use formal academic and engineering monochrome styling: white fill (`#FFFFFF`), black stroke (`#000000`, `1.5 px`), black `Arial` typography (`11 pt` node text, `10 pt` bold branch labels), and orthogonal black flowlines (`1.4 px`) for maximum print and screen contrast (21:1).

| No | ISO 5807 Symbol | Draw.io Shape | Visual Form | Purpose in System Flowcharts | Example Label |
| :-: | :--- | :--- | :--- | :--- | :--- |
| 1 | **Terminator** | `mxgraph.flowchart.terminator` | Rounded capsule / oval | Marks the entry point (`Start`) and exit point (`End`) of each diagram | `Start (ESP32 Power ON)`, `End (System Halted)` |
| 2 | **Preparation** | `hexagon` | Six-sided hexagon | Hardware GPIO setup, PWM timer allocation, I2C bus setup, Wi-Fi AP startup, and HTTP server registration | `Initialize ESP32 Hardware...` |
| 3 | **Input / Output** | `parallelogram` | Slanted parallelogram | Physical sensor sampling (`D34`, `D4/D5`, `D27`), servo PWM output (`D13/D12`), and HTTP GET/JSON packets (`/api/*`) | `Read Digital Signal from Flame Sensor D34 (Every 5 ms)` |
| 4 | **Display** | `display` | Bullet / curved display | Visual telemetry rendered on the 128x64 SSD1306 OLED screen or Web Dashboard UI | `Flash OLED "! FIRE WARNING !" & Play Web Alarm (/alarm.mp3)` |
| 5 | **Process** | `rounded=0` | Sharp-corner rectangle | Internal state mutations, anti-flicker filtering, distance math, and servo motion curves (`easeInOutSine` / `easeSoftClose`) | `Soft-Close Door Smoothly from 0° to 180° (7000 ms)` |
| 6 | **Decision** | `rhombus` | Diamond | Strict binary condition evaluation with two labeled exit ports (`Yes` / `No`); multi-option checks use cascaded diamonds | `Is Fire Detected?`, `Action == "Open 2s"?` |
| 7 | **Delay** | `delay` | D-shaped block | Explicit hold duration or polling wait interval | `Hold Door Open (2 Seconds)` |
| 8 | **Flowline** | `orthogonalEdgeStyle` | Straight orthogonal arrow | Execution direction from left to right and top to bottom without shared multi-branch ports | Labels: `Yes`, `No`, `No (Safe)` |

---

## 2. Diagram 1: Complete System Flowchart (Sequential 4-Stage Pipeline)

*(Matches **Tab 1: `1. Complete System Flowchart (Start-End)`** in `IotSmarthome.drawio`)*

The master diagram mirrors the single-threaded non-blocking execution of `setup()` and `loop()` in `src/main.cpp`:
- **Top Row (Boot & Initialization):** `M01` Start -> `M02` Hardware Init -> `M03` Wi-Fi AP & HTTP Server -> `M04` OLED Ready Splash -> `M05` Sensor Sampling & HTTP Polling.
- **Stage 1 (Column 1):** Fire Detection & Emergency Evacuation (Priority 1, `C1_1` to `C1_6`).
- **Stage 2 (Column 2):** Web Dashboard Controls using cascaded binary decision diamonds (`C2_1` to `C2_8`).
- **Stage 3 (Column 3):** Ultrasonic Automatic Door with 2-second hold and anti-pinch reversal (`C3_1` to `C3_6`).
- **Stage 4 (Column 4):** Non-blocking DHT11 telemetry (deferred during servo movement) and hot-plug SSD1306 OLED rendering (`C4_1` to `C4_6`).
- **Bottom Row (Output & Loop Check):** `M06` Servo PWM Output -> `M07` Web Dashboard Sync -> `M08` Power OFF Check -> `M09` End (or loop back to `M05`).

### Step-by-Step Blueprint Table

| Section | ID | ISO 5807 Symbol | Shape Text | Next Node / Branch |
| :--- | :--- | :--- | :--- | :--- |
| **Top Row** | `M01` | **Terminator** | `Start (ESP32 Power ON)` | -> `M02` |
| **Top Row** | `M02` | **Preparation** | `Initialize ESP32 Hardware: Servo D13/D12 (Closed 180°), Fan D23 (OFF), Flame D34, Ultrasonic D4/D5, DHT11 D27, OLED D21/D22` | -> `M03` |
| **Top Row** | `M03` | **Preparation** | `Start Standalone Wi-Fi AP ("RumahJoglo-SmartHome", 192.168.4.1) & HTTP Web Server (Port 80)` | -> `M04` |
| **Top Row** | `M04` | **Display** | `Display "System Ready" Status & IP Address on OLED Screen` | -> `M05` |
| **Top Row** | `M05` | **Input / Output** | `Sample All Sensors (Flame D34, HC-SR04, DHT11 D27) & Poll HTTP Requests (/api/*)` | -> `C1_1` (Stage 1) |
| **Stage 1 (Fire)** | `C1_1` | **Decision** | `Flame Sensor (D34) Detects Fire? (Filter >= 15 ms)` | **Yes** -> `C1_2`<br>**No (Safe)** -> `C2_1` |
| **Stage 1 (Fire)** | `C1_2` | **Process** | `Emergency Evacuation Mode: Force Door Open to 0° & Lock Web "Close" Button` | -> `C1_3` |
| **Stage 1 (Fire)** | `C1_3` | **Input / Output** | `Drive Servo PWM (D13/D12) Open Immediately to 0°` | -> `C1_4` |
| **Stage 1 (Fire)** | `C1_4` | **Display** | `Flash OLED "! FIRE WARNING !" & Play Web Alarm (/alarm.mp3)` | -> `C1_5` |
| **Stage 1 (Fire)** | `C1_5` | **Decision** | `Fire Extinguished & Quiet >= 1.2 Seconds?` | **No (Fire Active)** -> `C1_3`<br>**Yes** -> `C1_6` |
| **Stage 1 (Fire)** | `C1_6` | **Process** | `Clear Evacuation, Stop Alarm, & Set Auto-Close Timer (+2 Seconds)` | -> `C2_1` (Stage 2) |
| **Stage 2 (Web)** | `C2_1` | **Decision** | `Web Button Pressed on Dashboard?` | **Yes** -> `C2_2`<br>**No** -> `C3_1` |
| **Stage 2 (Web)** | `C2_2` | **Decision** | `Action == "Open 2s"?` | **Yes** -> `C2_5`<br>**No** -> `C2_3` |
| **Stage 2 (Web)** | `C2_3` | **Decision** | `Action == "Keep Open"?` | **Yes** -> `C2_6`<br>**No** -> `C2_4` |
| **Stage 2 (Web)** | `C2_4` | **Decision** | `Action == "Close"?` | **Yes** -> `C2_7`<br>**No (Polarity)** -> `C2_8` |
| **Stage 2 (Web)** | `C2_5` | **Process** | `Open 2 Seconds: Open to 0°, Hold 2 s, Soft-Close to 180°` | -> `C3_1` (Stage 3) |
| **Stage 2 (Web)** | `C2_6` | **Process** | `Keep Open: Open to 0° & Latch (Ignore Ultrasonic Sensor)` | -> `C3_1` (Stage 3) |
| **Stage 2 (Web)** | `C2_7` | **Process** | `Close (If No Fire Active): Soft-Close Door to 180°` | -> `C3_1` (Stage 3) |
| **Stage 2 (Web)** | `C2_8` | **Process** | `Toggle Flame Polarity: Switch Active-HIGH / Active-LOW` | -> `C3_1` (Stage 3) |
| **Stage 3 (Auto)** | `C3_1` | **Decision** | `Door in "Keep Open" Mode or Evacuating?` | **Yes (Bypass)** -> `C4_1`<br>**No** -> `C3_2` |
| **Stage 3 (Auto)** | `C3_2` | **Input / Output** | `Calculate HC-SR04 Distance (D4/D5) = (Duration * 0.034) / 2 cm` | -> `C3_3` |
| **Stage 3 (Auto)** | `C3_3` | **Decision** | `Object Detected? (Distance <= Trigger Limit)` | **Yes** -> `C3_4`<br>**No** -> `C3_5` |
| **Stage 3 (Auto)** | `C3_4` | **Process** | `Open Door to 0° / Extend 2 s Hold Timer / Anti-Pinch Reverse Open` | -> `C4_1` (Stage 4) |
| **Stage 3 (Auto)** | `C3_5` | **Decision** | `Door Auto-Open & 2-Second Hold Expired?` | **Yes** -> `C3_6`<br>**No** -> `C4_1` |
| **Stage 3 (Auto)** | `C3_6` | **Process** | `Soft-Close Door Smoothly to 180° (7000 ms Curve)` | -> `C4_1` (Stage 4) |
| **Stage 4 (DHT/OLED)** | `C4_1` | **Decision** | `Door Stationary & DHT11 Interval >= 2.1 s?` | **Yes** -> `C4_2`<br>**No (Defer)** -> `C4_6` |
| **Stage 4 (DHT/OLED)** | `C4_2` | **Input / Output** | `Read Temperature (°C) & Humidity (%) from DHT11 Sensor (Pin D27)` | -> `C4_3` |
| **Stage 4 (DHT/OLED)** | `C4_3` | **Decision** | `DHT11 Data Valid? (-10..80 °C & 0..100%)` | **Yes** -> `C4_4`<br>**No** -> `C4_5` |
| **Stage 4 (DHT/OLED)** | `C4_4` | **Process** | `Store New Temp & Humidity (Reset Failure Counter)` | -> `C4_6` |
| **Stage 4 (DHT/OLED)** | `C4_5` | **Process** | `Retain Last Valid Reading (Max 5 Consecutive Failures)` | -> `C4_6` |
| **Stage 4 (DHT/OLED)** | `C4_6` | **Display** | `SSD1306 OLED (D21/D22 Hot-Plug): Render IP, Wayang, Temp, & FIRE: SAFE` | -> `M06` (Bottom Row) |
| **Bottom Row** | `M06` | **Input / Output** | `Send Door Servo PWM Signal (D13 & D12: 50 Hz, Open 0° / Soft-Close 180°)` | -> `M07` |
| **Bottom Row** | `M07` | **Display** | `Update Web Dashboard (Devices Tab: 5 Cards & Distance Gauge, Pinout Info Tab)` | -> `M08` |
| **Bottom Row** | `M08` | **Decision** | `Is System Powered OFF?` | **No (Continue Loop)** -> `M05`<br>**Yes** -> `M09` |
| **Bottom Row** | `M09` | **Terminator** | `End (System Halted)` | *(Done)* |

---

## 3. Diagram 2: Subsystem 1 - Fire Detection & Emergency Evacuation

*(Matches **Tab 2: `2. Fire Detection & Evacuation`** in `IotSmarthome.drawio`)*

| No | ID | ISO 5807 Symbol | Shape Text | Next Node / Branch |
| :-: | :--- | :--- | :--- | :--- |
| 1 | `A01` | **Terminator** | `Start` | -> `A02` |
| 2 | `A02` | **Preparation** | `Initialize Flame Sensor D34 (Default Active-HIGH)` | -> `A03` |
| 3 | `A03` | **Decision** | `Toggle Polarity via Web?` | **Yes** -> `A03A`<br>**No** -> `A04` |
| 4 | `A03A` | **Process** | `Invert Flame Polarity (Active-HIGH / Active-LOW)` | -> `A04` |
| 5 | `A04` | **Input / Output** | `Read Digital Signal from Flame Sensor D34 (Every 5 ms)` | -> `A05` |
| 6 | `A05` | **Process** | `Apply Anti-Flicker Filter (Active >= 15 ms / Streak >= 2)` | -> `A06` |
| 7 | `A06` | **Decision** | `Is Fire Detected?` | **Yes (Emergency)** -> `A07`<br>**No (Safe)** -> `A12` |
| 8 | `A07` | **Process** | `Emergency Evacuation Mode: Force Door Open to 0° & Lock Web "Close" Button` | -> `A08` |
| 9 | `A08` | **Input / Output** | `Drive Servo PWM (D13/D12) Open Immediately to 0°` | -> `A09` |
| 10 | `A09` | **Display** | `Flash OLED "! FIRE WARNING !" & Play Web Alarm (/alarm.mp3)` | -> `A10` |
| 11 | `A10` | **Decision** | `Fire Extinguished & Quiet >= 1.2 Seconds?` | **No (Fire Active)** -> `A04`<br>**Yes** -> `A11` |
| 12 | `A11` | **Process** | `Clear Evacuation State, Stop Alarm, & Set Auto-Close Timer (+2 Seconds)` | -> `A13` |
| 13 | `A12` | **Display** | `Display "Safe / Normal" Status on OLED & Web Dashboard` | -> `A13` |
| 14 | `A13` | **Terminator** | `End` | *(Done)* |

---

## 4. Diagram 3: Subsystem 2 - Door Servo Control (Open & Soft-Close)

*(Matches **Tab 3: `3. Door Servo Control`** in `IotSmarthome.drawio`)*

| No | ID | ISO 5807 Symbol | Shape Text | Next Node / Branch |
| :-: | :--- | :--- | :--- | :--- |
| 1 | `P01` | **Terminator** | `Start` | -> `P02` |
| 2 | `P02` | **Preparation** | `Initialize Servo D13/D12 (Closed 180°) & Ultrasonic HC-SR04 (D4/D5)` | -> `P03A` |
| 3 | `P03A` | **Decision** | `Command == "Keep Open"?` | **Yes** -> `P04A`<br>**No** -> `P03B` |
| 4 | `P04A` | **Input / Output** | `Web Button: "Keep Open"` | -> `P05A` |
| 5 | `P05A` | **Process** | `Open to 0° & Hold Open Continuously` | -> `P11` |
| 6 | `P03B` | **Decision** | `Command == "Open 2s"?` | **Yes** -> `P04B`<br>**No** -> `P03C` |
| 7 | `P04B` | **Input / Output** | `Web Button: "Open 2 Seconds"` | -> `P06` |
| 8 | `P03C` | **Decision** | `Trigger == Ultrasonic?` | **Yes** -> `P04C`<br>**No (Close)** -> `P04D` |
| 9 | `P04C` | **Input / Output** | `Ultrasonic Sensor (D4/D5): Measure Distance (cm)` | -> `P05C` |
| 10 | `P05C` | **Decision** | `Distance > 0 cm & <= Trigger Threshold?` | **Yes** -> `P06`<br>**No** -> `P12` |
| 11 | `P04D` | **Input / Output** | `Web Button: "Close" (If No Fire)` | -> `P08` |
| 12 | `P06` | **Input / Output** | `Drive Servo (D13/D12) Open to 0° (5000 ms Sine Curve)` | -> `P07` |
| 13 | `P07` | **Delay** | `Hold Door Open (2 Seconds) (Extend While Object Present)` | -> `P08` |
| 14 | `P08` | **Process** | `Soft-Close Door Smoothly from 0° to 180° (7000 ms)` | -> `P09` |
| 15 | `P09` | **Decision** | `Object Detected While Closing?` | **Yes (Anti-Pinch Reverse)** -> `P06`<br>**No** -> `P10` |
| 16 | `P10` | **Input / Output** | `Send Smooth PWM Until Door Fully Closed at 180°` | -> `P11` |
| 17 | `P11` | **Display** | `Update Door Status on Web Dashboard (Closed / Open / Auto Cycle)` | -> `P12` |
| 18 | `P12` | **Terminator** | `End` | *(Done)* |

---

## 5. Diagram 4: Subsystem 3 - DHT11 Telemetry & Hot-Plug OLED Display

*(Matches **Tab 4: `4. DHT11 & Hot-Plug OLED`** in `IotSmarthome.drawio`)*

| No | ID | ISO 5807 Symbol | Shape Text | Next Node / Branch |
| :-: | :--- | :--- | :--- | :--- |
| 1 | `D01` | **Terminator** | `Start` | -> `D02` |
| 2 | `D02` | **Preparation** | `Initialize DHT11 (D27) & OLED I2C Bus (D21/D22)` | -> `D03` |
| 3 | `D03` | **Decision** | `Door Stationary & DHT11 Interval >= 2.1 s?` | **Yes** -> `D04`<br>**No (Defer)** -> `D07` |
| 4 | `D04` | **Input / Output** | `Read Temperature (°C) & Humidity (%) from DHT11 (Pin D27)` | -> `D05` |
| 5 | `D05` | **Decision** | `Data Valid (-10..80 °C & 0..100%)?` | **Yes** -> `D06A`<br>**No** -> `D06B` |
| 6 | `D06A` | **Process** | `Store New Temp & Humidity (Reset Failure Counter)` | -> `D07` |
| 7 | `D06B` | **Process** | `Retain Last Valid Reading (Max 5 Consecutive Failures)` | -> `D07` |
| 8 | `D07` | **Decision** | `OLED Display Active (0x3C / 0x3D)?` | **Yes** -> `D08A`<br>**No** -> `D08B` |
| 9 | `D08A` | **Display** | `Render IP, Wayang Animation, Temp °C, Humidity %, & "FIRE: SAFE" on OLED` | -> `D09` |
| 10 | `D08B` | **Process** | `Auto Re-Scan I2C Every 1500 ms (Hot-Plug Recovery)` | -> `D09` |
| 11 | `D09` | **Display** | `Update "Room Temperature" Card & Progress Bar on Web Dashboard` | -> `D10` |
| 12 | `D10` | **Terminator** | `End` | *(Done)* |

---

## 6. Diagram 5: Subsystem 4 - Web Dashboard Workflow

*(Matches **Tab 5: `5. Web Dashboard Workflow`** in `IotSmarthome.drawio`)*

| No | ID | ISO 5807 Symbol | Shape Text | Next Node / Branch |
| :-: | :--- | :--- | :--- | :--- |
| 1 | `W01` | **Terminator** | `Start` | -> `W02` |
| 2 | `W02` | **Preparation** | `Connect Wi-Fi "RumahJoglo-SmartHome" & Open http://192.168.4.1` | -> `W03` |
| 3 | `W03` | **Display** | `Load Dashboard Page & Unlock Browser Audio Context` | -> `W04` |
| 4 | `W04` | **Input / Output** | `Send GET /api/status to ESP32 (Poll Every 1000 ms)` | -> `W05` |
| 5 | `W05` | **Decision** | `Selected Navigation Tab?` | **Devices** -> `W06A`<br>**Pinout Info** -> `W06B` |
| 6 | `W06A` | **Display** | `"Devices" Tab (5 Cards): Door, Flame Sensor, Door Trigger, Obstacle Distance Gauge, & Room Temp` | -> `W07` |
| 7 | `W06B` | **Display** | `"Pinout Info" Tab: Automation & Fire Evacuation Flow Diagram + GPIO Specifications` | -> `W07` |
| 8 | `W07` | **Decision** | `Flame Status == TRUE (Emergency)?` | **Yes** -> `W07A`<br>**No** -> `W08` |
| 9 | `W07A` | **Display** | `Red Alert "FIRE HAZARD!" & Play Alarm (/alarm.mp3)` | -> `W08` |
| 10 | `W08` | **Decision** | `User Clicked Control Button (Door / Flame Polarity)?` | **Yes** -> `W08A`<br>**No** -> `W09` |
| 11 | `W08A` | **Input / Output** | `Send GET (/api/door or /api/flame) & Receive Updated JSON` | -> `W09` |
| 12 | `W09` | **Delay** | `Wait Polling Interval (1000 ms)` | -> `W10` |
| 13 | `W10` | **Decision** | `Close Dashboard Page?` | **No** -> `W04`<br>**Yes** -> `W11` |
| 14 | `W11` | **Terminator** | `End` | *(Done)* |

---

## 7. Quick Reference: GPIO Pinout & Firmware Parameters

| Component / Dashboard Card | GPIO Pin | Firmware Constants & Behavior (`include/config.h`) |
| :--- | :--- | :--- |
| **Door Servo (Primary & Parallel)** | `GPIO 13` & `GPIO 12` | PWM `50 Hz` (`500` to `2400 us`, `20 ms` tick), closed `180°`, open `0°`, open duration `5000 ms` (`easeInOutSine`), hold `2000 ms`, soft-close `7000 ms` (`easeSoftClose`); web controls: `Open 2s`, `Keep Open`, `Close` |
| **5-Way Flame Sensor (LM393 DO)** | `GPIO 34` (3.3 V) | Default `Active-HIGH` (runtime polarity toggle supported), sampling `5 ms` (`200 Hz`), anti-flicker confirmation `15 ms` (or streak `>= 2`), release hold `1200 ms`, global evacuation override + web audio `/alarm.mp3` |
| **Ultrasonic Distance Sensor (HC-SR04)** | `Trig D4`, `Echo D5` | `40 kHz` sonar, `10 us` trigger pulse, `40 ms` ping interval, `25000 us` echo timeout, trigger threshold `<= 10 cm` (`MAX_DISTANCE_CM`) |
| **Room Temperature & Humidity (DHT11)** | `GPIO 27` | Single-wire (`INPUT_PULLUP`), read interval `2100 ms` (deferred while door is opening or closing), valid bounds `-10..80 °C` and `0..100%`, retains last valid reading for up to `5` consecutive failures |
| **OLED Display (SSD1306 128x64)** | `SDA D21`, `SCL D22` | I2C address `0x3C` or `0x3D`, hot-plug re-scan every `1500 ms`, refresh `50 ms` (~20 FPS), renders IP, Wayang animation, temperature, humidity, and flashing fire alert |
| **Fan Relay (1-Channel)** | `GPIO 23` | Initialized safely OFF before `pinMode(..., OUTPUT)` (`Active-LOW` default) |
| **Standalone Wi-Fi AP & Web Server** | Port `80` | SSID `"RumahJoglo-SmartHome"`, IP `192.168.4.1`, **Devices** tab (5 cards) and **Pinout Info** tab |
