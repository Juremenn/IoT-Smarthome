// Smart Home Miniatur Rumah Joglo (ESP32)
// Setiap bagian ada di modulnya sendiri:
//   door        servo pintu D13, gerak halus non-blocking
//   sensors     sensor api D34, sentuh D4, ultrasonik HC-SR04
//   automation  mode pemicu otomatis pintu
//   oled        layar SSD1306 opsional
//   app_network WiFi Access Point mandiri
//   web_server  route HTTP dan file dashboard (src/web/)

#include <Arduino.h>

#include "automation.h"
#include "config.h"
#include "door.h"
#include "app_network.h"
#include "oled.h"
#include "sensors.h"
#include "web_server.h"

namespace {
unsigned long lastSerialLog = 0;
}

void setup() {
  Serial.begin(115200);

  esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("\n[SISTEM] Status Boot / Reset: ");
  if (reason == ESP_RST_BROWNOUT) {
    Serial.println("BROWNOUT DETECTED! Tegangan listrik drop (cek suplai power servo/sensor)!");
  } else if (reason == ESP_RST_PANIC) {
    Serial.println("EXCEPTION / GURU MEDITATION PANIC");
  } else if (reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT) {
    Serial.println("WATCHDOG RESET (Loop terhalang/macet)");
  } else if (reason == ESP_RST_SW) {
    Serial.println("Software Reset");
  } else if (reason == ESP_RST_POWERON) {
    Serial.println("Power-On Normal");
  } else {
    Serial.printf("Kode: %d\n", (int)reason);
  }

  // Servo dipasang paling awal supaya pintu langsung diam di posisi tertutup.
  doorBegin();
  ultrasonicBegin();
  flameBegin();
  dhtBegin();

  oledBegin();
  oledShowBoot("Memulai Sistem...");
  networkBegin();
  webBegin();
  oledShowSplash();

  touchBegin();

  Serial.println("\n=============================================");
  Serial.println("  SMART HOME RUMAH JOGLO - IOT SYSTEM");
  Serial.printf("  Servo Pintu: GPIO D%d\n", cfg::PIN_SERVO);
  Serial.printf("  Sensor Api 5-Way: GPIO D%d\n", cfg::PIN_FLAME);
  Serial.printf("  Sensor Suhu (DHT11): GPIO D%d\n", cfg::PIN_DHT);
  Serial.printf("  Sensor Ultrasonik: Trig=D%d, Echo=D%d\n", ultrasonicTrigPin(),
                ultrasonicEchoPin());
  Serial.printf("  Dashboard Web: http://%s\n", networkIp().c_str());
  Serial.println("=============================================\n");
}

void loop() {
  unsigned long now = millis();

  networkMaintain(now);
  webHandle();
  dhtUpdate(now);
  automationUpdate(now);
  doorUpdate(now);
  oledUpdate(now);

  if (now - lastSerialLog >= 3000) {
    lastSerialLog = now;
    if (dhtIsValid()) {
      Serial.printf("[STATUS] Pintu: %s | Suhu: %.1f C (%.0f%%) | Pemicu: %d | Touch(D4): %d | Flame: %s (Raw: %d) | OLED: %s\n",
                    doorStatusText(), dhtTemperature(), dhtHumidity(), triggerMode(), touchRawValue(),
                    flameDetected() ? "API!" : "Aman", flameRaw(), oledIsActive() ? "Aktif" : "Offline");
    } else {
      Serial.printf("[STATUS] Pintu: %s | Suhu: [%s] | Pemicu: %d | Touch(D4): %d | Flame: %s (Raw: %d) | OLED: %s\n",
                    doorStatusText(), dhtStatusMessage(), triggerMode(), touchRawValue(),
                    flameDetected() ? "API!" : "Aman", flameRaw(), oledIsActive() ? "Aktif" : "Offline");
    }
  }

  delay(1);
}
