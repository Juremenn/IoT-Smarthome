#include "fan.h"

#include <Arduino.h>

#include "config.h"

namespace {
bool fanOn = false;
bool activeLow = cfg::FAN_ACTIVE_LOW_DEFAULT;

void applyOutput() {
  int level = activeLow ? (fanOn ? LOW : HIGH) : (fanOn ? HIGH : LOW);
  digitalWrite(cfg::PIN_FAN, level);
  Serial.printf("[KIPAS] Pin: D%d | Status: %s | Fisik: %s (%s)\n",
                cfg::PIN_FAN, fanOn ? "ON" : "OFF",
                level == HIGH ? "HIGH" : "LOW",
                activeLow ? "Active-LOW" : "Active-HIGH");
}
} // namespace

void fanBegin() {
  // Set level mati terlebih dahulu sebelum pinMode OUTPUT agar relay tidak berkedip saat boot
  int offLevel = activeLow ? HIGH : LOW;
  digitalWrite(cfg::PIN_FAN, offLevel);
  pinMode(cfg::PIN_FAN, OUTPUT);
  digitalWrite(cfg::PIN_FAN, offLevel);
  fanOn = false;
  Serial.printf("[KIPAS] Inisialisasi relay di GPIO D%d (Mode: %s, Awal: OFF)\n",
                cfg::PIN_FAN, activeLow ? "Active-LOW" : "Active-HIGH");
}

void fanSetState(bool on) {
  fanOn = on;
  applyOutput();
}

void fanToggle() {
  fanOn = !fanOn;
  applyOutput();
}

bool fanIsOn() { return fanOn; }
bool fanActiveLow() { return activeLow; }

void fanToggleLogic() {
  activeLow = !activeLow;
  applyOutput();
}
