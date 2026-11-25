# Desain Optimasi Performa Menyeluruh ESP32 Rumah Joglo

**Tanggal:** 2026-11-25  
**Proyek:** Smart Home Miniatur Rumah Joglo (ESP32)  
**Status:** Disetujui Pengguna  
**Baseline:** Flash `1.092.031 B` (`83,3%`), RAM `51.932 B` (`15,8%`), OLED 20 FPS blocking ~25 ms, `pulseIn` blocking 15 ms, DHT11 blocking ~25 ms di Core 1, polling web 250 ms tanpa optimistic UI.

## 1. Sensor Ultrasonik (High Sensitivity & Zero CPU Blocking)
- **Hardware Interrupt (`IRAM_ATTR` ISR + `esp_timer_get_time()`)**: Trigger 10 us setiap `45 ms`, lebar pulsa `ECHO` diukur di ISR dengan strict timeout `8000 us` (~135 cm). CPU **0 us blocking** menunggu pantulan.
- **Filter Cepat Anti-False Trigger**: 3-sample rolling median ditambah konfirmasi 2x berturut-turut `<= 10 cm` (~90 ms total reaksi). Tetap aktif saat pintu sedang membuka atau menutup.
- **Isolasi Task Lambat**:
  - `DHT11` dipindah ke FreeRTOS task ringan di **Core 0** (`dhtTask`, interval `2100 ms`, proteksi spinlock `portMUX_TYPE`) sehingga Core 1 tidak pernah stall ~25 ms.
  - Refresh normal OLED diturunkan dari `50 ms` (20 FPS) ke `500 ms` (2 FPS) dan `180 ms` saat darurat api; `delay(150)` & `delay(5)` dihapus.
  - `delay(1)` di `main.cpp` diganti cooperative yield periodik (`vTaskDelay(1)` tiap 20 ms).

## 2. Zero-Delay Web Control
- `WiFi.setSleep(false)` dipertahankan; `server.enableDelay(false)` (`TCP_NODELAY`) diaktifkan pada `WebServer`.
- `handleStatus()` memakai `static char jsonBuf[384]` + `snprintf()` (zero `String` heap churn) dan `handleDoor()` membalas instan (< 10 ms) tanpa menunggu gerakan servo.
- Frontend `web_src/app.js`: polling `400 ms` dengan `AbortController`, pembatalan poll otomatis saat tombol ditekan, sequence guard (`stateVersion`), dan **optimistic UI** langsung pada klik `Buka 2 detik`, `Buka terus`, `Tutup`, dan toggle `Active-HIGH`/`Active-LOW`.

## 3. Kecepatan Compile & Upload (Flashing)
- `tools/sync_to_esp.py` melakukan minifikasi + kompresi **gzip** level 9 untuk HTML/CSS/JS ke `src/web/web_assets_ui.cpp`, dan memisahkan biner statis berat (`joglo.png`, `alarm.mp3`) ke `src/web/web_assets_static.cpp` dengan deklarasi ringan `src/web/web_assets.h`.
- `web_server.cpp` melayani aset UI dengan `Content-Encoding: gzip` + `Cache-Control`.
- `platformio.ini`: tambah `-DCORE_DEBUG_LEVEL=0 -Os` dan pertahankan `upload_speed = 921600` (catatan fallback `460800` untuk CH340).
