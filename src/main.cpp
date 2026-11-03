#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==========================================
// KONFIGURASI JARINGAN & WEB SERVER
// ==========================================
const char* WIFI_SSID = "167778";        // SSID WiFi Rumah Anda
const char* WIFI_PASS = "cek1+3=2";      // Password WiFi Rumah Anda

const char* AP_SSID = "RumahJoglo-SmartHome";
const char* AP_PASS = "";                // Bebas password

WebServer server(80);
String ipAddressStr = "192.168.4.1";
bool isAccessPoint = true;

// ==========================================
// KONFIGURASI LAYAR OLED SSD1306 (I2C)
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledAktif = false;
uint8_t oledAddress = 0x3C;
unsigned long lastOledScanAttempt = 0;

// ==========================================
// PIN HARDWARE
// ==========================================
int trigPin = 4;             // HC-SR04 Trig (Default D4)
int echoPin = 5;             // HC-SR04 Echo (Default D5)
const int servoPin1 = 13;    // Servo Daun Pintu 1 (D13)
const int servoPin2 = 12;    // Servo Daun Pintu 2 (D12)
const int flamePin = 34;     // Sensor Api 5-Way (D34)
int fanPin = 32;             // Kipas / Relay Kipas (D32)
const int TOUCH_PIN = 4;     // Touch0 ESP32 (Pin D4)

const int MAX_DISTANCE = 20; // Batas pemicu buka pintu (20 cm)

// ==========================================
// KONTROL KIPAS & RELAY
// ==========================================
bool fanActiveLow = false;    // Default: Active-HIGH
bool fanStateLogical = false; // Status kipas (false = mati, true = nyala)

// ==========================================
// SENSOR API 5-WAY (FLAME DETECTOR D34)
// ==========================================
bool flameActiveHigh = true;   // Default modul 5-way: 1 = Api, 0 = Aman
bool flameDetected = false;
int flameRawValue = 0;
bool lastFlameAlert = false;
int flameDebounceCounter = 0;
const int FLAME_DEBOUNCE_THRESHOLD = 3;

// ==========================================
// SUDUT SERVO (Pintu Kupu-tarung)
// ==========================================
const int SERVO1_CLOSE = 0;    // Pintu 1 tertutup rapat (0°)
const int SERVO1_OPEN  = 90;   // Pintu 1 terbuka (90°)

const int SERVO2_CLOSE = 180;  // Pintu 2 tertutup rapat (180°)
const int SERVO2_OPEN  = 90;   // Pintu 2 terbuka (90°)

Servo servo1;
Servo servo2;

// ==========================================
// STATUS SISTEM & TIMING
// ==========================================
bool doorManualOpen = false;
bool doorAutoCycleRunning = false;
bool triggerAutoOpenFromWeb = false;
bool autoDoorUltrasonic = false;
bool autoServoEnabled = true;

// Mode Pemicu Pintu Otomatis:
// 1: Sensor Sentuh (Touch Pin D4)
// 2: Sensor Api (D34 - Evakuasi darurat api)
// 3: Sensor Ultrasonik (HC-SR04)
// 0: Manual via Web saja
int doorTriggerMode = 1;

int touchRawValue = 1380;
int touchBaseline = 1380;
const int TOUCH_DELTA_THRESHOLD = 250;

int currentDistance = 0;
int missedEchoCount = 0;
long lastEchoDuration = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialLog = 0;
unsigned long lastWifiCheck = 0;

// Forward Declarations
bool cekDanInisialisasiOLED(bool paksa = false);
void tampilkanLayarBoot(const char* statusText);
void tampilkanSplash();
void tampilkanStandby(int distance);
void tampilkanTerbuka(int sisaMs, int totalMs);
void tampilkanMenutup();
void tampilkanBahayaApi();
int bacaJarak();
String scanUltrasonicPins();
void handleUltrasonic();
void applyFanOutput();
void setFanState(bool active);
void jalankanSiklusBukaPintu();
void setupWiFi();
void setupWebServer();

// ==========================================
// LOGIKA KONTROL KIPAS
// ==========================================
void applyFanOutput() {
  pinMode(fanPin, OUTPUT);
  int physicalValue = (fanActiveLow) ? (fanStateLogical ? LOW : HIGH) : (fanStateLogical ? HIGH : LOW);
  digitalWrite(fanPin, physicalValue);
  Serial.printf("[KIPAS] Pin: D%d | Logika: %s | Fisik: %s\n",
    fanPin, fanStateLogical ? "ON" : "OFF", physicalValue == HIGH ? "HIGH" : "LOW");
}

void setFanState(bool active) {
  fanStateLogical = active;
  applyFanOutput();
}

void bukaPintuManual() {
  doorManualOpen = true;
  servo1.write(SERVO1_OPEN);
  servo2.write(SERVO2_OPEN);
  Serial.println("[PINTU] Dibuka MANUAL via Web");
}

void tutupPintuManual() {
  doorManualOpen = false;
  tampilkanMenutup();
  servo1.write(SERVO1_CLOSE);
  servo2.write(SERVO2_CLOSE);
  delay(500);
  Serial.println("[PINTU] Ditutup MANUAL via Web");
}

// ==========================================
// LAYAR OLED SSD1306 (HOT-PLUG & AUTO-RECOVERY)
// ==========================================
bool cekDanInisialisasiOLED(bool paksa) {
  if (oledAktif) return true;
  unsigned long now = millis();
  if (!paksa && (now - lastOledScanAttempt < 1500)) return false;
  lastOledScanAttempt = now;

  Wire.begin(21, 22);
  Wire.setClock(100000); // 100 kHz standar stabil untuk initial handshake

  // Ping alamat 0x3C dan 0x3D
  Wire.beginTransmission(0x3C);
  byte err3C = Wire.endTransmission();

  Wire.beginTransmission(0x3D);
  byte err3D = Wire.endTransmission();

  uint8_t targetAddr = 0;
  if (err3C == 0) {
    targetAddr = 0x3C;
  } else if (err3D == 0) {
    targetAddr = 0x3D;
  }

  if (targetAddr != 0) {
    Serial.printf("[OLED] Terdeteksi chip SSD1306 di alamat 0x%02X! Memulai inisialisasi...\n", targetAddr);
    if (display.begin(SSD1306_SWITCHCAPVCC, targetAddr)) {
      oledAktif = true;
      oledAddress = targetAddr;
      Wire.setClock(400000);
      display.clearDisplay();
      display.display();
      tampilkanSplash();
      Serial.printf("[OLED] SUKSES! Layar OLED aktif di 0x%02X\n", targetAddr);
      return true;
    } else {
      Serial.println("[OLED] Gagal memanggil display.begin()");
    }
  } else {
    // Reset bus jika terjadi bus busy / freeze
    if (err3C >= 4 || err3D >= 4) {
      Wire.end();
      delay(5);
      Wire.begin(21, 22);
    }
  }
  return false;
}

