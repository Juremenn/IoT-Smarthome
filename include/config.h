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

// Sensor Ultrasonik (ISR non-blocking + median-3)
constexpr int MAX_DISTANCE_CM = 10;
constexpr unsigned long ULTRASONIC_PING_MS = 45;     // trigger non-blocking tiap 45 ms (~22 Hz)
constexpr unsigned long ULTRASONIC_TIMEOUT_US = 8000; // batas echo ~135 cm (0 us blocking via ISR)
constexpr int ULTRASONIC_CONFIRM_COUNT = 2;          // 2x bacaan median berturut-turut <= 10 cm

// Sensor api: Mode Active-HIGH (1 = Api, 0 = Aman)
constexpr bool FLAME_ACTIVE_HIGH_DEFAULT = true;
constexpr unsigned long FLAME_CONFIRM_MS = 15;       // reaksi sangat cepat deteksi nyala api (15 ms)
constexpr unsigned long FLAME_RELEASE_MS = 1200;     // tahan status bahaya 1.2 detik agar stabil
constexpr unsigned long FLAME_READ_MS = 5;           // pemantauan pin tiap 5 ms (200 Hz)

// Sensor DHT11 (Core 0 task anti self-heating + kalibrasi + EMA)
constexpr unsigned long DHT_READ_MS = 3000;          // 3 detik: mencegah self-heating DHT11
constexpr int DHT_MAX_CONSECUTIVE_FAILS = 3;         // pertahankan nilai lama hingga 3x gagal; >3x tandai gagal
constexpr float DHT_MAX_TEMP_JUMP_C = 3.0f;          // tolak spike suhu > 3.0 C dalam 1 siklus
constexpr float DHT_MAX_HUM_JUMP_PCT = 15.0f;        // tolak spike kelembapan > 15% dalam 1 siklus
constexpr int DHT_OUTLIER_CONFIRM_LIMIT = 2;         // maks 2x tolak; ke-3 konsisten diterima (saat pindah ruangan)
constexpr float DHT_EMA_ALPHA = 0.2f;                // filter EMA (0.2 baru + 0.8 lama)

// Konstanta Kalibrasi DHT11 (default 0.0 agar bisa disetel live via /api/calibrate & NVS)
constexpr bool DHT_USE_TWO_POINT_CAL = false;        // true = aktifkan 2-point linear suhu di bawah
constexpr float DHT_RAW_LOW = 20.0f;                 // titik raw rendah (misal ruangan ber-AC)
constexpr float DHT_REF_LOW = 20.0f;                 // suhu termometer acuan di titik rendah
constexpr float DHT_RAW_HIGH = 35.0f;                // titik raw tinggi (misal ruangan panas)
constexpr float DHT_REF_HIGH = 35.0f;                // suhu termometer acuan di titik tinggi
constexpr float DHT_TEMP_OFFSET_DEFAULT = 0.0f;      // offset suhu default (C), bisa diubah live via NVS
constexpr float DHT_HUM_OFFSET_DEFAULT = 0.0f;       // offset kelembapan default (%), bisa diubah live via NVS

// OLED refresh (hemat waktu I2C agar Core 1 responsif)
constexpr unsigned long OLED_REFRESH_MS = 500;       // 2 FPS normal (hemat ~450 ms/detik I2C)
constexpr unsigned long OLED_FIRE_REFRESH_MS = 180;  // kedip cepat saat darurat api

} // namespace cfg
