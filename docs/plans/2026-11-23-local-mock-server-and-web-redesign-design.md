# Design Document: Local Mock Server & Web Redesign Workflow

**Date:** 2026-11-23  
**Status:** Approved  
**Topic:** IoT Smart Home (Miniatur Rumah Joglo) - Local Development Environment & Web Redesign  

---

## 1. Context & Objective
The IoT Smart Home project runs on an ESP32 controlling multiple peripherals:
- Servo door (smooth easing, auto-cycle, manual control, fire evacuation)
- 5-Way flame sensor (D34)
- Touch sensor (D4)
- Ultrasonic distance sensor HC-SR04 (Trig D4, Echo D5)
- Temperature & Humidity DHT11 (D27)
- Fan relay (D23, Active-LOW)
- SSD1306 OLED display (I2C)

Currently, the web interface is stored directly inside C++ header files (`src/web/index_html.h`, `src/web/style_css.h`, `src/web/app_js.h`) as raw string literals. The user is currently away from the physical ESP32 board and wants to run the web interface locally/live to redesign its UI/UX, while still being able to test interactive sensor triggers and control states.

---

## 2. Architecture & Components

### 2.1 File Structure
```
IoT-Smarthome/
├── web_src/                      # Clean source files for web development & redesign
│   ├── index.html                # Main HTML markup
│   ├── style.css                 # CSS styles, variables, typography, layout
│   └── app.js                    # Client-side JavaScript controller & polling
├── tools/
│   ├── dev_server.py             # Python HTTP mock server + simulation engine
│   └── sync_to_esp.py            # Converter: web_src/ -> src/web/*.h (PROGMEM)
└── src/web/                      # Firmware header files (generated/synced)
    ├── index_html.h
    ├── style_css.h
    ├── app_js.h
    └── alarm_mp3.h
```

### 2.2 Python Mock Server (`tools/dev_server.py`)
- Built using Python's standard `http.server` (zero extra dependencies).
- Maintains live state matching the ESP32 firmware:
  - `door`: `TERTUTUP`, `AUTO_CYCLE`, `TERBUKA`, `EVAKUASI` (with asynchronous movement simulation)
  - `fan`: Boolean state (`true`/`false`), active logic
  - `flame`: Flame detected (`true`/`false`), raw value, active logic
  - `touch`: Raw, baseline, touchDetected
  - `distance`: Simulated distance in cm
  - `temp`: Temperature in °C
  - `humidity`: Humidity in %
  - `uptime`, `ip`: Simulated network attributes
- Endpoints handled:
  - `GET /` -> Serves `web_src/index.html` with injected simulator drawer.
  - `GET /style.css` -> Serves `web_src/style.css`.
  - `GET /app.js` -> Serves `web_src/app.js`.
  - `GET /alarm.mp3` -> Synthesized fallback / dummy mp3 response.
  - `GET /api/status` -> Returns JSON identical to ESP32 firmware status schema.
  - `GET /api/door` -> Handles `action=auto|open|close|set_mode`.
  - `GET /api/fan` -> Handles `toggle=1|state=on/off|toggle_logic=1`.
  - `GET /api/flame` -> Handles `toggle_logic=1`.
  - `GET /api/ultrasonic` -> Handles `swap=1|scan=1`.
  - `POST /api/simulator/update` -> Updates simulated sensor parameters from the simulator drawer.

### 2.3 Zero-Impact Simulator UI Drawer
- On serving `GET /`, the dev server injects a compact floating widget (docked bottom-right) containing:
  - Temperature slider (0 - 50 °C) & Humidity slider (20 - 90 %).
  - Fire alarm trigger button (simulates fire hazard).
  - Touch sensor pulse button (simulates touching D4).
  - Distance slider (0 - 30 cm) to test ultrasonic auto-opening.
  - Real-time readout of door status and fan relay status.
- `web_src/index.html` remains clean and ready for direct ESP32 deployment.

### 2.4 Synchronization Tool (`tools/sync_to_esp.py`)
- Reads `web_src/index.html`, `web_src/style.css`, and `web_src/app.js`.
- Writes formatted C++ PROGMEM headers to `src/web/index_html.h`, `src/web/style_css.h`, and `src/web/app_js.h`.
- Verifies syntax integrity to ensure PlatformIO compiles without errors.

---

## 3. Implementation Steps
1. Extract current web assets from `src/web/*.h` into `web_src/`.
2. Implement `tools/dev_server.py` with mock REST API and simulator drawer injection.
3. Implement `tools/sync_to_esp.py` to keep firmware headers in sync.
4. Verify server startup, API behavior, and live web access in browser (`http://localhost:8080`).
5. Enable user to modify `web_src/style.css` and `web_src/index.html` freely with immediate feedback.
