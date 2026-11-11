#pragma once

// Mode pemicu otomatis pintu
// 0: manual via web, 1: sentuh D4, 2: sensor api (evakuasi), 3: ultrasonik
enum TriggerMode : int { MODE_MANUAL = 0, MODE_TOUCH = 1, MODE_FLAME = 2, MODE_ULTRASONIC = 3 };

int triggerMode();
void setTriggerMode(int mode);

// Baca sensor sesuai mode lalu gerakkan pintu bila perlu. Dipanggil tiap loop.
void automationUpdate(unsigned long now);
