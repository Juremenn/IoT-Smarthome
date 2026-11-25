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

// Hierarki Warna ANSI Semantik (Industrial Engineering Telemetry)
constexpr const char *ANSI_RESET     = "\x1b[0m";
constexpr const char *ANSI_GRAY      = "\x1b[90m";
constexpr const char *ANSI_WHITE     = "\x1b[37m";
constexpr const char *ANSI_BWHITE    = "\x1b[1;37m";
constexpr const char *ANSI_GREEN     = "\x1b[92m";
constexpr const char *ANSI_CYAN      = "\x1b[96m";
constexpr const char *ANSI_YELLOW    = "\x1b[93m";
constexpr const char *ANSI_BRED      = "\x1b[1;91m";
constexpr const char *ANSI_ALERT_BG  = "\x1b[1;41;97m";

// Konfigurasi Layout & Timing (Fixed-Width 78 Kolom, Open-Border)
constexpr int TUI_WIDTH = 78;
constexpr int COL_PIN = 19;
constexpr int COL_STATUS = 31;
constexpr int COL_DETAIL = 49;
constexpr int BAR_WIDTH = 12;

constexpr int LOG_CAP = 5;
constexpr int LOG_MSG_LEN = 59;  // 58 char + NUL (10 + 1 + 6 + 1 + 58 = 76 <= 78)
constexpr unsigned long REDRAW_MS = 300;
constexpr unsigned long EVENT_MIN_MS = 80;

struct LogEntry {
  uint32_t uptimeSec;
  TerminalTag tag;
  char msg[LOG_MSG_LEN];
};

// Buffer statis tunggal di BSS: nol fragmentasi heap (zero String)
char frameBuf[2048];
LogEntry logRing[LOG_CAP];
int logCount = 0;
int logHead = 0;
unsigned long lastRedraw = 0;
bool screenInitialized = false;
bool logDirty = true;

// Writer ringan berbasis snprintf ke buffer statis
class FrameWriter {
 public:
  FrameWriter(char *buf, size_t cap) : buf_(buf), cap_(cap), pos_(0) {
    if (cap_ > 0) buf_[0] = '\0';
  }

  void append(const char *s) {
    if (!s || pos_ + 1 >= cap_) return;
    size_t n = strlen(s);
    size_t avail = cap_ - 1 - pos_;
    if (n > avail) n = avail;
    memcpy(buf_ + pos_, s, n);
    pos_ += n;
    buf_[pos_] = '\0';
  }

  void appendRepeat(char ch, int count) {
    if (count <= 0 || pos_ + 1 >= cap_) return;
    size_t n = (size_t)count;
    size_t avail = cap_ - 1 - pos_;
    if (n > avail) n = avail;
    memset(buf_ + pos_, ch, n);
    pos_ += n;
    buf_[pos_] = '\0';
  }

  void appendf(const char *fmt, ...) {
    if (pos_ + 1 >= cap_) return;
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(buf_ + pos_, cap_ - pos_, fmt, args);
    va_end(args);
    if (written <= 0) return;
    size_t adv = (size_t)written;
    size_t avail = cap_ - 1 - pos_;
    pos_ += (adv < avail) ? adv : avail;
  }

  void endLine() { append("\x1b[K\r\n"); }
  void moveToCol(int col) { appendf("\x1b[%dG", col); }

  const char *data() const { return buf_; }
  size_t len() const { return pos_; }

 private:
  char *buf_;
  size_t cap_;
  size_t pos_;
};

void formatUptime(uint32_t sec, char *out, size_t n) {
  uint32_t h = (sec / 3600U) % 100U;
  uint32_t m = (sec % 3600U) / 60U;
  uint32_t s = sec % 60U;
  snprintf(out, n, "%02lu:%02lu:%02lu",
           (unsigned long)h, (unsigned long)m, (unsigned long)s);
}

void sanitizeAscii(char *s) {
  for (char *p = s; *p != '\0'; ++p) {
    unsigned char c = (unsigned char)*p;
    if (c < 0x20 || c > 0x7E) *p = ' ';
  }
}

