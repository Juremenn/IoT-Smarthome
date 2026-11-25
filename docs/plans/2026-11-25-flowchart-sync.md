# Flowchart Sync (Website & IoT) Implementation Plan

> **For Antigravity:** REQUIRED SUB-SKILL: Load executing-plans to implement this plan task-by-task.

**Goal:** Synchronize the Web Dashboard (`web_src/`), ESP32 embedded web headers (`src/web/`), and local mock server (`tools/dev_server.py`) with `Flowchart_iot_final.jpg` while keeping existing extra firmware features (Fan `D23`, Touch `D4`).

**Architecture:** Use `d.maxDistance` from ESP32 `/api/status` (`cfg::MAX_DISTANCE_CM = 10` in `include/config.h`) as the single source of truth for the ultrasonic trigger threshold, fix global fire evacuation status in `web_src/app.js`, add OLED status & connection state handling, and add the missing DHT11 monitoring flow + updated Fire/OLED evacuation flow in `web_src/index.html`.

**Tech Stack:** Vanilla HTML/CSS/JS (`web_src/`), Python 3 (`tools/dev_server.py`, `tools/sync_to_esp.py`, `tools/test_server.py`), PlatformIO / Arduino ESP32 (`src/`).

---

### Task 1: Update Local Mock Server Threshold (`tools/dev_server.py`)

**Files:**
- Modify: `tools/dev_server.py:49-53`
- Test: `tools/test_server.py`

**Step 1: Update `maxDistance` in `tools/dev_server.py`**

Change line 52 from `self.maxDistance = 15` to `self.maxDistance = 10` so the mock server matches `cfg::MAX_DISTANCE_CM = 10` in `include/config.h` and `Flowchart_iot_final.jpg`.

```python
        # Sensor Ultrasonik
        self.trigPin = 4
        self.echoPin = 5
        self.distance = 25
        self.maxDistance = 10
        self.rawEcho = 1450
```

**Step 2: Run server test to verify**

Start `tools/dev_server.py` in background or run `python tools/test_server.py` against it and verify `ALL DEV SERVER TESTS PASSED SUCCESSFULLY!`.

**Step 3: Commit**

```bash
git add tools/dev_server.py
git commit -m "fix(sim): align ultrasonic maxDistance to 10 cm per flowchart"
```

---

### Task 2: Fix Evacuation Logic, Dynamic Threshold, & OLED/Connection States in `web_src/app.js`

**Files:**
- Modify: `web_src/app.js:10-22, 53-112, 155-185, 224-240`

**Step 1: Update `CONFIG` and state in `web_src/app.js`**

- Change `CONFIG.ULTRASONIC_TRIGGER_CM` from `15` to `10`.
- Track `isOffline` state so `pollStatus()` updates the connection/OLED indicator cleanly on success and failure (R-27).

```javascript
  const CONFIG = {
    USE_SIMULATOR: true,
    ESP32_BASE_URL: window.location.origin || "http://127.0.0.1:8080",
    POLL_INTERVAL_MS: 1000,
    ULTRASONIC_TRIGGER_CM: 10
  };
```

**Step 2: Fix `isEvacuation` & dynamic `threshold` in `renderDashboard(d)`**

- Line 68: change `const isEvacuation = d.doorTriggerMode === 2 && d.flame;` to `const isEvacuation = Boolean(d.flame);` (global fire evacuation priority matching `src/automation.cpp:32`).
- Line 164: compute `const threshold = (typeof d.maxDistance === 'number' && d.maxDistance > 0) ? d.maxDistance : CONFIG.ULTRASONIC_TRIGGER_CM;`.
- Update OLED status text (`$('oledStatusText')` and `$('infoOledChip')`) from `d.oled` and `d.oledAddress`.
- Update Flow 3 conduits (`conduit3A`, `conduit3B`, `flowNodeDhtOut`) based on `d.tempValid`.

**Step 3: Update `pollStatus()` error handling**

In `catch (err)` of `pollStatus()`, update `$('oledStatusText')` to `Koneksi Terputus` (no `connPill` element, preserving `tools/test_server.py` assertion).

**Step 4: Commit**

```bash
git add web_src/app.js
git commit -m "fix(web): sync 10cm threshold, global fire evacuation, and OLED/DHT states"
```

---

### Task 3: Update HTML & CSS (`web_src/index.html`, `web_src/style.css`)

**Files:**
- Modify: `web_src/index.html:19-30, 124-126, 204-233, 340-357, 421-425`
- Modify: `web_src/style.css` (header status text & responsive alignment)

**Step 1: Update Header Bar & 10 cm labels in `web_src/index.html`**

- Add a clean text status indicator in `.header-bar` (without `connPill` class/id) showing `OLED: Memuat...` (`id="oledStatusText"`).
- Update `triggerDetailText` (line 125) from `Pemicu &le; 15 cm` to `Pemicu &le; 10 cm`.
- Update Flow 1 node (line 231) from `&le; 15 cm` to `&le; 10 cm`.
- Update Pinout Card 3 (line 422) from `Ambang &le; 15 cm` to `Ambang &le; 10 cm`.

**Step 2: Update Flow 2 & Add Flow 3 (DHT11) in `web_src/index.html`**

- In Flow 2 (`Evakuasi Bahaya Api`), update `flowNodeEvac` description from `Pintu Buka Penuh` to `Pintu + Alarm + OLED`.
- Add Flow 3 (`Pemantauan Suhu & Kelembaban`) after Flow 2 inside `.automation-flows`:
  - Node 1: `Sensor DHT11` (`D27`)
  - Conduit `conduit3A`
  - Node 2: `Validasi Data` (`>= 2.1s`)
  - Conduit `conduit3B`
  - Node 3 (`flowNodeDhtOut`): `OLED & Web` (`Suhu & Lembab`)

**Step 3: Add minimal CSS in `web_src/style.css`**

Style `.header-status` (plain muted text, `font-size: 12px`, high contrast `#57534E` on `#F5F5F4`, >= 4.5:1) and mobile stacking under `@media (max-width: 640px)`.

**Step 4: Commit**

```bash
git add web_src/index.html web_src/style.css
git commit -m "feat(web): add DHT11 flow, OLED status indicator, and 10cm labels"
```

---

### Task 4: Sync Embedded Web Headers & Verify Full System

**Files:**
- Modify (auto-generated): `src/web/index_html.h`, `src/web/app_js.h`, `src/web/style_css.h`

**Step 1: Sync `web_src/` to `src/web/`**

Run: `python tools/sync_to_esp.py`
Expected: `Synced: web_src/index.html -> src/web/index_html.h`, `Synced: web_src/style.css -> src/web/style_css.h`, `Synced: web_src/app.js -> src/web/app_js.h`.

**Step 2: Verify ESP32 firmware build**

Run: `pio run`
Expected: `SUCCESS` with 0 errors.

**Step 3: Verify mock server & tests**

Run `python tools/test_server.py` against `tools/dev_server.py` and verify all assertions pass (`ALL DEV SERVER TESTS PASSED SUCCESSFULLY!`).

**Step 4: Commit**

```bash
git add src/web/index_html.h src/web/app_js.h src/web/style_css.h
git commit -m "chore(esp32): sync updated web_src assets into PROGMEM headers"
```
