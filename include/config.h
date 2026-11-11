#pragma once

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

namespace cfg {

// Jaringan
constexpr const char *AP_SSID = "RumahJoglo-SmartHome";
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 6000;
constexpr unsigned long WIFI_CHECK_INTERVAL_MS = 10000;

// Pin
constexpr int PIN_SERVO = 13;        // pin servo utama D13
constexpr int PIN_SERVO_ALT = 12;    // pin servo alternatif D12 (jika dicolok ke D12)
constexpr int PIN_FLAME = 34;        // flame sensor 5-way, DO
constexpr int PIN_TOUCH = 4;         // Touch0
constexpr int PIN_TRIG_DEFAULT = 4;  // HC-SR04 (bisa ditukar dari web)
constexpr int PIN_ECHO_DEFAULT = 5;
constexpr int PIN_FAN_DEFAULT = 32;  // bisa diganti lewat /api/fan?pin=
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
constexpr int MAX_DISTANCE_CM = 20;
constexpr int TOUCH_DELTA_THRESHOLD = 250;
constexpr int TOUCH_BASELINE_FALLBACK = 1380;
// Modul api yang terpasang mengeluarkan 0 saat aman (terukur di D34), jadi 1 = api.
constexpr bool FLAME_ACTIVE_HIGH_DEFAULT = true;
// Api baru dianggap ada kalau sinyal aktif terus selama FLAME_CONFIRM_MS (lonjakan singkat diabaikan),
// dan baru dianggap padam setelah sinyal tenang selama FLAME_RELEASE_MS (alarm tidak hidup-mati).
constexpr unsigned long FLAME_CONFIRM_MS = 500;
constexpr unsigned long FLAME_RELEASE_MS = 1500;
constexpr unsigned long FLAME_READ_MS = 15;
constexpr unsigned long TOUCH_READ_MS = 40;
constexpr unsigned long ULTRASONIC_PING_MS = 90;

} // namespace cfg
