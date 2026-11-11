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
unsigned long activeSince = 0;    // 0 = sinyal sedang tidak aktif
unsigned long quietSince = 0;     // 0 = sinyal sedang aktif
unsigned long ignoredPulses = 0;  // lonjakan singkat yang diabaikan (diagnostik)
unsigned long lastIgnoredLog = 0;
} // namespace

void flameBegin() {
  pinMode(cfg::PIN_FLAME, INPUT);
  flameValue = digitalRead(cfg::PIN_FLAME);
}

void flameUpdate(unsigned long now) {
  if (now - lastFlameRead < cfg::FLAME_READ_MS) return;
  lastFlameRead = now;

  flameValue = digitalRead(cfg::PIN_FLAME);
  bool active = flameHigh ? (flameValue == HIGH) : (flameValue == LOW);

  if (active) {
    if (activeSince == 0) activeSince = now;
    quietSince = 0;
  } else {
    if (activeSince != 0 && !flameNow && now - activeSince < cfg::FLAME_CONFIRM_MS) {
      // Sinyal sempat aktif sebentar lalu hilang: noise atau pantulan cahaya, bukan api.
      ignoredPulses++;
      if (now - lastIgnoredLog >= 5000) {
        lastIgnoredLog = now;
        Serial.printf("[API] Lonjakan singkat diabaikan (%lu ms), total %lu\n", now - activeSince,
                      ignoredPulses);
      }
    }
    activeSince = 0;
    if (quietSince == 0) quietSince = now;
  }

  bool detected = flameNow;
  if (!flameNow && activeSince != 0 && now - activeSince >= cfg::FLAME_CONFIRM_MS) detected = true;
  if (flameNow && quietSince != 0 && now - quietSince >= cfg::FLAME_RELEASE_MS) detected = false;
  if (detected && !flameNow) {
    Serial.printf("\n[PERINGATAN API] Sensor 5-Way mendeteksi nyala api di D34! (Nilai Raw: %d)\n",
                  flameValue);
  } else if (!detected && flameNow) {
    Serial.println("[STATUS API] Api padam / kondisi aman kembali.");
  }
  flameNow = detected;
}

bool flameDetected() { return flameNow; }
int flameRaw() { return flameValue; }
bool flameActiveHigh() { return flameHigh; }

void flameToggleLogic() {
  flameHigh = !flameHigh;
  activeSince = 0;
  quietSince = 0;
  flameNow = false;
  Serial.printf("[FLAME] Logika sensor api 5-way diubah: %s\n",
                flameHigh ? "Active-HIGH (1=Api, 0=Aman)" : "Active-LOW (0=Api, 1=Aman)");
}

// ==========================================
// SENSOR SENTUH
// ==========================================
namespace {
int touchRaw = cfg::TOUCH_BASELINE_FALLBACK;
int touchBaseline = cfg::TOUCH_BASELINE_FALLBACK;
unsigned long lastTouchRead = 0;
} // namespace

void touchBegin() {
  long sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += touchRead(cfg::PIN_TOUCH);
    delay(10);
  }
  touchBaseline = sum / 5;
  if (touchBaseline <= 0) touchBaseline = cfg::TOUCH_BASELINE_FALLBACK;
  Serial.printf("[TOUCH] Baseline Pin D4: %d\n", touchBaseline);
}

void touchUpdate(unsigned long now, bool enabled) {
  if (!enabled || now - lastTouchRead < cfg::TOUCH_READ_MS) return;
  lastTouchRead = now;
  touchRaw = touchRead(cfg::PIN_TOUCH);
}

int touchRawValue() { return touchRaw; }
int touchBaselineValue() { return touchBaseline; }

bool touchIsTouched() {
  return abs(touchRaw - touchBaseline) >= cfg::TOUCH_DELTA_THRESHOLD && touchRaw > 0;
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
  pinMode(t, OUTPUT);
  digitalWrite(t, LOW);
  pinMode(e, INPUT);
  delayMicroseconds(4);
  digitalWrite(t, HIGH);
  delayMicroseconds(10);
  digitalWrite(t, LOW);

  duration = pulseIn(e, HIGH, 15000);  // 15 ms timeout (~2.5 m)
  if (duration >= 60 && duration <= 15000) {
    dist = duration * 0.034 / 2;
    return true;
  }
  dist = 0;
  return false;
}
} // namespace

void ultrasonicBegin() {
  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT_PULLDOWN);
}

void ultrasonicUpdate(unsigned long now, bool enabled) {
  if (!enabled || now - lastPing < cfg::ULTRASONIC_PING_MS) return;
  lastPing = now;

  int d = 0;
  ping(trigPin, echoPin, d, lastEchoUs);
  if (d > 0 && d <= 300) {
    distanceCm = d;
    missedEcho = 0;
  } else if (++missedEcho >= 4) {
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

String ultrasonicScanPins(int fanPin) {
  Serial.println("\n[ULTRASONIK] Memindai pin secara otomatis...");
  static const int pairs[][2] = {{4, 5},   {5, 4},   {5, 18},  {18, 5},  {18, 19}, {19, 18},
                                 {4, 18},  {18, 4},  {5, 19},  {19, 5},  {23, 19}, {19, 23},
                                 {16, 17}, {17, 16}, {25, 26}, {26, 25}, {27, 14}, {14, 27}};

  for (const auto &p : pairs) {
    int t = p[0];
    int e = p[1];
    if (t == 13 || t == 12 || t == 21 || t == 22 || t == 34 || t == fanPin) continue;
    if (e == 13 || e == 12 || e == 21 || e == 22 || e == fanPin) continue;

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
  Serial.println("[ULTRASONIK] Modul ultrasonik belum merespon.");
  return "Belum merespon (menunggu sensor baru)";
}