void tampilkanLayarBoot(const char* statusText) {
  if (!oledAktif) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(18, 2);
  display.print("RUMAH JOGLO IoT");
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

  display.setCursor(8, 28);
  display.print(statusText);

  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(8, 54);
  display.print("Memulai sistem...");

  display.display();
}

void tampilkanSplash() {
  if (!oledAktif) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(18, 2);
  display.print("RUMAH JOGLO IoT");
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

  display.setCursor(8, 22);
  display.print(isAccessPoint ? "Mode: AP Mandiri" : "Mode: WiFi Station");

  display.setCursor(8, 34);
  display.print("Status: Siap Aktif");

  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(4, 54);
  display.print("IP: ");
  display.print(ipAddressStr);

  display.display();
}

void tampilkanBahayaApi() {
  if (!oledAktif) return;
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

void tampilkanStandby(int distance) {
  if (!oledAktif) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // 1. Header
  display.setCursor(18, 2);
  display.print("RUMAH JOGLO IoT");
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

  // 2. Status Pintu
  display.setCursor(4, 16);
  display.print("Pintu : ");
  if (doorManualOpen) {
    display.print("TERBUKA (MAN)");
  } else if (doorAutoCycleRunning) {
    display.print("TERBUKA (AUTO)");
  } else {
    display.print("TERTUTUP");
  }

  // 3. Mode Pemicu
  display.setCursor(4, 27);
  if (doorTriggerMode == 1) {
    display.print("Pemicu: Sentuh D4");
  } else if (doorTriggerMode == 2) {
    display.print("Pemicu: Sensor Api");
  } else if (doorTriggerMode == 3) {
    display.print("Jarak : ");
    if (distance > 0 && distance <= 300) {
      display.printf("%d cm", distance);
    } else {
      display.print("Standby");
    }
  } else {
    display.print("Pemicu: Manual Web");
  }

  // 4. Status Api
  display.setCursor(4, 38);
  display.print("Api   : ");
  if (flameDetected) {
    display.print("! BAHAYA !");
  } else {
    display.print("AMAN");
  }

  // 5. Footer IP
  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(4, 54);
  display.print("IP: ");
  display.print(ipAddressStr);

  display.display();
}

void tampilkanTerbuka(int sisaMs, int totalMs) {
  if (!oledAktif) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(18, 2);
  display.print("RUMAH JOGLO IoT");
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

  display.setCursor(22, 20);
  display.print("PINTU TERBUKA");

  display.setCursor(16, 32);
  if (doorTriggerMode == 1) {
    display.print("Sentuhan Terdeteksi");
  } else if (doorTriggerMode == 2) {
    display.print("Pemicu Sensor Api");
  } else {
    display.print("Selamat Datang!");
  }

  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(4, 54);
  float sisaDetik = sisaMs / 1000.0;
  display.printf("Tutup dlm: %.1fs", sisaDetik);

  display.display();
}

void tampilkanMenutup() {
  if (!oledAktif) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(18, 2);
  display.print("RUMAH JOGLO IoT");
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

  display.setCursor(22, 22);
  display.print("PINTU MENUTUP");

  display.setCursor(20, 36);
  display.print("Mohon tunggu...");

  display.drawFastHLine(0, 50, 128, SSD1306_WHITE);
  display.setCursor(4, 54);
  display.print("IP: ");
  display.print(ipAddressStr);

  display.display();
}

// ==========================================
// PEMBACAAN SENSOR ULTRASONIK (HC-SR04)
// ==========================================
bool testUltrasonicPair(int t, int e, int &dist, long &duration) {
  pinMode(t, OUTPUT);
  digitalWrite(t, LOW);
  pinMode(e, INPUT);
  delayMicroseconds(4);

  digitalWrite(t, HIGH);
  delayMicroseconds(10);
  digitalWrite(t, LOW);

  duration = pulseIn(e, HIGH, 15000); // 15ms timeout (~2.5m)
  if (duration >= 60 && duration <= 15000) {
    dist = duration * 0.034 / 2;
    return true;
  }
  return false;
}

String scanUltrasonicPins() {
  Serial.println("\n[ULTRASONIK] Memindai pin secara otomatis...");

  int priorityPairs[][2] = {
    {4, 5},   {5, 4},
    {5, 18},  {18, 5},
    {18, 19}, {19, 18},
    {4, 18},  {18, 4},
    {5, 19},  {19, 5},
    {23, 19}, {19, 23},
    {16, 17}, {17, 16},
    {25, 26}, {26, 25},
    {27, 14}, {14, 27}
  };
  int numPriority = sizeof(priorityPairs) / sizeof(priorityPairs[0]);

  int dist = 0;
  long dur = 0;

  for (int i = 0; i < numPriority; i++) {
    int t = priorityPairs[i][0];
    int e = priorityPairs[i][1];

    if (t == 13 || t == 12 || t == 21 || t == 22 || t == 34 || t == fanPin) continue;
    if (e == 13 || e == 12 || e == 21 || e == 22 || e == fanPin) continue;

    for (int retry = 0; retry < 2; retry++) {
      if (testUltrasonicPair(t, e, dist, dur)) {
        trigPin = t;
        echoPin = e;
        lastEchoDuration = dur;
        currentDistance = dist;
        Serial.printf("[ULTRASONIK] Ditemukan! Trig=D%d, Echo=D%d -> Jarak: %d cm\n", t, e, dist);
        return "Ditemukan: Trig=D" + String(t) + ", Echo=D" + String(e);
      }
      delay(15);
    }
  }

  Serial.println("[ULTRASONIK] Modul ultrasonik belum merespon.");
  return "Belum merespon (menunggu sensor baru)";
}

int bacaJarak() {
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT);
  delayMicroseconds(4);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 15000); // 15ms timeout (~2.5m)
  lastEchoDuration = duration;

  if (duration >= 60 && duration <= 15000) {
    return (int)(duration * 0.034 / 2);
  }
  return 0;
}

// ==========================================
// SIKLUS BUKA PINTU OTOMATIS
// ==========================================
void jalankanSiklusBukaPintu() {
  doorAutoCycleRunning = true;
  doorManualOpen = false;

  Serial.println(">>> [PINTU] Membuka Pintu (Siklus Otomatis)...");

  servo1.write(SERVO1_OPEN);
  servo2.write(SERVO2_OPEN);

  const int DURASI_BUKA_MS = 2000;
  unsigned long waktuMulaiBuka = millis();
  unsigned long lastSubOled = 0;

  while (millis() - waktuMulaiBuka < DURASI_BUKA_MS) {
    unsigned long subNow = millis();
    int sisaWaktu = DURASI_BUKA_MS - (subNow - waktuMulaiBuka);

    if (subNow - lastSubOled >= 150) {
      lastSubOled = subNow;
      tampilkanTerbuka(sisaWaktu, DURASI_BUKA_MS);
    }
    server.handleClient();
    delay(10);
  }

  Serial.println(">>> [PINTU] Siklus selesai -> Pintu Menutup Kembali");
  tampilkanMenutup();

  servo1.write(SERVO1_CLOSE);
  servo2.write(SERVO2_CLOSE);

  unsigned long waktuTutup = millis();
  while (millis() - waktuTutup < 600) {
    server.handleClient();
    delay(10);
  }

  Serial.println(">>> [STATUS] Pintu Tertutup Rapat. Kembali Standby.\n");
  doorAutoCycleRunning = false;
  lastDisplayUpdate = millis();
}

