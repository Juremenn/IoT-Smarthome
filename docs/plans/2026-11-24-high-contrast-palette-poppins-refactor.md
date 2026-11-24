# High-Contrast Palette & 100% Poppins Typography Implementation Plan

> **For Antigravity:** REQUIRED SUB-SKILL: Load executing-plans to implement this plan task-by-task.

**Goal:** Overhaul the color palette and typography across the entire Rumah Joglo web app, replacing muddy beige-on-beige with a crisp, high-contrast light theme and replacing all monospace fonts with unified Poppins.

**Architecture:** Refactor CSS custom properties in `web_src/style.css` for a stone/white/neutral-gray design system with surgical terracotta and emergency red accents. Unify typography by removing Google Fonts `JetBrains Mono` and `Cormorant Garamond` from `web_src/index.html`, enforcing 100% `Poppins` with `font-variant-numeric: tabular-nums;`. Clean badge formatting and flow node card contrast, then synchronize firmware byte headers via `tools/sync_to_esp.py`.

**Tech Stack:** HTML5, CSS3 Custom Properties, Vanilla JavaScript (ES6+), Python 3 (Dev server & sync tooling), ESP32 C++ PROGMEM headers.

---

### Task 1: Update Google Fonts & Header Brand Typography in HTML

**Files:**
- Modify: `web_src/index.html:1-35`

**Step 1: Check existing font references in `web_src/index.html`**
Run: `python -c "content = open('web_src/index.html', encoding='utf-8').read(); print([l for l in content.splitlines() if 'fonts.googleapis' in l])"`
Expected: Link containing `Cormorant+Garamond`, `Poppins`, and `JetBrains+Mono`.

**Step 2: Update font link and header title in `web_src/index.html`**
- Replace font `<link>` with:
  `https://fonts.googleapis.com/css2?family=Poppins:wght@400;500;600;700;800&display=swap`
- Verify header title `<h1 class="brand-title">Rumah Joglo</h1>` is styled using Poppins.

**Step 3: Verify HTML font tags**
Run: `python -c "content = open('web_src/index.html', encoding='utf-8').read(); assert 'JetBrains' not in content; assert 'Cormorant' not in content; print('Font link clean!')"`
Expected: `Font link clean!`

**Step 4: Commit**
```bash
git add web_src/index.html
git commit -m "style: switch Google Fonts to Poppins only and eliminate monospace link"
```

---

### Task 2: Refactor Palette Tokens in `web_src/style.css`

**Files:**
- Modify: `web_src/style.css:1-55`

**Step 1: Replace CSS root variables in `web_src/style.css`**
- `--bg-canvas`: `#F5F5F4`
- `--bg-card`: `#FFFFFF`
- `--bg-surface-muted`: `#F3F4F6`
- `--border-subtle`: `#E5E7EB`
- `--border-subtle-hover`: `#D1D5DB`
- `--text-primary`: `#111111`
- `--text-secondary`: `#525252`
- `--text-muted`: `#737373`
- `--accent-terracotta`: `#A84A28`
- `--accent-terracotta-tint`: `#FDF4F0`
- `--status-danger`: `#DC2626`
- `--status-danger-tint`: `#FEF2F2`
- `--status-safe`: `#16A34A`
- `--status-safe-tint`: `#F0FDF4`
- Map legacy `--border-brass` variables to `--border-subtle` so no component retains yellowish borders.

**Step 2: Verify CSS variables load without syntax errors**
Run: `python -c "css = open('web_src/style.css', encoding='utf-8').read(); assert '#F5F5F4' in css; assert '#111111' in css; print('Palette tokens updated!')"`
Expected: `Palette tokens updated!`

**Step 3: Commit**
```bash
git add web_src/style.css
git commit -m "style: overhaul design system tokens to crisp light theme"
```

---

### Task 3: Unify Typography to 100% Poppins with Tabular Numbers

**Files:**
- Modify: `web_src/style.css:25-35, 240-270, 500-530, 800-830`

