# Desain Penyesuaian Website & IoT dengan `Flowchart_iot_final.jpg`

**Tanggal:** 2026-11-25  
**Mode Antislop:** DURING (diterapkan selama perencanaan & eksekusi)

## 1. Tujuan & Cakupan
Menyelaraskan sistem IoT (ESP32) dan Web Dashboard (`web_src/`) dengan alur pada `Flowchart_iot_final.jpg` sambil mempertahankan fitur ekstra yang sudah ada pada firmware (Kipas `D23` dan Touch `D4`).

## 2. Arsitektur & Alur IoT Firmware (`src/`, `include/config.h`)
Firmware ESP32 sudah menerapkan 3 cabang paralel non-blocking sesuai flowchart:
1. **Inisialisasi & Sistem Siap:** `setup()` di `src/main.cpp` menginisialisasi servo (`D13`/`D12`), sensor api (`D34`), DHT11 (`D27`), ultrasonik (`D4`/`D5`), relay kipas (`D23`), dan OLED (`D21`/`D22`), mengaktifkan Wi-Fi AP (`RumahJoglo-SmartHome`) serta Web Server (`port 80`), lalu menampilkan `"Sistem Siap"` / `"ONLINE"` di OLED (`src/oled.cpp`).
2. **Cabang Sensor Api (`D34`):** `flameUpdate()` + `doorSetEvacuation(flameDetected())` di `src/automation.cpp` membuka dan menahan pintu saat api terdeteksi, OLED menampilkan `"! PERINGATAN API !"` (aman: `"API: AMAN"`), dan Web Dashboard mengaktifkan mode bahaya serta alarm audio.
3. **Cabang Sensor DHT11 (`D27`):** `dhtUpdate()` di `src/sensors.cpp` mempertahankan nilai valid terakhir hingga 5x gagal baca (`DHT_MAX_CONSECUTIVE_FAILS`), lalu menampilkan `"--.- °C"` di OLED dan `"--"` di Web saat invalid.
4. **Cabang Sensor Ultrasonik (`D4`/`D5`):** `automationUpdate()` memicu `doorStartCycle()` saat `mode == MODE_ULTRASONIC` dan `0 < distance <= 10 cm` (`cfg::MAX_DISTANCE_CM = 10`).

Tidak diperlukan perubahan logika inti pada firmware ESP32.

## 3. Penyesuaian Website (`web_src/`) & Kepatuhan Antislop
Website mempertahankan identitas visual *Yogyakarta Keraton Minimalism* (`#F5F5F4`, Poppins, palet celadon/terracotta/danger):

- **`web_src/app.js`:**
  - Perbaiki kondisi evakuasi api dari `d.doorTriggerMode === 2 && d.flame` menjadi `Boolean(d.flame)` agar konsisten dengan evakuasi global di firmware.
  - Gunakan `d.maxDistance` dari `/api/status` (fallback `CONFIG.ULTRASONIC_TRIGGER_CM = 10`) sebagai sumber tunggal ambang jarak ultrasonik, dan perbarui teks `triggerDetailText` secara dinamis.
  - Perbarui indikator status **OLED** (`Aktif` + alamat I2C / `Offline`) dan status koneksi (**Loading / Terhubung / Terputus**) untuk memenuhi R-27 (UI States).
  - Animasikan Flow 3 (DHT11) saat `d.tempValid === true`.

- **`web_src/index.html` & `web_src/style.css`:**
  - Ubah semua referensi ambang `15 cm` menjadi `10 cm` (`triggerDetailText`, Flow 1, dan Card Pinout Ultrasonik).
  - Tambahkan indikator status **OLED** & koneksi di header bar secara responsif (R-03, R-25).
  - Perbarui **Flow 2 (*Evakuasi Bahaya Api*)** di tab `Info Pinout` agar node akhir mencerminkan `Pintu Buka + Alarm + OLED`.
  - Tambahkan **Flow 3 (*Pemantauan Suhu & Kelembaban*)**: `Sensor DHT11 (D27)` -> `Validasi Data` -> `OLED & Web Dashboard`.

## 4. Simulator, Sinkronisasi, & Verifikasi
- Ubah `self.maxDistance = 10` di `tools/dev_server.py`.
- Jalankan `python tools/sync_to_esp.py` untuk memperbarui `src/web/index_html.h`, `src/web/app_js.h`, dan `src/web/style_css.h`.
- Verifikasi build firmware dengan `pio run` dan uji dashboard/API melalui `tools/test_server.py` serta pemeriksaan UI interaktif & responsif.