// ==========================================
// WEBSITE DASHBOARD & DIAGNOSTIK
// ==========================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Rumah Joglo IoT</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #0b0f17;
      --surface: #131926;
      --surface-elevated: #1c2436;
      --surface-subtle: #0f1420;
      --border: rgba(255, 255, 255, 0.08);
      --border-focus: rgba(56, 189, 248, 0.4);
      --text-main: #f1f5f9;
      --text-muted: #94a3b8;
      --text-dim: #64748b;
      --emerald: #10b981;
      --rose: #f43f5e;
      --amber: #f59e0b;
      --cyan: #38bdf8;
      --radius-card: 8px;
      --radius-btn: 6px;
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      font-family: 'Poppins', -apple-system, BlinkMacSystemFont, sans-serif;
      -webkit-tap-highlight-color: transparent;
    }
    
    body {
      background: var(--bg);
      color: var(--text-main);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      padding: 24px 16px 48px;
    }

    .app-shell {
      width: 100%;
      max-width: 640px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    /* Header */
    .app-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 4px 0 8px;
    }
    .brand-wrap {
      display: flex;
      align-items: center;
      gap: 12px;
    }
    .brand-logo {
      width: 38px;
      height: 38px;
      border-radius: 6px;
      background: var(--surface-elevated);
      border: 1px solid var(--border);
      display: flex;
      align-items: center;
      justify-content: center;
      color: var(--cyan);
    }
    .brand-title {
      font-size: 18px;
      font-weight: 700;
      color: #fff;
      line-height: 1.2;
    }
    .brand-subtitle {
      font-size: 11px;
      color: var(--text-muted);
      font-weight: 400;
    }

    /* Header Metadata (No round dots, no button pills) */
    .header-meta {
      display: flex;
      align-items: center;
      gap: 10px;
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: 6px;
      padding: 6px 12px;
    }
    .meta-item {
      display: flex;
      align-items: center;
      gap: 6px;
      font-size: 11px;
    }
    .meta-label {
      color: var(--text-dim);
      font-weight: 500;
      font-size: 10px;
      letter-spacing: 0.5px;
    }
    .meta-value {
      font-weight: 600;
      color: var(--text-muted);
    }
    .meta-value.val-online {
      color: var(--emerald);
    }
    .meta-value.val-offline {
      color: var(--rose);
    }
    .meta-divider {
      width: 1px;
      height: 12px;
      background: var(--border);
    }

    /* Status Badges: Flat rectangular info tag, NOT a round button */
    .status-badge {
      display: inline-flex;
      align-items: center;
      padding: 3px 8px;
      border-radius: 4px;
      font-size: 11px;
      font-weight: 600;
      letter-spacing: 0.3px;
      text-transform: uppercase;
      line-height: 1.2;
    }
    .status-badge.safe {
      background: rgba(16, 185, 129, 0.1);
      color: #34d399;
      border: 1px solid rgba(16, 185, 129, 0.25);
    }
    .status-badge.danger {
      background: rgba(244, 63, 94, 0.1);
      color: #fb7185;
      border: 1px solid rgba(244, 63, 94, 0.25);
    }
    .status-badge.neutral {
      background: rgba(255, 255, 255, 0.04);
      color: var(--text-muted);
      border: 1px solid rgba(255, 255, 255, 0.08);
    }
    .status-badge.accent {
      background: rgba(56, 189, 248, 0.1);
      color: var(--cyan);
      border: 1px solid rgba(56, 189, 248, 0.25);
    }

    /* Cards */
    .card {
      background: var(--surface);
      border: 1px solid var(--border);
      border-radius: var(--radius-card);
      padding: 18px 20px;
      box-shadow: 0 4px 16px rgba(0, 0, 0, 0.2);
    }
    .card-top {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 14px;
    }
    .card-header-left {
      display: flex;
      align-items: center;
      gap: 10px;
    }
    .card-icon-box {
      width: 32px;
      height: 32px;
      border-radius: 6px;
      background: var(--surface-subtle);
      border: 1px solid var(--border);
      display: flex;
      align-items: center;
      justify-content: center;
      color: var(--text-muted);
    }
    .card-heading {
      font-size: 15px;
      font-weight: 600;
      color: #fff;
      line-height: 1.25;
    }
    .card-subtext {
      font-size: 11px;
      color: var(--text-muted);
      font-weight: 400;
    }

    /* Architectural Double-Door Stage */
    .door-stage {
      background: var(--surface-subtle);
      border: 1px solid var(--border);
      border-radius: 6px;
      padding: 14px 18px;
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 14px;
    }
    .door-view {
      display: flex;
      align-items: center;
      gap: 14px;
    }
    .door-frame {
      width: 44px;
      height: 44px;
      perspective: 350px;
      display: flex;
      align-items: center;
      justify-content: center;
      background: rgba(0, 0, 0, 0.4);
      border: 1px solid var(--border);
      border-radius: 4px;
      position: relative;
    }
    .door-wing {
      width: 50%;
      height: 100%;
      background: #242c3d;
      border: 1px solid rgba(255, 255, 255, 0.1);
      position: relative;
      transition: transform 0.4s cubic-bezier(0.34, 1.56, 0.64, 1);
    }
    .door-wing.left {
      transform-origin: left center;
      border-top-left-radius: 2px;
      border-bottom-left-radius: 2px;
    }
    .door-wing.right {
      transform-origin: right center;
      border-top-right-radius: 2px;
      border-bottom-right-radius: 2px;
    }
    .door-wing::after {
      content: "";
      position: absolute;
      top: 50%;
      width: 2px;
      height: 6px;
      background: #f59e0b;
      transform: translateY(-50%);
    }
    .door-wing.left::after { right: 2px; }
    .door-wing.right::after { left: 2px; }

    .door-stage.is-open .door-wing.left {
      transform: rotateY(-80deg);
    }
    .door-stage.is-open .door-wing.right {
      transform: rotateY(80deg);
    }
    .door-stage.is-open .door-frame {
      border-color: rgba(16, 185, 129, 0.6);
    }

    .door-status-title {
      font-size: 17px;
      font-weight: 700;
      color: #fff;
      line-height: 1.2;
    }
    .door-status-note {
      font-size: 11px;
      color: var(--text-muted);
      margin-top: 2px;
    }

    /* Buttons */
    .btn-row {
      display: flex;
      gap: 8px;
      margin-bottom: 8px;
    }
    .btn {
      flex: 1;
      padding: 11px 14px;
      font-size: 13px;
      font-weight: 500;
      border-radius: var(--radius-btn);
      border: none;
      cursor: pointer;
      transition: background 0.15s ease, opacity 0.15s ease;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }
    .btn:active {
      opacity: 0.85;
    }
    .btn-primary-white {
      background: #f8fafc;
      color: #0b0f17;
      font-weight: 600;
    }
    .btn-primary-white:hover {
      background: #ffffff;
    }
    .btn-secondary-glass {
      background: var(--surface-elevated);
      color: #e2e8f0;
      border: 1px solid var(--border);
    }
    .btn-secondary-glass:hover {
      background: #232c42;
      border-color: rgba(255, 255, 255, 0.15);
    }

    /* Mode Selector Tabs (No 4-cell table buttons) */
    .control-label {
      font-size: 11px;
      font-weight: 600;
      color: var(--text-dim);
      text-transform: uppercase;
      letter-spacing: 0.6px;
      margin: 16px 0 6px;
    }
    .mode-nav {
      display: flex;
      border-bottom: 1px solid var(--border);
      gap: 4px;
      overflow-x: auto;
    }
    .mode-tab {
      background: transparent;
      border: none;
      border-bottom: 2px solid transparent;
      color: var(--text-muted);
      font-size: 12px;
      font-weight: 500;
      padding: 8px 12px;
      cursor: pointer;
      display: inline-flex;
      align-items: center;
      gap: 6px;
      white-space: nowrap;
      transition: color 0.15s ease, border-color 0.15s ease;
      margin-bottom: -1px;
    }
    .mode-tab:hover {
      color: var(--text-main);
    }
    .mode-tab.active {
      color: var(--cyan);
      border-bottom-color: var(--cyan);
      font-weight: 600;
    }

    /* Info Context Banner */
    .context-box {
      margin-top: 10px;
      background: var(--surface-subtle);
      border: 1px solid var(--border);
      border-radius: 6px;
      padding: 10px 14px;
      font-size: 11px;
      line-height: 1.5;
    }
    .context-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 4px;
    }
    .context-title {
      font-weight: 600;
      color: #fff;
    }
    .context-desc {
      color: var(--text-muted);
    }

    /* Sensors Grid */
    .grid-2 {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 16px;
    }
    @media (max-width: 580px) {
      .grid-2 {
        grid-template-columns: 1fr;
      }
    }

    /* Sensor Details */
    .metric-row {
      display: flex;
      align-items: baseline;
      justify-content: space-between;
      margin-bottom: 10px;
    }
    .metric-big {
      font-size: 28px;
      font-weight: 700;
      color: #fff;
      font-variant-numeric: tabular-nums;
      line-height: 1;
    }
    .metric-unit {
      font-size: 12px;
      color: var(--text-muted);
      font-weight: 500;
      margin-left: 2px;
    }

    /* Gauge Bar (Clean Flat Bar, Not a Pill Capsule) */
    .gauge-track {
      width: 100%;
      height: 4px;
      background: var(--surface-elevated);
      border-radius: 2px;
      position: relative;
      overflow: hidden;
      margin-bottom: 6px;
    }
    .gauge-threshold {
      position: absolute;
      left: 50%;
      top: 0;
      bottom: 0;
      width: 2px;
      background: var(--amber);
      z-index: 2;
    }
    .gauge-bar-fill {
      height: 100%;
      width: 0%;
      background: var(--emerald);
      border-radius: 2px;
      transition: width 0.2s ease, background 0.2s ease;
    }
    .gauge-text-row {
      display: flex;
      justify-content: space-between;
      font-size: 10px;
      color: var(--text-dim);
    }

    /* Subtle Action Buttons */
    .subtle-action-btn {
      width: 100%;
      background: var(--surface-subtle);
      border: 1px solid var(--border);
      border-radius: 6px;
      color: var(--text-muted);
      font-size: 11px;
      font-weight: 500;
      padding: 8px 12px;
      cursor: pointer;
      margin-top: 10px;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      transition: all 0.15s ease;
    }
    .subtle-action-btn:hover {
      background: var(--surface-elevated);
      color: var(--text-main);
      border-color: rgba(255, 255, 255, 0.15);
    }

    .footer-note {
      text-align: center;
      font-size: 11px;
      color: var(--text-dim);
      margin-top: 8px;
    }
  </style>