void buildAsciiBar(float ratio, int width, char *out, size_t outSize) {
  if (outSize == 0) return;
  if (ratio < 0.0f) ratio = 0.0f;
  if (ratio > 1.0f) ratio = 1.0f;
  int maxW = (int)outSize - 1;
  if (width > maxW) width = maxW;
  int filled = (int)(ratio * (float)width + 0.5f);
  for (int i = 0; i < width; ++i) {
    out[i] = (i < filled) ? '#' : '.';
  }
  out[width] = '\0';
}

const char *tagLabel(TerminalTag tag) {
  switch (tag) {
    case TerminalTag::Door: return "[DOOR]";
    case TerminalTag::Us:   return "[US]  ";
    case TerminalTag::Fire: return "[FIRE]";
    case TerminalTag::Sys:
    default:                return "[SYS] ";
  }
}

const char *tagColor(TerminalTag tag) {
  switch (tag) {
    case TerminalTag::Door: return ANSI_YELLOW;
    case TerminalTag::Us:   return ANSI_CYAN;
    case TerminalTag::Fire: return ANSI_BRED;
    case TerminalTag::Sys:
    default:                return ANSI_GREEN;
  }
}

void appendRule(FrameWriter &w, char ch) {
  w.append(ANSI_GRAY);
  w.appendRepeat(ch, TUI_WIDTH);
  w.append(ANSI_RESET);
  w.endLine();
}

void renderHeader(FrameWriter &w, unsigned long now) {
  char upBuf[12];
  formatUptime((uint32_t)(now / 1000UL), upBuf, sizeof(upBuf));

  // Baris 1: Judul kiri, Uptime rata kanan (kolom 64..78 = 15 char)
  w.appendf("%sRUMAH JOGLO // SMART HOME IOT%s", ANSI_BWHITE, ANSI_RESET);
  w.moveToCol(TUI_WIDTH - 15 + 1);
  w.appendf("%sUPTIME %s%s%s", ANSI_GRAY, ANSI_BWHITE, upBuf, ANSI_RESET);
  w.endLine();

  // Baris 2: Hardware/IP kiri, SSID AP rata kanan
  const char *ip = networkIp().c_str();
  w.appendf("%sESP32 | %shttp://%s%s", ANSI_GRAY, ANSI_CYAN, ip, ANSI_RESET);

  int apLen = 3 + (int)strlen(cfg::AP_SSID);  // "AP " + SSID
  int apCol = TUI_WIDTH - apLen + 1;
  if (apCol < 42) apCol = 42;
  w.moveToCol(apCol);
  w.appendf("%sAP %s%s%s", ANSI_GRAY, ANSI_WHITE, cfg::AP_SSID, ANSI_RESET);
  w.endLine();

  appendRule(w, '=');
}

void renderTelemetryRowPrefix(FrameWriter &w, const char *name, const char *pin) {
  w.appendf("%s%-16.16s%s", ANSI_WHITE, name, ANSI_RESET);
  w.moveToCol(COL_PIN);
  w.appendf("%s%-10.10s%s", ANSI_GRAY, pin, ANSI_RESET);
  w.moveToCol(COL_STATUS);
}

