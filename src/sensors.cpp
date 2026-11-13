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
                                 {16, 17}, {17, 16}, {25, 26}, {26, 25}};

  for (const auto &p : pairs) {
    int t = p[0];
    int e = p[1];
    if (t == 13 || t == 12 || t == 21 || t == 22 || t == 34 || t == fanPin || t == cfg::PIN_DHT) continue;
    if (e == 13 || e == 12 || e == 21 || e == 22 || e == fanPin || e == cfg::PIN_DHT) continue;

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

// ==========================================
// SENSOR SUHU & KELEMBABAN (DHT11 / DHT22)
// Driver ESP32 dengan microsecond-timing & portENTER_CRITICAL
// ==========================================
namespace {
float currentTemp = 0.0f;
float currentHumidity = 0.0f;
float lastGoodTemp = 0.0f;
float lastGoodHum = 0.0f;
bool tempValid = false;
int consecutiveFails = 0;
unsigned long lastDhtRead = 0;
const char *statusStr = "Menunggu sensor...";

enum class DhtStatus {
  OK,
  STUCK_LOW,
  STUCK_HIGH_NO_RESPONSE,
  TIMEOUT_HANDSHAKE,
  TIMEOUT_DATA,
  CHECKSUM_ERROR,
  OUT_OF_RANGE
};

DhtStatus readDhtRaw(float &outTemp, float &outHum) {
  // Pastikan pin dalam keadaan INPUT_PULLUP
  pinMode(cfg::PIN_DHT, INPUT_PULLUP);
  delayMicroseconds(50);

  // Jika sebelum start pin sudah LOW, kemungkinan korslet ke GND atau modul rusak
  if (digitalRead(cfg::PIN_DHT) == LOW) {
    return DhtStatus::STUCK_LOW;
  }

  // 1. Sinyal Start: Tarik garis ke LOW minimal 18-20 ms
  pinMode(cfg::PIN_DHT, OUTPUT);
  digitalWrite(cfg::PIN_DHT, LOW);
  delay(20);

  // 2. Masuki critical section agar FreeRTOS task / interrupt tidak mengganggu timing pulsa
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  portENTER_CRITICAL(&mux);

  // Lepaskan garis ke INPUT_PULLUP (tarik ke HIGH)
  pinMode(cfg::PIN_DHT, INPUT_PULLUP);
  delayMicroseconds(30);

  // 3. Tangkap respon sensor: sensor harus menarik ke LOW dalam waktu ~20-50 µs
  uint32_t t = micros();
  while (digitalRead(cfg::PIN_DHT) == HIGH) {
    if (micros() - t > 100) {
      portEXIT_CRITICAL(&mux);
      return DhtStatus::STUCK_HIGH_NO_RESPONSE;
    }
  }

  // 4. Pulsa respon LOW dari sensor (~80 µs)
  t = micros();
  while (digitalRead(cfg::PIN_DHT) == LOW) {
    if (micros() - t > 120) {
      portEXIT_CRITICAL(&mux);
      return DhtStatus::TIMEOUT_HANDSHAKE;
    }
  }

  // 5. Pulsa respon HIGH dari sensor (~80 µs)
  t = micros();
  while (digitalRead(cfg::PIN_DHT) == HIGH) {
    if (micros() - t > 120) {
      portEXIT_CRITICAL(&mux);
      return DhtStatus::TIMEOUT_HANDSHAKE;
    }
  }

  // 6. Baca 40 bit data (5 byte)
  uint8_t data[5] = {0, 0, 0, 0, 0};
  for (int i = 0; i < 40; i++) {
    // Pulsa LOW 50 µs pembuka bit
    t = micros();
    while (digitalRead(cfg::PIN_DHT) == LOW) {
      if (micros() - t > 80) {
        portEXIT_CRITICAL(&mux);
        return DhtStatus::TIMEOUT_DATA;
      }
    }

    // Ukur durasi pulsa HIGH
    t = micros();
    while (digitalRead(cfg::PIN_DHT) == HIGH) {
      if (micros() - t > 120) {
        portEXIT_CRITICAL(&mux);
        return DhtStatus::TIMEOUT_DATA;
      }
    }
    uint32_t highDur = micros() - t;

    data[i / 8] <<= 1;
    // Logika 1: pulsa HIGH ~70 µs (> 40 µs). Logika 0: pulsa HIGH ~26-28 µs (< 40 µs).
    if (highDur > 40) {
      data[i / 8] |= 1;
    }
  }

  portEXIT_CRITICAL(&mux);

  // 7. Verifikasi Checksum
  uint8_t sum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
  if (data[4] != sum) {
    return DhtStatus::CHECKSUM_ERROR;
  }

  // 8. Parsing nilai suhu dan kelembaban
  // Format standar DHT11
  float t11 = data[2] + (data[3] & 0x7F) * 0.1f;
  if (data[3] & 0x80) t11 = -t11;
  float h11 = data[0] + data[1] * 0.1f;

  if (t11 >= 0.0f && t11 <= 60.0f && h11 >= 15.0f && h11 <= 95.0f) {
    outTemp = t11;
    outHum = h11;
    return DhtStatus::OK;
  }

  // Coba format 16-bit DHT22 (jika modul yang dipasang DHT22/AM2302)
  float t22 = (((data[2] & 0x7F) << 8) | data[3]) * 0.1f;
  if (data[2] & 0x80) t22 = -t22;
  float h22 = ((data[0] << 8) | data[1]) * 0.1f;

  if (t22 >= -20.0f && t22 <= 70.0f && h22 >= 0.0f && h22 <= 100.0f) {
    outTemp = t22;
    outHum = h22;
    return DhtStatus::OK;
  }

  if (t11 > 0 || h11 > 0) {
    outTemp = t11;
    outHum = h11;
    return DhtStatus::OK;
  }

  return DhtStatus::OUT_OF_RANGE;
}
} // namespace

void dhtBegin() {
  pinMode(cfg::PIN_DHT, INPUT_PULLUP);
  Serial.printf("[DHT11] Driver hardware terkalibrasi di GPIO D%d\n", cfg::PIN_DHT);
}

void dhtUpdate(unsigned long now) {
  if (now - lastDhtRead < cfg::DHT_READ_MS) return;
  lastDhtRead = now;

  float t = 0.0f;
  float h = 0.0f;
  DhtStatus res = readDhtRaw(t, h);

  if (res == DhtStatus::OK) {
    currentTemp = t;
    currentHumidity = h;
    lastGoodTemp = t;
    lastGoodHum = h;
    tempValid = true;
    consecutiveFails = 0;
    statusStr = "OK";
  } else {
    consecutiveFails++;
    switch (res) {
      case DhtStatus::STUCK_LOW:
        statusStr = "Pin D27 korslet ke GND";
        break;
      case DhtStatus::STUCK_HIGH_NO_RESPONSE:
        statusStr = "Pin D27 tidak merespon (cek kabel Data/VCC/GND)";
        break;
      case DhtStatus::TIMEOUT_HANDSHAKE:
        statusStr = "Timeout handshake awal DHT11";
        break;
      case DhtStatus::TIMEOUT_DATA:
        statusStr = "Timeout transmisi bit DHT11";
        break;
      case DhtStatus::CHECKSUM_ERROR:
        statusStr = "Checksum error (gangguan sinyal)";
        break;
      default:
        statusStr = "Nilai suhu di luar rentang wajar";
        break;
    }

    // Jika gagal, pertahankan nilai baik terakhir sampai batas toleransi
    if (consecutiveFails < cfg::DHT_MAX_CONSECUTIVE_FAILS && lastGoodTemp > 0.0f) {
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

