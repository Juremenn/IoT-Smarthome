#include "oled.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "automation.h"
#include "config.h"
#include "door.h"
#include "app_network.h"
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
// Proporsional & estetik: menyisakan ruang tengah 82 px yang lapang dan rapi
// ============================================================================

// Wayang Kiri: 20x36 pixel (Gagah / Bima / Gatotkaca)
const unsigned char PROGMEM wayang_left_bmp[] = {
  0x00, 0x30, 0x00,
  0x00, 0x78, 0x00,
  0x00, 0x78, 0x00,
  0x01, 0x78, 0x00,
  0x01, 0xFC, 0x00,
  0x0B, 0xFC, 0x00,
  0x0F, 0xFC, 0x00,
  0x0F, 0xFC, 0x00,
  0x0F, 0xFE, 0x00,
  0x0F, 0xDE, 0x00,
  0x1F, 0xBF, 0x00,
  0x1F, 0xFC, 0x00,
  0x1F, 0xF8, 0x00,
  0x1F, 0xF8, 0x00,
  0x3F, 0xFC, 0x00,
  0x3F, 0x6C, 0x00,
  0x3F, 0x6C, 0x00,
  0x3F, 0x4E, 0x00,
  0x3F, 0x86, 0x00,
  0x3F, 0xA6, 0x00,
  0x3F, 0xA6, 0x00,
  0x3F, 0xF2, 0x00,
  0x3F, 0xF3, 0x00,
  0x3F, 0xF3, 0x00,
  0x3F, 0xF1, 0x00,
  0x7F, 0xF9, 0x80,
  0x7F, 0xF1, 0x80,
  0xFF, 0xF1, 0xB0,
  0xFF, 0xF0, 0xF0,
  0xFF, 0xF0, 0x60,
  0xFB, 0xB0, 0x00,
  0xF2, 0xF9, 0x00,
  0xFE, 0x7A, 0x00,
  0xF8, 0x1C, 0x00,
  0xFF, 0xFE, 0x00,
  0x7F, 0xFE, 0x00
};

