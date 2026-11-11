#include "network.h"

#include <WiFi.h>

#include "config.h"
#include "oled.h"

namespace {
String ip = "192.168.4.1";
bool accessPoint = true;
unsigned long lastCheck = 0;
} // namespace

void networkBegin() {
  if (strlen(WIFI_SSID) > 0) {
    Serial.print("[WIFI] Menghubungkan ke WiFi: ");
    Serial.println(WIFI_SSID);
    oledShowBoot("Konek ke WiFi...");

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);  // tanpa modem sleep: dashboard jauh lebih responsif
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < cfg::WIFI_CONNECT_TIMEOUT_MS) {
      delay(200);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      accessPoint = false;
      ip = WiFi.localIP().toString();
      Serial.print("[WIFI] Terhubung! IP: ");
      Serial.println(ip);
      oledShowBoot("WiFi Terhubung!");
      delay(300);
      return;
    }
    Serial.println("[WIFI] Gagal terhubung ke WiFi. Beralih ke Access Point...");
    oledShowBoot("Beralih ke AP...");
    delay(300);
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(cfg::AP_SSID);
  accessPoint = true;
  ip = WiFi.softAPIP().toString();
  Serial.println("[WIFI] Mode AP Mandiri Aktif!");
  Serial.printf("[WIFI] SSID: %s | URL: http://%s\n", cfg::AP_SSID, ip.c_str());
}

void networkMaintain(unsigned long now) {
  if (accessPoint || now - lastCheck < cfg::WIFI_CHECK_INTERVAL_MS) return;
  lastCheck = now;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Sinyal terputus! Mencoba rekoneksi...");
    WiFi.reconnect();
  }
}

const String &networkIp() { return ip; }
bool networkIsAccessPoint() { return accessPoint; }