</head>
<body>
  <div class="app-shell">
    
    <!-- HEADER -->
    <header class="app-header">
      <div class="brand-wrap">
        <div class="brand-logo">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M3 10.5L12 3l9 7.5V20a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
            <polyline points="9 22 9 12 15 12 15 22"/>
          </svg>
        </div>
        <div>
          <h1 class="brand-title">Rumah Joglo</h1>
          <p class="brand-subtitle">Smart Home Miniatur IoT</p>
        </div>
      </div>
      <div class="header-meta">
        <div class="meta-item">
          <span class="meta-label">OLED</span>
          <span class="meta-value val-offline" id="oledLabel">Offline</span>
        </div>
        <div class="meta-divider"></div>
        <div class="meta-item">
          <span class="meta-label">IP</span>
          <span class="meta-value val-online" id="ipLabel">172.20.10.3</span>
        </div>
      </div>
    </header>

    <!-- CARD 1: PINTU MINIATUR DUAL SERVO -->
    <div class="card" id="doorCard">
      <div class="card-top">
        <div class="card-header-left">
          <div class="card-icon-box">
            <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <path d="M18 20V6a2 2 0 0 0-2-2H8a2 2 0 0 0-2 2v14"/>
              <path d="M2 20h20"/>
              <line x1="14" y1="12" x2="14.01" y2="12"/>
            </svg>
          </div>
          <div>
            <div class="card-heading">Pintu Kupu-Tarung</div>
            <div class="card-subtext">Dual Servo SG90 (Pin D13 & D12)</div>
          </div>
        </div>
        <span class="status-badge neutral" id="doorBadge">Tertutup</span>
      </div>

      <!-- Architectural Double-Door Stage -->
      <div class="door-stage" id="doorStage">
        <div class="door-view">
          <div class="door-frame">
            <div class="door-wing left"></div>
            <div class="door-wing right"></div>
          </div>
          <div>
            <div class="door-status-title" id="doorTitle">TERTUTUP</div>
            <div class="door-status-note" id="doorNote">Posisi Standby (0° & 180°)</div>
          </div>
        </div>
      </div>

      <!-- Quick Action Controls -->
      <div class="btn-row">
        <button class="btn btn-primary-white" onclick="controlDoor('auto')">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
            <polyline points="23 4 23 10 17 10"/>
            <path d="M20.49 15a9 9 0 1 1-2.12-9.36L23 10"/>
          </svg>
          Buka Pintu (2 Detik)
        </button>
      </div>
      <div class="btn-row">
        <button class="btn btn-secondary-glass" onclick="controlDoor('open')">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>
            <path d="M7 11V7a5 5 0 0 1 9.9-1"/>
          </svg>
          Buka Terus
        </button>
        <button class="btn btn-secondary-glass" onclick="controlDoor('close')">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <rect x="3" y="11" width="18" height="11" rx="2" ry="2"/>
            <path d="M7 11V7a5 5 0 0 1 10 0v4"/>
          </svg>
          Tutup Rapat
        </button>
      </div>

      <!-- Mode Selector Tabs (No 4-cell table) -->
      <div class="control-label">Pemicu Otomatis Pintu</div>
      <nav class="mode-nav" role="tablist">
        <button class="mode-tab" id="tabMode1" onclick="setTriggerMode(1)">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M12 11V3a1 1 0 0 0-2 0v8"/><path d="M10 8.5a1 1 0 0 0-2 0V13"/><path d="M14 8.5a1 1 0 0 1 2 0V12"/><path d="M18 10.5a1 1 0 0 1 2 0V15a7 7 0 0 1-7 7h-1a7 7 0 0 1-5.6-2.8L5 17.5a2 2 0 0 1 3.2-2.4L10 16"/>
          </svg>
          Sentuh (D4)
        </button>
        <button class="mode-tab" id="tabMode2" onclick="setTriggerMode(2)">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 3z"/>
          </svg>
          Sensor Api
        </button>
        <button class="mode-tab" id="tabMode3" onclick="setTriggerMode(3)">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M4.93 19.07a10 10 0 0 1 0-14.14"/><path d="M7.76 16.24a6 6 0 0 1 0-8.48"/><path d="M10.59 13.41a2 2 0 0 1 0-2.82"/><line x1="12" y1="12" x2="12.01" y2="12"/>
          </svg>
          Ultrasonik
        </button>
        <button class="mode-tab" id="tabMode0" onclick="setTriggerMode(0)">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <rect x="3" y="5" width="18" height="14" rx="2"/><circle cx="8" cy="12" r="2" fill="currentColor"/>
          </svg>
          Manual
        </button>
      </nav>

      <!-- Dynamic Context Box -->
      <div class="context-box">
        <div class="context-row">
          <span class="context-title" id="modeTitle">Sensor Sentuh (Pin D4)</span>
          <span class="status-badge accent" id="modeBadge">Siap Sentuh</span>
        </div>
        <p class="context-desc" id="modeDesc">Sentuh ujung kabel jumper di Pin D4 dengan jari untuk membuka pintu miniatur 2 detik.</p>
      </div>
    </div>

    <!-- SENSORS GRID -->
    <div class="grid-2">
      <!-- DETEKTOR API -->
      <div class="card" id="flameCard">
        <div class="card-top">
          <div class="card-header-left">
            <div class="card-icon-box">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 3z"/>
              </svg>
            </div>
            <div>
              <div class="card-heading">Detektor Api</div>
              <div class="card-subtext">5-Way Flame Sensor (D34)</div>
            </div>
          </div>
          <span class="status-badge safe" id="flameBadge">Aman</span>
        </div>

        <div class="context-box" style="margin-top:0;">
          <div class="context-row">
            <span>Status:</span>
            <strong id="flameStatusText" style="color:#34d399;">Aman (Normal)</strong>
          </div>
          <div class="context-row" style="margin-top:6px; color:var(--text-dim);">
            <span>Nilai D34: <span id="flameRawVal" style="color:var(--text-muted);">0</span></span>
            <span>Logika: <span id="flameLogicVal" style="color:var(--text-muted);">Active-HIGH</span></span>
          </div>
        </div>

        <button class="subtle-action-btn" onclick="toggleFlameLogic()">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M16 3l4 4-4 4"/><path d="M20 7H4"/><path d="M8 21l-4-4 4-4"/><path d="M4 17h16"/>
          </svg>
          Balik Logika (<span id="logicBtnText">Active-LOW</span>)
        </button>
      </div>

      <!-- SENSOR ULTRASONIK -->
      <div class="card" id="usCard">
        <div class="card-top">
          <div class="card-header-left">
            <div class="card-icon-box">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M4.93 19.07a10 10 0 0 1 0-14.14"/><path d="M7.76 16.24a6 6 0 0 1 0-8.48"/><path d="M10.59 13.41a2 2 0 0 1 0-2.82"/><line x1="12" y1="12" x2="12.01" y2="12"/>
              </svg>
            </div>
            <div>
              <div class="card-heading">Sensor Jarak</div>
              <div class="card-subtext" id="usPinSub">HC-SR04 (Trig D4, Echo D5)</div>
            </div>
          </div>
          <span class="status-badge neutral" id="usBadge">Standby</span>
        </div>

        <div class="metric-row">
          <div>
            <span class="metric-big" id="distNum">--</span>
            <span class="metric-unit">cm</span>
          </div>
          <div style="text-align:right;">
            <div style="font-size:10px; color:var(--text-dim); text-transform:uppercase;">Batas Pemicu</div>
            <div style="font-size:12px; font-weight:600; color:var(--amber);">≤ 20 cm</div>
          </div>
        </div>

        <div class="gauge-track">
          <div class="gauge-threshold" title="Batas Ambang 20cm"></div>
          <div class="gauge-bar-fill" id="distFill"></div>
        </div>
        <div class="gauge-text-row">
          <span>0 cm</span>
          <span style="color:var(--amber);">20 cm (Buka)</span>
          <span>40+ cm</span>
        </div>

        <button class="subtle-action-btn" onclick="swapUltrasonicPins()" id="swapBtn">
          <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M16 3l4 4-4 4"/><path d="M20 7H4"/><path d="M8 21l-4-4 4-4"/><path d="M4 17h16"/>
          </svg>
          Tukar Pin Trig ⇄ Echo
        </button>
      </div>
    </div>

    <div class="footer-note">
      Rumah Joglo IoT System &bull; ESP32 DevKit
    </div>
  </div>

  <script>
    function updateStatus() {
      fetch('/api/status')
        .then(res => res.json())
        .then(data => {
          // IP Status
          const ipLabel = document.getElementById('ipLabel');
          if (ipLabel && data.ip) {
            ipLabel.innerText = data.ip;
            ipLabel.className = 'meta-value val-online';
          }

          // OLED Status
          const oledLabel = document.getElementById('oledLabel');
          if (oledLabel) {
            if (data.oled) {
              oledLabel.innerText = '0x' + Number(data.oledAddress).toString(16).toUpperCase();
              oledLabel.className = 'meta-value val-online';
            } else {
              oledLabel.innerText = 'Offline';
              oledLabel.className = 'meta-value val-offline';
            }
          }

          // Door Status & Stage
          const isOpen = (data.door === 'TERBUKA' || data.door === 'AUTO_CYCLE');
          const isAuto = (data.door === 'AUTO_CYCLE');
          const stage = document.getElementById('doorStage');
          const badge = document.getElementById('doorBadge');
          const title = document.getElementById('doorTitle');
          const note = document.getElementById('doorNote');

          if (isOpen) {
            stage.className = 'door-stage is-open';
            badge.innerText = isAuto ? 'Membuka (2s)' : 'Terbuka';
            badge.className = 'status-badge safe';
            title.innerText = isAuto ? 'MEMBUKA (OTOMATIS)' : 'TERBUKA';
            note.innerText = 'Posisi 90° (Dual Servo Aktif)';
          } else {
            stage.className = 'door-stage';
            badge.innerText = 'Tertutup';
            badge.className = 'status-badge neutral';
            title.innerText = 'TERTUTUP';
            note.innerText = 'Posisi Standby (0° & 180°)';
          }

          // Mode Tabs (0, 1, 2, 3)
          const mode = data.doorTriggerMode !== undefined ? data.doorTriggerMode : 1;
          [0, 1, 2, 3].forEach(m => {
            const tab = document.getElementById('tabMode' + m);
            if (tab) tab.className = 'mode-tab' + (m === mode ? ' active' : '');
          });

          // Dynamic Mode Feedback
          const mTitle = document.getElementById('modeTitle');
          const mBadge = document.getElementById('modeBadge');
          const mDesc = document.getElementById('modeDesc');

          if (mode === 1) { // Touch
            mTitle.innerText = 'Sensor Sentuh (Pin D4)';
            if (data.touchDetected) {
              mBadge.innerText = 'Tersentuh!';
              mBadge.className = 'status-badge safe';
            } else {
              mBadge.innerText = 'Siap Sentuh (' + (data.touchRaw || 0) + ')';
              mBadge.className = 'status-badge accent';
            }
            mDesc.innerText = 'Sentuh ujung kabel jumper di Pin D4 dengan jari untuk membuka pintu miniatur 2 detik.';
          } else if (mode === 2) { // Flame
            mTitle.innerText = 'Pemicu: Sensor Api D34';
            mBadge.innerText = data.flame ? 'Bahaya Api!' : 'Monitoring';
            mBadge.className = data.flame ? 'status-badge danger' : 'status-badge neutral';
            mDesc.innerText = 'Pintu otomatis membuka jika terdeteksi nyala api untuk simulasi evakuasi darurat.';
          } else if (mode === 3) { // Ultrasonic
            mTitle.innerText = 'Pemicu: Ultrasonik HC-SR04';
            const det = data.handDetected || (data.distance > 0 && data.distance <= 20);
            mBadge.innerText = det ? 'Objek Terdeteksi' : 'Standby';
            mBadge.className = det ? 'status-badge safe' : 'status-badge neutral';
            mDesc.innerText = 'Pintu membuka saat ada objek berada pada jarak ≤ 20 cm.';
          } else { // Manual
            mTitle.innerText = 'Mode Manual';
            mBadge.innerText = 'Otomatis Nonaktif';
            mBadge.className = 'status-badge neutral';
            mDesc.innerText = 'Pintu hanya dapat dibuka dan ditutup menggunakan tombol kendali di atas.';
          }

          // Flame Sensor Card
          const flameBadge = document.getElementById('flameBadge');
          const flameText = document.getElementById('flameStatusText');
          const flameCard = document.getElementById('flameCard');
          document.getElementById('flameRawVal').innerText = data.flameRaw + (data.flameRaw === 1 ? ' (HIGH)' : ' (LOW)');
          document.getElementById('flameLogicVal').innerText = data.flameActiveHigh ? 'Active-HIGH' : 'Active-LOW';
          document.getElementById('logicBtnText').innerText = data.flameActiveHigh ? 'Active-LOW' : 'Active-HIGH';

          if (data.flame) {
            flameBadge.innerText = 'Bahaya Api!';
            flameBadge.className = 'status-badge danger';
            flameText.innerText = 'Bahaya: Terdeteksi Api!';
            flameText.style.color = '#fb7185';
            flameCard.style.borderColor = 'rgba(244, 63, 94, 0.4)';
          } else {
            flameBadge.innerText = 'Aman';
            flameBadge.className = 'status-badge safe';
            flameText.innerText = 'Aman (Normal)';
            flameText.style.color = '#34d399';
            flameCard.style.borderColor = 'var(--border)';
          }

          // Ultrasonic Card
          if (data.trigPin && data.echoPin) {
            document.getElementById('usPinSub').innerText = 'HC-SR04 (Trig D' + data.trigPin + ', Echo D' + data.echoPin + ')';
          }
          const dist = data.distance;
          const maxD = data.maxDistance || 20;
          const isUsDetected = data.handDetected !== undefined ? data.handDetected : (dist > 0 && dist <= maxD);
          const distNum = document.getElementById('distNum');
          const usBadge = document.getElementById('usBadge');
          const distFill = document.getElementById('distFill');

          if (dist > 0 && dist <= 300) {
            distNum.innerText = dist;
          } else {
            distNum.innerText = '> 40';
          }

          if (isUsDetected) {
            usBadge.innerText = 'Terdeteksi (' + dist + ' cm)';
            usBadge.className = 'status-badge safe';
            let pct = Math.min(100, Math.max(0, Math.round(((40 - dist) / 40) * 100)));
            distFill.style.width = pct + '%';
            distFill.style.background = 'var(--emerald)';
          } else {
            usBadge.innerText = 'Standby';
            usBadge.className = 'status-badge neutral';
            if (dist > maxD && dist <= 40) {
              let pct = Math.min(100, Math.max(0, Math.round(((40 - dist) / 40) * 100)));
              distFill.style.width = pct + '%';
              distFill.style.background = 'var(--amber)';
            } else {
              distFill.style.width = '0%';
              distFill.style.background = 'var(--emerald)';
            }
          }
        })
        .catch(() => {
          const ipLabel = document.getElementById('ipLabel');
          if (ipLabel) {
            ipLabel.innerText = 'Offline';
            ipLabel.className = 'meta-value val-offline';
          }
        });
    }

    function controlDoor(act) {
      fetch('/api/door?action=' + act).then(() => updateStatus());
    }

    function setTriggerMode(m) {
      fetch('/api/door?action=set_mode&mode=' + m).then(() => updateStatus());
    }

    function toggleFlameLogic() {
      fetch('/api/flame?toggle_logic=1').then(() => updateStatus());
    }

    function swapUltrasonicPins() {
      const btn = document.getElementById('swapBtn');
      btn.innerText = 'Menukar Pin...';
      fetch('/api/ultrasonic?swap=1')
        .then(r => r.json())
        .then(() => {
          btn.innerHTML = `<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M16 3l4 4-4 4"/><path d="M20 7H4"/><path d="M8 21l-4-4 4-4"/><path d="M4 17h16"/></svg> Tukar Pin Trig ⇄ Echo`;
          updateStatus();
        });
    }

    setInterval(updateStatus, 400);
    window.onload = updateStatus;
  </script>
