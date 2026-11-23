# Local Mock Server & Web Redesign Implementation Plan

> **For Antigravity:** REQUIRED SUB-SKILL: Load executing-plans to implement this plan task-by-task.

**Goal:** Create a lightweight Python-based local mock server and live simulation environment for the Smart Home Miniatur Rumah Joglo project, allowing full UI/UX redesign and interactive testing without a physical ESP32.

**Architecture:** Web assets are extracted from C++ headers into a clean `web_src/` directory. A zero-dependency Python mock server (`tools/dev_server.py`) serves these static files and simulates the ESP32 REST API while injecting an interactive hardware simulation drawer into the browser. A synchronization tool (`tools/sync_to_esp.py`) converts `web_src/` back into PROGMEM C++ header files whenever ready to flash.

**Tech Stack:** Python 3.15 (`http.server`, `urllib`, `threading`), HTML5, CSS3, Vanilla JavaScript (ES6+), Arduino/C++ (PlatformIO).

---

### Task 1: Extract Web Assets from C++ Headers into `web_src/`

**Files:**
- Create: `web_src/index.html`
- Create: `web_src/style.css`
- Create: `web_src/app.js`
- Reference: `src/web/index_html.h`, `src/web/style_css.h`, `src/web/app_js.h`

**Step 1: Write an extraction script to accurately pull raw literals**
Run python one-liner or extraction script to parse the `R"rawliteral(...)rawliteral"` contents from header files into `web_src/`.

**Step 2: Verify extracted files**
Confirm `web_src/index.html`, `web_src/style.css`, and `web_src/app.js` contain valid HTML/CSS/JS without C++ wrappers.

**Step 3: Commit extracted files**
```bash
git add web_src/
git commit -m "feat(web): extract web assets to standalone web_src directory"
```

---

### Task 2: Implement Firmware Synchronizer (`tools/sync_to_esp.py`)

**Files:**
- Create: `tools/sync_to_esp.py`
- Modify: `src/web/index_html.h`, `src/web/style_css.h`, `src/web/app_js.h`

**Step 1: Write `tools/sync_to_esp.py`**
Implement the script to read `web_src/index.html`, `web_src/style.css`, and `web_src/app.js` and serialize them into C++ header files with exact `PROGMEM` syntax:
```python
# tools/sync_to_esp.py
from pathlib import Path
...
```

**Step 2: Test round-trip verification**
Run `python tools/sync_to_esp.py` and verify `git diff src/web/` produces no unintended changes against original headers.

**Step 3: Commit synchronizer**
```bash
git add tools/sync_to_esp.py
git commit -m "feat(tools): add sync_to_esp script for PROGMEM header generation"
```

---

### Task 3: Implement Python Mock Server & State Simulator (`tools/dev_server.py`)

**Files:**
- Create: `tools/dev_server.py`

**Step 1: Implement ESP32 state machine and HTTP handler**
- Simulate states: door phases (`TERTUTUP`, `AUTO_CYCLE`, `TERBUKA`), fan state, flame state, temperature, humidity, ultrasonic distance, touch values.
- Handle `/api/status`, `/api/door`, `/api/fan`, `/api/flame`, `/api/ultrasonic`.
- Handle `/api/simulator/update` (POST) to allow manual hardware parameter changes.
- In `GET /`, inject a docked floating UI drawer before `</body>` with sliders and buttons for real-time sensor control.

**Step 2: Add audio fallback endpoint**
Serve a synthetic audio response or dummy MP3 header on `GET /alarm.mp3`.

**Step 3: Commit mock dev server**
```bash
git add tools/dev_server.py
git commit -m "feat(tools): add python mock server with live simulator drawer"
```

---

### Task 4: Automated Verification of Mock Server Endpoints

**Files:**
- Create: `tools/test_server.py` (temporary test client)

**Step 1: Write test script to verify all REST endpoints**
Write a quick Python test script to query `http://127.0.0.1:8080/api/status`, toggle fan, trigger door auto-cycle, and check simulated values.

**Step 2: Execute verification test**
Run: `python tools/test_server.py`
Expected: All tests pass with HTTP 200 and expected JSON payload.

**Step 3: Clean up test script & commit**
```bash
git commit -m "test: verify dev server endpoints and simulator behavior"
```

---

### Task 5: Launch Live Server & Provide Redesign Instructions

**Files:**
- Modify: `README.md` (add local dev server section)

**Step 1: Document local dev commands**
Add a short note in `README.md` explaining how to start the mock server and sync back to ESP32.

**Step 2: Start server on background port**
Start `python tools/dev_server.py` and verify it is accessible at `http://localhost:8080`.

**Step 3: Guide user on live redesign**
Show user how modifying `web_src/style.css` and `web_src/index.html` updates the live dashboard in browser.
