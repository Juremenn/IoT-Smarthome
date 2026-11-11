# 🏡 IoT Smart Home - Rumah Joglo
### Pintu Otomatis Servo (Gerak Halus), Sensor Api, Sensor Jarak, Sensor Sentuh, Kipas & Dashboard Web

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Project-orange?logo=platformio)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/Board-ESP32%20DevKit%20V1-red?logo=espressif)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue?logo=arduino)](https://www.arduino.cc/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B-00599C?logo=c%2B%2B)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Smart home miniatur Rumah Joglo berbasis **ESP32**. Dashboard web disajikan langsung dari ESP32 dan bisa dibuka tanpa internet.

---

## ✨ Fitur

- **Pintu servo (D13), gerak halus:** membuka 0° ke 180° dalam 3,0 s dan menutup dalam 3,9 s dengan kurva ease-in-out (mulai pelan, cepat di tengah, melambat sampai berhenti). Gerakan tidak memblokir, jadi web server tetap responsif.
- **Tiga perintah pintu:** Buka Pintu (2 Detik), Buka Terus, Tutup Rapat.
- **Pemicu otomatis (pilih satu):**
  - **Sentuh (D4):** sentuh ujung kabel jumper di D4.
  - **Sensor Api (D34):** pintu dibuka dan ditahan terbuka selama api terdeteksi (evakuasi). Tutup Rapat dikunci sampai api padam.
  - **Ultrasonik (HC-SR04):** objek ≤ 20 cm membuka pintu. Selama objek masih ada, pintu tetap terbuka; kalau objek muncul saat pintu menutup, pintu berbalik membuka.
  - **Manual:** hanya lewat tombol web.
- **Detektor api:** status Aman/Bahaya, nilai pin, logika Active-HIGH/LOW yang bisa dibalik.
- **Kipas (D32):** nyala/mati dan balik logika relay dari web.
- **OLED SSD1306 (opsional):** dideteksi otomatis lewat I2C (hot-plug).
- **WiFi:** mencoba WiFi rumah, kalau gagal menjadi hotspot `RumahJoglo-SmartHome` di `http://192.168.4.1`.

---

## 🛠️ Komponen & Wiring

| Komponen | Pin modul | Pin ESP32 |
| :--- | :--- | :--- |
| Servo SG90 / MG90S | Sinyal (oranye) | **D13** |
| | VCC / GND | 5V (VIN) / GND |
| Flame sensor 5-way (LM393) | DO | **D34** |
| | VCC / GND | 3V3 / GND |
| Sensor sentuh | Ujung kabel jumper | **D4** (Touch0) |
| HC-SR04 | Trig / Echo | **D4 / D5** (bisa ditukar atau dipindai dari web) |
| | VCC / GND | 5V / GND |
| Kipas 5V via relay/driver | IN | **D32** |
| OLED SSD1306 128x64 (opsional) | SDA / SCL | D21 / D22 |

> Catatan: sensor sentuh dan Trig HC-SR04 sama-sama memakai D4. Firmware hanya membaca sentuh di mode Sentuh dan ultrasonik di mode Ultrasonik, tapi kalau keduanya dipasang bersamaan sinyalnya bisa saling ganggu. Pindahkan HC-SR04 dengan tombol **Pindai Pin Otomatis** di dashboard.

> Flame sensor diberi daya **3V3**, karena D34 hanya tahan 3,3 V.

---

## 📁 Struktur Kode

```
include/
  config.h            pin, sudut servo, waktu gerak, ambang sensor
  secrets.example.h   contoh kredensial WiFi (masuk git)
  secrets.h           kredensial WiFi asli (TIDAK masuk git)
src/
  main.cpp            setup() dan loop() saja
  door.*              servo pintu, gerak halus, siklus buka-tahan-tutup
  sensors.*           sensor api, sentuh, ultrasonik
  automation.*        mode pemicu otomatis
  fan.*               kipas / relay
  oled.*              layar SSD1306
  network.*           WiFi station / access point
  web_server.*        route HTTP
  web/index_html.h    HTML dashboard   (PROGMEM, route /)
  web/style_css.h     CSS dashboard    (PROGMEM, route /style.css)
  web/app_js.h        JS dashboard     (PROGMEM, route /app.js)
```

### API HTTP (semua GET, respons JSON status)

| Route | Parameter |
| :--- | :--- |
| `/api/status` | |
| `/api/door` | `action=auto` / `open` / `close`, atau `action=set_mode&mode=0..3` |
| `/api/flame` | `toggle_logic=1` |
| `/api/fan` | `set=0/1`, `toggle`, `pin=N`, `toggle_logic` |
| `/api/ultrasonic` | `swap=1`, atau `scan=1` (balas `{result, trigPin, echoPin}`) |

---

## 📱 Build & Upload

1. Salin `include/secrets.example.h` menjadi `include/secrets.h`, lalu isi SSID dan password WiFi.
2. Upload:
   ```bash
   pio run --target upload
   pio device monitor
   ```
3. Buka IP yang tampil di Serial Monitor atau OLED dari browser HP/laptop.

---

## 📄 Lisensi

Proyek ini dirilis di bawah lisensi [MIT License](LICENSE). Bebas digunakan, dimodifikasi, dan dikembangkan untuk keperluan edukasi dan proyek IoT pribadi.