// Wayang Kanan: 20x36 pixel (Satria / Arjuna / Janaka)
const unsigned char PROGMEM wayang_right_bmp[] = {
  0x08, 0x00, 0x00,
  0x1E, 0xC0, 0x00,
  0x1F, 0x80, 0x00,
  0x3F, 0xC0, 0x00,
  0x3F, 0xC8, 0x00,
  0x3F, 0xF0, 0x00,
  0x3F, 0xF0, 0x00,
  0x7F, 0xF0, 0x00,
  0x77, 0xF0, 0x00,
  0x7B, 0xF0, 0x00,
  0x7F, 0xF0, 0x00,
  0x0F, 0xFF, 0x00,
  0x0D, 0xFF, 0xC0,
  0x0D, 0xF0, 0xE0,
  0x1A, 0xF0, 0xE0,
  0x1A, 0xF8, 0x60,
  0x19, 0xF8, 0x30,
  0x19, 0xFC, 0x30,
  0x19, 0xFC, 0x30,
  0x1B, 0xFC, 0x60,
  0x1F, 0xF8, 0x40,
  0x17, 0xF8, 0xC0,
  0x17, 0xFD, 0x80,
  0x1F, 0xFF, 0x80,
  0x1F, 0xFF, 0x00,
  0x1F, 0xFF, 0x00,
  0x3F, 0xFF, 0x00,
  0x3F, 0xFF, 0x00,
  0xEC, 0xEF, 0x00,
  0xAC, 0xEF, 0x00,
  0x1C, 0xFF, 0x00,
  0x1C, 0x5F, 0x80,
  0x1E, 0xEF, 0x80,
  0x3F, 0xFE, 0x00,
  0x3F, 0xFE, 0x00,
  0x00, 0x42, 0x00
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

// ============================================================================
// HARDWARE DETECTION & SETUP
// ============================================================================

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
// HEADER & FOOTER BERSIH
// ============================================================================

void drawHeader() {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Badge Hotspot AP di kanan atas (tampilan clean tanpa tulisan judul)
  display.drawRoundRect(106, 1, 20, 9, 2, SSD1306_WHITE);
  display.setCursor(110, 2);
  display.print("AP");

  display.drawFastHLine(0, 11, 128, SSD1306_WHITE);
}

void drawFooter() {
  display.drawFastHLine(0, 52, 128, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  // Alamat IP tepat di tengah bawah (simetris)
  display.setCursor(19, 54);
  display.printf("IP: %s", networkIp().c_str());
}

// ============================================================================
// TAMPILAN STANDBY (RAPI, SIMETRIS, LAPANG)
// ============================================================================

void drawStandby() {
  display.clearDisplay();
  drawHeader();

  // Animasi goyang lembut kedua wayang
  int step = (animFrame / 12) % 4;
  int posL = (step == 1) ? 2 : ((step == 3) ? 1 : 2);
  int posR = (step == 1) ? 106 : ((step == 3) ? 107 : 106);

  // 1. Wayang Kiri (Bima/Gatotkaca) di X = 2, Y = 14
  display.drawBitmap(posL, 14, wayang_left_bmp, 20, 36, SSD1306_WHITE);

  // 2. Wayang Kanan (Arjuna) di X = 106, Y = 14
  display.drawBitmap(posR, 14, wayang_right_bmp, 20, 36, SSD1306_WHITE);

  // --- AREA TENGAH BERSIH & RAPI (X = 24..104, Lebar 80 px) ---

  // Baris 1 (Y = 14): Status Pintu (Badge Terpusat)
  if (doorIsManualOpen()) {
    display.fillRoundRect(27, 14, 74, 10, 2, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(33, 15);
    display.print("BUKA (MAN)");
    display.setTextColor(SSD1306_WHITE);
  } else if (doorIsAutoCycle()) {
    display.fillRoundRect(27, 14, 74, 10, 2, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(35, 15);
    display.print("TERBUKA");
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.drawRoundRect(27, 14, 74, 10, 2, SSD1306_WHITE);
    display.setCursor(32, 15);
    display.print("PINTU RAPAT");
  }

  // Baris 2 (Y = 27): Suhu & Kelembaban (DHT11)
  if (dhtIsValid()) {
    display.setCursor(32, 27);
    display.printf("%.1f C | %d%%", dhtTemperature(), (int)dhtHumidity());
  } else {
    display.setCursor(36, 27);
    display.print("Suhu: --.- C");
  }

  // Baris 3 (Y = 39): Sensor Pemicu Interaktif
  int mode = triggerMode();
  if (mode == MODE_ULTRASONIC) {
    int dist = ultrasonicDistance();
    display.setCursor(30, 39);
    if (dist > 0 && dist <= 250) {
      display.printf("JARAK: %d cm", dist);
    } else {
      display.print("JARAK: -- cm");
    }
  } else if (mode == MODE_TOUCH) {
    if (touchIsTouched()) {
      display.fillRoundRect(27, 38, 74, 10, 2, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(30, 39);
      display.print("* TERSENTUH *");
      display.setTextColor(SSD1306_WHITE);
    } else {
      display.setCursor(30, 39);
      display.print("SENTUH: SIAP");
    }
  } else if (mode == MODE_FLAME) {
    display.setCursor(32, 39);
    display.print("SENSOR: AMAN");
  } else {
    display.setCursor(32, 39);
    display.print("KONTROL: WEB");
  }

  drawFooter();
  display.display();
}

// ============================================================================
// ANIMASI PINTU MEMBUKA / TERBUKA (2 WAYANG MEMBUKA GAPURA)
// ============================================================================

void drawOpen(unsigned long remainingMs) {
  display.clearDisplay();
  drawHeader();

  // Kedua wayang bergeser ke tepi layar membuka gapura
  display.drawBitmap(0, 14, wayang_left_bmp, 20, 36, SSD1306_WHITE);
  display.drawBitmap(108, 14, wayang_right_bmp, 20, 36, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 15);
  display.print(doorPhase() == DoorPhase::Opening ? "PINTU BUKA" : "PINTU BUKA");

  display.setCursor(26, 27);
  int mode = triggerMode();
  if (mode == MODE_TOUCH) {
    display.print("Via: Sentuhan");
  } else if (mode == MODE_FLAME && doorIsEvacuating()) {
    display.print("Evakuasi Api!");
  } else if (mode == MODE_ULTRASONIC) {
    display.print("Objek Dekat");
  } else {
    display.print("Selamat Datang");
  }

  // Bar progress countdown menutup
  if (!doorIsEvacuating()) {
    float frac = (float)remainingMs / (float)cfg::DOOR_HOLD_MS;
    int fillW = constrain((int)(frac * 74), 0, 74);
    display.drawRoundRect(26, 39, 76, 7, 2, SSD1306_WHITE);
    if (fillW > 0) {
      display.fillRect(27, 40, fillW, 5, SSD1306_WHITE);
    }
  }

  drawFooter();
  display.display();
}

// ============================================================================
// ANIMASI PINTU MENUTUP
// ============================================================================

void drawClosing() {
  display.clearDisplay();
  drawHeader();

  display.drawBitmap(2, 14, wayang_left_bmp, 20, 36, SSD1306_WHITE);
  display.drawBitmap(106, 14, wayang_right_bmp, 20, 36, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 18);
  display.print("MENUTUP...");

  display.setCursor(28, 30);
  display.print("Hati - Hati");

  int arrowCycle = (animFrame / 3) % 4;
  display.setCursor(38 + (arrowCycle * 4), 40);
  display.print("> >");

  drawFooter();
  display.display();
}

// ============================================================================
// PERINGATAN DARURAT BAHAYA API
// ============================================================================

void drawFire(unsigned long now) {
  display.clearDisplay();

  bool flash = (now / 200) % 2;
  if (flash) {
    display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }

  display.drawBitmap(2, 14, wayang_left_bmp, 20, 36, flash ? SSD1306_BLACK : SSD1306_WHITE);
  display.drawBitmap(106, 14, wayang_right_bmp, 20, 36, flash ? SSD1306_BLACK : SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(12, 3);
  display.print("! PERINGATAN API !");
  display.drawFastHLine(0, 12, 128, flash ? SSD1306_BLACK : SSD1306_WHITE);

  const unsigned char *flameBmp = ((now / 150) % 2 == 0) ? flame_f0_bmp : flame_f1_bmp;
  display.drawBitmap(30, 20, flameBmp, 12, 12, flash ? SSD1306_BLACK : SSD1306_WHITE);

  display.setCursor(46, 22);
  display.print("ADA API!");

  display.setCursor(28, 36);
  display.print("EVAKUASI AKTIF");

  display.drawFastHLine(0, 50, 128, flash ? SSD1306_BLACK : SSD1306_WHITE);
  display.setCursor(26, 54);
  display.print("PINTU TERBUKA");

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

  display.drawBitmap(4, 14, wayang_left_bmp, 20, 36, SSD1306_WHITE);
  display.drawBitmap(104, 14, wayang_right_bmp, 20, 36, SSD1306_WHITE);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(30, 16);
  display.print("RUMAH JOGLO");

  display.setCursor(34, 28);
  display.print("Smart Home");

  display.drawRoundRect(26, 44, 76, 12, 2, SSD1306_WHITE);
  display.setCursor(30, 46);
  display.print(statusText);

  display.display();
}

void oledShowSplash() {
  if (!active) return;
  display.clearDisplay();

  display.drawBitmap(4, 14, wayang_left_bmp, 20, 36, SSD1306_WHITE);
  display.drawBitmap(104, 14, wayang_right_bmp, 20, 36, SSD1306_WHITE);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(30, 16);
  display.print("RUMAH JOGLO");

  display.setCursor(34, 28);
  display.print("Sistem Siap");

  display.setCursor(30, 46);
  display.print("AP: Mandiri");

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

  DoorPhase phase = doorPhase();
  if (flameDetected()) {
    drawFire(now);
  } else if (phase == DoorPhase::Closing) {
    drawClosing();
  } else if (doorIsAutoCycle() && (phase == DoorPhase::Opening || phase == DoorPhase::Open)) {
    drawOpen(doorHoldRemainingMs(now));
  } else {
    drawStandby();
  }
}

bool oledIsActive() { return active; }
uint8_t oledAddress() { return address; }
