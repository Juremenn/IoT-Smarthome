#include "sensors.h"

#include "config.h"

// ==========================================
// SENSOR API
// ==========================================
namespace {
bool flameHigh = cfg::FLAME_ACTIVE_HIGH_DEFAULT;  // true: 1 = api, 0 = aman
bool flameNow = false;
int flameValue = 0;
unsigned long lastFlameRead = 0;
unsigned long activeSince = 0;
unsigned long quietSince = 0;
bool activeTracking = false;
bool quietTracking = false;
int activeStreak = 0;             // integrator anti-flicker api
} // namespace

void flameBegin() {
  pinMode(cfg::PIN_FLAME, INPUT);
  flameValue = digitalRead(cfg::PIN_FLAME);
  Serial.printf("[API] Inisialisasi pin D%d: Nilai raw awal = %d (Mode: %s)\n",
                cfg::PIN_FLAME, flameValue,
                flameHigh ? "Active-HIGH (1=Api)" : "Active-LOW (0=Api, 1=Aman)");
}

void flameUpdate(unsigned long now) {
  if (now - lastFlameRead < cfg::FLAME_READ_MS) return;
  lastFlameRead = now;

  flameValue = digitalRead(cfg::PIN_FLAME);
  bool active = flameHigh ? (flameValue == HIGH) : (flameValue == LOW);

  if (active) {
    if (!activeTracking) {
      activeSince = now;
      activeTracking = true;
    }
    quietTracking = false;
    if (activeStreak < 8) activeStreak++;
  } else {
    // Toleransi flicker: kurangi bertahap agar api korek yang bergetar tidak langsung ter-reset
    if (activeStreak > 0) activeStreak--;
    if (activeStreak == 0) {
      activeTracking = false;
      if (!quietTracking) {
        quietSince = now;
        quietTracking = true;
      }
    }
  }

  bool detected = flameNow;
  // Deteksi cepat jika sinyal bertahan >= FLAME_CONFIRM_MS atau streak >= 2 kali berturut-turut
  if (!flameNow && ((activeTracking && now - activeSince >= cfg::FLAME_CONFIRM_MS) || activeStreak >= 2)) {
    detected = true;
  }
  // Pertahankan status bahaya minimal FLAME_RELEASE_MS setelah tenang agar alarm stabil
  if (flameNow && quietTracking && now - quietSince >= cfg::FLAME_RELEASE_MS) {
    detected = false;
  }

  if (detected && !flameNow) {
    Serial.printf("\n>>> [PERINGATAN API] Api terdeteksi di D34! (Raw: %d, Mode: %s)\n",
                  flameValue, flameHigh ? "Active-HIGH" : "Active-LOW");
  } else if (!detected && flameNow) {
    Serial.println(">>> [STATUS API] Api padam / kondisi aman kembali.");
  }
  flameNow = detected;
}

bool flameDetected() { return flameNow; }
int flameRaw() { return flameValue; }
bool flameActiveHigh() { return flameHigh; }

void flameToggleLogic() {
  flameHigh = !flameHigh;
  activeTracking = false;
  quietTracking = false;
  activeStreak = 0;
  flameNow = false;
  Serial.printf("[FLAME] Logika sensor api diubah: %s (Raw saat ini: %d)\n",
                flameHigh ? "Active-HIGH (1=Api, 0=Aman)" : "Active-LOW (0=Api, 1=Aman)",
                digitalRead(cfg::PIN_FLAME));
}