void renderTelemetryTable(FrameWriter &w) {
  // Header tabel
  w.appendf("%s%-16s", ANSI_GRAY, "SENSOR/AKTUATOR");
  w.moveToCol(COL_PIN);
  w.appendf("%-10s", "PIN");
  w.moveToCol(COL_STATUS);
  w.appendf("%-16s", "STATUS");
  w.moveToCol(COL_DETAIL);
  w.appendf("DETAIL / VISUAL BAR%s", ANSI_RESET);
  w.endLine();

  appendRule(w, '-');

  // 1. Pintu Utama (Servo D13)
  DoorPhase dp = doorPhase();
  float frac = doorOpenFraction();
  int pct = (int)(frac * 100.0f + 0.5f);
  char doorBar[BAR_WIDTH + 1];
  buildAsciiBar(frac, BAR_WIDTH, doorBar, sizeof(doorBar));

  const char *doorColor = ANSI_BWHITE;
  const char *doorLabel = "TERTUTUP";
  const char *barColor = ANSI_GREEN;
  if (doorIsEvacuating()) {
    doorColor = ANSI_ALERT_BG;
    doorLabel = "[EVAKUASI API]";
    barColor = ANSI_BRED;
  } else if (dp == DoorPhase::Opening) {
    doorColor = ANSI_YELLOW;
    doorLabel = "MEMBUKA...";
    barColor = ANSI_YELLOW;
  } else if (dp == DoorPhase::Closing) {
    doorColor = ANSI_CYAN;
    doorLabel = "MENUTUP...";
    barColor = ANSI_CYAN;
  } else if (dp == DoorPhase::Open) {
    doorColor = ANSI_GREEN;
    doorLabel = doorIsAutoCycle() ? "TERBUKA (AUTO)" : "TERBUKA MANUAL";
    barColor = ANSI_GREEN;
  }

  renderTelemetryRowPrefix(w, "Pintu Utama", "D13");
  w.appendf("%s%-14s%s", doorColor, doorLabel, ANSI_RESET);
  w.moveToCol(COL_DETAIL);
  w.appendf("%s[%s%s%s] %s%3d%%%s",
            ANSI_GRAY, barColor, doorBar, ANSI_GRAY,
            ANSI_BWHITE, pct, ANSI_RESET);
  w.endLine();

  // 2. Sensor Api (D34)
  bool fire = flameDetected();
  renderTelemetryRowPrefix(w, "Sensor Api", "D34");
  w.appendf("%s%-14s%s",
            fire ? ANSI_ALERT_BG : ANSI_GREEN,
            fire ? "[BAHAYA: API!]" : "AMAN",
            ANSI_RESET);
  w.moveToCol(COL_DETAIL);
  w.appendf("%sRaw:%d  %s%s",
            ANSI_GRAY, flameRaw(),
            flameActiveHigh() ? "Active-HIGH" : "Active-LOW",
            ANSI_RESET);
  w.endLine();

  // 3. Ultrasonik (HC-SR04 D25/D26)
  int dist = ultrasonicDistance();
  bool nearObj = (dist > 0 && dist <= cfg::MAX_DISTANCE_CM);
  char usPin[16];
  snprintf(usPin, sizeof(usPin), "D%d/D%d", ultrasonicTrigPin(), ultrasonicEchoPin());

  char distText[16];
  if (dist > 0) {
    snprintf(distText, sizeof(distText), "%3d cm", dist);
  } else {
    snprintf(distText, sizeof(distText), " -- cm");
  }

  float distRatio = (dist > 0) ? (1.0f - (float)min(dist, 60) / 60.0f) : 0.0f;
  char distBar[BAR_WIDTH + 1];
  buildAsciiBar(distRatio, BAR_WIDTH, distBar, sizeof(distBar));

  const char *usColor = nearObj ? ANSI_YELLOW : ANSI_CYAN;
  renderTelemetryRowPrefix(w, "Ultrasonik", usPin);
  w.appendf("%s%s%s", usColor, distText, ANSI_RESET);
  w.moveToCol(COL_DETAIL);
  if (nearObj) {
    w.appendf("%s[%s%s%s] %sPICU <=%dcm%s",
              ANSI_GRAY, ANSI_YELLOW, distBar, ANSI_GRAY,
              ANSI_YELLOW, cfg::MAX_DISTANCE_CM, ANSI_RESET);
  } else {
    w.appendf("%s[%s%s%s] %s%s%s",
              ANSI_GRAY, ANSI_CYAN, distBar, ANSI_GRAY,
              ANSI_GRAY, (dist > 0) ? "Standby" : "Luar jangkau", ANSI_RESET);
  }
  w.endLine();

  // 4. Suhu & Kelembaban (DHT11 D27)
  char dhtVal[20];
  const char *dhtColor = ANSI_YELLOW;
  const char *dhtDetail = "DHT11 Menunggu...";
  if (dhtIsValid()) {
    snprintf(dhtVal, sizeof(dhtVal), "%4.1f C / %2.0f%%", dhtTemperature(), dhtHumidity());
    dhtColor = ANSI_GREEN;
    dhtDetail = dhtIsCalibrated() ? "DHT11 OK (CAL)" : "DHT11 OK";
  } else {
    snprintf(dhtVal, sizeof(dhtVal), "--.- C / --%%");
    dhtColor = ANSI_YELLOW;
    dhtDetail = dhtStatusMessage();
  }

  renderTelemetryRowPrefix(w, "Suhu & Lembab", "D27");
  w.appendf("%s%-14s%s", dhtColor, dhtVal, ANSI_RESET);
  w.moveToCol(COL_DETAIL);
  w.appendf("%s%.29s%s", ANSI_GRAY, dhtDetail, ANSI_RESET);
  w.endLine();

  // 5. Layar OLED (SSD1306 I2C D21/D22)
  bool oledOk = oledIsActive();
  char oledPin[12];
  if (oledOk) {
    snprintf(oledPin, sizeof(oledPin), "I2C 0x%02X", oledAddress());
  } else {
    snprintf(oledPin, sizeof(oledPin), "I2C --");
  }

  renderTelemetryRowPrefix(w, "Layar OLED", oledPin);
  w.appendf("%s%-14s%s",
            oledOk ? ANSI_GREEN : ANSI_BRED,
            oledOk ? "AKTIF" : "OFFLINE",
            ANSI_RESET);
  w.moveToCol(COL_DETAIL);
  w.appendf("%sSSD1306 D%d/D%d%s", ANSI_GRAY, cfg::PIN_SDA, cfg::PIN_SCL, ANSI_RESET);
  w.endLine();
}

