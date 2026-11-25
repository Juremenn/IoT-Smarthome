#pragma once
#include <stdint.h>

// Antarmuka Serial Monitor TUI (78 kolom, pure 7-bit ASCII, Open-Border, zero heap).
// Memperbarui tampilan in-place tanpa flicker dan tanpa memblokir loop servo 20 ms.

enum class TerminalTag : uint8_t {
  Sys,
  Door,
  Us,
  Fire,
};

void terminalBegin();
void terminalUpdate(unsigned long now);
void terminalLog(const char *fmt, ...);
void terminalLogTag(TerminalTag tag, const char *fmt, ...);
