// Smart Home Miniatur Rumah Joglo (ESP32)
// Arsitektur Dual-Core Non-Blocking:
//   Core 0: Wi-Fi AP stack (setSleep=false) + FreeRTOS dhtTask (DHT11 D27)
//   Core 1: webHandle() prioritas utama, flameUpdate(), ultrasonicUpdate() (ISR),
//           doorUpdate() (tick servo 20 ms), oledUpdate() (2 FPS), terminalUpdate() (ASCII TUI)

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "app_network.h"
#include "automation.h"
#include "config.h"
#include "door.h"
#include "oled.h"
#include "sensors.h"
#include "terminal_ui.h"
#include "web_server.h"

namespace {
unsigned long lastIdleYield = 0;
}  // namespace

void setup() {
  terminalBegin();

  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_BROWNOUT) {
    terminalLogTag(TerminalTag::Sys, "PERINGATAN: Brownout reset (cek suplai 5V)");
  } else if (reason == ESP_RST_PANIC) {
    terminalLogTag(TerminalTag::Sys, "PERINGATAN: Reboot karena panic");
  } else if (reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT) {
    terminalLogTag(TerminalTag::Sys, "PERINGATAN: Reboot karena watchdog");
  }

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
  automationUpdate(now);
  doorUpdate(now);

  // Layani ulang request web segera sebelum tugas tampilan agar tombol buka/tutup instan
  webHandle();

  bool doorIdle = (doorPhase() == DoorPhase::Closed || doorPhase() == DoorPhase::Open);
  if (doorIdle || flameDetected()) {
    oledUpdate(now);
  }
  terminalUpdate(now);

  // Beri napas 1 tick ke IDLE1 WDT tiap 20 ms tanpa memblokir setiap iterasi loop
  if (now - lastIdleYield >= 20) {
    lastIdleYield = now;
    vTaskDelay(1);
  }
}
