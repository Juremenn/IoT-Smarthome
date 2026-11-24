# Animated Pulse Packet Flow & Minimalist Text Implementation Plan

> **For Antigravity:** REQUIRED SUB-SKILL: Load executing-plans to implement this plan task-by-task.

**Goal:** Implement 9Router-inspired animated pulse packet conduits between automation flow nodes and remove narrative paragraphs across the dashboard for a crisp, high-density light-theme UI.

**Architecture:** Replace static arrow connectors with responsive SVG conduit tracks and animated traveling pulse packet dots powered by hardware-accelerated CSS keyframes. Clean up text elements in both HTML and JavaScript to display succinct telemetry labels and spec badges without explanatory narrative paragraphs.

**Tech Stack:** Vanilla HTML5, CSS3 Keyframes & SVG transforms, JavaScript (ES6+ HAL), Python (sync & test tools).

---

### Task 1: Overhaul Connector Markup and Strip Narrative Paragraphs in `web_src/index.html`

**Files:**
- Modify: `web_src/index.html`

**Step 1: Update flow connectors and remove text clutter**
- Replace static `.flow-connector` chevron icons in Flow 1 and Flow 2 with `.flow-conduit` elements containing SVG track paths and animated pulse dots.
- Strip `<p class="hero-desc">` from the top hero card.
- Strip all four `<div class="pinout-card-body"><p>...</p></div>` narrative blocks from the sensor grid cards.
- Simplify sub-labels in View 1 (Door, Flame, Ultrasonic, Distance).

**Step 2: Verify HTML syntax and zero em dashes**
Run: `python -c "content = open('web_src/index.html', encoding='utf-8').read(); assert '—' not in content, 'em-dash found'; print('HTML valid and clean.')"`
Expected: `HTML valid and clean.`

**Step 3: Commit**
```bash
git add web_src/index.html
git commit -m "feat(ui): add animated flow conduit markup and strip narrative text"
```

---

### Task 2: Implement Animated Pulse Packet CSS Keyframes in `web_src/style.css`

**Files:**
- Modify: `web_src/style.css`

**Step 1: Add conduit and pulse keyframe styles**
- Style `.flow-conduit` as a flexible, centered bridge connecting adjacent `.flow-node` elements.
- Create `@keyframes packetFlow`: animate the traveling signal packet smoothly from 0% to 100% horizontally.
- Define speed and color variants:
  - Standard polling (Celadon/Gold, 2s duration, continuous glide).
  - Ultrasonic triggered active state (Terracotta `#A84A28`, 0.75s duration with subtle node ping).
  - Flame danger active state (Crimson `#DC2626`, 0.6s rapid loop).
- Clean up `.pinout-card` layout: remove body paragraph spacing, tighten padding to `16px 18px`, and format spec chips cleanly under the header.

**Step 2: Verify CSS valid and zero em dashes**
Run: `python -c "content = open('web_src/style.css', encoding='utf-8').read(); assert '—' not in content, 'em-dash found'; print('CSS valid and clean.')"`
Expected: `CSS valid and clean.`

**Step 3: Commit**
```bash
git add web_src/style.css
git commit -m "style: implement 9router animated pulse packet keyframes"
```

---

### Task 3: Synchronize HAL Dynamic Labels and Trigger States in `web_src/app.js`

**Files:**
- Modify: `web_src/app.js`

**Step 1: Update dynamic rendering logic**
- Update `doorSubText` values to concise strings: `'Standby'`, `'Otomatis'`, `'Terbuka Terus'`, `'Bahaya Api'`.
- Update `flameSubText` values to `'Normal'` and `'Bahaya Api'`.
- Update `distSubLabel` values to `'Di Luar Jangkauan'`, `'Sangat Dekat'`, `'Mendekat'`, `'Sedang'`, `'Standby'`.
- Toggle active conduit animation classes on Flow 1 and Flow 2 based on live sensor trigger states.

**Step 2: Verify JS and test server execution**
Run: `python tools/test_server.py`
Expected: `ALL DEV SERVER TESTS PASSED SUCCESSFULLY!`

**Step 3: Commit**
```bash
git add web_src/app.js
git commit -m "feat(hal): sync compact status labels and dynamic trigger animations"
```

---

### Task 4: ESP32 Firmware Synchronization & Full Verification

**Files:**
- Modify: `src/web/index_html.h`
- Modify: `src/web/style_css.h`
- Modify: `src/web/app_js.h`

**Step 1: Synchronize Web Assets to ESP32 Flash Headers**
Run: `python tools/sync_to_esp.py`
Expected: All headers successfully generated and synced.

**Step 2: Run End-to-End Server Tests**
Run: `python tools/test_server.py`
Expected: `ALL DEV SERVER TESTS PASSED SUCCESSFULLY!`

**Step 3: Commit**
```bash
git add src/web/
git commit -m "chore(firmware): sync animated flow and compact layout headers"
```
