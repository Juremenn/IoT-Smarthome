#include "automation.h"

#include <Arduino.h>

#include "config.h"
#include "door.h"
#include "sensors.h"
#include "terminal_ui.h"

namespace {
int mode = MODE_ULTRASONIC;
bool wasTriggered = false;  // untuk log satu kali per pemicuan
} // namespace

int triggerMode() { return mode; }

void setTriggerMode(int m) {
  if ((m != MODE_MANUAL && m != MODE_ULTRASONIC) || m == mode) return;
  mode = m;
  wasTriggered = false;
  terminalLogTag(TerminalTag::Sys, "Mode pemicu diubah ke %d", mode);
}

void automationUpdate(unsigned long now) {
  flameUpdate(now);
  // Ultrasonik memakai ISR non-blocking (0 us wait), sehingga tetap aktif di semua fase pintu
  ultrasonicUpdate(now, mode == MODE_ULTRASONIC, false);

  // Evakuasi api (Prioritas Darurat Global): pintu dibuka penuh dan ditahan selama api terdeteksi.
  doorSetEvacuation(flameDetected());

  bool triggered = (mode == MODE_ULTRASONIC) && ultrasonicTriggered();

  // Selama objek valid terkonfirmasi <= 10 cm, buka/perpanjang pintu otomatis (Servo D13).
  // "Buka Terus" dan jeda "Tutup" manual dari web tidak diganggu oleh sensor.
  if (triggered && !doorIsManualOpen() && !doorIsManualClosing(now)) {
    if (!wasTriggered) {
      terminalLogTag(TerminalTag::Us, "Objek %d cm -> buka pintu", ultrasonicDistance());
    }
    doorStartCycle();
  }
  wasTriggered = triggered;
}
