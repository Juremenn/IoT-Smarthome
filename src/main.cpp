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

  // Inisialisasi ESP32 & Perangkat Keras (Servo D13, Api D34, DHT11 D27, Ultrasonik D4/D5, OLED)
  doorBegin();
  ultrasonicBegin();
  flameBegin();
  dhtBegin();

  oledBegin();
  oledShowBoot("Memulai Sistem...");
  networkBegin();
  webBegin();
  oledShowSplash();

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

  if (now - lastSerialLog >= 3000) {
    lastSerialLog = now;
    char distBuf[12];
    if (ultrasonicDistance() > 0) {
      snprintf(distBuf, sizeof(distBuf), "%3d cm", ultrasonicDistance());
    } else {
      snprintf(distBuf, sizeof(distBuf), " -- cm");
    }
    if (dhtIsValid()) {
      Serial.printf("[STATUS] Pintu: %-10s | Jarak: %s | Suhu: %4.1f C (%2.0f%%) | Api: %-4s | OLED: %s\n",
                    doorStatusText(), distBuf, dhtTemperature(), dhtHumidity(),
                    flameDetected() ? "API!" : "Aman", oledIsActive() ? "Aktif" : "Offline");
    } else {
      Serial.printf("[STATUS] Pintu: %-10s | Jarak: %s | Suhu: --.- C (--) | Api: %-4s | OLED: %s\n",
                    doorStatusText(), distBuf,
                    flameDetected() ? "API!" : "Aman", oledIsActive() ? "Aktif" : "Offline");
    }
  }

  delay(1);
}
