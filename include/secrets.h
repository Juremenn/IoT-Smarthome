#pragma once

// Salin file ini menjadi include/secrets.h lalu isi dengan WiFi rumah.
// include/secrets.h tidak ikut masuk git (lihat .gitignore).
// Kalau SSID dikosongkan, ESP32 langsung menyalakan hotspot sendiri.
constexpr const char *WIFI_SSID = "167778";
constexpr const char *WIFI_PASS = "cyclenecessary";
