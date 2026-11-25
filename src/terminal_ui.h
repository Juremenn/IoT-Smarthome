#pragma once

// Antarmuka Terminal (ANSI TUI in-place) untuk Serial Monitor PlatformIO.
// Memperbarui tampilan di tempat tanpa scroll berulang dan tanpa memblokir loop servo.

void terminalBegin();
void terminalUpdate(unsigned long now);
void terminalLog(const char *fmt, ...);
