#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==========================================
// KONFIGURASI LAYAR OLED SSD1306 (I2C)
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledAktif = false;

// ==========================================
// PIN HARDWARE
// ==========================================
const int trigPin = 5;
const int echoPin = 18;
const int servoPin1 = 13; // Servo 1 (Pin D13)
const int servoPin2 = 12; // Servo 2 (Pin D12)

const int MAX_DISTANCE = 20; // Jarak pemicu buka pintu (maksimal 20 cm)

// ==========================================
// SUDUT SERVO (Pintu Kupu-tarung Berlawanan Arah)
// ==========================================
const int SERVO1_CLOSE = 0;    // Servo 1 posisi tertutup (0°)
const int SERVO1_OPEN  = 90;   // Servo 1 posisi terbuka (90°)

const int SERVO2_CLOSE = 180;  // Servo 2 posisi tertutup (180°, kebalikan Servo 1)
const int SERVO2_OPEN  = 90;   // Servo 2 posisi terbuka (90°)

Servo servo1;
Servo servo2;

// Variabel status & timing
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialLog = 0;
int lastLoggedDistance = -1;
int animDot = 0;

// ==========================================
// FUNGSI TAMPILAN OLED (GUI MODERN & INTERAKTIF)
// ==========================================

// 1. Layar Pembuka (Splash Screen) saat ESP32 pertama kali menyala
void tampilkanSplash() {
  if (!oledAktif) return;
  display.clearDisplay();

  // Border ganda elegan
  display.drawRoundRect(2, 2, 124, 60, 4, SSD1306_WHITE);
  display.drawRoundRect(4, 4, 120, 56, 3, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(30, 12);
  display.println("SMART HOME");

  // Banner pita terbalik (inverted)
  display.fillRoundRect(12, 24, 104, 15, 3, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(24, 28);
  display.println("RUMAH  JOGLO");

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 45);
  display.println("Dual Servo Door");

  display.display();
  delay(1800);
}

// 2. Layar Standby (Pintu Tertutup dengan Gauge Jarak Real-time)
void tampilkanStandby(int distance) {
  if (!oledAktif) return;
  display.clearDisplay();

  // Header Bar (Inverted modern ribbon)
  display.fillRoundRect(0, 0, 128, 12, 2, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(16, 2);
  display.print("RUMAH JOGLO - IOT");

  // Status Badge
  display.setTextColor(SSD1306_WHITE);
  display.drawRoundRect(6, 16, 116, 15, 3, SSD1306_WHITE);
  display.setCursor(16, 20);
  display.print("[ PINTU TERTUTUP ]");

  // Info Jarak
  display.setCursor(10, 35);
  display.print("Jarak: ");
  if (distance > 0 && distance <= 100) {
    display.print(distance);
    display.print(" cm");
  } else {
    display.print(">100 cm");
  }

  // Visual Proximity Bar (Gauge mendekat: semakin dekat tangan, bar semakin penuh)
  // Rentang deteksi visual: 40 cm ke 0 cm
  display.drawRect(10, 46, 108, 6, SSD1306_WHITE);
  if (distance > 0 && distance <= 40) {
    int barWidth = map(constrain(distance, 1, 40), 40, 1, 0, 104);
    display.fillRect(12, 48, barWidth, 2, SSD1306_WHITE);
  }

  // Teks Petunjuk Animasi di Bawah
  display.setCursor(10, 55);
  display.print("Dekatkan tangan");
  for (int i = 0; i <= (animDot % 3); i++) {
    display.print(".");
  }

  display.display();
}

// 3. Layar Pintu Terbuka dengan Animasi Progress Bar Countdown (1.5 detik)
void tampilkanTerbuka(int sisaMs, int totalMs) {
  if (!oledAktif) return;
  display.clearDisplay();

  // Header
  display.fillRoundRect(0, 0, 128, 12, 2, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(18, 2);
  display.print("= PINTU TERBUKA =");

  // Kotak Pesan Selamat Datang
  display.setTextColor(SSD1306_WHITE);
  display.drawRoundRect(4, 15, 120, 24, 4, SSD1306_WHITE);
  display.setCursor(16, 19);
  display.print("SELAMAT DATANG!");

  // Simbol Pintu Terbuka Keluar
  display.setCursor(12, 28);
  display.print("<[BUKA]     [BUKA]>");

  // Countdown Progress Bar (Menghitung mundur 1.5 detik)
  display.setCursor(6, 43);
  display.print("Tutup:");

  int barWidth = map(constrain(sisaMs, 0, totalMs), 0, totalMs, 0, 56);
  display.drawRect(44, 43, 58, 7, SSD1306_WHITE);
  display.fillRect(45, 44, barWidth, 5, SSD1306_WHITE);

  // Angka Sisa Detik (Contoh: 1.5s, 0.9s)
  display.setCursor(104, 43);
  float sisaDetik = sisaMs / 1000.0;
  display.print(sisaDetik, 1);
  display.print("s");

  display.setCursor(20, 54);
  display.print("Silakan Masuk :)");

  display.display();
}

// 4. Layar Pintu Menutup Kembali
void tampilkanMenutup() {
  if (!oledAktif) return;
  display.clearDisplay();

  display.fillRoundRect(0, 0, 128, 12, 2, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(16, 2);
  display.print("RUMAH JOGLO - IOT");

  display.setTextColor(SSD1306_WHITE);
  display.drawRoundRect(6, 17, 116, 42, 4, SSD1306_WHITE);

  display.setCursor(20, 24);
  display.print(">>> MENUTUP <<<");

  display.setCursor(14, 36);
  display.print("[ ==>>     <<== ]");

  display.setCursor(24, 48);
  display.print("Mohon Tunggu...");

  display.display();
}

// ==========================================
// PEMBACAAN SENSOR ULTRASONIK
// ==========================================
int bacaJarak() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Timeout 30ms (~5 meter) agar tidak menghalangi eksekusi ESP32
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) return 0;
  return duration * 0.034 / 2;
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Inisialisasi I2C Fast Mode (400kHz) pada SDA=D21, SCL=D22
  Wire.begin(21, 22);
  Wire.setClock(400000);

  // Inisialisasi Layar OLED
  if (display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    oledAktif = true;
    tampilkanSplash();
  } else {
    Serial.println("[ERROR] Layar OLED tidak terdeteksi (periksa kabel SDA/SCL).");
  }

  // Konfigurasi servo 1 & 2
  servo1.setPeriodHertz(50);
  servo1.attach(servoPin1, 500, 2400);

  servo2.setPeriodHertz(50);
  servo2.attach(servoPin2, 500, 2400);

  // Posisi awal tertutup rapat
  servo1.write(SERVO1_CLOSE);
  servo2.write(SERVO2_CLOSE);

  // Cetak header rapi ke Serial Monitor (TIDAK SPAM!)
  Serial.println("\n=============================================");
  Serial.println("  SMART HOME RUMAH JOGLO - DUAL SERVO DOOR   ");
  Serial.println("  Sensor: HC-SR04 | Layar: OLED SSD1306      ");
  Serial.println("=============================================");
  Serial.println("[INFO] Sistem siap & aktif dalam mode Standby.\n");
}

// ==========================================
// LOOP UTAMA
// ==========================================
void loop() {
  int distance = bacaJarak();
  unsigned long now = millis();

  // HANYA CETAK SERIAL JIKA TANGAN MENDATANGI SENSOR (TIDAK SPAM!)
  if (distance > 0 && distance <= MAX_DISTANCE) {
    Serial.print(">>> [DETEKSI] Tangan terdeteksi! Jarak: ");
    Serial.print(distance);
    Serial.println(" cm -> Pintu Membuka");

    // 1. Gerakkan kedua servo untuk membuka pintu
    servo1.write(SERVO1_OPEN);
    servo2.write(SERVO2_OPEN);

    // 2. Animasi hitung mundur (1.5 detik) di layar OLED
    const int DURASI_BUKA_MS = 1500;
    unsigned long waktuMulaiBuka = millis();

    while (millis() - waktuMulaiBuka < DURASI_BUKA_MS) {
      int sisaWaktu = DURASI_BUKA_MS - (millis() - waktuMulaiBuka);
      tampilkanTerbuka(sisaWaktu, DURASI_BUKA_MS);
      delay(30); // Animasi halus ~30 FPS
    }

    // 3. Waktu 1.5 detik selesai -> Mulai menutup
    Serial.println(">>> [TIMER] 1.5 detik selesai -> Pintu Menutup Kembali");
    tampilkanMenutup();

    servo1.write(SERVO1_CLOSE);
    servo2.write(SERVO2_CLOSE);

    // Jeda 800ms agar servo selesai bergerak sampai posisi tertutup sempurna
    delay(800);
    Serial.println(">>> [STATUS] Pintu Tertutup Rapat. Kembali Standby.\n");

    // Reset timer display
    lastDisplayUpdate = millis();
  } else {
    // Mode Standby: Perbarui layar OLED tiap 150ms agar animasi titik halus & tanpa lag
    if (now - lastDisplayUpdate >= 150) {
      lastDisplayUpdate = now;
      animDot++;
      tampilkanStandby(distance);
    }

    // Cetak log jarak ke Serial secara tenang (hanya tiap 3 detik jika jarak berubah signifikan)
    if (now - lastSerialLog >= 3000) {
      lastSerialLog = now;
      if (abs(distance - lastLoggedDistance) >= 5) {
        lastLoggedDistance = distance;
        Serial.print("[STANDBY] Sensor normal. Objek terjauh: ");
        if (distance > 0 && distance <= 100) {
          Serial.print(distance);
          Serial.println(" cm");
        } else {
          Serial.println(">100 cm");
        }
      }
    }
  }

  delay(20);
}