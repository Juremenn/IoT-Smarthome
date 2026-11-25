#include "automation.h"

#include <Arduino.h>

#include "config.h"
#include "door.h"
#include "sensors.h"

namespace {
int mode = MODE_ULTRASONIC;
bool wasTriggered = false;  // untuk log satu kali per pemicuan
} // namespace

int triggerMode() { return mode; }

void setTriggerMode(int m) {
  if ((m != MODE_MANUAL && m != MODE_ULTRASONIC) || m == mode) return;
  mode = m;
  wasTriggered = false;
  Serial.printf("[PINTU] Mode pemicu buka diubah ke: %d\n", mode);
}

void automationUpdate(unsigned long now) {
  flameUpdate(now);
  ultrasonicUpdate(now, mode == MODE_ULTRASONIC);

  // Evakuasi api (Prioritas Darurat Global): pintu dibuka penuh dan ditahan selama api terdeteksi.
  doorSetEvacuation(flameDetected());

  bool triggered = false;
  if (mode == MODE_ULTRASONIC) {
    int d = ultrasonicDistance();
    triggered = d > 0 && d <= cfg::MAX_DISTANCE_CM;
  }

  // Selama objek valid <= 10 cm, buka pintu otomatis (Servo D13).
  // "Buka Terus" dan jeda "Tutup" manual dari web tidak diganggu oleh sensor.
  if (triggered && !doorIsManualOpen() && !doorIsManualClosing(now)) {
    if (!wasTriggered) {
      Serial.printf(">>> [ULTRASONIK] Objek terdeteksi (%d cm)\n", ultrasonicDistance());
    }
    doorStartCycle();
  }
  wasTriggered = triggered;
}
