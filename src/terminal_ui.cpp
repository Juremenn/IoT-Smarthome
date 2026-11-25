#include "terminal_ui.h"

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app_network.h"
#include "config.h"
#include "door.h"
#include "oled.h"
#include "sensors.h"

namespace {

// Kode warna ANSI (didukung penuh oleh VS Code Terminal + monitor_raw = yes)
constexpr const char *RST    = "\033[0m";
constexpr const char *BOLD   = "\033[1m";
constexpr const char *DIM    = "\033[2m";
constexpr const char *RED    = "\033[91m";
constexpr const char *GREEN  = "\033[92m";
constexpr const char *YELLOW = "\033[93m";
constexpr const char *CYAN   = "\033[96m";
constexpr const char *WHITE  = "\033[97m";
constexpr const char *GRAY   = "\033[90m";
constexpr const char *BG_RED = "\033[41;97m";

constexpr int INNER_W = 64;
constexpr int LOG_CAP = 4;
constexpr int LOG_LEN = 61;  // maks 60 char + NUL
constexpr unsigned long REDRAW_MS = 200;

char logRing[LOG_CAP][LOG_LEN];
int logCount = 0;
int logHead = 0;
unsigned long lastRedraw = 0;
bool screenCleared = false;

void formatUptime(unsigned long sec, char *out, size_t n) {
  unsigned long h = sec / 3600;
  unsigned long m = (sec % 3600) / 60;
  unsigned long s = sec % 60;
  snprintf(out, n, "%02lu:%02lu:%02lu", h, m, s);
}

void appendRule(String &out, const char *left, const char *right) {
  out += GRAY;
  out += left;
  for (int i = 0; i < INNER_W; i++) {
    out += "\xE2\x94\x80";  // ─
  }
  out += right;
  out += RST;
  out += "\033[K\r\n";
}

void buildBar(float ratio, int width, char *out, size_t outSize) {
  if (ratio < 0.0f) ratio = 0.0f;
  if (ratio > 1.0f) ratio = 1.0f;
  int filled = (int)(ratio * width + 0.5f);
  size_t pos = 0;
  for (int i = 0; i < width && pos + 4 < outSize; i++) {
    const char *blk = (i < filled) ? "\xE2\x96\x88" : "\xE2\x96\x91";  // █ / ░
    memcpy(out + pos, blk, 3);
    pos += 3;
  }
  out[pos] = '\0';
}

void renderFrame(unsigned long now) {
  // Pastikan buffer TX cukup agar Serial.print tidak memblokir tick servo 20 ms
  if (Serial.availableForWrite() < 1200) return;

  String out;
  out.reserve(1800);

  if (!screenCleared) {
    out += "\033[2J\033[H\033[?25l";
    screenCleared = true;
  } else {
    out += "\033[H";
  }

  char upBuf[12];
  formatUptime(now / 1000, upBuf, sizeof(upBuf));

  char line[192];

  // Header
  appendRule(out, "\xE2\x95\xAD", "\xE2\x95\xAE");  // ╭ ... ╮

  snprintf(line, sizeof(line),
           "%s\xE2\x94\x82%s  %s%sRUMAH JOGLO - SMART HOME IOT%s     %sWeb:%s %shttp://%-15s%s  %s\xE2\x94\x82%s\033[K\r\n",
           GRAY, RST, BOLD, YELLOW, RST, DIM, RST, CYAN, networkIp().c_str(), RST, GRAY, RST);
  out += line;

  snprintf(line, sizeof(line),
           "%s\xE2\x94\x82%s  %sESP32 DevKit V1 / Live Monitor%s   %sUptime:%s %s%-19s%s  %s\xE2\x94\x82%s\033[K\r\n",
           GRAY, RST, DIM, RST, DIM, RST, WHITE, upBuf, RST, GRAY, RST);
  out += line;

  appendRule(out, "\xE2\x94\x9C", "\xE2\x94\xA4");  // ├ ... ┤

  // 1. Pintu (Servo D13)
  DoorPhase dp = doorPhase();
  float frac = doorOpenFraction();
  int pct = (int)(frac * 100.0f + 0.5f);
  char doorBar[48];
  buildBar(frac, 14, doorBar, sizeof(doorBar));

  const char *doorColor = WHITE;
  const char *doorLabel = "TERTUTUP";
  if (doorIsEvacuating()) {
    doorColor = BG_RED;
    doorLabel = " EVAKUASI API ";
  } else if (dp == DoorPhase::Opening) {
    doorColor = YELLOW;
    doorLabel = "MEMBUKA...";
  } else if (dp == DoorPhase::Closing) {
    doorColor = CYAN;
    doorLabel = "MENUTUP...";
  } else if (dp == DoorPhase::Open) {
    doorColor = GREEN;
    doorLabel = doorIsAutoCycle() ? "TERBUKA (AUTO)" : "TERBUKA MANUAL";
  }

  snprintf(line, sizeof(line),
           "%s\xE2\x94\x82%s  %sPINTU (D13)      %s%s%-15s%s  [%s%s%s] %3d%%       %s\xE2\x94\x82%s\033[K\r\n",
           GRAY, RST, DIM, RST, doorColor, doorLabel, RST,
           doorIsEvacuating() ? RED : GREEN, doorBar, RST, pct, GRAY, RST);
  out += line;

  // 2. Sensor Api (D34)
  bool fire = flameDetected();
  snprintf(line, sizeof(line),
           "%s\xE2\x94\x82%s  %sSENSOR API (D34) %s%s%-15s%s  %sRaw:%d (%-11s)%s         %s\xE2\x94\x82%s\033[K\r\n",
           GRAY, RST, DIM, RST,
           fire ? BG_RED : GREEN, fire ? " BAHAYA: API!  " : "AMAN", RST,
           DIM, flameRaw(), flameActiveHigh() ? "Active-HIGH" : "Active-LOW", RST, GRAY, RST);
  out += line;

  // 3. Ultrasonik (D25/D26)
  int dist = ultrasonicDistance();
  bool nearObj = (dist > 0 && dist <= cfg::MAX_DISTANCE_CM);
  char distStr[16];
  if (dist > 0) {
    snprintf(distStr, sizeof(distStr), "%3d cm", dist);
  } else {
    snprintf(distStr, sizeof(distStr), " -- cm");
  }
  float distRatio = (dist > 0) ? (1.0f - min(dist, 60) / 60.0f) : 0.0f;
  char distBar[40];
  buildBar(distRatio, 10, distBar, sizeof(distBar));

  snprintf(line, sizeof(line),
           "%s\xE2\x94\x82%s  %sULTRASONIK       %s%s%-15s%s  [%s%s%s] %-13s  %s\xE2\x94\x82%s\033[K\r\n",
           GRAY, RST, DIM, RST,
           nearObj ? YELLOW : CYAN, distStr, RST,
           nearObj ? YELLOW : CYAN, distBar, RST,
           nearObj ? "TERDETEKSI" : (dist > 0 ? "Standby" : "Luar Jangkau"), GRAY, RST);
  out += line;

  // 4. Suhu & Kelembaban (DHT11 D27)
  char dhtStr[20];
  const char *dhtColor = YELLOW;
  const char *dhtNote = "DHT11 (D27) Menunggu...";
  if (dhtIsValid()) {
    snprintf(dhtStr, sizeof(dhtStr), "%4.1f C / %2.0f%%", dhtTemperature(), dhtHumidity());
    dhtColor = GREEN;
    dhtNote = "DHT11 (D27) OK";
  } else {
    snprintf(dhtStr, sizeof(dhtStr), "--.- C / --%%");
    dhtColor = YELLOW;
    dhtNote = "DHT11 (D27) Tidak terbaca";
  }
  snprintf(line, sizeof(line),
           "%s\xE2\x94\x82%s  %sSUHU & LEMBAB    %s%s%-15s%s  %s%-26s%s  %s\xE2\x94\x82%s\033[K\r\n",
           GRAY, RST, DIM, RST, dhtColor, dhtStr, RST, DIM, dhtNote, RST, GRAY, RST);
  out += line;

  // 5. OLED (D21/D22)
  bool oledOk = oledIsActive();
  char oledVal[20];
  if (oledOk) {
    snprintf(oledVal, sizeof(oledVal), "AKTIF (0x%02X)", oledAddress());
  } else {
    snprintf(oledVal, sizeof(oledVal), "OFFLINE");
  }
  snprintf(line, sizeof(line),
           "%s\xE2\x94\x82%s  %sLAYAR OLED       %s%s%-15s%s  %sI2C D21/D22               %s  %s\xE2\x94\x82%s\033[K\r\n",
           GRAY, RST, DIM, RST,
           oledOk ? GREEN : RED, oledVal, RST, DIM, RST, GRAY, RST);
  out += line;

  // Log Kejadian Terakhir (Ring Buffer 4 Baris)
  appendRule(out, "\xE2\x94\x9C", "\xE2\x94\xA4");  // ├ ... ┤

  snprintf(line, sizeof(line),
           "%s\xE2\x94\x82%s  %s%sLOG KEJADIAN TERAKHIR%s                                         %s\xE2\x94\x82%s\033[K\r\n",
           GRAY, RST, BOLD, WHITE, RST, GRAY, RST);
  out += line;

  for (int i = 0; i < LOG_CAP; i++) {
    if (i < logCount) {
      int idx = (logHead - logCount + i + LOG_CAP) % LOG_CAP;
      snprintf(line, sizeof(line),
               "%s\xE2\x94\x82%s  %s%-60.60s%s  %s\xE2\x94\x82%s\033[K\r\n",
               GRAY, RST, WHITE, logRing[idx], RST, GRAY, RST);
    } else {
      snprintf(line, sizeof(line),
               "%s\xE2\x94\x82%s  %s%-60s%s  %s\xE2\x94\x82%s\033[K\r\n",
               GRAY, RST, DIM, "-", RST, GRAY, RST);
    }
    out += line;
  }

  appendRule(out, "\xE2\x95\xB0", "\xE2\x95\xAF");  // ╰ ... ╯

  Serial.print(out);
}

} // namespace

void terminalBegin() {
  Serial.setTxBufferSize(2048);
  Serial.begin(115200);
  delay(50);
  screenCleared = false;
  lastRedraw = 0;
  terminalLog("Sistem siap - AP %s", cfg::AP_SSID);
}

void terminalLog(const char *fmt, ...) {
  char msg[50];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  char upBuf[16];
  formatUptime(millis() / 1000, upBuf, sizeof(upBuf));
  snprintf(logRing[logHead], LOG_LEN, "%-8.8s  %s", upBuf, msg);

  logHead = (logHead + 1) % LOG_CAP;
  if (logCount < LOG_CAP) logCount++;
}

void terminalUpdate(unsigned long now) {
  if (now - lastRedraw < REDRAW_MS) return;
  lastRedraw = now;
  renderFrame(now);
}