</body>
</html>
)rawliteral";

// ==========================================
// ROUTING WEB SERVER
// ==========================================
void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  String doorStr = "TERTUTUP";
  if (doorAutoCycleRunning) {
    doorStr = "AUTO_CYCLE";
  } else if (doorManualOpen) {
    doorStr = "TERBUKA";
  }

  bool isHand = (doorTriggerMode == 3) && (currentDistance > 0 && currentDistance <= MAX_DISTANCE);
  bool isTouched = (doorTriggerMode == 1) && (abs(touchRawValue - touchBaseline) >= TOUCH_DELTA_THRESHOLD && touchRawValue > 0);

  String json = "{";
  json += "\"flame\":" + String(flameDetected ? "true" : "false") + ",";
  json += "\"flameRaw\":" + String(flameRawValue) + ",";
  json += "\"flameActiveHigh\":" + String(flameActiveHigh ? "true" : "false") + ",";
  json += "\"fan\":" + String(fanStateLogical ? "true" : "false") + ",";
  json += "\"fanPin\":" + String(fanPin) + ",";
  json += "\"fanActiveLow\":" + String(fanActiveLow ? "true" : "false") + ",";
  json += "\"isScanning\":false,";
  json += "\"scannedPin\":-1,";
  json += "\"door\":\"" + doorStr + "\",";
  json += "\"doorTriggerMode\":" + String(doorTriggerMode) + ",";
  json += "\"oled\":" + String(oledAktif ? "true" : "false") + ",";
  json += "\"oledAddress\":\"" + (oledAktif ? String("0x") + String(oledAddress, HEX) : "none") + "\",";
  json += "\"touchRaw\":" + String(touchRawValue) + ",";
  json += "\"touchBaseline\":" + String(touchBaseline) + ",";
  json += "\"touchDetected\":" + String(isTouched ? "true" : "false") + ",";
  json += "\"autoDoorUltrasonic\":" + String(doorTriggerMode == 3 ? "true" : "false") + ",";
  json += "\"autoServo\":" + String(doorTriggerMode != 0 ? "true" : "false") + ",";
  json += "\"trigPin\":" + String(trigPin) + ",";
  json += "\"echoPin\":" + String(echoPin) + ",";
  json += "\"distance\":" + String(currentDistance) + ",";
  json += "\"maxDistance\":" + String(MAX_DISTANCE) + ",";
  json += "\"handDetected\":" + String((isHand || isTouched) ? "true" : "false") + ",";
  json += "\"rawEcho\":" + String(lastEchoDuration) + ",";
  json += "\"ip\":\"" + ipAddressStr + "\",";
  json += "\"uptime\":" + String(millis() / 1000);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "application/json", json);
}

