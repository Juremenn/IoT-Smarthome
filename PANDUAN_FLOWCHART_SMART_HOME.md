# Panduan Flowchart Lengkap Sistem IoT Smart Home Miniatur Rumah Joglo (ESP32)

Dokumen ini merupakan panduan perancangan flowchart **Hitam Putih standar ISO 5807** dengan tata letak **menyamping per fitur lalu mengalir ke bawah dari `Start` hingga `End`**, selaras dengan tampilan Web Dashboard (**Perangkat** & **Info Pinout**) serta perilaku perangkat keras miniatur Rumah Joglo:
- **Tanpa kontrol kipas/fan** (kipas berjalan otomatis / tidak dikontrol melalui website).
- **Gerakan pintu servo:** saat **membuka** bergerak **cepat/normal langsung ke `0°`**, dan saat **menutup** bergerak **pelan & halus (*soft-close*) ke `180°`**.

Seluruh isi dokumen ini 100% sinkron dengan file siap buka **`FLOWCHART_SMART_HOME_RUMAH_JOGLO.drawio`**.

---

## Daftar Isi

1. [Kamus Simbol Standar ISO 5807 (Hitam Putih)](#1-kamus-simbol-standar-iso-5807-hitam-putih)
2. [Flowchart Lengkap Sistem Keseluruhan (Menyamping 4 Fitur → `End`)](#2-flowchart-lengkap-sistem-keseluruhan-menyamping-4-fitur--end)
3. [Flowchart Fitur 1: Deteksi Api & Evakuasi Darurat (`Start` → `End`)](#3-flowchart-fitur-1-deteksi-api--evakuasi-darurat-start--end)
4. [Flowchart Fitur 2: Kendali Pintu Servo — Buka Cepat & Tutup Pelan Halus (`Start` → `End`)](#4-flowchart-fitur-2-kendali-pintu-servo--buka-cepat--tutup-pelan-halus-start--end)
5. [Flowchart Fitur 3: Monitoring Suhu DHT11 & Layar OLED (`Start` → `End`)](#5-flowchart-fitur-3-monitoring-suhu-dht11--layar-oled-start--end)
6. [Flowchart Fitur 4: Alur Web Dashboard — Perangkat & Info Pinout (`Start` → `End`)](#6-flowchart-fitur-4-alur-web-dashboard--perangkat--info-pinout-start--end)
7. [Referensi Cepat Pin GPIO & Fitur Web Dashboard](#7-referensi-cepat-pin-gpio--fitur-web-dashboard)

---

## 1. Kamus Simbol Standar ISO 5807 (Hitam Putih)

Gunakan gaya standar laporan/skripsi: **isi kotak putih (`#FFFFFF`), garis tepi hitam (`#000000`, `1,5 pt`), teks hitam (`Arial` 10–11 pt), dan panah hitam**.

| No | Nama Simbol (ISO 5807) | Nama Bentuk di Draw.io / Visio / Word | Bentuk Fisik | Fungsi dalam Flowchart | Contoh Isi |
| :-: | :--- | :--- | :--- | :--- | :--- |
| 1 | **Terminator** | `Terminator` / `Start/End` | Oval / kapsul | Menandai titik awal (`Start`) dan titik akhir (`End`) diagram | `Start`, `End` |
| 2 | **Preparation** | `Preparation` | Segi enam (heksagon) | Inisialisasi awal hardware ESP32, pin GPIO, Wi-Fi AP, dan Web Server | `Inisialisasi Hardware ESP32...` |
| 3 | **Input / Output (Data)** | `Data` / `Input-Output` | Jajaran genjang | Pembacaan sensor (`D34`, `D4/D5`, `D27`), pengiriman pulsa PWM servo (`D13/D12`), serta request/respons HTTP JSON | `Baca Seluruh Sensor...` / `Kirim PWM Servo` |
| 4 | **Display** | `Display` | Bentuk peluru (kiri melengkung runcing, kanan melengkung bulat) | Menampilkan informasi visual ke pengguna pada **Layar OLED SSD1306** atau **Web Dashboard** | `OLED Berkedip "! PERINGATAN API !"` |
| 5 | **Process** | `Process` | Persegi panjang | Pengolahan logika internal, filter anti-flicker, perhitungan *soft-close*, dan eksekusi mode | `Tutup Pintu Pelan & Halus (Soft-Close)` |
| 6 | **Decision** | `Decision` | Belah ketupat (*diamond*) | Percabangan logika dengan label cabang (`Ya`/`Tidak` atau pilihan tombol/menu) | `Sensor Api (D34) Mendeteksi Api?` |
| 7 | **Delay** | `Delay` | Bentuk huruf **D** | Menunjukkan jeda waktu tunggu eksplisit | `Tahan Pintu Terbuka (2 Detik)` |
| 8 | **Flowline** | `Arrow` | Garis panah lurus | Menunjukkan urutan arah alur menyamping lalu ke bawah hingga `End` | Label: `Ya`, `Tidak` |

---

## 2. Flowchart Lengkap Sistem Keseluruhan (Menyamping 4 Fitur → `End`)

*(Sesuai **Tab 1: `1. Flowchart Lengkap (Start-End)`** pada file `FLOWCHART_SMART_HOME_RUMAH_JOGLO.drawio`)*

Tata letak dibuat **menyamping terlebih dahulu lalu turun ke bawah** agar tidak memanjang dan mudah dibaca sekali lihat:
- **Baris Atas (Kiri → Kanan):** `Start` → Inisialisasi Hardware → Aktifkan Wi-Fi AP & Web Server → Splash OLED → Baca Seluruh Sensor & Request Web.
- **4 Kolom Fitur Menyamping (Sejajar Kiri → Kanan):**
  - **Kolom 1:** Fitur Deteksi Api & Evakuasi Darurat (Prioritas 1)
  - **Kolom 2:** Fitur Kontrol Web Dashboard (`Buka 2 detik`, `Buka terus`, `Tutup`, `Ganti Polaritas Api`)
  - **Kolom 3:** Fitur Pintu Otomatis Ultrasonik (`HC-SR04`, buka cepat `0°`, tahan `2s`, tutup pelan halus `180°`, anti-terjepit)
  - **Kolom 4:** Fitur Suhu & Kelembaban `DHT11` + Layar `OLED SSD1306`
- **Baris Bawah (Gabung ke Tengah → `End`):** Kirim Sinyal PWM Servo → Perbarui Web Dashboard → Keputusan `Power OFF?` → `End`.

### Tabel Cetak Biru Langkah demi Langkah

| Bagian | ID | Nama Simbol ISO 5807 | Isi Teks Bentuk | Arah Panah / Cabang |
| :--- | :--- | :--- | :--- | :--- |
| **Baris Atas** | `M01` | **Terminator** | `Start (ESP32 ON)` | → `M02` (kanan) |
| **Baris Atas** | `M02` | **Preparation** | `Inisialisasi Hardware ESP32: Servo D13/D12 (Tutup 180°), Api D34, Ultrasonik D4/D5, DHT11 D27, OLED D21/D22` | → `M03` (kanan) |
| **Baris Atas** | `M03` | **Preparation** | `Aktifkan Wi-Fi AP Mandiri ("RumahJoglo-SmartHome", 192.168.4.1) & Web Server HTTP Port 80` | → `M04` (kanan) |
| **Baris Atas** | `M04` | **Display** | `Tampilkan Status "Sistem Siap" & Alamat IP pada Layar OLED` | → `M05` (kanan) |
| **Baris Atas** | `M05` | **Input / Output** | `Baca Seluruh Sensor (Api D34, HC-SR04, DHT11 D27) & Terima Request Web (/api/*)` | → menyebar ke **Kolom 1, 2, 3, 4** |
| **Kolom 1 (Api)** | `C1_1` | **Decision** | `Sensor Api (D34) Mendeteksi Api? (Filter >= 15 ms)` | **Ya** → `C1_2`<br>**Tidak** → `M06` |
| **Kolom 1 (Api)** | `C1_2` | **Process** | `Mode Evakuasi Darurat: Paksa Buka Penuh Pintu (0°) & Kunci Tombol "Tutup" Web` | → `C1_3` |
| **Kolom 1 (Api)** | `C1_3` | **Input / Output** | `Kirim PWM Servo (D13/D12) Buka Cepat Penuh ke 0°` | → `C1_4` |
| **Kolom 1 (Api)** | `C1_4` | **Display** | `OLED Berkedip "! PERINGATAN API !" & Web Putar Alarm (/alarm.mp3)` | → `C1_5` |
| **Kolom 1 (Api)** | `C1_5` | **Decision** | `Api Sudah Padam & Tenang >= 1,2 Detik?` | **Tidak** → `C1_3`<br>**Ya** → `C1_6` |
| **Kolom 1 (Api)** | `C1_6` | **Process** | `Nonaktifkan Evakuasi, Matikan Alarm, & Set Timer Tutup (+2 Detik)` | → `M06` |
| **Kolom 2 (Web)** | `C2_1` | **Decision** | `Pengguna Menekan Tombol di Web Dashboard?` | **Ya** → `C2_2`<br>**Tidak** → `M06` |
| **Kolom 2 (Web)** | `C2_2` | **Decision** | `Tombol Apa yang Ditekan?` | **Buka 2d** → `C2_3`<br>**Buka Terus** → `C2_4`<br>**Tutup** → `C2_5`<br>**Polaritas** → `C2_6` |
| **Kolom 2 (Web)** | `C2_3` | **Process** | `Buka 2 Detik: Buka Cepat ke 0°, Tahan 2s, lalu Tutup Pelan Halus ke 180°` | → `M06` |
| **Kolom 2 (Web)** | `C2_4` | **Process** | `Buka Terus: Buka Cepat ke 0° & Tahan Terbuka (Abaikan Sensor Ultrasonik)` | → `M06` |
| **Kolom 2 (Web)** | `C2_5` | **Process** | `Tutup (Bila Tidak Ada Api): Tutup Pintu Pelan Halus ke 180°` | → `M06` |
| **Kolom 2 (Web)** | `C2_6` | **Process** | `Ganti Polaritas Api: Ubah Active-HIGH ↔ Active-LOW` | → `M06` |
| **Kolom 3 (Auto)** | `C3_1` | **Decision** | `Pintu Mode "Buka Terus" atau Sedang Evakuasi?` | **Ya** → `M06`<br>**Tidak** → `C3_2` |
| **Kolom 3 (Auto)** | `C3_2` | **Input / Output** | `Hitung Jarak HC-SR04 (D4/D5) = (Durasi × 0,034) / 2 cm` | → `C3_3` |
| **Kolom 3 (Auto)** | `C3_3` | **Decision** | `Objek Terdeteksi? (Jarak <= Ambang Pemicu)` | **Ya** → `C3_4`<br>**Tidak** → `C3_5` |
| **Kolom 3 (Auto)** | `C3_4` | **Process** | `Buka Pintu Cepat ke 0° / Perpanjang Tahan 2s / Anti-Terjepit Balik Buka` | → `M06` |
| **Kolom 3 (Auto)** | `C3_5` | **Decision** | `Pintu Terbuka Otomatis & Tahan 2 Detik Habis?` | **Ya** → `C3_6`<br>**Tidak** → `M06` |
| **Kolom 3 (Auto)** | `C3_6` | **Process** | `Tutup Pintu Pelan & Halus (Soft-Close) ke 180°` | → `M06` |
| **Kolom 4 (Suhu)** | `C4_1` | **Decision** | `Pintu Diam & Jeda Baca DHT11 >= 2,1 Detik?` | **Ya** → `C4_2`<br>**Tidak** → `C4_6` |
| **Kolom 4 (Suhu)** | `C4_2` | **Input / Output** | `Baca Suhu (°C) & Kelembaban (%) dari Sensor DHT11 (Pin D27)` | → `C4_3` |
| **Kolom 4 (Suhu)** | `C4_3` | **Decision** | `Data DHT11 Valid? (-10..80 °C & 0..100%)` | **Ya** → `C4_4`<br>**Tidak** → `C4_5` |
| **Kolom 4 (Suhu)** | `C4_4` | **Process** | `Simpan Suhu & Kelembaban Baru (Reset Penghitung Gagal)` | → `C4_6` |
| **Kolom 4 (Suhu)** | `C4_5` | **Process** | `Pertahankan Nilai Terakhir (Toleransi Maks 5x Gagal)` | → `C4_6` |
| **Kolom 4 (Suhu)** | `C4_6` | **Display** | `OLED SSD1306 (D21/D22 Hot-Plug): Tampilkan IP, Wayang, Suhu, & API: AMAN` | → `M07` |
| **Baris Bawah** | `M06` | **Input / Output** | `Kirim Sinyal PWM Servo Pintu (D13 & D12: Buka Cepat 0° / Tutup Pelan Halus 180°)` | → `M07` (kanan) |
| **Baris Bawah** | `M07` | **Display** | `Perbarui Web Dashboard (Tab Perangkat: 5 Kartu & Gauge Jarak, serta Tab Info Pinout)` | → `M08` |
| **Baris Bawah** | `M08` | **Decision** | `Apakah Sistem Dimatikan (Power OFF)?` | **Tidak** → `M05`<br>**Ya** → `M09` |
| **Baris Bawah** | `M09` | **Terminator** | `End (Sistem Berhenti)` | *(Selesai)* |

---

## 3. Flowchart Fitur 1: Deteksi Api & Evakuasi Darurat (`Start` → `End`)

*(Sesuai **Tab 2: `2. Fitur Deteksi Api`** pada file `FLOWCHART_SMART_HOME_RUMAH_JOGLO.drawio`)*

| No | ID | Simbol ISO 5807 | Isi Teks | Arah Panah |
| :-: | :--- | :--- | :--- | :--- |
| 1 | `A01` | **Terminator** | `Start` | → `A02` |
| 2 | `A02` | **Preparation** | `Inisialisasi Sensor Api D34 (Default Active-HIGH)` | → `A03` |
| 3 | `A03` | **Decision** | `Ubah Polaritas di Web?` | **Ya** → `A03A`<br>**Tidak** → `A04` |
| 4 | `A03A` | **Process** | `Balik Polaritas Api (Active-HIGH ↔ Active-LOW)` | → `A04` |
| 5 | `A04` | **Input / Output** | `Baca Sinyal Digital Sensor Api D34 (Tiap 5 ms)` | → `A05` |
| 6 | `A05` | **Process** | `Filter Anti-Flicker (Aktif >= 15 ms / Streak >= 2)` | → `A06` |
| 7 | `A06` | **Decision** | `Apakah Api Terdeteksi?` | **Ya** → `A07`<br>**Tidak** → `A12` |
| 8 | `A07` | **Process** | `Mode Evakuasi Darurat: Paksa Buka Penuh Pintu (0°) & Kunci Tombol "Tutup" Web` | → `A08` |
| 9 | `A08` | **Input / Output** | `Kirim PWM Servo (D13/D12) Buka Cepat ke 0°` | → `A09` |
| 10 | `A09` | **Display** | `OLED Berkedip "! PERINGATAN API !" & Web Putar Alarm (/alarm.mp3)` | → `A10` |
| 11 | `A10` | **Decision** | `Api Sudah Padam & Tenang >= 1,2 Detik?` | **Tidak** → `A04`<br>**Ya** → `A11` |
| 12 | `A11` | **Process** | `Nonaktifkan Evakuasi, Hentikan Alarm, & Set Timer Tutup Pelan Halus (+2 Detik)` | → `A13` |
| 13 | `A12` | **Display** | `Tampilkan Status "Aman / Normal" pada OLED & Web Dashboard` | → `A13` |
| 14 | `A13` | **Terminator** | `End` | *(Selesai)* |

---

## 4. Flowchart Fitur 2: Kendali Pintu Servo — Buka Cepat & Tutup Pelan Halus (`Start` → `End`)

*(Sesuai **Tab 3: `3. Fitur Pintu Servo`** pada file `FLOWCHART_SMART_HOME_RUMAH_JOGLO.drawio`)*

| No | ID | Simbol ISO 5807 | Isi Teks | Arah Panah |
| :-: | :--- | :--- | :--- | :--- |
| 1 | `P01` | **Terminator** | `Start` | → `P02` |
| 2 | `P02` | **Preparation** | `Inisialisasi Servo D13/D12 (Tertutup 180°) & Ultrasonik HC-SR04 (D4/D5)` | → `P03` |
| 3 | `P03` | **Decision** | `Sumber Perintah Pintu?` | **Buka Terus** → `P04A`<br>**Buka 2d** → `P04B`<br>**Ultrasonik** → `P04C`<br>**Tutup** → `P04D` |
| 4 | `P04A` | **Input / Output** | `Tombol Web: "Buka Terus"` | → `P05A` |
| 5 | `P05A` | **Process** | `Buka Cepat ke 0° & Tahan Terbuka Terus` | → `P11` |
| 6 | `P04B` | **Input / Output** | `Tombol Web: "Buka 2 detik"` | → `P06` |
| 7 | `P04C` | **Input / Output** | `Sensor Ultrasonik: Ukur Jarak (cm)` | → `P05C` |
| 8 | `P05C` | **Decision** | `Jarak > 0 cm & <= Ambang Pemicu?` | **Ya** → `P06`<br>**Tidak** → `P12` |
| 9 | `P04D` | **Input / Output** | `Tombol Web: "Tutup"` | → `P08` |
| 10 | `P06` | **Input / Output** | `Buka Pintu Cepat / Biasa ke Sudut 0° (Servo D13/D12)` | → `P07` |
| 11 | `P07` | **Delay** | `Tahan Pintu Terbuka (2 Detik) (Diperpanjang Bila Objek Masih Ada)` | → `P08` |
| 12 | `P08` | **Process** | `Tutup Pintu Pelan & Halus (Soft-Close) dari 0° ke 180°` | → `P09` |
| 13 | `P09` | **Decision** | `Ada Objek Saat Pintu Menutup?` | **Ya (Anti-Terjepit)** → `P06`<br>**Tidak** → `P10` |
| 14 | `P10` | **Input / Output** | `Kirim PWM Halus Hingga Pintu Tertutup Rapat (180°)` | → `P11` |
| 15 | `P11` | **Display** | `Perbarui Status Pintu di Web Dashboard (Tertutup / Terbuka / Otomatis)` | → `P12` |
| 16 | `P12` | **Terminator** | `End` | *(Selesai)* |

---

## 5. Flowchart Fitur 3: Monitoring Suhu DHT11 & Layar OLED (`Start` → `End`)

*(Sesuai **Tab 4: `4. Fitur DHT11 & OLED`** pada file `FLOWCHART_SMART_HOME_RUMAH_JOGLO.drawio`)*

| No | ID | Simbol ISO 5807 | Isi Teks | Arah Panah |
| :-: | :--- | :--- | :--- | :--- |
| 1 | `D01` | **Terminator** | `Start` | → `D02` |
| 2 | `D02` | **Preparation** | `Inisialisasi DHT11 (D27) & Bus I2C OLED (D21/D22)` | → `D03` |
| 3 | `D03` | **Decision** | `Pintu Diam & Jeda DHT11 >= 2,1 Detik?` | **Ya** → `D04`<br>**Tidak** → `D07` |
| 4 | `D04` | **Input / Output** | `Baca Suhu (°C) & Kelembaban (%) dari DHT11 (Pin D27)` | → `D05` |
| 5 | `D05` | **Decision** | `Data Valid (-10..80 °C & 0..100%)?` | **Ya** → `D06A`<br>**Tidak** → `D06B` |
| 6 | `D06A` | **Process** | `Simpan Suhu & Kelembaban Baru (Reset Penghitung Gagal)` | → `D07` |
| 7 | `D06B` | **Process** | `Pertahankan Nilai Terakhir (Toleransi Maks 5x Gagal)` | → `D07` |
| 8 | `D07` | **Decision** | `Layar OLED Aktif (0x3C / 0x3D)?` | **Ya** → `D08A`<br>**Tidak** → `D08B` |
| 9 | `D08A` | **Display** | `Tampilkan IP, Animasi Wayang, Suhu °C, Lembab %, & "API: AMAN" di OLED` | → `D09` |
| 10 | `D08B` | **Process** | `Scan Ulang I2C Otomatis Tiap 1500 ms (Hot-Plug)` | → `D09` |
| 11 | `D09` | **Display** | `Perbarui Kartu "Suhu Ruang" & Progress Bar pada Web Dashboard` | → `D10` |
| 12 | `D10` | **Terminator** | `End` | *(Selesai)* |

---

## 6. Flowchart Fitur 4: Alur Web Dashboard — Perangkat & Info Pinout (`Start` → `End`)

*(Sesuai **Tab 5: `5. Alur Web Dashboard`** pada file `FLOWCHART_SMART_HOME_RUMAH_JOGLO.drawio`)*

| No | ID | Simbol ISO 5807 | Isi Teks | Arah Panah |
| :-: | :--- | :--- | :--- | :--- |
| 1 | `W01` | **Terminator** | `Start` | → `W02` |
| 2 | `W02` | **Preparation** | `Konek Wi-Fi "RumahJoglo-SmartHome" & Buka http://192.168.4.1` | → `W03` |
| 3 | `W03` | **Display** | `Muat Halaman Dashboard & Unlock Audio Browser` | → `W04` |
| 4 | `W04` | **Input / Output** | `Kirim GET /api/status ke ESP32 (Polling Tiap 1000 ms)` | → `W05` |
| 5 | `W05` | **Decision** | `Tab Menu yang Dipilih?` | **Perangkat** → `W06A`<br>**Info Pinout** → `W06B` |
| 6 | `W06A` | **Display** | `Tab "Perangkat" (5 Kartu): Pintu, Sensor Api, Pemicu Pintu, Jarak Halangan (Gauge), & Suhu Ruang` | → `W07` |
| 7 | `W06B` | **Display** | `Tab "Info Pinout": Diagram Alur Otomasi & Evakuasi Api, serta Spesifikasi Pin GPIO` | → `W07` |
| 8 | `W07` | **Decision** | `Status Api == TRUE (Bahaya)?` | **Ya** → `W07A`<br>**Tidak** → `W08` |
| 9 | `W07A` | **Display** | `Alert Merah "BAHAYA: API!" & Putar Alarm (/alarm.mp3)` | → `W08` |
| 10 | `W08` | **Decision** | `Pengguna Menekan Tombol (Pintu / Polaritas Api)?` | **Ya** → `W08A`<br>**Tidak** → `W09` |
| 11 | `W08A` | **Input / Output** | `Kirim GET (/api/door atau /api/flame) & Terima JSON Terbaru` | → `W09` |
| 12 | `W09` | **Delay** | `Tunggu Interval Polling (1000 ms)` | → `W10` |
| 13 | `W10` | **Decision** | `Tutup Halaman Dashboard?` | **Tidak** → `W04`<br>**Ya** → `W11` |
| 14 | `W11` | **Terminator** | `End` | *(Selesai)* |

---

## 7. Referensi Cepat Pin GPIO & Fitur Web Dashboard

| Komponen / Kartu Web | Pin GPIO | Parameter & Perilaku Sistem |
| :--- | :--- | :--- |
| **Pintu (Servo Utama & Paralel)** | `GPIO 13` & `GPIO 12` | PWM `50 Hz` (`500–2400 µs`), Tertutup `180°`, Terbuka `0°`, **buka cepat/biasa**, tahan `2 detik`, **tutup pelan & halus (*soft-close*)**; Tombol Web: `Buka 2 detik`, `Buka terus`, `Tutup` |
| **Sensor Api 5-Way (LM393 DO)** | `GPIO 34` (3,3 V) | Default `Active-HIGH` (tombol `Ganti: Active-LOW`), sampling `5 ms`, filter `15 ms`, rilis `1200 ms`, evakuasi global + alarm `/alarm.mp3` |
| **Pemicu Pintu & Jarak Halangan (HC-SR04)** | `Trig D4`, `Echo D5` | Sonar `40 kHz`, pulsa `10 µs`, ping `40 ms`, ambang pemicu `≤ 10 cm` (firmware) / `≤ 15 cm` (UI), gauge 4-segmen (`Sangat Dekat`, `Mendekat`, `Sedang`, `Standby`) |
| **Suhu Ruang & Kelembaban (DHT11)** | `GPIO 27` | Single-wire, interval `2100 ms` (ditunda saat servo bergerak), valid `-10..80 °C` & `0..100%`, toleransi `5x` gagal |
| **Layar OLED SSD1306 128×64** | `SDA D21`, `SCL D22` | I2C `0x3C`/`0x3D`, hot-plug scan `1500 ms`, refresh `50 ms` (~20 FPS), menampilkan IP, Wayang, Suhu, Kelembaban, & Peringatan Api berkedip |
| **Wi-Fi AP & Web Dashboard** | Port `80` | SSID `"RumahJoglo-SmartHome"`, IP `192.168.4.1`, Tab **Perangkat** (5 kartu) & Tab **Info Pinout**, polling `1000 ms` |