void renderEventLog(FrameWriter &w) {
  appendRule(w, '-');
  w.appendf("%sEVENT LOG (%d TERAKHIR)%s", ANSI_BWHITE, LOG_CAP, ANSI_RESET);
  w.endLine();

  for (int i = 0; i < LOG_CAP; ++i) {
    if (i < logCount) {
      int idx = (logHead - logCount + i + LOG_CAP) % LOG_CAP;
      const LogEntry &e = logRing[idx];
      char ts[12];
      formatUptime(e.uptimeSec, ts, sizeof(ts));
      w.appendf("%s[%s]%s %s%s%s %s%.58s%s",
                ANSI_GRAY, ts, ANSI_RESET,
                tagColor(e.tag), tagLabel(e.tag), ANSI_RESET,
                ANSI_WHITE, e.msg, ANSI_RESET);
    } else {
      w.appendf("%s[--:--:--] [---]  -%s", ANSI_GRAY, ANSI_RESET);
    }
    w.endLine();
  }

  appendRule(w, '=');
}

void pushLogEntry(TerminalTag tag, const char *fmt, va_list args) {
  LogEntry &slot = logRing[logHead];
  slot.uptimeSec = (uint32_t)(millis() / 1000UL);
  slot.tag = tag;
  vsnprintf(slot.msg, sizeof(slot.msg), fmt, args);
  sanitizeAscii(slot.msg);

  logHead = (logHead + 1) % LOG_CAP;
  if (logCount < LOG_CAP) logCount++;
  logDirty = true;
}

void renderFrame(unsigned long now) {
  FrameWriter w(frameBuf, sizeof(frameBuf));

  if (!screenInitialized) {
    w.append("\x1b[2J\x1b[H\x1b[?25l");
  } else {
    w.append("\x1b[H");
  }

  renderHeader(w, now);
  renderTelemetryTable(w);
  renderEventLog(w);

  // Guard non-blocking: kirim hanya jika ruang TX UART mencukupi seluruh frame
  if ((size_t)Serial.availableForWrite() < w.len()) return;

  Serial.write((const uint8_t *)w.data(), w.len());
  screenInitialized = true;
  lastRedraw = now;
  logDirty = false;
}

}  // namespace

void terminalBegin() {
  Serial.setTxBufferSize(2048);
  Serial.begin(115200);
  screenInitialized = false;
  lastRedraw = 0;
  logCount = 0;
  logHead = 0;
  logDirty = true;
  terminalLogTag(TerminalTag::Sys, "Sistem siap - AP %s", cfg::AP_SSID);
}

void terminalLog(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  pushLogEntry(TerminalTag::Sys, fmt, args);
  va_end(args);
}

void terminalLogTag(TerminalTag tag, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  pushLogEntry(tag, fmt, args);
  va_end(args);
}

void terminalUpdate(unsigned long now) {
  unsigned long elapsed = now - lastRedraw;
  bool periodicDue = (elapsed >= REDRAW_MS);
  bool eventDue = (logDirty && elapsed >= EVENT_MIN_MS);
  if (!periodicDue && !eventDue) return;
  renderFrame(now);
}