// ==========================================
// ULTRASONIK HC-SR04 (Trig D25, Echo D26)
// ==========================================
namespace {
int distanceCm = 0;
int missedEcho = 0;
long lastEchoUs = 0;
unsigned long lastPing = 0;

bool ping(int &dist, long &duration, unsigned long timeoutUs) {
  // Anti-freeze untuk modul HC-SR04: bebaskan Echo jika tertahan HIGH dari timeout sebelumnya
  if (digitalRead(cfg::PIN_ECHO) == HIGH) {
    pinMode(cfg::PIN_ECHO, OUTPUT);
    digitalWrite(cfg::PIN_ECHO, LOW);
    delayMicroseconds(5);
    pinMode(cfg::PIN_ECHO, INPUT_PULLDOWN);
  }

  // Pulsa trigger 10 µs
  digitalWrite(cfg::PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(cfg::PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(cfg::PIN_TRIG, LOW);

  duration = pulseIn(cfg::PIN_ECHO, HIGH, timeoutUs);
  // Filter glitch listrik < 100 µs (~1.7 cm, di bawah batas fisik HC-SR04 2 cm)
  if (duration >= 100 && duration <= (long)timeoutUs) {
    dist = (int)(duration * 0.034f / 2.0f);
    if (dist < 1) dist = 1;
    return true;
  }
  dist = 0;
  return false;
}
} // namespace

void ultrasonicBegin() {
  pinMode(cfg::PIN_TRIG, OUTPUT);
  digitalWrite(cfg::PIN_TRIG, LOW);
  pinMode(cfg::PIN_ECHO, INPUT_PULLDOWN);
}

void ultrasonicUpdate(unsigned long now, bool enabled, bool shortTimeout) {
  if (!enabled) return;
  if (now - lastPing < cfg::ULTRASONIC_PING_MS) return;
  lastPing = now;

  // Saat pintu sedang menutup, pakai timeout 6 ms (~100 cm) agar tick servo 20 ms tidak tersendat
  unsigned long timeoutUs = shortTimeout ? 6000UL : 15000UL;
  int d = 0;
  if (ping(d, lastEchoUs, timeoutUs) && d > 0 && d <= 250) {
    distanceCm = d;
    missedEcho = 0;
  } else if (++missedEcho >= 2) {
    distanceCm = 0;
  }
}

int ultrasonicDistance() { return distanceCm; }
long ultrasonicLastEcho() { return lastEchoUs; }
int ultrasonicTrigPin() { return cfg::PIN_TRIG; }
int ultrasonicEchoPin() { return cfg::PIN_ECHO; }

// ==========================================
// SENSOR SUHU & KELEMBABAN (DHT11 / DHT22)
// Menggunakan library resmi Adafruit DHT
// ==========================================
#include <DHT.h>

namespace {
DHT dht(cfg::PIN_DHT, DHT11);
float currentTemp = 0.0f;
float currentHumidity = 0.0f;
float lastGoodTemp = 0.0f;
float lastGoodHum = 0.0f;
bool hasGoodReading = false;
bool tempValid = false;
int consecutiveFails = 0;
unsigned long lastDhtRead = 0;
const char *statusStr = "Menunggu sensor...";
} // namespace

void dhtBegin() {
  pinMode(cfg::PIN_DHT, INPUT_PULLUP);
  dht.begin();
  Serial.printf("[DHT11] Driver resmi Adafruit aktif di GPIO D%d\n", cfg::PIN_DHT);
}

void dhtUpdate(unsigned long now) {
  // Beri waktu 1.5 detik awal setelah boot agar DHT11 stabil, dan hormati jeda >2000ms Adafruit
  if (now < 1500) return;
  if (hasGoodReading || consecutiveFails > 0) {
    if (now - lastDhtRead < cfg::DHT_READ_MS) return;
  }
  lastDhtRead = now;

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h) && t >= -10.0f && t <= 80.0f && h >= 0.0f && h <= 100.0f) {
    currentTemp = t;
    currentHumidity = h;
    lastGoodTemp = t;
    lastGoodHum = h;
    hasGoodReading = true;
    tempValid = true;
    consecutiveFails = 0;
    statusStr = "OK";
  } else {
    consecutiveFails++;
    statusStr = "Tidak terdeteksi (cek kabel D27/VCC/GND)";
    if (hasGoodReading && consecutiveFails < cfg::DHT_MAX_CONSECUTIVE_FAILS) {
      tempValid = true;
      currentTemp = lastGoodTemp;
      currentHumidity = lastGoodHum;
    } else {
      tempValid = false;
      currentTemp = 0.0f;
      currentHumidity = 0.0f;
    }
  }
}

float dhtTemperature() { return currentTemp; }
float dhtHumidity() { return currentHumidity; }
bool dhtIsValid() { return tempValid; }
const char *dhtStatusMessage() { return statusStr; }

