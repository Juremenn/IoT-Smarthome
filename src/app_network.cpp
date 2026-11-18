#include "app_network.h"

#include <WiFi.h>

#include "config.h"
#include "oled.h"

namespace {
String ip = "192.168.4.1";
} // namespace

void networkBegin() {
  oledShowBoot("Mode AP Mandiri...");

  // ESP32 murni berjalan sebagai Hotspot mandiri (Access Point)
  // Tanpa mencoba menghubungkan ke WiFi rumah sehingga boot instan dan bebas bug koneksi
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);  // Matikan modem-sleep agar respon HTTP instan tanpa jeda latency
  WiFi.softAP(cfg::AP_SSID);
  ip = WiFi.softAPIP().toString();

  Serial.println("\n[WIFI] ===================================");
  Serial.println("[WIFI] Mode AP Mandiri Aktif!");
  Serial.printf("[WIFI] Sambungkan ke WiFi : %s\n", cfg::AP_SSID);
  Serial.printf("[WIFI] Buka di Browser     : http://%s\n", ip.c_str());
  Serial.println("[WIFI] ===================================\n");
}

void networkMaintain(unsigned long now) {
  // Mode AP mandiri selalu aktif tanpa perlu rekoneksi router
}

const String &networkIp() { return ip; }
bool networkIsAccessPoint() { return true; }