void handleFlame() {
  if (server.hasArg("toggle_logic")) {
    flameActiveHigh = !flameActiveHigh;
    Serial.printf("[FLAME] Logika sensor api 5-way diubah: %s\n",
      flameActiveHigh ? "Active-HIGH (1=Api, 0=Aman)" : "Active-LOW (0=Api, 1=Aman)");
  }
  handleStatus();
}

void handleFan() {
  if (server.hasArg("set")) {
    int val = server.arg("set").toInt();
    setFanState(val == 1);
  } else if (server.hasArg("toggle")) {
    setFanState(!fanStateLogical);
  } else if (server.hasArg("pin")) {
    fanPin = server.arg("pin").toInt();
    applyFanOutput();
  } else if (server.hasArg("toggle_logic")) {
    fanActiveLow = !fanActiveLow;
    applyFanOutput();
  }
  handleStatus();
}

void handleDoor() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "auto") {
      triggerAutoOpenFromWeb = true;
    } else if (action == "open") {
      bukaPintuManual();
    } else if (action == "close") {
      tutupPintuManual();
    } else if (action == "set_mode" && server.hasArg("mode")) {
      doorTriggerMode = server.arg("mode").toInt();
      autoDoorUltrasonic = (doorTriggerMode == 3);
      autoServoEnabled = (doorTriggerMode != 0);
      Serial.printf("[PINTU] Mode pemicu buka diubah ke: %d\n", doorTriggerMode);
    }
  }
  handleStatus();
}

