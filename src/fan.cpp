#include "fan.h"

#include <Arduino.h>

#include "config.h"

namespace {
int pin = cfg::PIN_FAN_DEFAULT;
bool activeLow = false;  // default relay/driver Active-HIGH
bool on = false;

void apply() {
  pinMode(pin, OUTPUT);
  int level = activeLow ? (on ? LOW : HIGH) : (on ? HIGH : LOW);
  digitalWrite(pin, level);
  Serial.printf("[KIPAS] Pin: D%d | Logika: %s | Fisik: %s\n", pin, on ? "ON" : "OFF",
                level == HIGH ? "HIGH" : "LOW");
}
} // namespace

void fanBegin() {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  on = false;
}

void fanSet(bool state) {
  on = state;
  apply();
}

void fanToggle() { fanSet(!on); }

void fanSetPin(int newPin) {
  pin = newPin;
  apply();
}

void fanToggleLogic() {
  activeLow = !activeLow;
  apply();
}

bool fanIsOn() { return on; }
int fanPin() { return pin; }
bool fanIsActiveLow() { return activeLow; }
