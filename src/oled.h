#pragma once
#include <stdint.h>

// OLED SSD1306 opsional, hot-plug: dicari ulang tiap 1,5 detik kalau belum terpasang.
void oledBegin();
void oledShowBoot(const char *statusText);
void oledShowSplash();
void oledUpdate(unsigned long now);
bool oledIsActive();
uint8_t oledAddress();
