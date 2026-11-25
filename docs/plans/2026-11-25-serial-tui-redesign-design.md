# Desain Refactor Serial Monitor TUI ESP32 (`antislop`)

**Tanggal:** 2026-11-25  
**Proyek:** Smart Home Miniatur Rumah Joglo (ESP32)  
**Status:** Disetujui Pengguna  
**Dials Antislop:** `ENERGY 1` / `RHYTHM 2` / `MOTION 1`

## 1. Masalah yang Diperbaiki
1. **Karakter korup (`♦`)**: Pengiriman border & progress bar 3-byte UTF-8 (`─`, `█`, `░`) sebesar ~1,9 KB tiap 200 ms di 115200 baud menyebabkan byte terpotong di UART/Serial monitor.
2. **Border kanan (`│`) bergeser**: Padding spasi manual selisih kolom dan bergeser setiap kali byte UTF-8 rusak.
3. **Fragmentasi heap**: Penggunaan `String out; out.reserve(1800);` berulang di `renderFrame()`.

## 2. Arsitektur Solusi (Pendekatan A)
- **Pure 7-bit ASCII + Single Static Buffer**: Seluruh glyph memakai ASCII murni (`=`, `-`, `#`, `.`, `[`, `]`) yang dirender ke `static char frameBuf[2048]` memakai `FrameWriter` berbasis `snprintf`, dikirim sekali lewat `Serial.write(frameBuf, len)` tanpa satu pun alokasi `String`.
- **Open-Border 78 Kolom + ANSI Column Lock**: Tanpa garis vertikal kiri/kanan. Header dan 4 kolom telemetri dikunci dengan `\x1b[<col>G` (`1`, `19`, `31`, `49`) dan `\x1b[K\r\n` agar rata presisi di semua perubahan digit.
- **Flicker-Free & Non-Blocking**: Inisialisasi layar sekali (`\x1b[2J\x1b[H\x1b[?25l`), refresh in-place (`\x1b[H` + `\x1b[K`), interval `300 ms` atau event-driven (`dirty` flag, debounce `80 ms`), dengan guard `Serial.availableForWrite() >= len` agar loop servo `20 ms` tidak pernah terblokir.
- **Hierarki Warna ANSI Semantik**:
  - Dim Gray (`\x1b[90m`): divider, label pin, metadata pasif, timestamp log.
  - Bold White (`\x1b[1;37m`): judul header, nilai uptime, header log.
  - Green (`\x1b[92m`), Cyan (`\x1b[96m`), Yellow (`\x1b[93m`), Bold Red (`\x1b[1;91m` / `\x1b[1;41;97m`): status sensor & tag log (`[SYS]`, `[DOOR]`, `[US]`, `[FIRE]`).
- **Ring Buffer 5 Log**: Menyimpan 5 kejadian terakhir dengan format `[HH:MM:SS] [TAG] Pesan` serta sanitasi ASCII 7-bit.
