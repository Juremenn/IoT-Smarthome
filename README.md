# 🏡 IoT Smart Home - Rumah Joglo
### Pintu Otomatis Servo (Gerak Halus), Sensor Api, Sensor Jarak, Sensor Sentuh, Sensor Suhu DHT11, Relay Kipas & Dashboard Web

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Project-orange?logo=platformio)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/Board-ESP32%20DevKit%20V1-red?logo=espressif)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue?logo=arduino)](https://www.arduino.cc/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B-00599C?logo=c%2B%2B)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Smart home miniatur Rumah Joglo berbasis **ESP32**. Dashboard web disajikan langsung dari ESP32 melalui hotspot Access Point mandiri dan dapat diakses tanpa koneksi internet.

---

## ✨ Fitur

- **Pintu servo (D13, alternatif D12), gerak halus:** membuka dari 180° (tertutup) ke 0° (terbuka) dalam **5,0 detik** dengan kurva *ease-in-out sine* dan menutup kembali dalam **7,0 detik** dengan kurva *soft-close*. Pergerakan bersifat *non-blocking* (update tiap 20 ms) sehingga web server tetap responsif.
- **Tiga perintah pintu:** Buka 2 Detik (*auto-cycle*), Buka Terus (*manual open*), dan Tutup Rapat.
- **Pemicu otomatis (pilih satu mode):**
  - **Ultrasonik (HC-SR04 — default):** objek berjarak `1–10 cm` membuka pintu. Selama objek bertahan, waktu tahan 2 detik diperpanjang; jika objek muncul saat pintu sedang menutup, pintu langsung berbalik membuka.
  - **Sentuh (D4 / Touch0):** sentuh ujung kabel jumper di D4 (dengan *baseline* adaptif & filter median 3 sampel).
  - **Sensor Api (D34):** jika aktif, pintu langsung dibuka penuh ke 0° dan ditahan terbuka selama api terdeteksi (mode evakuasi). Tombol Tutup di web dikunci hingga api padam.
  - **Manual:** hanya dikendalikan lewat tombol dashboard web.
- **Detektor Api Anti-Flicker & Alarm Web:** status Aman/Bahaya dengan konfirmasi cepat (`15 ms` / *streak* ≥ 2), penahan rilis (`1200 ms`), opsi balik logika Active-HIGH/LOW, serta pemutaran audio alarm `/alarm.mp3` di browser.
- **Sensor Suhu & Kelembaban (DHT11 — D27):** pembacaan berkala setiap `2100 ms` (ditunda saat servo sedang bergerak agar gerak pintu tidak tersendat), dengan toleransi gagal baca hingga 5 kali berturut-turut.
- **Kendali Relay Kipas (D23):** kontrol ON/OFF langsung dari dashboard web beserta opsi pembalikan logika Active-LOW/HIGH.
- **OLED SSD1306 (I2C D21/D22 — opsional):** deteksi *hot-plug* otomatis (`0x3C` / `0x3D`), menampilkan IP, animasi sepasang wayang, suhu, kelembaban, status aman, serta layar peringatan berkedip saat bahaya api.
- **WiFi Access Point Mandiri:** ESP32 memancarkan hotspot `RumahJoglo-SmartHome` dengan IP `192.168.4.1` secara instan tanpa bergantung pada router eksternal.

---

## 🛠️ Komponen & Wiring

| Komponen | Pin Modul | Pin ESP32 |
| :--- | :--- | :--- |
| Servo SG90 / MG90S | Sinyal (oranye) | **D13** (alternatif paralel: **D12**) |
| | VCC / GND | 5V (VIN) / GND |
| Flame sensor 5-way (LM393) | DO | **D34** |
| | VCC / GND | 3V3 / GND |
| Sensor sentuh | Ujung kabel jumper | **D4** (Touch0) |
| HC-SR04 | Trig / Echo | **D4 / D5** (bisa ditukar atau dipindai dari web) |
| | VCC / GND | 5V / GND |
| Sensor DHT11 | Data | **D27** |
| | VCC / GND | 3V3 / GND |
| Relay Kipas 1-Channel | IN | **D23** (default Active-LOW) |
| | VCC / GND | 5V / GND |
| OLED SSD1306 128x64 (opsional) | SDA / SCL | **D21 / D22** |

> **Catatan Pin D4:** Sensor sentuh dan `Trig` HC-SR04 sama-sama memakai **D4** secara default. Firmware hanya membaca sentuh pada Mode Sentuh dan ultrasonik pada Mode Ultrasonik, namun jika dipasang bersamaan fisiknya dapat saling mengganggu. Pindahkan pin HC-SR04 melalui fitur **Pindai** di tab Pengaturan dashboard.

> **Catatan Daya Flame Sensor:** Beri daya **3V3** pada modul flame sensor karena pin **D34** ESP32 hanya mendukung level logika 3,3 V.

---

## 📊 Diagram & Flowchart Sistem

Seluruh diagram di bawah ini disusun menggunakan bentuk simbol standar flowchart (**ANSI/ISO**) agar sesuai dengan kebutuhan dokumentasi teknis, laporan, maupun presentasi:

| Simbol | Bentuk di Diagram | Fungsi |
| :--- | :--- | :--- |
| **Terminator** | Kapsul / Oval `([...])` | Titik mulai (*Start*) dan selesai (*End*) suatu alur |
| **Proses** | Persegi Panjang `[...]` | Inisialisasi, kalkulasi, perubahan state, dan kendali aktuator |
| **Keputusan (*Decision*)** | Belah Ketupat `{...}` | Percabangan logika kondisi (`Ya` / `Tidak` atau pilihan mode) |
| **Input / Output (I/O)** | Jajaran Genjang `[/.../]` | Pembacaan data sensor, pengiriman HTTP/JSON, tampilan OLED & Serial |
| **Subroutine** | Persegi Panjang Garis Ganda `[[...]]` | Pemanggilan fungsi/modul terpisah (seperti `automationUpdate`, `doorUpdate`) |

---

### 1. Arsitektur Sistem & Aliran Data

Diagram blok berikut menggambarkan hubungan antara perangkat input (sensor), modul pemrosesan di ESP32, aktuator/layar lokal, dan dashboard web pada perangkat pengguna.

```mermaid
flowchart LR
    classDef hwIn fill:#e0f2fe,stroke:#0284c7,stroke-width:2px,color:#0c4a6e
    classDef mcu fill:#dcfce7,stroke:#16a34a,stroke-width:2px,color:#14532d
    classDef hwOut fill:#fef3c7,stroke:#d97706,stroke-width:2px,color:#78350f
    classDef client fill:#f3e8ff,stroke:#9333ea,stroke-width:2px,color:#581c87

    subgraph INPUT["Perangkat Input (Sensor)"]
        S_FLAME[/"Sensor Api 5-Way\nGPIO D34"/]:::hwIn
        S_TOUCH[/"Sensor Sentuh\nGPIO D4 (Touch0)"/]:::hwIn
        S_US[/"Ultrasonik HC-SR04\nTrig D4 / Echo D5"/]:::hwIn
        S_DHT[/"Sensor DHT11\nGPIO D27"/]:::hwIn
    end

    subgraph ESP32["Mikrokontroler ESP32"]
        MOD_SENS["sensors.cpp\nAkuisisi & Filter Data"]:::mcu
        MOD_AUTO["automation.cpp\nEvaluasi Mode & Evakuasi"]:::mcu
        MOD_DOOR["door.cpp\nKurva Sine & Soft-Close"]:::mcu
        MOD_FAN["fan.cpp\nKontrol Relay Kipas"]:::mcu
        MOD_OLED["oled.cpp\nRender Wayang & Status"]:::mcu
        MOD_NET["app_network.cpp\nWiFi AP: RumahJoglo-SmartHome"]:::mcu
        MOD_WEB["web_server.cpp\nHTTP Server Port 80 (/api/*)"]:::mcu
    end

    subgraph OUTPUT["Aktuator & Display Lokal"]
        A_SERVO[/"Servo Pintu\nGPIO D13 / D12"/]:::hwOut
        A_FAN[/"Relay Kipas\nGPIO D23"/]:::hwOut
        A_OLED[/"OLED SSD1306\nI2C D21 / D22"/]:::hwOut
    end

    subgraph USER["Perangkat Pengguna (HP / Laptop)"]
        WEB_UI[/"Dashboard Web\nhttp://192.168.4.1\nPolling 300ms + Alarm MP3"/]:::client
    end

    S_FLAME --> MOD_SENS
    S_TOUCH --> MOD_SENS
    S_US --> MOD_SENS
    S_DHT --> MOD_SENS

    MOD_SENS --> MOD_AUTO
    MOD_SENS --> MOD_OLED
    MOD_SENS --> MOD_WEB

    MOD_AUTO --> MOD_DOOR
    MOD_DOOR --> A_SERVO
    MOD_FAN --> A_FAN
    MOD_OLED --> A_OLED

    MOD_NET <--> WEB_UI
    MOD_WEB <--> WEB_UI
    MOD_WEB --> MOD_DOOR
    MOD_WEB --> MOD_AUTO
    MOD_WEB --> MOD_FAN
    MOD_WEB --> MOD_SENS
```

---

### 2. Flowchart Alur Sistem Utama (`setup()` dan `loop()`)

Menggambarkan siklus hidup firmware pada [`src/main.cpp`](src/main.cpp) sejak ESP32 dinyalakan hingga berjalan terus-menerus di dalam `loop()`.

```mermaid
flowchart TD
    classDef term fill:#dcfce7,stroke:#15803d,stroke-width:2px,color:#14532d
    classDef proc fill:#dbeafe,stroke:#1d4ed8,stroke-width:1.5px,color:#1e3a8a
    classDef dec fill:#fef9c3,stroke:#ca8a04,stroke-width:2px,color:#713f12
    classDef io fill:#f3e8ff,stroke:#7e22ce,stroke-width:1.5px,color:#581c87
    classDef sub fill:#e0e7ff,stroke:#4338ca,stroke-width:1.5px,color:#312e81

    START(["Mulai (ESP32 Power ON)"]):::term
    SER_INIT["Inisialisasi Serial Monitor (115200 bps)"]:::proc
    RST_CHK[/"Baca Alasan Reset (esp_reset_reason)"/]:::io
    HW_BEGIN["Inisialisasi Awal Aktuator & Sensor:\n- doorBegin() (Servo D13/D12 ke 180 deg)\n- fanBegin() (Relay D23 OFF)\n- ultrasonicBegin(), flameBegin(), dhtBegin()"]:::proc
    OLED_BOOT[["oledBegin() & oledShowBoot('Memulai Sistem...')"]]:::sub
    NET_WEB[["networkBegin() & webBegin()\n(Aktifkan Hotspot AP + Server Port 80)"]]:::sub
    OLED_SPLASH[/"Tampilkan Splash Screen 'ONLINE' & IP di OLED"/]:::io
    CHK_TOUCH{"triggerMode() == MODE_TOUCH?"}:::dec
    TOUCH_CAL[["touchBegin()\nKalibrasi Baseline D4"]]:::sub
    SER_INFO[/"Cetak Konfigurasi Pin & IP Dashboard ke Serial"/]:::io

    LOOP_NET["networkMaintain(now)"]:::proc
    LOOP_WEB[["webHandle()\nLayani Permintaan HTTP Klien"]]:::sub
    CHK_PHASE{"Pintu Sedang Diam?\n(Phase Closed atau Open)"}:::dec
    LOOP_DHT[["dhtUpdate(now)\nBaca Suhu & Kelembaban D27"]]:::sub
    LOOP_AUTO[["automationUpdate(now)\nBaca Sensor & Evaluasi Pemicu"]]:::sub
    LOOP_DOOR[["doorUpdate(now)\nUpdate Pulsa Halus Servo"]]:::sub
    LOOP_OLED[["oledUpdate(now)\nRefresh Tampilan OLED"]]:::sub
    CHK_LOG{"Selisih Waktu >= 3000 ms?"}:::dec
    SER_LOG[/"Cetak Status Sistem ke Serial Monitor"/]:::io
    DELAY_LOOP["Jeda Singkat delay(1)"]:::proc

    START --> SER_INIT --> RST_CHK --> HW_BEGIN --> OLED_BOOT --> NET_WEB --> OLED_SPLASH --> CHK_TOUCH
    CHK_TOUCH -- "Ya" --> TOUCH_CAL --> SER_INFO
    CHK_TOUCH -- "Tidak" --> SER_INFO

    SER_INFO --> LOOP_NET
    LOOP_NET --> LOOP_WEB --> CHK_PHASE
    CHK_PHASE -- "Ya" --> LOOP_DHT --> LOOP_AUTO
    CHK_PHASE -- "Tidak (Servo Bergerak)" --> LOOP_AUTO
    LOOP_AUTO --> LOOP_DOOR --> LOOP_OLED --> CHK_LOG
    CHK_LOG -- "Ya" --> SER_LOG --> DELAY_LOOP
    CHK_LOG -- "Tidak" --> DELAY_LOOP
    DELAY_LOOP --> LOOP_NET
```

---

### 3. Flowchart Jaringan WiFi Access Point & Layanan Web Server

Menggambarkan cara ESP32 membangun hotspot mandiri pada [`src/app_network.cpp`](src/app_network.cpp) serta melayani endpoint API pada [`src/web_server.cpp`](src/web_server.cpp) untuk dashboard web ([`src/web/app_js.h`](src/web/app_js.h)).

```mermaid
flowchart TD
    classDef term fill:#dcfce7,stroke:#15803d,stroke-width:2px,color:#14532d
    classDef proc fill:#dbeafe,stroke:#1d4ed8,stroke-width:1.5px,color:#1e3a8a
    classDef dec fill:#fef9c3,stroke:#ca8a04,stroke-width:2px,color:#713f12
    classDef io fill:#f3e8ff,stroke:#7e22ce,stroke-width:1.5px,color:#581c87

    START(["Mulai Setup Jaringan & Web"]):::term
    AP_MODE["Set WiFi Mode ke WIFI_AP\n& Matikan Modem-Sleep"]:::proc
    AP_START["Aktifkan SoftAP SSID:\n'RumahJoglo-SmartHome'"]:::proc
    GET_IP[/"Ambil IP AP (192.168.4.1)\n& Cetak Info ke Serial"/]:::io
    REG_ROUTE["Daftarkan Route Aset (/, /style.css, /app.js, /alarm.mp3)\n& Endpoint API (/api/status, /door, /fan, /flame, /ultrasonic)"]:::proc
    SRV_START["Jalankan HTTP WebServer di Port 80"]:::proc

    WAIT_REQ{"Ada HTTP Request Masuk?"}:::dec
    CHK_ROUTE{"Jenis Endpoint Request?"}:::dec

    R_ASSET[/"Kirim Aset Statis dari PROGMEM\n(HTML / CSS / JS / Audio MP3)"/]:::io
    R_STATUS["Kumpulkan Status Sensor, Pintu,\nKipas, OLED & Uptime"]:::proc
    R_DOOR["Proses Aksi Pintu / Set Mode\n(auto / open / close / set_mode)"]:::proc
    R_FAN["Proses Kipas D23\n(toggle / state / toggle_logic)"]:::proc
    R_FLAME["Balik Logika Sensor Api D34\n(Active-HIGH / Active-LOW)"]:::proc
    R_US["Tukar Pin atau Pindai Otomatis\nPin Ultrasonik HC-SR04"]:::proc
    SEND_JSON[/"Kirim Respons JSON Status\n(HTTP 200 application/json)"/]:::io
    NEXT_LOOP(["Kembali ke Loop Utama"]):::term

    START --> AP_MODE --> AP_START --> GET_IP --> REG_ROUTE --> SRV_START --> WAIT_REQ
    WAIT_REQ -- "Tidak" --> NEXT_LOOP
    WAIT_REQ -- "Ya" --> CHK_ROUTE

    CHK_ROUTE -- "Aset Web" --> R_ASSET --> NEXT_LOOP
    CHK_ROUTE -- "/api/status" --> R_STATUS --> SEND_JSON
    CHK_ROUTE -- "/api/door" --> R_DOOR --> R_STATUS
    CHK_ROUTE -- "/api/fan" --> R_FAN --> R_STATUS
    CHK_ROUTE -- "/api/flame" --> R_FLAME --> R_STATUS
    CHK_ROUTE -- "/api/ultrasonic" --> R_US --> SEND_JSON
    SEND_JSON --> NEXT_LOOP
```

---

### 4. Flowchart Logika Otomasi Pintu & Gerak Halus Servo

Menggambarkan inti pengendalian pintu pada [`src/automation.cpp`](src/automation.cpp) dan [`src/door.cpp`](src/door.cpp), termasuk prioritas **Evakuasi Api**, mode pemicu (**Sentuh / Ultrasonik / Manual**), pencegahan terjepit (*re-open*), serta kurva pergerakan halus *Ease-In-Out Sine* dan *Soft-Close*.

```mermaid
flowchart TD
    classDef term fill:#dcfce7,stroke:#15803d,stroke-width:2px,color:#14532d
    classDef proc fill:#dbeafe,stroke:#1d4ed8,stroke-width:1.5px,color:#1e3a8a
    classDef dec fill:#fef9c3,stroke:#ca8a04,stroke-width:2px,color:#713f12
    classDef io fill:#f3e8ff,stroke:#7e22ce,stroke-width:1.5px,color:#581c87
    classDef sub fill:#e0e7ff,stroke:#4338ca,stroke-width:1.5px,color:#312e81
    classDef danger fill:#fee2e2,stroke:#dc2626,stroke-width:2px,color:#7f1d1d

    START(["Mulai Siklus Otomasi Pintu"]):::term
    UPD_SENS[["Update Pembacaan Sensor:\nflameUpdate, touchUpdate, ultrasonicUpdate"]]:::sub
    CHK_EVAC{"Mode == MODE_FLAME\nDAN Api Terdeteksi?"}:::dec

    EVAC_ON["Aktifkan Evakuasi (evacuation = true):\nPaksa Buka Penuh ke 0 deg & Kunci Tutup Web"]:::danger
    EVAC_OFF["Nonaktifkan Evakuasi (evacuation = false)"]:::proc
    CHK_MAN_OPEN{"Pintu Sedang Mode\n'Buka Terus' Manual?"}:::dec
    KEEP_OPEN["Abaikan Sensor Pemicu\n(Pertahankan Posisi Terbuka)"]:::proc

    CHK_MODE{"Mode Pemicu Aktif?"}:::dec
    DO_TOUCH[/"Baca Touch0 D4 (Median 3x)"/]:::io
    DEC_TOUCH{"Delta >= Ambang Batas?\n(4% Baseline, maks 55)"}:::dec
    DO_US[/"Kirim Pulsa Trig 10 us\n& Ukur Pantulan Echo"/]:::io
    CALC_US["Hitung Jarak = (Durasi * 0.034) / 2 cm"]:::proc
    DEC_US{"0 < Jarak <= 10 cm?"}:::dec
    DO_MANUAL["triggered = false\n(Tunggu Perintah Web)"]:::proc

    TRIG_TRUE["Set triggered = true"]:::proc
    TRIG_FALSE["Set triggered = false"]:::proc
    CHK_TRIG{"triggered == true?"}:::dec
    START_CYCLE[["doorStartCycle()\nBuka 5000ms / Perpanjang Tahan 2000ms\n/ Balik Buka Jika Menutup"]]:::sub

    CHK_PHASE{"doorUpdate: Pintu Sedang\nOpening atau Closing?"}:::dec
    CHK_TICK{"Interval >= 20 ms\n(SERVO_TICK_MS)?"}:::dec
    CALC_CURVE["Hitung Sudut:\n- Buka (5000ms): easeInOutSine\n- Tutup (7000ms): easeSoftClose"]:::proc
    WRITE_SERVO[/"Kirim Pulsa writeMicroseconds()\nke Pin D13 & D12"/]:::io
    CHK_DONE{"Gerak Selesai (t >= 1.0)?"}:::dec
    SET_PHASE["Update Phase Open/Closed\n& Jadwalkan closeAt (+2000ms) jika AutoCycle"]:::proc

    CHK_HOLD{"Phase Open & AutoCycle &\n!Evacuation & Waktu Tahan Habis?"}:::dec
    START_CLOSE["Mulai Gerak Menutup Halus\n(startMove ke 180 deg / 7000ms)"]:::proc
    END_NODE(["Selesai Siklus Pintu"]):::term

    START --> UPD_SENS --> CHK_EVAC
    CHK_EVAC -- "Ya (DARURAT)" --> EVAC_ON --> CHK_PHASE
    CHK_EVAC -- "Tidak" --> EVAC_OFF --> CHK_MAN_OPEN

    CHK_MAN_OPEN -- "Ya" --> KEEP_OPEN --> CHK_PHASE
    CHK_MAN_OPEN -- "Tidak" --> CHK_MODE

    CHK_MODE -- "Sentuh (1)" --> DO_TOUCH --> DEC_TOUCH
    DEC_TOUCH -- "Ya" --> TRIG_TRUE
    DEC_TOUCH -- "Tidak" --> TRIG_FALSE

    CHK_MODE -- "Ultrasonik (3)" --> DO_US --> CALC_US --> DEC_US
    DEC_US -- "Ya" --> TRIG_TRUE
    DEC_US -- "Tidak" --> TRIG_FALSE

    CHK_MODE -- "Manual (0) / Api (2)" --> DO_MANUAL --> CHK_TRIG
    TRIG_TRUE --> CHK_TRIG
    TRIG_FALSE --> CHK_TRIG

    CHK_TRIG -- "Ya" --> START_CYCLE --> CHK_PHASE
    CHK_TRIG -- "Tidak" --> CHK_PHASE

    CHK_PHASE -- "Ya" --> CHK_TICK
    CHK_TICK -- "Tidak" --> END_NODE
    CHK_TICK -- "Ya" --> CALC_CURVE --> WRITE_SERVO --> CHK_DONE
    CHK_DONE -- "Belum" --> END_NODE
    CHK_DONE -- "Selesai" --> SET_PHASE --> END_NODE

    CHK_PHASE -- "Tidak (Diam)" --> CHK_HOLD
    CHK_HOLD -- "Ya" --> START_CLOSE --> END_NODE
    CHK_HOLD -- "Tidak" --> END_NODE
```

---

### 5. Flowchart Sensor Api Anti-Flicker & Sensor Suhu DHT11

#### 5A. Alur Deteksi Api 5-Way (`flameUpdate` pada [`src/sensors.cpp`](src/sensors.cpp))

```mermaid
flowchart TD
    classDef term fill:#dcfce7,stroke:#15803d,stroke-width:2px,color:#14532d
    classDef proc fill:#dbeafe,stroke:#1d4ed8,stroke-width:1.5px,color:#1e3a8a
    classDef dec fill:#fef9c3,stroke:#ca8a04,stroke-width:2px,color:#713f12
    classDef io fill:#f3e8ff,stroke:#7e22ce,stroke-width:1.5px,color:#581c87
    classDef danger fill:#fee2e2,stroke:#dc2626,stroke-width:2px,color:#7f1d1d

    START(["Mulai flameUpdate(now)"]):::term
    CHK_INT{"Selang Waktu >= 5 ms\n(200 Hz)?"}:::dec
    READ_PIN[/"Baca digitalRead(D34)"/]:::io
    CHK_ACTIVE{"Sinyal Sesuai Logika\nActive-HIGH / Active-LOW?"}:::dec

    ACT_YES["Catat activeSince, Reset quietTracking,\nNaikkan activeStreak (maks 8)"]:::proc
    ACT_NO["Turunkan activeStreak bertahap\n(Toleransi Api Bergetar)"]:::proc
    CHK_STREAK0{"activeStreak == 0?"}:::dec
    SET_QUIET["Catat waktu tenang (quietSince)"]:::proc

    CHK_CONF{"Belum Bahaya &\n(Aktif >= 15ms atau Streak >= 2)?"}:::dec
    SET_DANGER["Set Status BAHAYA API\n& Cetak Peringatan Serial"]:::danger
    CHK_REL{"Sedang Bahaya &\nTenang >= 1200 ms?"}:::dec
    SET_SAFE["Set Status AMAN Kembali"]:::proc
    END_NODE(["Selesai flameUpdate"]):::term

    START --> CHK_INT
    CHK_INT -- "Tidak" --> END_NODE
    CHK_INT -- "Ya" --> READ_PIN --> CHK_ACTIVE

    CHK_ACTIVE -- "Ya" --> ACT_YES --> CHK_CONF
    CHK_ACTIVE -- "Tidak" --> ACT_NO --> CHK_STREAK0
    CHK_STREAK0 -- "Ya" --> SET_QUIET --> CHK_CONF
    CHK_STREAK0 -- "Tidak" --> CHK_CONF

    CHK_CONF -- "Ya" --> SET_DANGER --> END_NODE
    CHK_CONF -- "Tidak" --> CHK_REL
    CHK_REL -- "Ya" --> SET_SAFE --> END_NODE
    CHK_REL -- "Tidak" --> END_NODE
```

#### 5B. Alur Sensor Suhu & Kelembaban (`dhtUpdate` pada [`src/sensors.cpp`](src/sensors.cpp))

```mermaid
flowchart TD
    classDef term fill:#dcfce7,stroke:#15803d,stroke-width:2px,color:#14532d
    classDef proc fill:#dbeafe,stroke:#1d4ed8,stroke-width:1.5px,color:#1e3a8a
    classDef dec fill:#fef9c3,stroke:#ca8a04,stroke-width:2px,color:#713f12
    classDef io fill:#f3e8ff,stroke:#7e22ce,stroke-width:1.5px,color:#581c87

    START(["Mulai dhtUpdate(now)"]):::term
    CHK_BOOT{"now >= 1500 ms DAN\nSelang >= 2100 ms?"}:::dec
    READ_DHT[/"Baca Suhu & Kelembaban\nLibrary Adafruit DHT11 (D27)"/]:::io
    CHK_VALID{"Data Valid?\n(Bukan NaN, -10..80 C, 0..100%)"}:::dec
    SAVE_GOOD["Simpan Suhu & Kelembaban Baru,\nReset consecutiveFails = 0, tempValid = true"]:::proc
    INC_FAIL["Tambah consecutiveFails++\nSet Pesan Cek Kabel D27"]:::proc
    CHK_TOL{"Pernah Valid &\nconsecutiveFails < 5?"}:::dec
    KEEP_LAST["Pertahankan Nilai Terakhir\n(Toleransi Gagal Sementara)"]:::proc
    SET_INVALID["Set tempValid = false\n(Tampilkan --.-)"]:::proc
    END_NODE(["Selesai dhtUpdate"]):::term

    START --> CHK_BOOT
    CHK_BOOT -- "Tidak" --> END_NODE
    CHK_BOOT -- "Ya" --> READ_DHT --> CHK_VALID
    CHK_VALID -- "Ya" --> SAVE_GOOD --> END_NODE
    CHK_VALID -- "Tidak" --> INC_FAIL --> CHK_TOL
    CHK_TOL -- "Ya" --> KEEP_LAST --> END_NODE
    CHK_TOL -- "Tidak" --> SET_INVALID --> END_NODE
```

---

### 6. Flowchart Layar OLED SSD1306 & Kendali Relay Kipas

Menggambarkan deteksi *hot-plug* I2C serta pergantian tampilan OLED ([`src/oled.cpp`](src/oled.cpp)) dan kendali aman relay kipas ([`src/fan.cpp`](src/fan.cpp)).

```mermaid
flowchart TD
    classDef term fill:#dcfce7,stroke:#15803d,stroke-width:2px,color:#14532d
    classDef proc fill:#dbeafe,stroke:#1d4ed8,stroke-width:1.5px,color:#1e3a8a
    classDef dec fill:#fef9c3,stroke:#ca8a04,stroke-width:2px,color:#713f12
    classDef io fill:#f3e8ff,stroke:#7e22ce,stroke-width:1.5px,color:#581c87
    classDef danger fill:#fee2e2,stroke:#dc2626,stroke-width:2px,color:#7f1d1d

    subgraph OLED_FLOW["Alur Layar OLED SSD1306 (oledUpdate)"]
        O_START(["Mulai oledUpdate(now)"]):::term
        O_ACT{"OLED Sudah Aktif?"}:::dec
        O_SCAN{"Selang Scan >= 1500 ms?"}:::dec
        O_I2C[/"Scan Alamat I2C 0x3C / 0x3D\nvia Pin D21 (SDA) & D22 (SCL)"/]:::io
        O_FPS{"Selang Refresh >= 50 ms\n(~20 FPS)?"}:::dec
        O_FLAME{"flameDetected() == true?"}:::dec
        O_DRAW_FIRE[/"Tampilkan Layar Bahaya Berkedip:\n'PERINGATAN API! EVAKUASI AKTIF'"/]:::danger
        O_DRAW_MAIN[/"Tampilkan Layar Normal:\nIP, Animasi Wayang, Suhu, Kelembaban & 'API: AMAN'"/]:::io
        O_END(["Selesai oledUpdate"]):::term

        O_START --> O_ACT
        O_ACT -- "Tidak" --> O_SCAN
        O_SCAN -- "Tidak" --> O_END
        O_SCAN -- "Ya" --> O_I2C --> O_END
        O_ACT -- "Ya" --> O_FPS
        O_FPS -- "Tidak" --> O_END
        O_FPS -- "Ya" --> O_FLAME
        O_FLAME -- "Ya" --> O_DRAW_FIRE --> O_END
        O_FLAME -- "Tidak" --> O_DRAW_MAIN --> O_END
    end

    subgraph FAN_FLOW["Alur Relay Kipas (fan.cpp - GPIO D23)"]
        F_START(["Mulai Inisialisasi Kipas"]):::term
        F_INIT["Set Level OFF Sebelum pinMode OUTPUT\n(Cegah Relay Berkedip Saat Boot)"]:::proc
        F_CMD{"Ada Perintah /api/fan?"}:::dec
        F_LOGIC["Ubah State fanOn (ON/OFF)\natau Balik Active-LOW/HIGH"]:::proc
        F_OUT[/"Tulis Level Fisik HIGH/LOW ke GPIO D23\n& Cetak Log Serial"/]:::io
        F_END(["Kipas Standby"]):::term

        F_START --> F_INIT --> F_CMD
        F_CMD -- "Ya" --> F_LOGIC --> F_OUT --> F_CMD
        F_CMD -- "Tidak" --> F_END
    end
```

---

## 📁 Struktur Kode

```text
include/
  config.h            konfigurasi pin, sudut servo, durasi gerak, ambang sensor
src/
  main.cpp            setup() dan loop() utama
  door.*              servo pintu D13/D12, kurva gerak halus, siklus buka-tahan-tutup
  fan.*               kontrol relay kipas D23 (ON/OFF & Active-LOW/HIGH)
  sensors.*           sensor api D34, sentuh D4, ultrasonik HC-SR04, suhu DHT11 D27
  automation.*        evaluasi mode pemicu otomatis & prioritas evakuasi api
  oled.*              layar SSD1306 (deteksi hot-plug, animasi wayang, layar bahaya)
  app_network.*       Access Point mandiri (Hotspot RumahJoglo-SmartHome)
  web_server.*        route HTTP & API JSON
  web/index_html.h    HTML dashboard   (PROGMEM, route /)
  web/style_css.h     CSS dashboard    (PROGMEM, route /style.css)
  web/app_js.h        JS dashboard     (PROGMEM, route /app.js)
  web/alarm_mp3.h     Audio alarm api  (PROGMEM, route /alarm.mp3)
```

### API HTTP (semua GET, respons JSON status)

| Route | Parameter | Keterangan |
| :--- | :--- | :--- |
| `/api/status` | — | Status lengkap sensor, pintu, kipas, OLED, IP, dan *uptime* |
| `/api/door` | `action=auto\|open\|close`, atau `action=set_mode&mode=0..3` | Kendali pintu & pemilihan mode pemicu (0=Manual, 1=Sentuh, 2=Api, 3=Ultrasonik) |
| `/api/fan` | `toggle=1`, `state=on\|off\|1\|0`, atau `toggle_logic=1` | Kendali relay kipas D23 dan balik logika Active-LOW/HIGH |
| `/api/flame` | `toggle_logic=1` | Balik logika sensor api (Active-HIGH / Active-LOW) |
| `/api/ultrasonic` | `swap=1`, atau `scan=1` | Tukar pin Trig/Echo atau pindai pin otomatis (`{result, trigPin, echoPin}`) |

---

## 📱 Build & Upload

1. Hubungkan ESP32 ke komputer melalui kabel USB.
2. Upload firmware via PlatformIO:
   ```bash
   pio run --target upload
   pio device monitor
   ```
3. Sambungkan HP/Laptop ke WiFi **`RumahJoglo-SmartHome`** lalu buka **`http://192.168.4.1`** di browser.

---

## 📄 Lisensi

Proyek ini dirilis di bawah lisensi [MIT License](LICENSE). Bebas digunakan, dimodifikasi, dan dikembangkan untuk keperluan edukasi dan proyek IoT pribadi.
