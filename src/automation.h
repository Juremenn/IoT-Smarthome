#pragma once

// Mode pemicu otomatis pintu (sesuai flowchart)
// 0: manual via web (Standby / Kontrol Web), 3: ultrasonik HC-SR04 (<= 10 cm)
enum TriggerMode : int { MODE_MANUAL = 0, MODE_ULTRASONIC = 3 };

int triggerMode();
void setTriggerMode(int mode);

// Baca sensor sesuai mode lalu gerakkan pintu bila perlu. Dipanggil tiap loop.
void automationUpdate(unsigned long now);
