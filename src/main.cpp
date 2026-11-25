// Smart Home Miniatur Rumah Joglo (ESP32)
// Setiap bagian ada di modulnya sendiri (sesuai Flowchart_iot_final):
//   door        servo pintu D13, gerak halus non-blocking
//   sensors     sensor api D34, DHT11 D27, ultrasonik HC-SR04 D4/D5
//   automation  evakuasi api global + buka pintu otomatis ultrasonik (<= 10 cm)
//   oled        layar SSD1306 I2C D21/D22
//   app_network WiFi Access Point mandiri
//   web_server  route HTTP dan file dashboard (src/web/)

#include <Arduino.h>

#include "automation.h"
#include "config.h"
#include "door.h"
#include "app_network.h"
#include "oled.h"
#include "sensors.h"
#include "terminal_ui.h"
#include "web_server.h"

void setup() {
  terminalBegin();

  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_BROWNOUT) {
    terminalLog("PERINGATAN: Brownout reset (cek suplai 5V)");
  } else if (reason == ESP_RST_PANIC) {
    terminalLog("PERINGATAN: Reboot karena panic");
  } else if (reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT) {
    terminalLog("PERINGATAN: Reboot karena watchdog");
  }

  // Inisialisasi ESP32 & Perangkat Keras (Servo D13, Api D34, DHT11 D27, Ultrasonik D25/D26, OLED)
  doorBegin();
  ultrasonicBegin();
  flameBegin();
  dhtBegin();

  oledBegin();
  oledShowBoot("Memulai Sistem...");
  networkBegin();
  webBegin();
  oledShowSplash();
}

void loop() {
  unsigned long now = millis();

  networkMaintain(now);
  webHandle();
  // Tunda pembacaan DHT11 (~25ms blocking) saat servo sedang bergerak agar gerak pintu tetap halus
  bool doorIdle = (doorPhase() == DoorPhase::Closed || doorPhase() == DoorPhase::Open);
  if (doorIdle) {
    dhtUpdate(now);
  }
  automationUpdate(now);
  doorUpdate(now);
  // Tunda refresh OLED normal (~25ms I2C) saat servo bergerak agar tick 20ms tidak patah-patah.
  // Saat darurat api, OLED tetap langsung diperbarui.
  if (doorIdle || flameDetected()) {
    oledUpdate(now);
  }
  // Perbarui tampilan TUI terminal secara in-place (tanpa scroll berulang, non-blocking)
  terminalUpdate(now);

  delay(1);
}
