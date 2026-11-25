#pragma once

namespace cfg {

// Jaringan (Mode AP Mandiri)
constexpr const char *AP_SSID = "RumahJoglo-SmartHome";

// Pin (100% sesuai Flowchart_iot_final)
constexpr int PIN_SERVO = 13;        // Servo pintu utama D13
constexpr int PIN_SERVO_ALT = 12;    // Mirror PWM D12 (cadangan jika kabel servo tercolok di D12)
constexpr int PIN_FLAME = 34;        // Sensor api 5-way, DO
constexpr int PIN_TRIG = 25;         // Ultrasonik HC-SR04 Trig (D25)
constexpr int PIN_ECHO = 26;         // Ultrasonik HC-SR04 Echo (D26)
constexpr int PIN_DHT = 27;          // Sensor DHT11 suhu & kelembaban (D27)
constexpr int PIN_SDA = 21;          // OLED SSD1306 SDA
constexpr int PIN_SCL = 22;          // OLED SSD1306 SCL

// Servo pintu
// Di miniatur ini pintu tertutup saat servo di 180 derajat dan terbuka di 0 derajat.
constexpr int SERVO_CLOSE = 180;
constexpr int SERVO_OPEN = 0;
constexpr int SERVO_MIN_US = 500;
constexpr int SERVO_MAX_US = 2400;

// Gerak halus, 0 -> 180 derajat.
constexpr unsigned long DOOR_OPEN_MS = 7000;       // buka penuh sehalus menutup (soft-start & soft-end)
constexpr unsigned long DOOR_CLOSE_MS = 7000;      // tutup pelan + mendarat lembut
constexpr unsigned long DOOR_EVAC_OPEN_MS = 1800;  // buka cepat khusus darurat api (tetap halus)
constexpr unsigned long DOOR_HOLD_MS = 2000;       // "Buka Pintu (2 Detik)"
constexpr unsigned long SERVO_TICK_MS = 20;        // servo membaca pulsa tiap 20 ms (50 Hz)

// Sensor
constexpr int MAX_DISTANCE_CM = 10;
// Sensor api: Mode Active-HIGH (1 = Api, 0 = Aman)
constexpr bool FLAME_ACTIVE_HIGH_DEFAULT = true;
// Api baru dianggap ada kalau sinyal aktif terus selama FLAME_CONFIRM_MS (lonjakan singkat diabaikan),
// dan baru dianggap padam setelah sinyal tenang selama FLAME_RELEASE_MS (alarm tidak hidup-mati).
constexpr unsigned long FLAME_CONFIRM_MS = 15;    // Reaksi sangat cepat deteksi nyala api (15 ms)
constexpr unsigned long FLAME_RELEASE_MS = 1200;  // Tahan status bahaya 1.2 detik agar stabil
constexpr unsigned long FLAME_READ_MS = 5;        // Pemantauan pin tiap 5 ms (200 Hz)
constexpr unsigned long ULTRASONIC_PING_MS = 30;  // Sampling jarak responsif (~33x per detik)
constexpr unsigned long DHT_READ_MS = 2100;       // > 2000 ms (MIN_INTERVAL library DHT) agar selalu baca fisik
constexpr int DHT_MAX_CONSECUTIVE_FAILS = 5;      // Pertahankan nilai lama hingga 5x gagal baca berturut-turut

} // namespace cfg
