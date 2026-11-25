#include "sensors.h"

#include <DHT.h>
#include <Preferences.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

#include "config.h"
#include "terminal_ui.h"

// ==========================================
// SENSOR API (D34)
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
int activeStreak = 0;  // integrator anti-flicker api
}  // namespace

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
    if (!activeTracking) {
      activeSince = now;
      activeTracking = true;
    }
    quietTracking = false;
    if (activeStreak < 8) activeStreak++;
  } else {
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
  if (!flameNow && ((activeTracking && now - activeSince >= cfg::FLAME_CONFIRM_MS) || activeStreak >= 2)) {
    detected = true;
  }
  if (flameNow && quietTracking && now - quietSince >= cfg::FLAME_RELEASE_MS) {
    detected = false;
  }

  if (detected && !flameNow) {
    terminalLogTag(TerminalTag::Fire, "API TERDETEKSI di D%d! Evakuasi aktif", cfg::PIN_FLAME);
  } else if (!detected && flameNow) {
    terminalLogTag(TerminalTag::Fire, "Api padam - kondisi aman kembali");
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
  terminalLogTag(TerminalTag::Fire, "Logika api: %s", flameHigh ? "Active-HIGH" : "Active-LOW");
}

// ==========================================
// ULTRASONIK HC-SR04 (Trig D25, Echo D26)
// Interrupt-Driven (0 us CPU wait) + 3-Sample Rolling Median + 2x Confirm
// ==========================================
namespace {
portMUX_TYPE usMux = portMUX_INITIALIZER_UNLOCKED;

volatile int64_t echoRiseUs = 0;
volatile int64_t echoWidthUs = 0;
volatile bool echoMeasuring = false;
volatile bool echoReady = false;

bool pingInFlight = false;
int64_t pingStartUs = 0;
unsigned long lastPing = 0;

int medBuf[3] = {0, 0, 0};
int medIdx = 0;
int medCount = 0;
int distanceCm = 0;
int nearStreak = 0;
int missedEcho = 0;
long lastEchoUs = 0;

void IRAM_ATTR onEchoChange() {
  int level = gpio_get_level((gpio_num_t)cfg::PIN_ECHO);
  int64_t nowUs = esp_timer_get_time();
  if (level != 0) {
    echoRiseUs = nowUs;
    echoMeasuring = true;
  } else if (echoMeasuring) {
    int64_t width = nowUs - echoRiseUs;
    echoMeasuring = false;
    if (width > 0 && width <= (int64_t)cfg::ULTRASONIC_TIMEOUT_US) {
      echoWidthUs = width;
      echoReady = true;
    }
  }
}

int median3(int a, int b, int c) {
  if (a > b) { int t = a; a = b; b = t; }
  if (b > c) { int t = b; b = c; c = t; }
  if (a > b) { int t = a; a = b; b = t; }
  return b;
}

void pushSample(int rawCm) {
  medBuf[medIdx] = rawCm;
  medIdx = (medIdx + 1) % 3;
  if (medCount < 3) medCount++;

  if (medCount == 3) {
    distanceCm = median3(medBuf[0], medBuf[1], medBuf[2]);
  } else {
    distanceCm = rawCm;
  }

  if (distanceCm > 0 && distanceCm <= cfg::MAX_DISTANCE_CM) {
    if (nearStreak < 4) nearStreak++;
  } else {
    nearStreak = 0;
  }
}

void resetEchoIfStuck() {
  if (gpio_get_level((gpio_num_t)cfg::PIN_ECHO) == 0) return;
  detachInterrupt(digitalPinToInterrupt(cfg::PIN_ECHO));
  pinMode(cfg::PIN_ECHO, OUTPUT);
  digitalWrite(cfg::PIN_ECHO, LOW);
  delayMicroseconds(5);
  pinMode(cfg::PIN_ECHO, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(cfg::PIN_ECHO), onEchoChange, CHANGE);
}

void fireTriggerPulse() {
  resetEchoIfStuck();

  portENTER_CRITICAL(&usMux);
  echoReady = false;
  echoMeasuring = false;
  echoWidthUs = 0;
  portEXIT_CRITICAL(&usMux);

  digitalWrite(cfg::PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(cfg::PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(cfg::PIN_TRIG, LOW);

  pingStartUs = esp_timer_get_time();
  pingInFlight = true;
}
}  // namespace

void ultrasonicBegin() {
  pinMode(cfg::PIN_TRIG, OUTPUT);
  digitalWrite(cfg::PIN_TRIG, LOW);
  pinMode(cfg::PIN_ECHO, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(cfg::PIN_ECHO), onEchoChange, CHANGE);
}

void ultrasonicUpdate(unsigned long now, bool enabled, bool shortTimeout) {
  (void)shortTimeout;  // ISR non-blocking selalu memakai batas ketat ULTRASONIC_TIMEOUT_US

  // 1. Proses hasil echo dari ISR tanpa menahan CPU
  if (pingInFlight) {
    bool ready = false;
    int64_t width = 0;
    portENTER_CRITICAL(&usMux);
    if (echoReady) {
      ready = true;
      width = echoWidthUs;
      echoReady = false;
    }
    portEXIT_CRITICAL(&usMux);

    if (ready) {
      pingInFlight = false;
      lastEchoUs = (long)width;
      // Filter glitch listrik < 100 us (~1.7 cm, di bawah batas fisik HC-SR04 2 cm)
      if (width >= 100) {
        int cm = (int)((float)width * 0.034f * 0.5f);
        if (cm < 1) cm = 1;
        if (cm <= 150) {
          pushSample(cm);
          missedEcho = 0;
        }
      }
    } else if ((esp_timer_get_time() - pingStartUs) >= (int64_t)cfg::ULTRASONIC_TIMEOUT_US) {
      pingInFlight = false;
      portENTER_CRITICAL(&usMux);
      echoMeasuring = false;
      portEXIT_CRITICAL(&usMux);
      resetEchoIfStuck();

      if (++missedEcho >= 2) {
        distanceCm = 0;
        nearStreak = 0;
        medCount = 0;
        medIdx = 0;
      }
    }
  }

  // 2. Mulai ping baru tiap ULTRASONIC_PING_MS (45 ms)
  if (!enabled || pingInFlight) return;
  if (now - lastPing < cfg::ULTRASONIC_PING_MS) return;
  lastPing = now;
  fireTriggerPulse();
}

int ultrasonicDistance() { return distanceCm; }
bool ultrasonicTriggered() { return nearStreak >= cfg::ULTRASONIC_CONFIRM_COUNT; }
long ultrasonicLastEcho() { return lastEchoUs; }
int ultrasonicTrigPin() { return cfg::PIN_TRIG; }
int ultrasonicEchoPin() { return cfg::PIN_ECHO; }

// ==========================================
// SENSOR SUHU & KELEMBABAN DHT11 (D27)
// Anti Self-Heating (3000 ms Core 0) + Outlier Guard + 2-Point/Offset + EMA + NVS
// ==========================================
namespace {
constexpr const char *NVS_DHT_NS = "dhtcal";

DHT dht(cfg::PIN_DHT, DHT11);
portMUX_TYPE dhtMux = portMUX_INITIALIZER_UNLOCKED;

float currentTemp = 0.0f;
float currentHumidity = 0.0f;
float emaTemp = 0.0f;
float emaHumidity = 0.0f;
float lastRawTemp = 0.0f;
float lastRawHum = 0.0f;
float tempOffset = cfg::DHT_TEMP_OFFSET_DEFAULT;
float humOffset = cfg::DHT_HUM_OFFSET_DEFAULT;

bool hasRawBaseline = false;
bool emaInitialized = false;
bool hasGoodReading = false;
bool tempValid = false;
int consecutiveFails = 0;
int outlierStreak = 0;
const char *statusStr = "Menunggu sensor...";

float calibrateTemperature(float rawT, float tOff) {
  float cal = rawT;
  if (cfg::DHT_USE_TWO_POINT_CAL && fabsf(cfg::DHT_RAW_HIGH - cfg::DHT_RAW_LOW) > 0.01f) {
    cal = cfg::DHT_REF_LOW +
          (rawT - cfg::DHT_RAW_LOW) * (cfg::DHT_REF_HIGH - cfg::DHT_REF_LOW) /
              (cfg::DHT_RAW_HIGH - cfg::DHT_RAW_LOW);
  }
  return cal + tOff;
}

float calibrateHumidity(float rawH, float hOff) {
  float cal = rawH + hOff;
  if (cal < 0.0f) return 0.0f;
  if (cal > 100.0f) return 100.0f;
  return cal;
}

void dhtTaskFn(void * /*arg*/) {
  // Jeda awal 1.5 dtk agar suplai & sensor DHT11 stabil sebelum baca pertama
  vTaskDelay(pdMS_TO_TICKS(1500));

  for (;;) {
    float rawT = dht.readTemperature();
    float rawH = dht.readHumidity();

    bool validRange = !isnan(rawT) && !isnan(rawH) &&
                      rawT >= -10.0f && rawT <= 80.0f &&
                      rawH >= 0.0f && rawH <= 100.0f;

    portENTER_CRITICAL(&dhtMux);
    if (validRange) {
      consecutiveFails = 0;

      // Outlier rejection: tolak lonjakan tiba-tiba >3 C atau >15% RH dalam 1 siklus
      bool isSpike = false;
      if (hasRawBaseline) {
        isSpike = (fabsf(rawT - lastRawTemp) > cfg::DHT_MAX_TEMP_JUMP_C) ||
                  (fabsf(rawH - lastRawHum) > cfg::DHT_MAX_HUM_JUMP_PCT);
      }

      if (isSpike && outlierStreak < cfg::DHT_OUTLIER_CONFIRM_LIMIT) {
        outlierStreak++;
      } else {
        outlierStreak = 0;
        lastRawTemp = rawT;
        lastRawHum = rawH;
        hasRawBaseline = true;

        float calT = calibrateTemperature(rawT, tempOffset);
        float calH = calibrateHumidity(rawH, humOffset);

        if (!emaInitialized) {
          emaTemp = calT;
          emaHumidity = calH;
          emaInitialized = true;
        } else {
          const float a = cfg::DHT_EMA_ALPHA;
          emaTemp = a * calT + (1.0f - a) * emaTemp;
          emaHumidity = a * calH + (1.0f - a) * emaHumidity;
        }

        currentTemp = emaTemp;
        currentHumidity = emaHumidity;
        hasGoodReading = true;
        tempValid = true;
        statusStr = "OK";
      }
    } else {
      consecutiveFails++;
      if (hasGoodReading && consecutiveFails <= cfg::DHT_MAX_CONSECUTIVE_FAILS) {
        // Pertahankan nilai valid terakhir selama gagal <= 3x berturut-turut
        tempValid = true;
        statusStr = "Gangguan baca sementara";
      } else {
        tempValid = false;
        currentTemp = 0.0f;
        currentHumidity = 0.0f;
        statusStr = "Sensor gagal >3x (cek D27)";
      }
    }
    portEXIT_CRITICAL(&dhtMux);

    // Interval ketat 3000 ms untuk mencegah self-heating DHT11
    vTaskDelay(pdMS_TO_TICKS(cfg::DHT_READ_MS));
  }
}
}  // namespace

void dhtBegin() {
  pinMode(cfg::PIN_DHT, INPUT_PULLUP);
  dht.begin();

  // Muat offset kalibrasi persisten dari NVS (jika pernah disimpan lewat /api/calibrate)
  Preferences prefs;
  if (prefs.begin(NVS_DHT_NS, true)) {
    float savedT = prefs.getFloat("t_off", cfg::DHT_TEMP_OFFSET_DEFAULT);
    float savedH = prefs.getFloat("h_off", cfg::DHT_HUM_OFFSET_DEFAULT);
    prefs.end();

    if (!isnan(savedT) && !isnan(savedH)) {
      portENTER_CRITICAL(&dhtMux);
      tempOffset = constrain(savedT, -30.0f, 30.0f);
      humOffset = constrain(savedH, -50.0f, 50.0f);
      portEXIT_CRITICAL(&dhtMux);
    }
  }

  if (dhtIsCalibrated()) {
    terminalLogTag(TerminalTag::Sys, "Kalibrasi DHT aktif: T %+.1fC, H %+.1f%%",
                   dhtTempOffset(), dhtHumOffset());
  }

  xTaskCreatePinnedToCore(dhtTaskFn, "dhtTask", 3072, nullptr, 1, nullptr, 0);
}

void dhtUpdate(unsigned long /*now*/) {
  // Pembacaan dan filtering berjalan paralel di Core 0 (zero blocking di loop utama)
}

void dhtSetCalibration(float newTempOffset, float newHumOffset) {
  float tOff = constrain(newTempOffset, -30.0f, 30.0f);
  float hOff = constrain(newHumOffset, -50.0f, 50.0f);

  Preferences prefs;
  if (prefs.begin(NVS_DHT_NS, false)) {
    prefs.putFloat("t_off", tOff);
    prefs.putFloat("h_off", hOff);
    prefs.end();
  }

  portENTER_CRITICAL(&dhtMux);
  tempOffset = tOff;
  humOffset = hOff;
  if (hasRawBaseline) {
    emaTemp = calibrateTemperature(lastRawTemp, tempOffset);
    emaHumidity = calibrateHumidity(lastRawHum, humOffset);
    emaInitialized = true;
    currentTemp = emaTemp;
    currentHumidity = emaHumidity;
  }
  portEXIT_CRITICAL(&dhtMux);

  terminalLogTag(TerminalTag::Sys, "Kalibrasi DHT: T %+.1fC, H %+.1f%%", tOff, hOff);
}

void dhtResetCalibration() {
  Preferences prefs;
  if (prefs.begin(NVS_DHT_NS, false)) {
    prefs.clear();
    prefs.end();
  }

  portENTER_CRITICAL(&dhtMux);
  tempOffset = cfg::DHT_TEMP_OFFSET_DEFAULT;
  humOffset = cfg::DHT_HUM_OFFSET_DEFAULT;
  if (hasRawBaseline) {
    emaTemp = calibrateTemperature(lastRawTemp, tempOffset);
    emaHumidity = calibrateHumidity(lastRawHum, humOffset);
    emaInitialized = true;
    currentTemp = emaTemp;
    currentHumidity = emaHumidity;
  }
  portEXIT_CRITICAL(&dhtMux);

  terminalLogTag(TerminalTag::Sys, "Kalibrasi DHT direset ke default");
}

float dhtTemperature() {
  portENTER_CRITICAL(&dhtMux);
  float v = currentTemp;
  portEXIT_CRITICAL(&dhtMux);
  return v;
}

float dhtHumidity() {
  portENTER_CRITICAL(&dhtMux);
  float v = currentHumidity;
  portEXIT_CRITICAL(&dhtMux);
  return v;
}

float dhtRawTemperature() {
  portENTER_CRITICAL(&dhtMux);
  float v = lastRawTemp;
  portEXIT_CRITICAL(&dhtMux);
  return v;
}

float dhtRawHumidity() {
  portENTER_CRITICAL(&dhtMux);
  float v = lastRawHum;
  portEXIT_CRITICAL(&dhtMux);
  return v;
}

float dhtTempOffset() {
  portENTER_CRITICAL(&dhtMux);
  float v = tempOffset;
  portEXIT_CRITICAL(&dhtMux);
  return v;
}

float dhtHumOffset() {
  portENTER_CRITICAL(&dhtMux);
  float v = humOffset;
  portEXIT_CRITICAL(&dhtMux);
  return v;
}

bool dhtIsValid() {
  portENTER_CRITICAL(&dhtMux);
  bool v = tempValid;
  portEXIT_CRITICAL(&dhtMux);
  return v;
}

bool dhtIsCalibrated() {
  portENTER_CRITICAL(&dhtMux);
  bool cal = cfg::DHT_USE_TWO_POINT_CAL ||
             fabsf(tempOffset) > 0.001f ||
             fabsf(humOffset) > 0.001f;
  portEXIT_CRITICAL(&dhtMux);
  return cal;
}

const char *dhtStatusMessage() {
  portENTER_CRITICAL(&dhtMux);
  const char *s = statusStr;
  portEXIT_CRITICAL(&dhtMux);
  return s;
}
