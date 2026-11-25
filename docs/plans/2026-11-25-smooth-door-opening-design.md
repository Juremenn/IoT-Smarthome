# Desain Gerakan Buka Pintu Halus & Redam Jitter Servo

**Tanggal:** 2026-11-25  
**Pendekatan:** Pendekatan 1 — Kurva *Soft-Start + Soft-End* (`7000 ms`) + Evakuasi Cepat (`1800 ms`) + Redam Jitter Loop

## 1. Profil Gerak & Easing (`include/config.h`, `src/door.cpp`)
- `DOOR_OPEN_MS = 7000` (identik dengan `DOOR_CLOSE_MS = 7000`).
- `DOOR_EVAC_OPEN_MS = 1800` khusus saat `evacuation == true`.
- Gerakan buka normal dan tutup sama-sama memakai `easeSoftEdge(t)` (`easeInOutSine(1.0f - powf(1.0f - t, 1.8f))`) agar start lembut dan akhir gerakan mendarat halus tanpa hentakan.
- Evakuasi api memakai `easeInOutSine(t)` selama `1800 ms`; bila api terdeteksi ketika pintu sedang membuka pelan, `doorSetEvacuation(true)` langsung mempercepat sisa jarak buka.

## 2. Redam Jitter Loop (`src/main.cpp`, `src/automation.cpp`)
- Saat `doorPhase() == Opening || Closing`:
  - `dhtUpdate()` tetap ditunda.
  - `oledUpdate()` ditunda selama kondisi aman (`!flameDetected()`) agar transfer I2C (~25 ms) tidak memotong tick servo `20 ms`. Saat api terdeteksi, OLED langsung berjalan penuh.
- Saat `doorPhase() == Opening`:
  - `ultrasonicUpdate()` ditunda agar timeout `pulseIn` (hingga 15 ms) tidak menyebabkan micro-jitter saat membuka. Saat `Closed`, `Open`, atau `Closing`, ultrasonik tetap aktif sehingga objek yang datang ketika pintu menutup langsung membalikkan pintu membuka.
- `flameUpdate()` tetap berjalan penuh setiap `5 ms` di semua fase.

## 3. Sinkronisasi Web & Verifikasi
- Perbarui label durasi `5s` -> `7s` pada `web_src/index.html`, sync ke `src/web/` dengan `tools/sync_to_esp.py`, uji dengan `tools/test_server.py`, dan build dengan `pio run`.