**Step 1: Remove all monospace font declarations in `web_src/style.css`**
- Set `--font-brand`, `--font-display`, `--font-sans` to `"Poppins", system-ui, -apple-system, sans-serif`.
- Remove `--font-mono`.
- Replace `font-family: var(--font-mono)` in `.telemetry-mono`, `.mono-token`, `.flow-node-token`, and pinout tables with:
  `font-family: var(--font-sans); font-variant-numeric: tabular-nums;`

**Step 2: Verify zero monospace references remain in `web_src/style.css`**
Run: `python -c "css = open('web_src/style.css', encoding='utf-8').read(); assert 'JetBrains' not in css; assert 'font-mono' not in css; print('Monospace completely removed!')"`
Expected: `Monospace completely removed!`

**Step 3: Commit**
```bash
git add web_src/style.css
git commit -m "style: remove monospace and apply Poppins tabular numbers across all UI"
```

---

### Task 4: Clean Up Badge Formatting & Refactor Flow 1 & Flow 2 Cards

**Files:**
- Modify: `web_src/index.html`
- Modify: `web_src/style.css`

**Step 1: Format Hero Chip in `web_src/index.html`**
- Change `ESP32 DevKit V1 (<span class="mono-token">240 MHz</span>)` to:
  `ESP32 DevKit V1 · <span class="flow-token">240 MHz</span>`

**Step 2: Update Flow 1 & Flow 2 Card Styling in `web_src/style.css`**
- `.automation-flows`: background `#F8F9FA`, border `1px solid var(--border-subtle)`, radius `12px`.
- `.flow-node`: background `#FFFFFF`, border `1px solid var(--border-subtle)`, shadow `0 1px 3px rgba(0, 0, 0, 0.04)`.
- `.flow-connector`: hairline border `#D1D5DB`.

**Step 3: Verify formatting in HTML & CSS**
Run: `python -c "html = open('web_src/index.html', encoding='utf-8').read(); assert '·' in html; print('Badges formatted!')"`
Expected: `Badges formatted!`

**Step 4: Commit**
```bash
git add web_src/index.html web_src/style.css
git commit -m "style: clean badge formatting and optimize flow card contrast"
```

---

### Task 5: Synchronize Firmware Headers

**Files:**
- Modify: `src/web/index_html.h`
- Modify: `src/web/style_css.h`
- Script: `tools/sync_to_esp.py`

**Step 1: Run synchronization script**
Run: `python tools/sync_to_esp.py`
Expected: `Synced: web_src/index.html -> src/web/index_html.h` and `Synced: web_src/style.css -> src/web/style_css.h`.

**Step 2: Verify firmware headers contain updated styles and fonts**
Run: `python -c "header = open('src/web/style_css.h', encoding='utf-8').read(); assert '#F5F5F4' in header; assert 'JetBrains' not in header; print('Firmware headers in sync!')"`
Expected: `Firmware headers in sync!`

**Step 3: Commit**
```bash
git add src/web/index_html.h src/web/style_css.h
git commit -m "chore: synchronize web assets to ESP32 firmware headers"
```

---

### Task 6: Comprehensive Verification Test

**Files:**
- Test: `tools/test_server.py`

**Step 1: Run dev server test suite**
Run: `python tools/test_server.py`
Expected: `ALL DEV SERVER TESTS PASSED SUCCESSFULLY!`

**Step 2: Check responsive layout & contrast**
Run: `python -c "import urllib.request; res = urllib.request.urlopen('http://127.0.0.1:8080/').read().decode('utf-8'); assert 'Poppins' in res; assert 'JetBrains' not in res; print('Live endpoint verified!')"`
Expected: `Live endpoint verified!`

**Step 3: Check em dash antislop compliance**
Run: `python -c "for path in ['web_src/index.html', 'web_src/style.css']: assert '—' not in open(path, encoding='utf-8').read(); print('Zero em dashes in source files!')"`
Expected: `Zero em dashes in source files!`
