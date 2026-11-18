#include "oled.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <math.h>
#include <string.h>

#include "app_network.h"
#include "config.h"
#include "sensors.h"

namespace {

Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool active = false;
uint8_t address = 0x3C;
unsigned long lastScan = 0;
unsigned long lastDraw = 0;
uint16_t animFrame = 0;

// ============================================================================
// BITMAP WAYANG KULIT DUA KARAKTER (PROGMEM - 20x36 pixel)
// ============================================================================

// Wayang Kiri: 20x36 pixel (Gagah / Bima / Gatotkaca)
const unsigned char PROGMEM wayang_left_bmp[] = {
  0x00, 0x30, 0x00, 0x00, 0x78, 0x00, 0x00, 0x78, 0x00, 0x01, 0x78, 0x00,
  0x01, 0xFC, 0x00, 0x0B, 0xFC, 0x00, 0x0F, 0xFC, 0x00, 0x0F, 0xFC, 0x00,
  0x0F, 0xFE, 0x00, 0x0F, 0xDE, 0x00, 0x1F, 0xBF, 0x00, 0x1F, 0xFC, 0x00,
  0x1F, 0xF8, 0x00, 0x1F, 0xF8, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0x6C, 0x00,
  0x3F, 0x6C, 0x00, 0x3F, 0x4E, 0x00, 0x3F, 0x86, 0x00, 0x3F, 0xA6, 0x00,
  0x3F, 0xA6, 0x00, 0x3F, 0xF2, 0x00, 0x3F, 0xF3, 0x00, 0x3F, 0xF3, 0x00,
  0x3F, 0xF1, 0x00, 0x7F, 0xF9, 0x80, 0x7F, 0xF1, 0x80, 0xFF, 0xF1, 0xB0,
  0xFF, 0xF0, 0xF0, 0xFF, 0xF0, 0x60, 0xFB, 0xB0, 0x00, 0xF2, 0xF9, 0x00,
  0xFE, 0x7A, 0x00, 0xF8, 0x1C, 0x00, 0xFF, 0xFE, 0x00, 0x7F, 0xFE, 0x00
};

// Wayang Kanan: 20x36 pixel (Satria / Arjuna / Janaka)
const unsigned char PROGMEM wayang_right_bmp[] = {
  0x08, 0x00, 0x00, 0x1E, 0xC0, 0x00, 0x1F, 0x80, 0x00, 0x3F, 0xC0, 0x00,
  0x3F, 0xC8, 0x00, 0x3F, 0xF0, 0x00, 0x3F, 0xF0, 0x00, 0x7F, 0xF0, 0x00,
  0x77, 0xF0, 0x00, 0x7B, 0xF0, 0x00, 0x7F, 0xF0, 0x00, 0x0F, 0xFF, 0x00,
  0x0D, 0xFF, 0xC0, 0x0D, 0xF0, 0xE0, 0x1A, 0xF0, 0xE0, 0x1A, 0xF8, 0x60,
  0x19, 0xF8, 0x30, 0x19, 0xFC, 0x30, 0x19, 0xFC, 0x30, 0x1B, 0xFC, 0x60,
  0x1F, 0xF8, 0x40, 0x17, 0xF8, 0xC0, 0x17, 0xFD, 0x80, 0x1F, 0xFF, 0x80,
  0x1F, 0xFF, 0x00, 0x1F, 0xFF, 0x00, 0x3F, 0xFF, 0x00, 0x3F, 0xFF, 0x00,
  0xEC, 0xEF, 0x00, 0xAC, 0xEF, 0x00, 0x1C, 0xFF, 0x00, 0x1C, 0x5F, 0x80,
  0x1E, 0xEF, 0x80, 0x3F, 0xFE, 0x00, 0x3F, 0xFE, 0x00, 0x00, 0x42, 0x00
};

// Ikon Api (12x12)
const unsigned char PROGMEM flame_f0_bmp[] = {
  0x06, 0x00, 0x0F, 0x00, 0x1F, 0x80, 0x37, 0xC0,
  0x63, 0xC0, 0xE1, 0xE0, 0xC0, 0xE0, 0xE0, 0xE0,
  0x71, 0xC0, 0x3B, 0x80, 0x1F, 0x00, 0x0E, 0x00
};
const unsigned char PROGMEM flame_f1_bmp[] = {
  0x03, 0x00, 0x07, 0x80, 0x1F, 0x80, 0x3F, 0xC0,
  0x79, 0xC0, 0xF0, 0xE0, 0xE0, 0x70, 0x70, 0x70,
  0x38, 0xE0, 0x1D, 0xC0, 0x0F, 0x80, 0x07, 0x00
};

// Mikro-ikon UI (PROGMEM)
// Tetes Kelembaban (5x7)
const unsigned char PROGMEM drop_icon_bmp[] = {
  0x20, 0x20, 0x70, 0xF8, 0xD8, 0xF8, 0x70
};
// Perisai Siaga Api (7x7)
const unsigned char PROGMEM shield_icon_bmp[] = {
  0x7C, 0xFE, 0xD6, 0xC6, 0x7C, 0x38, 0x10
};

// ============================================================================
// UTILITAS GAMBAR & DETEKSI HARDWARE
// ============================================================================

void drawCentered(int16_t y, const char *txt, int16_t minX = 0, int16_t maxX = 128) {
  int16_t w = (int16_t)strlen(txt) * 6;
  int16_t x = minX + ((maxX - minX) - w) / 2;
  if (x < minX) x = minX;
  display.setCursor(x, y);
  display.print(txt);
}

bool detect(bool force) {
  if (active) return true;
  unsigned long now = millis();
  if (!force && now - lastScan < 1500) return false;
  lastScan = now;

  Wire.begin(cfg::PIN_SDA, cfg::PIN_SCL);
  Wire.setClock(100000);
  Wire.beginTransmission(0x3C);
  byte err3C = Wire.endTransmission();
  Wire.beginTransmission(0x3D);
  byte err3D = Wire.endTransmission();

  uint8_t target = (err3C == 0) ? 0x3C : (err3D == 0) ? 0x3D : 0;
  if (target != 0) {
    Serial.printf("[OLED] Terdeteksi chip SSD1306 di alamat 0x%02X! Memulai inisialisasi...\n", target);
    if (display.begin(SSD1306_SWITCHCAPVCC, target)) {
      active = true;
      address = target;
      Wire.setClock(400000);
      display.clearDisplay();
      display.display();
      Serial.printf("[OLED] SUKSES! Layar OLED aktif di 0x%02X\n", target);
      return true;
    }
  } else if (err3C >= 4 || err3D >= 4) {
    Wire.end();
    delay(5);
    Wire.begin(cfg::PIN_SDA, cfg::PIN_SCL);
  }
  return false;
}

// ============================================================================
// HEADER BERSIH (ZONA KUNING Y = 0..14: HANYA ALAMAT IP)
// ============================================================================

void drawIpHeader() {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  char buf[24];
  snprintf(buf, sizeof(buf), "IP: %s", networkIp().c_str());
  drawCentered(3, buf);
  display.drawFastHLine(0, 14, 128, SSD1306_WHITE);
}

void drawWayangPair(int8_t offsetX) {
  // Dipusatkan secara vertikal pada zona biru (Y = 16..63 -> 21..56)
  display.drawBitmap(2 - offsetX, 21, wayang_left_bmp, 20, 36, SSD1306_WHITE);
  display.drawBitmap(106 + offsetX, 21, wayang_right_bmp, 20, 36, SSD1306_WHITE);
}

// ============================================================================
// TAMPILAN UTAMA CLEAN (IP + WAYANG + SUHU + STATUS API)
// ============================================================================

void drawCleanMain() {
  display.clearDisplay();
  drawIpHeader();

  // Ayunan halus 1 px pada sepasang wayang
  int8_t sway = ((animFrame / 24) % 2 == 0) ? 0 : 1;
  drawWayangPair(sway);

  // --- AREA TENGAH CLEAN (X = 24..104, Y = 16..63) ---

  // 1. Suhu Utama Besar & Jelas (Y = 19..34)
  char tempBuf[8];
  if (dhtIsValid()) {
    snprintf(tempBuf, sizeof(tempBuf), "%.1f", dhtTemperature());
  } else {
    snprintf(tempBuf, sizeof(tempBuf), "--.-");
  }

  int16_t len = (int16_t)strlen(tempBuf);
  int16_t totalW = len * 12 + 16;  // angka (size 2) + simbol derajat + 'C' (size 2)
  int16_t startX = 24 + (80 - totalW) / 2;

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(startX, 19);
  display.print(tempBuf);

  int16_t degX = startX + len * 12 + 1;
  display.drawCircle(degX, 22, 2, SSD1306_WHITE);
  display.setCursor(degX + 5, 19);
  display.print("C");

  // 2. Keterangan Kelembaban (Y = 38..45)
  display.setTextSize(1);
  char humBuf[16];
  if (dhtIsValid()) {
    snprintf(humBuf, sizeof(humBuf), "Lembab %d%%", (int)lroundf(dhtHumidity()));
  } else {
    snprintf(humBuf, sizeof(humBuf), "Lembab --%%");
  }
  int16_t humW = 7 + (int16_t)strlen(humBuf) * 6;
  int16_t humX = 24 + (80 - humW) / 2;
  display.drawBitmap(humX, 38, drop_icon_bmp, 5, 7, SSD1306_WHITE);
  display.setCursor(humX + 7, 38);
  display.print(humBuf);

  // 3. Keterangan Status Api dalam Pill Rapi (Y = 49..61)
  display.drawRoundRect(26, 49, 76, 12, 3, SSD1306_WHITE);
  display.drawBitmap(31, 51, shield_icon_bmp, 7, 7, SSD1306_WHITE);
  display.setCursor(41, 51);
  display.print("API: AMAN");

  display.display();
}

// ============================================================================
// PERINGATAN DARURAT BAHAYA API
// ============================================================================

void drawFire(unsigned long now) {
  display.clearDisplay();

  bool flash = (now / 220) % 2;
  uint16_t fg = flash ? SSD1306_BLACK : SSD1306_WHITE;
  if (flash) {
    display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
  }
  display.setTextColor(fg);

  display.drawBitmap(0, 21, wayang_left_bmp, 20, 36, fg);
  display.drawBitmap(108, 21, wayang_right_bmp, 20, 36, fg);

  display.setTextSize(1);
  drawCentered(3, "! PERINGATAN API !");
  display.drawFastHLine(0, 14, 128, fg);

  const unsigned char *flameBmp = ((now / 150) % 2 == 0) ? flame_f0_bmp : flame_f1_bmp;
  display.drawBitmap(28, 22, flameBmp, 12, 12, fg);

  display.setCursor(44, 24);
  display.print("ADA API!");

  drawCentered(40, "EVAKUASI AKTIF", 21, 107);
  drawCentered(52, "PINTU TERBUKA", 21, 107);

  display.display();
}

} // namespace