void handleUltrasonic() {
  if (server.hasArg("swap")) {
    int tmp = trigPin;
    trigPin = echoPin;
    echoPin = tmp;
    pinMode(trigPin, OUTPUT);
    digitalWrite(trigPin, LOW);
    pinMode(echoPin, INPUT);
    Serial.printf("[ULTRASONIK] Pin ditukar: Trig=D%d, Echo=D%d\n", trigPin, echoPin);
  } else if (server.hasArg("scan")) {
    String res = scanUltrasonicPins();
    String json = "{\"result\":\"" + res + "\",\"trigPin\":" + String(trigPin) + ",\"echoPin\":" + String(echoPin) + "}";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
    return;
  }
  handleStatus();
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/flame", HTTP_GET, handleFlame);
  server.on("/api/fan", HTTP_GET, handleFan);
  server.on("/api/door", HTTP_GET, handleDoor);
  server.on("/api/ultrasonic", HTTP_GET, handleUltrasonic);

  server.begin();
  Serial.println("[WEB] Web Server aktif di port 80.");
}

// ==========================================
// INISIALISASI WIFI (STATION / ACCESS POINT)
// ==========================================
void setupWiFi() {
  bool connectedSTA = false;

  if (strlen(WIFI_SSID) > 0) {
    Serial.print("[WIFI] Menghubungkan ke WiFi: ");
    Serial.println(WIFI_SSID);
    tampilkanLayarBoot("Konek ke WiFi...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 6000) {
      delay(200);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      connectedSTA = true;
      isAccessPoint = false;
      ipAddressStr = WiFi.localIP().toString();
      Serial.print("[WIFI] Terhubung! IP: ");
      Serial.println(ipAddressStr);
      tampilkanLayarBoot("WiFi Terhubung!");
      delay(300);
    } else {
      Serial.println("[WIFI] Gagal terhubung ke WiFi. Beralih ke Access Point...");
      tampilkanLayarBoot("Beralih ke AP...");
      delay(300);
    }
  }

  if (!connectedSTA) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    isAccessPoint = true;
    ipAddressStr = WiFi.softAPIP().toString();
    Serial.println("[WIFI] Mode AP Mandiri Aktif!");
    Serial.printf("[WIFI] SSID: %s | URL: http://%s\n", AP_SSID, ipAddressStr.c_str());
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);

  // Inisialisasi Sensor Ultrasonik
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT_PULLDOWN);

  // Inisialisasi Sensor Api 5-Way
  pinMode(flamePin, INPUT);

  // Inisialisasi Pin Kipas / Relay
  pinMode(fanPin, OUTPUT);
  digitalWrite(fanPin, LOW);
  fanStateLogical = false;

  // Inisialisasi I2C & OLED
  Wire.begin(21, 22);
  cekDanInisialisasiOLED(true);

  if (oledAktif) {
    tampilkanLayarBoot("Memulai Sistem...");
  }

  setupWiFi();
  setupWebServer();

  if (oledAktif) {
    tampilkanSplash();
  }

  // Konfigurasi Servo Pintu Miniatur (SG90)
  servo1.setPeriodHertz(50);
  servo1.attach(servoPin1, 500, 2400);

  servo2.setPeriodHertz(50);
  servo2.attach(servoPin2, 500, 2400);

  servo1.write(SERVO1_CLOSE);
  servo2.write(SERVO2_CLOSE);

  // Rekam baseline sentuh awal pada Pin D4 (Touch0)
  long sumTouch = 0;
  for (int i = 0; i < 5; i++) {
    sumTouch += touchRead(TOUCH_PIN);
    delay(10);
  }
  touchBaseline = sumTouch / 5;
  if (touchBaseline <= 0) touchBaseline = 1380;
  Serial.printf("[TOUCH] Baseline Pin D4: %d\n", touchBaseline);

  Serial.println("\n=============================================");
  Serial.println("  SMART HOME RUMAH JOGLO - IOT SYSTEM        ");
  Serial.printf("  Sensor Api 5-Way: GPIO D%d                  \n", flamePin);
  Serial.printf("  Sensor Ultrasonik: Trig=D%d, Echo=D%d       \n", trigPin, echoPin);
  Serial.printf("  Kipas/Relay di Pin: GPIO D%d                \n", fanPin);
  Serial.printf("  Dashboard Web: http://%s\n", ipAddressStr.c_str());
  Serial.println("=============================================\n");
}

