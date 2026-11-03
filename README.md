# 🏡 IoT Smart Home - Rumah Joglo
### Sistem Pintu Otomatis Dual Servo, Sensor Api (Flame Detector), Kipas Pendingin 5V & Web Dashboard

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Project-orange?logo=platformio)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/Board-ESP32%20DevKit%20V1-red?logo=espressif)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue?logo=arduino)](https://www.arduino.cc/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B-00599C?logo=c%2B%2B)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Proyek sistem **Smart Home miniatur Rumah Joglo** berbasis mikrokontroler **ESP32**. Sistem ini mengintegrasikan:
1. Pembukaan dan penutupan dua daun pintu otomatis model kupu-tarung menggunakan dual motor servo simetris.
2. Deteksi jarak tangan *touchless* menggunakan sensor ultrasonik HC-SR04 (5V).
3. **Deteksi Kebakaran / Api (Flame Detector)** di pin **D34** dengan otomasi evakuasi darurat (pintu otomatis terbuka penuh saat ada api).
4. Pengendali sirkulasi udara kipas pendingin 5V (*cooling fan*).
5. Tampilan antarmuka grafis pada **Layar OLED SSD1306 (I2C)**.
6. **Website Dashboard Interaktif** yang responsif dan dapat diakses dari smartphone atau laptop melalui WiFi.

---

## ✨ Fitur Utama

- 🔥 **Deteksi Api Darurat (Flame Detector DO di D34):**
  - Pemantauan sensor api secara *real-time*.
  - **Prosedur Evakuasi Otomatis:** Saat api terdeteksi, kedua servo pintu langsung terbuka penuh secara darurat, layar OLED menampilkan peringatan `! PERINGATAN API !`, dan website menampilkan status bahaya merah berkedip.
  - Opsi balik logika sensor (Active-LOW / Active-HIGH) langsung dari web dashboard.
- ✋ **Deteksi Non-Kontak (*Touchless*):** Sensor ultrasonik HC-SR04 mendeteksi tangan $\le$ 20 cm untuk membuka pintu selama 2 detik lalu menutup rapat kembali. Dilengkapi proteksi *anti-stuck*.
- 🚪 **Dual Servo Kupu-Tarung Simetris:**
  - **Pintu 1 (Pin D13):** Tertutup `0°` $\rightarrow$ Terbuka `90°`
  - **Pintu 2 (Pin D12):** Tertutup `180°` $\rightarrow$ Terbuka `90°`
- 💨 **Kipas Pendingin 5V (*Cooling Fan*):** Kendali ON/OFF kipas di pin **D32** via Web Dashboard, dilengkapi fitur pengujian tegangan langsung.
- 🌐 **Web Server Dashboard Modern:**
  - Status live Sensor Api (D34), Sensor Ultrasonik (HC-SR04), Status Pintu, dan Kipas.
  - Mode koneksi WiFi Rumah (`WIFI_SSID = "Nirvana"`) dengan *fallback* otomatis ke Access Point mandiri (`RumahJoglo-SmartHome`).
- 📟 **Layar OLED SSD1306 (I2C):**
  - Menampilkan status standby, jarak tangan, status api `Api: AMAN` / `Api: BAHAYA!`, dan IP Web Server.

---

## 🛠️ Komponen & Hardware

| No | Komponen | Tipe / Spesifikasi | Pin ESP32 | Keterangan |
| :---: | :--- | :--- | :--- | :--- |
| 1 | **Mikrokontroler** | ESP32 DOIT DevKit V1 | - | Otak pemroses utama & Web Server |
| 2 | **Sensor Api** | Flame Detector Module (LM393) | **GPIO 34 (D34)** | Sinyal Digital Output (DO) deteksi api |
| 3 | **Sensor Jarak** | HC-SR04 Ultrasonic Sensor | **Trig: D5, Echo: D18** | Deteksi jarak tangan / tamu (5V) |
| 4 | **Aktuator Pintu** | 2x Micro Servo SG90 / MG90S | **S1: D13, S2: D12** | Penggerak pintu kupu-tarung |
| 5 | **Kipas Pendingin** | Mini Cooling Fan 5V DC | **GPIO 32 (D32)** | Sirkulasi udara via relay / driver |
| 6 | **Layar Teks / GUI** | OLED 0.96 inch SSD1306 (I2C) | **SDA: D21, SCL: D22** | Tampilan status teks & visual |
| 7 | **Sumber Daya** | 5V DC (Kabel USB / Adaptor) | **VIN & GND** | Suplai daya 5V untuk seluruh modul |

---

## 🔌 Skema Pin & Wiring Diagram

### 1. Sensor Api (Flame Detector)
| Pin Modul Sensor Api | Pin ESP32 | Keterangan |
| :--- | :--- | :--- |
| **VCC** | **3V3 atau 5V / VIN** | Daya modul |
| **GND** | **GND** | Ground |
| **DO (Digital Output / S)** | **GPIO 34 (D34)** | Sinyal deteksi api ke ESP32 |

### 2. Sensor Ultrasonik HC-SR04
| Pin HC-SR04 | Pin ESP32 | Keterangan |
| :--- | :--- | :--- |
| **VCC** | **5V / VIN** | Wajib daya 5 Volt |
| **GND** | **GND** | Ground |
| **TRIG** | **GPIO 5 (D5)** | Trigger pulsa ultrasonik |
| **ECHO** | **GPIO 18 (D18)** | Echo pantulan pulsa |

### 3. Dual Motor Servo (SG90)
| Pin Servo | Warna Kabel | Pin ESP32 | Keterangan |
| :--- | :--- | :--- | :--- |
| **Servo 1 (Signal)** | Oranye / Kuning | **GPIO 13 (D13)** | Kontrol PWM Pintu 1 |
| **Servo 2 (Signal)** | Oranye / Kuning | **GPIO 12 (D12)** | Kontrol PWM Pintu 2 |
| **VCC (Keduanya)** | Merah | **5V / VIN** | Daya motor servo |
| **GND (Keduanya)** | Cokelat / Hitam | **GND** | Ground bersama (*common ground*) |

### 4. Layar OLED SSD1306 I2C (128x64)
| Pin OLED | Pin ESP32 | Keterangan |
| :--- | :--- | :--- |
| **GND** | **GND** | Ground |
| **VCC** | **3V3** | Tegangan logika 3.3V |
| **SCL** | **GPIO 22 (D22)** | Jalur I2C Clock |
| **SDA** | **GPIO 21 (D21)** | Jalur I2C Data |

### 5. Kipas Pendingin 5V
| Pin Kipas / Driver | Pin ESP32 | Keterangan |
| :--- | :--- | :--- |
| **IN / Trigger Driver** | **GPIO 32 (D32)** | Sinyal kendali saklar kipas |
| **VCC Kipas (+)** | **5V / VIN** | Daya 5 Volt |
| **GND Kipas (-)** | **GND** (via relay) | Ground |

---

## 📱 Panduan Akses Web Dashboard

1. **Upload firmware ke ESP32:**
   ```bash
   pio run --target upload
   ```
2. Lihat alamat IP yang tertera di layar OLED atau Serial Monitor (contoh: `http://192.168.1.xxx` atau `http://192.168.4.1`).
3. Buka alamat tersebut di browser HP atau laptop.
4. Dashboard Smart Home Rumah Joglo siap digunakan secara *real-time*.

---

## 📄 Lisensi

Proyek ini dirilis di bawah lisensi [MIT License](LICENSE). Bebas digunakan, dimodifikasi, dan dikembangkan untuk keperluan edukasi dan proyek IoT pribadi.
