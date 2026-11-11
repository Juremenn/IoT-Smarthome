// Smart Home Miniatur Rumah Joglo (ESP32)
// Setiap bagian ada di modulnya sendiri:
//   door        servo pintu D13, gerak halus non-blocking
//   sensors     sensor api D34, sentuh D4, ultrasonik HC-SR04
//   automation  mode pemicu otomatis pintu
//   fan         kipas / relay
//   oled        layar SSD1306 opsional
//   network     WiFi rumah atau Access Point
//   web_server  route HTTP dan file dashboard (src/web/)

#include <Arduino.h>

#include "automation.h"
#include "config.h"
#include "door.h"
#include "fan.h"
#include "network.h"
#include "oled.h"
#include "sensors.h"
#include "web_server.h"

namespace {
unsigned long lastSerialLog = 0;
}

void setup() {
  Serial.begin(115200);

  // Servo dipasang paling awal supaya pintu langsung diam di posisi tertutup.
  doorBegin();
  ultrasonicBegin();
  flameBegin();
  fanBegin();

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
  Serial.printf("  Sensor Ultrasonik: Trig=D%d, Echo=D%d\n", ultrasonicTrigPin(),
                ultrasonicEchoPin());
  Serial.printf("  Kipas/Relay di Pin: GPIO D%d\n", fanPin());
  Serial.printf("  Dashboard Web: http://%s\n", networkIp().c_str());
  Serial.println("=============================================\n");
}

void loop() {
  unsigned long now = millis();

  networkMaintain(now);
  webHandle();
  automationUpdate(now);
  doorUpdate(now);
  oledUpdate(now);

  if (now - lastSerialLog >= 3000) {
    lastSerialLog = now;
    Serial.printf("[STATUS] Pintu: %s | Pemicu: %d | Touch(D4): %d | Flame: %s (Raw: %d) | OLED: %s\n",
                  doorStatusText(), triggerMode(), touchRawValue(),
                  flameDetected() ? "API!" : "Aman", flameRaw(), oledIsActive() ? "Aktif" : "Offline");
  }

  delay(2);
}
