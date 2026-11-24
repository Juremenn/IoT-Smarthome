# Design Doc: High-Contrast Palette & 100% Poppins Typography Refactor

**Date:** 2026-11-24  
**Project:** Smart Home Miniatur Rumah Joglo (ESP32)  
**Status:** Approved by User  

---

## 1. Problem Statement
The previous user interface utilized a beige-on-beige ("dluwang paper" and warm ivory) palette with muted umber text. While historically inspired by Yogyakarta Keraton motifs, the low contrast caused elements to feel muddy, washed out, and difficult to parse under typical screen conditions. Additionally, numeric readouts and telemetry tokens used `JetBrains Mono`, creating a visual clash with the primary geometric sans-serif typeface (`Poppins`).

---

## 2. Goals & Design Decisions

### 2.1 Clean High-Contrast Light Theme
* **Canvas (`--bg-canvas`):** `#F5F5F4` (Clean, neutral stone; eliminates sepia/yellowish cast).
* **Cards (`--bg-card`):** `#FFFFFF` (Pure white; provides unambiguous contrast against the canvas).
* **Inner Flow Panels & Badges (`--bg-surface-muted`):** `#F3F4F6` (Neutral light gray).
* **Borders (`--border-subtle`):** `#E5E7EB` (Crisp 1px hairline border; hover `#D1D5DB`).
* **Primary Text (`--text-primary`):** `#111111` (True sharp black across all headings, titles, numbers, and tokens).
* **Secondary Text (`--text-secondary`):** `#525252` (Neutral dark gray for subtitles and descriptions).
* **Muted Text (`--text-muted`):** `#737373` (Technical hints and auxiliary labels).
* **Javanese Accents (Surgical Scope):**
  * `--accent-terracotta`: `#A84A28` (Active nav tabs, primary CTA buttons, key icons).
  * `--status-danger`: `#DC2626` (Emergency flame alert and evacuation states).
  * `--status-safe`: `#16A34A` (Normal sensor telemetry).

### 2.2 100% Poppins Typography System
* **Unified Font Family:** Consolidate entire application into `Poppins` (weights: 400, 500, 600, 700, 800).
* **Elimination of Monospace:** Completely purge `JetBrains Mono` from Google Fonts `<link>` and stylesheet declarations.
* **Header Branding:** Set "Rumah Joglo" logo title in `Poppins 800` (`#111111`, letter spacing `-0.03em`), unifying modern geometric precision with the transparent vector silhouette.
* **Tabular Numbers:** Enforce `font-variant-numeric: tabular-nums;` on all numbers (`240 MHz`, `≤ 15 cm`, `5s`, `15ms`, `28.4 °C`, `64%`) and pinout badges (`GPIO 13`, `D34`) to eliminate jitter during status updates.

### 2.3 Badge & Flow Card Refinement
* **Badge Spacing & Formatting:** Standardize hero chip and sensor tags to `ESP32 DevKit V1 · 240 MHz` using middle dot (`·`) separator and `Poppins 500`.
* **Automation Flow Cards:**
  * Container: `#F8F9FA` background with `#E5E7EB` border.
  * Node Cards: `#FFFFFF` crisp white with `#E5E7EB` border and subtle elevation (`0 1px 3px rgba(0,0,0,0.04)`).
  * Flow Connectors: `#D1D5DB` hairline lines with neutral indicator arrows.

---

## 3. Architecture & Affected Files
1. `web_src/style.css`: Token definition rewrite, font family unification, component class updates.
2. `web_src/index.html`: Google Fonts URL cleanup, badge string refactoring, flow card structure updates.
3. `tools/sync_to_esp.py`: Firmware code generator.
4. `src/web/index_html.h` & `src/web/style_css.h`: Synchronized firmware headers.
5. `tools/test_server.py`: Regression test suite.
