#include "app_network.h"

#include <WiFi.h>
#include <esp_wifi.h>

#include "config.h"
#include "oled.h"

namespace {
String ip = "192.168.4.1";
} // namespace

void networkBegin() {
  oledShowBoot("Mode AP Mandiri...");

  // ESP32 murni berjalan sebagai Hotspot mandiri (Access Point)
  // Maksimalkan jangkauan pancaran: TX 19.5 dBm (maksimum), HT20 (sensitivitas lebih tinggi), tanpa modem-sleep
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
  esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  WiFi.softAP(cfg::AP_SSID, nullptr, 1, 0, 4);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  ip = WiFi.softAPIP().toString();

  Serial.println("\n[WIFI] ===================================");
  Serial.println("[WIFI] Mode AP Mandiri Aktif (TX Maksimum 19.5 dBm, HT20)!");
  Serial.printf("[WIFI] Sambungkan ke WiFi : %s\n", cfg::AP_SSID);
  Serial.printf("[WIFI] Buka di Browser     : http://%s\n", ip.c_str());
  Serial.println("[WIFI] ===================================\n");
}

void networkMaintain(unsigned long now) {
  // Mode AP mandiri selalu aktif tanpa perlu rekoneksi router
}

const String &networkIp() { return ip; }
bool networkIsAccessPoint() { return true; }
