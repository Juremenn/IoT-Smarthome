#include "automation.h"

#include <Arduino.h>

#include "config.h"
#include "door.h"
#include "sensors.h"

namespace {
int mode = MODE_TOUCH;
bool wasTriggered = false;  // untuk log satu kali per pemicuan
} // namespace

int triggerMode() { return mode; }

void setTriggerMode(int m) {
  mode = m;
  Serial.printf("[PINTU] Mode pemicu buka diubah ke: %d\n", mode);
}

void automationUpdate(unsigned long now) {
  flameUpdate(now);
  touchUpdate(now, mode == MODE_TOUCH);
  ultrasonicUpdate(now, mode == MODE_ULTRASONIC);

  // Mode api: pintu dibuka dan ditahan terbuka selama api masih terdeteksi.
  doorSetEvacuation(mode == MODE_FLAME && flameDetected());

  bool triggered = false;
  if (mode == MODE_TOUCH) {
    triggered = touchIsTouched();
  } else if (mode == MODE_ULTRASONIC) {
    int d = ultrasonicDistance();
    triggered = d > 0 && d <= cfg::MAX_DISTANCE_CM;
  }

  // Selama pemicu aktif, pintu tetap terbuka (waktu tahan diperpanjang).
  // "Buka Terus" dari web tidak diganggu oleh sensor.
  if (triggered && !doorIsManualOpen()) {
    if (!wasTriggered) {
      if (mode == MODE_TOUCH) {
        Serial.printf(">>> [TOUCH D4] Sentuhan terdeteksi (%d vs Base %d)\n", touchRawValue(),
                      touchBaselineValue());
      } else {
        Serial.printf(">>> [ULTRASONIK] Objek terdeteksi (%d cm)\n", ultrasonicDistance());
      }
    }
    doorStartCycle();
  }
  wasTriggered = triggered;
}