// ============================================================================
// FUNGSI PUBLIK OLED
// ============================================================================

void oledBegin() {
  Wire.begin(cfg::PIN_SDA, cfg::PIN_SCL);
  detect(true);
}

void oledShowBoot(const char *statusText) {
  if (!active) return;
  display.clearDisplay();

  drawIpHeader();
  drawWayangPair(0);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  drawCentered(34, "Memulai...", 24, 104);

  display.drawRoundRect(26, 48, 76, 12, 2, SSD1306_WHITE);
  // Tampilkan status singkat di bagian bawah bila muat, atau di kotak penuh
  if (strlen(statusText) <= 12) {
    drawCentered(50, statusText, 27, 101);
  } else {
    display.fillRect(4, 48, 120, 13, SSD1306_BLACK);
    display.drawRoundRect(4, 48, 120, 13, 2, SSD1306_WHITE);
    drawCentered(50, statusText, 6, 122);
  }

  display.display();
}

void oledShowSplash() {
  if (!active) return;
  display.clearDisplay();

  drawIpHeader();
  drawWayangPair(0);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  drawCentered(28, "Sistem Siap", 24, 104);

  display.fillRoundRect(30, 42, 68, 11, 2, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  drawCentered(44, "ONLINE", 30, 98);
  display.setTextColor(SSD1306_WHITE);

  display.display();
  delay(150);
}

void oledUpdate(unsigned long now) {
  if (!active) {
    detect(false);
    return;
  }
  // Frame refresh 50 ms (~20 FPS)
  if (now - lastDraw < 50) return;
  lastDraw = now;
  animFrame++;

  if (flameDetected()) {
    drawFire(now);
  } else {
    drawCleanMain();
  }
}

bool oledIsActive() { return active; }
uint8_t oledAddress() { return address; }