// ==========================================
// LOOP UTAMA
// ==========================================
void loop() {
  unsigned long now = millis();

  // 1. Hot-Plug / Auto-Reconnect Layar OLED
  if (!oledAktif) {
    cekDanInisialisasiOLED(false);
  }

  // 2. WiFi Auto-Reconnect Background Check (tiap 10 detik)
  if (!isAccessPoint && now - lastWifiCheck >= 10000) {
    lastWifiCheck = now;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WIFI] Sinyal terputus! Mencoba rekoneksi...");
      WiFi.reconnect();
    }
  }

  // 3. Tangani Permintaan Web Server
  server.handleClient();

  // 4. Baca Sensor Api 5-Way (D34) dengan Filter Debounce
  flameRawValue = digitalRead(flamePin);
  bool rawFlameCondition = (flameActiveHigh) ? (flameRawValue == HIGH) : (flameRawValue == LOW);

  if (rawFlameCondition) {
    if (flameDebounceCounter < FLAME_DEBOUNCE_THRESHOLD) flameDebounceCounter++;
  } else {
    if (flameDebounceCounter > 0) flameDebounceCounter--;
  }

  flameDetected = (flameDebounceCounter >= FLAME_DEBOUNCE_THRESHOLD);

  if (flameDetected) {
    if (!lastFlameAlert) {
      lastFlameAlert = true;
      Serial.printf("\n[PERINGATAN API] Sensor 5-Way mendeteksi nyala api di D34! (Nilai Raw: %d)\n", flameRawValue);
    }
  } else {
    if (lastFlameAlert) {
      lastFlameAlert = false;
      Serial.println("[STATUS API] Api padam / kondisi aman kembali.");
    }
  }

  // 5. Baca Sensor Ultrasonik secara berkala HANYA jika dalam mode ultrasonik (Mode 3)
  int distance = 0;
  if (doorTriggerMode == 3) {
    static unsigned long lastPingTime = 0;
    if (now - lastPingTime >= 90) {
      lastPingTime = now;
      int d = bacaJarak();
      if (d > 0 && d <= 300) {
        currentDistance = d;
        missedEchoCount = 0;
      } else {
        missedEchoCount++;
        if (missedEchoCount >= 4) {
          currentDistance = 0;
        }
      }
    }
    distance = currentDistance;
  }

  // 6. Baca Touch Pin D4 jika dalam mode sentuh (tiap 40ms)
  if (doorTriggerMode == 1) {
    static unsigned long lastTouchTime = 0;
    if (now - lastTouchTime >= 40) {
      lastTouchTime = now;
      touchRawValue = touchRead(TOUCH_PIN);
    }
  }

  // 7. Logika Pemicu Buka Pintu Otomatis
  bool triggerNow = triggerAutoOpenFromWeb;

  if (!doorManualOpen && !doorAutoCycleRunning) {
    if (doorTriggerMode == 1) {
      // Mode 1: Sentuh Pin D4
      if (abs(touchRawValue - touchBaseline) >= TOUCH_DELTA_THRESHOLD && touchRawValue > 0) {
        triggerNow = true;
        Serial.printf(">>> [TOUCH D4] Sentuhan jari terdeteksi (%d vs Base %d) -> Membuka pintu otomatis!\n",
          touchRawValue, touchBaseline);
      }
    } else if (doorTriggerMode == 2) {
      // Mode 2: Sensor Api (Evakuasi darurat)
      if (flameDetected) {
        triggerNow = true;
        Serial.println(">>> [API] Api terdeteksi -> Membuka pintu otomatis untuk evakuasi!");
      }
    } else if (doorTriggerMode == 3) {
      // Mode 3: Sensor Ultrasonik
      if (distance > 0 && distance <= MAX_DISTANCE) {
        triggerNow = true;
        Serial.printf(">>> [ULTRASONIK] Objek terdeteksi (%d cm) -> Membuka pintu otomatis!\n", distance);
      }
    }
  }

  // 8. Eksekusi Gerakan Pintu / Pembaruan Layar OLED
  if (triggerNow) {
    triggerAutoOpenFromWeb = false;
    jalankanSiklusBukaPintu();
  } else {
    // Perbarui layar OLED tiap 200ms
    if (now - lastDisplayUpdate >= 200) {
      lastDisplayUpdate = now;
      if (flameDetected && doorTriggerMode != 2) {
        tampilkanBahayaApi();
      } else {
        tampilkanStandby(currentDistance);
      }
    }

    // Log Serial berkala (tiap 3 detik)
    if (now - lastSerialLog >= 3000) {
      lastSerialLog = now;
      Serial.printf("[STATUS] Pintu: %s | Pemicu: %d | Touch(D4): %d | Flame: %s | OLED: %s\n",
        doorManualOpen ? "BUKA_MAN" : (doorAutoCycleRunning ? "AUTO" : "TUTUP"),
        doorTriggerMode, touchRawValue, flameDetected ? "API!" : "Aman",
        oledAktif ? "Aktif" : "Offline");
    }
  }

  delay(15);
}
