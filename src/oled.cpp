#include "oled.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include "automation.h"
#include "config.h"
#include "door.h"
#include "network.h"
#include "sensors.h"

namespace {
Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool active = false;
uint8_t address = 0x3C;
unsigned long lastScan = 0;
unsigned long lastDraw = 0;

bool detect(bool force) {
  if (active) return true;
  unsigned long now = millis();
  if (!force && now - lastScan < 1500) return false;
  lastScan = now;

  Wire.begin(cfg::PIN_SDA, cfg::PIN_SCL);
  Wire.setClock(100000);  // 100 kHz untuk handshake awal
  Wire.beginTransmission(0x3C);
  byte err3C = Wire.endTransmission();
  Wire.beginTransmission(0x3D);
  byte err3D = Wire.endTransmission();

  uint8_t target = (err3C == 0) ? 0x3C : (err3D == 0) ? 0x3D : 0;
  if (target != 0) {
    Serial.printf("[OLED] Terdeteksi chip SSD1306 di alamat 0x%02X! Memulai inisialisasi...\n",
                  target);
    if (display.begin(SSD1306_SWITCHCAPVCC, target)) {
      active = true;
      address = target;
      Wire.setClock(400000);
      display.clearDisplay();
      display.display();
      oledShowSplash();
      Serial.printf("[OLED] SUKSES! Layar OLED aktif di 0x%02X\n", target);
      return true;
    }
    Serial.println("[OLED] Gagal memanggil display.begin()");
  } else if (err3C >= 4 || err3D >= 4) {
    // Reset bus bila macet
    Wire.end();
    delay(5);
    Wire.begin(cfg::PIN_SDA, cfg::PIN_SCL);
  }
  return false;
}

void header() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(18, 2);
  display.print("RUMAH JOGLO IoT");
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);
}

void footerIp() {
  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(4, 54);
  display.print("IP: ");
  display.print(networkIp());
}

void drawFire() {
  display.clearDisplay();
  display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(12, 2);
  display.print("! PERINGATAN API !");
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(16, 22);
  display.print("ADA API!");
  display.setTextSize(1);
  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(4, 54);
  display.print("Peringatan Api (D34)");
  display.display();
}

void drawOpen(unsigned long remainingMs) {
  header();
  display.setCursor(22, 20);
  display.print(doorPhase() == DoorPhase::Opening ? "PINTU MEMBUKA" : "PINTU TERBUKA");
  display.setCursor(16, 32);
  int mode = triggerMode();
  if (mode == MODE_TOUCH) {
    display.print("Sentuhan Terdeteksi");
  } else if (mode == MODE_FLAME && doorIsEvacuating()) {
    display.print("Pemicu Sensor Api");
  } else {
    display.print("Selamat Datang!");
  }
  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(4, 54);
  if (doorIsEvacuating()) {
    display.print("Ditahan: ada api");
  } else {
    display.printf("Tutup dlm: %.1fs", remainingMs / 1000.0f);
  }
  display.display();
}

void drawClosing() {
  header();
  display.setCursor(22, 22);
  display.print("PINTU MENUTUP");
  display.setCursor(20, 36);
  display.print("Mohon tunggu...");
  footerIp();
  display.display();
}

void drawStandby() {
  header();
  display.setCursor(4, 16);
  display.print("Pintu : ");
  if (doorIsManualOpen()) {
    display.print("TERBUKA (MAN)");
  } else if (doorIsAutoCycle()) {
    display.print("TERBUKA (AUTO)");
  } else {
    display.print("TERTUTUP");
  }

  display.setCursor(4, 27);
  int mode = triggerMode();
  if (mode == MODE_TOUCH) {
    display.print("Pemicu: Sentuh D4");
  } else if (mode == MODE_FLAME) {
    display.print("Pemicu: Sensor Api");
  } else if (mode == MODE_ULTRASONIC) {
    display.print("Jarak : ");
    int d = ultrasonicDistance();
    if (d > 0 && d <= 300) {
      display.printf("%d cm", d);
    } else {
      display.print("Standby");
    }
  } else {
    display.print("Pemicu: Manual Web");
  }

  display.setCursor(4, 38);
  display.print("Api   : ");
  display.print(flameDetected() ? "! BAHAYA !" : "AMAN");
  footerIp();
  display.display();
}
} // namespace

void oledBegin() {
  Wire.begin(cfg::PIN_SDA, cfg::PIN_SCL);
  detect(true);
}

void oledShowBoot(const char *statusText) {
  if (!active) return;
  header();
  display.setCursor(8, 28);
  display.print(statusText);
  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(8, 54);
  display.print("Memulai sistem...");
  display.display();
}

void oledShowSplash() {
  if (!active) return;
  header();
  display.setCursor(8, 22);
  display.print(networkIsAccessPoint() ? "Mode: AP Mandiri" : "Mode: WiFi Station");
  display.setCursor(8, 34);
  display.print("Status: Siap Aktif");
  footerIp();
  display.display();
}

void oledUpdate(unsigned long now) {
  if (!active) {
    detect(false);
    return;
  }
  if (now - lastDraw < 200) return;
  lastDraw = now;

  DoorPhase phase = doorPhase();
  if (phase == DoorPhase::Closing) {
    drawClosing();
  } else if (doorIsAutoCycle() && (phase == DoorPhase::Opening || phase == DoorPhase::Open)) {
    drawOpen(doorHoldRemainingMs(now));
  } else if (flameDetected() && triggerMode() != MODE_FLAME) {
    drawFire();
  } else {
    drawStandby();
  }
}

bool oledIsActive() { return active; }
uint8_t oledAddress() { return address; }
