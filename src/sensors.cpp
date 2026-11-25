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
// ULTRASONIK HC-SR04
// ==========================================
namespace {
int trigPin = cfg::PIN_TRIG_DEFAULT;
int echoPin = cfg::PIN_ECHO_DEFAULT;
int distanceCm = 0;
int missedEcho = 0;
long lastEchoUs = 0;
unsigned long lastPing = 0;

bool ping(int t, int e, int &dist, long &duration) {
  // Tunggu singkat jika Echo masih HIGH dari pantulan sebelumnya tanpa memaksa OUTPUT LOW
  if (digitalRead(e) == HIGH) {
    delayMicroseconds(200);
    if (digitalRead(e) == HIGH) {
      dist = 0;
      duration = 0;
      return false;
    }
  }

  // Pulsa trigger 10 µs
  digitalWrite(t, LOW);
  delayMicroseconds(2);
  digitalWrite(t, HIGH);
  delayMicroseconds(10);
  digitalWrite(t, LOW);

  duration = pulseIn(e, HIGH, 25000);  // 25 ms timeout (~4.2 meter)
  if (duration >= 25 && duration <= 25000) {
    dist = (int)(duration * 0.034f / 2.0f);
    if (dist < 1) dist = 1;
    return true;
  }
  dist = 0;
  return false;
}
} // namespace

void ultrasonicBegin() {
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT);
}

void ultrasonicUpdate(unsigned long now, bool enabled) {
  static bool wasEnabled = false;
  if (!enabled) {
    wasEnabled = false;
    return;
  }
  if (!wasEnabled) {
    pinMode(trigPin, OUTPUT);
    digitalWrite(trigPin, LOW);
    pinMode(echoPin, INPUT);
    wasEnabled = true;
  }

  if (now - lastPing < cfg::ULTRASONIC_PING_MS) return;
  lastPing = now;

  int d = 0;
  ping(trigPin, echoPin, d, lastEchoUs);
  if (d > 0 && d <= 400) {
    distanceCm = d;
    missedEcho = 0;
  } else if (++missedEcho >= 3) {
    distanceCm = 0;
  }
}

int ultrasonicDistance() { return distanceCm; }
long ultrasonicLastEcho() { return lastEchoUs; }
int ultrasonicTrigPin() { return trigPin; }
int ultrasonicEchoPin() { return echoPin; }

void ultrasonicSwapPins() {
  int tmp = trigPin;
  trigPin = echoPin;
  echoPin = tmp;
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT);
  Serial.printf("[ULTRASONIK] Pin ditukar: Trig=D%d, Echo=D%d\n", trigPin, echoPin);
}

String ultrasonicScanPins() {
  Serial.println("\n[ULTRASONIK] Memindai pin secara otomatis...");
  static const int pairs[][2] = {{4, 5},   {5, 4},   {5, 18},  {18, 5},  {18, 19}, {19, 18},
                                 {4, 18},  {18, 4},  {5, 19},  {19, 5},  {23, 19}, {19, 23},
                                 {16, 17}, {17, 16}, {25, 26}, {26, 25}};

  for (const auto &p : pairs) {
    int t = p[0];
    int e = p[1];
    if (t == cfg::PIN_SERVO || t == cfg::PIN_SDA || t == cfg::PIN_SCL || t == cfg::PIN_FLAME || t == cfg::PIN_DHT) continue;
    if (e == cfg::PIN_SERVO || e == cfg::PIN_SDA || e == cfg::PIN_SCL || e == cfg::PIN_DHT) continue;

    pinMode(t, OUTPUT);
    digitalWrite(t, LOW);
    pinMode(e, INPUT);
    delayMicroseconds(20);

    for (int retry = 0; retry < 2; retry++) {
      int dist = 0;
      long dur = 0;
      if (ping(t, e, dist, dur)) {
        trigPin = t;
        echoPin = e;
        lastEchoUs = dur;
        distanceCm = dist;
        Serial.printf("[ULTRASONIK] Ditemukan! Trig=D%d, Echo=D%d -> Jarak: %d cm\n", t, e, dist);
        return "Ditemukan: Trig=D" + String(t) + ", Echo=D" + String(e);
      }
      delay(15);
    }
  }

  // Kembalikan pinMode aktif agar pin tidak tertinggal dalam kondisi salah
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT);
  Serial.println("[ULTRASONIK] Modul ultrasonik belum merespon.");
  return "Belum merespon (menunggu sensor baru)";
}

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

