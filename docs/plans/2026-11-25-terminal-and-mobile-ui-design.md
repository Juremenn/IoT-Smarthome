# Desain Perbaikan Terminal/Servo & UI Mobile (`antislop`)

**Tanggal:** 2026-11-25  
**Dials Antislop:** `ENERGY 1` / `RHYTHM 2` / `MOTION 2`

## 1. Perbaikan Bug Servo `0 ms` & Terminal (`src/`)
- **Root cause buka `0 ms`:** `loop()` mengambil `now = millis()` sebelum `automationUpdate()` (yang memanggil `pulseIn()`), sehingga `moveStart = millis()` bernilai lebih besar dari `now`, memicu unsigned underflow `now - moveStart` di `doorUpdate()`.
- **Solusi:** `doorUpdate()` membaca `millis()` segar dan menambahkan guard `if ((long)(now - moveStart) < 0) return;`.
- **Redam jeda `41 ms` saat menutup:** `ultrasonicUpdate(now, enabled, shortTimeout)` memakai timeout `6000 µs` (~100 cm) saat `doorPhase() == Closing`.
- **Log Terminal:** Merapikan format `[STATUS]` di `src/main.cpp` tanpa field redundant (`Pemicu: 3`).

## 2. Penyempurnaan UI Mobile & Kepatuhan `antislop` (`web_src/`)
- **Reflow Hibrida (`<= 640px`, R-03):** Grid 2 kolom di mana `Pintu`, `Sensor Api`, dan `Pemicu Pintu` membentang penuh (`1 / -1`), sedangkan `Jarak Halangan` & `Suhu Ruang` berdampingan `1fr 1fr`.
- **Tap Targets & Konsistensi (R-03, R-11):** Seluruh tombol (`btn-primary`, `btn-secondary`, `btn-pill`) minimal `44px` dengan radius konsisten `8px`.
- **Bottom Nav & Safe Area (R-03, R-39):** `.nav-inner` grid `1fr 1fr`, `padding-bottom: env(safe-area-inset-bottom)`, hapus `padding-bottom: 100px` ganda di `body`, serta lengkapi `role="tablist"` / `role="tab"` / `aria-selected`.
- **Anti-Overflow & Kontras/Warna (R-03, R-25):** Izinkan wrap pada `.header-status`, `.flow-node-*`, dan `.pinout-card-header`; batasi warna merah (`--status-danger`) hanya pada `.flow-node-danger .flow-node-desc`.
