#pragma once

namespace cfg {

// Jaringan (Mode AP Mandiri)
constexpr const char *AP_SSID = "RumahJoglo-SmartHome";

// Pin
constexpr int PIN_SERVO = 13;        // pin servo utama D13
constexpr int PIN_SERVO_ALT = 12;    // pin servo alternatif D12 (jika dicolok ke D12)
constexpr int PIN_FLAME = 34;        // flame sensor 5-way, DO
constexpr int PIN_TOUCH = 4;         // Touch0
constexpr int PIN_TRIG_DEFAULT = 4;  // HC-SR04 (bisa ditukar dari web)
constexpr int PIN_ECHO_DEFAULT = 5;
constexpr int PIN_DHT = 27;          // DHT11 sensor suhu & kelembaban (D27)
constexpr int PIN_FAN = 23;          // Relay kipas (D23)
constexpr int PIN_SDA = 21;
constexpr int PIN_SCL = 22;

// Servo pintu
// Di miniatur ini pintu tertutup saat servo di 180 derajat dan terbuka di 0 derajat.
constexpr int SERVO_CLOSE = 180;
constexpr int SERVO_OPEN = 0;
constexpr int SERVO_MIN_US = 500;
constexpr int SERVO_MAX_US = 2400;

// Gerak halus, 0 -> 180 derajat. Durasi yang sama dipakai animasi pintu di src/web/style_css.h.
constexpr unsigned long DOOR_OPEN_MS = 5000;   // buka penuh 180 derajat
constexpr unsigned long DOOR_CLOSE_MS = 7000;  // tutup lebih pelan + mendarat lembut (DOOR_CLOSE_MS juga di app_js.h)
constexpr unsigned long DOOR_HOLD_MS = 2000;   // "Buka Pintu (2 Detik)"
constexpr unsigned long SERVO_TICK_MS = 20;    // servo membaca pulsa tiap 20 ms

// Sensor
constexpr int MAX_DISTANCE_CM = 10;
constexpr int TOUCH_DELTA_THRESHOLD = 55;        // Peka terhadap sentuhan lembut (adaptif terhadap baseline)
constexpr int TOUCH_BASELINE_FALLBACK = 1380;
// Sensor api: Mode Active-HIGH (1 = Api, 0 = Aman)
constexpr bool FLAME_ACTIVE_HIGH_DEFAULT = true;
// Relay kipas 1-channel Songle umumnya Active-LOW (LOW = nyala, HIGH = mati)
constexpr bool FAN_ACTIVE_LOW_DEFAULT = true;
// Api baru dianggap ada kalau sinyal aktif terus selama FLAME_CONFIRM_MS (lonjakan singkat diabaikan),
// dan baru dianggap padam setelah sinyal tenang selama FLAME_RELEASE_MS (alarm tidak hidup-mati).
constexpr unsigned long FLAME_CONFIRM_MS = 15;    // Reaksi sangat cepat deteksi nyala api (15 ms)
constexpr unsigned long FLAME_RELEASE_MS = 1200;  // Tahan status bahaya 1.2 detik agar stabil
constexpr unsigned long FLAME_READ_MS = 5;        // Pemantauan pin tiap 5 ms (200 Hz)
constexpr unsigned long TOUCH_READ_MS = 10;       // Sampling sentuh 100x per detik
constexpr unsigned long ULTRASONIC_PING_MS = 40;  // Sampling jarak cepat (~25x per detik)
constexpr unsigned long DHT_READ_MS = 2100;       // > 2000 ms (MIN_INTERVAL library DHT) agar selalu baca fisik
constexpr int DHT_MAX_CONSECUTIVE_FAILS = 5;      // Pertahankan nilai lama hingga 5x gagal baca berturut-turut

} // namespace cfg
