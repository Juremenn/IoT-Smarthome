#include "door.h"

#include <Arduino.h>
#include <ESP32Servo.h>

#include "config.h"

namespace {

// Bukaan daun pintu diukur dari posisi tertutup: 0 = tertutup, OPEN = terbuka penuh.
// Arah servo (naik atau turun) diatur oleh SERVO_CLOSE/SERVO_OPEN di config.h.
constexpr float OPEN = (float)(cfg::SERVO_OPEN > cfg::SERVO_CLOSE ? cfg::SERVO_OPEN - cfg::SERVO_CLOSE
                                                                  : cfg::SERVO_CLOSE - cfg::SERVO_OPEN);
constexpr float HALF = OPEN / 2.0f;

Servo servo1; // Pin D13
Servo servo2; // Pin D12 (alternatif)

DoorPhase phase = DoorPhase::Closed;
float leafAngle = 0.0f;
float moveFrom = 0.0f;
float moveTo = 0.0f;
unsigned long moveStart = 0;
unsigned long moveDuration = 0;
unsigned long lastTick = 0;
unsigned long maxGap = 0;   // jeda terlama antar-update servo selama satu gerakan (diagnostik)
unsigned long closeAt = 0;  // 0 = tidak ada jadwal menutup
int lastUs = -1;

bool autoCycle = false;
bool manualOpen = false;
bool evacuation = false;

// Mulai pelan, cepat di tengah, melambat sampai berhenti.
float easeInOutSine(float t) { return -(cosf(PI * t) - 1.0f) / 2.0f; }

// Untuk menutup: waktu dilengkungkan supaya sebagian besar jarak ditempuh di awal,
// lalu bagian akhir mendekati kusen sangat pelan (soft close, tidak kejedot).
float easeSoftClose(float t) { return easeInOutSine(1.0f - powf(1.0f - t, 1.8f)); }

int servoAngleToUs(float servoAngle) {
  return cfg::SERVO_MIN_US +
         (int)lroundf(servoAngle / 180.0f * (cfg::SERVO_MAX_US - cfg::SERVO_MIN_US));
}

// writeMicroseconds memberi langkah jauh lebih halus daripada write(derajat).
void writeServo(float leaf) {
  float a = cfg::SERVO_CLOSE + (cfg::SERVO_OPEN - cfg::SERVO_CLOSE) * leaf / OPEN;
  int us = servoAngleToUs(a);
  if (us != lastUs) {
    if (servo1.attached()) servo1.writeMicroseconds(us);
    if (servo2.attached()) servo2.writeMicroseconds(us);
    lastUs = us;
  }
}

void onFullyOpen(unsigned long now) {
  phase = DoorPhase::Open;
  closeAt = (autoCycle && !evacuation) ? now + cfg::DOOR_HOLD_MS : 0;
}

void startMove(float target) {
  float distance = fabsf(target - leafAngle);
  if (distance < 0.5f) {
    leafAngle = target;
    writeServo(leafAngle);
    if (target > HALF) {
      onFullyOpen(millis());
    } else {
      phase = DoorPhase::Closed;
      autoCycle = false;
    }
    return;
  }
  bool opening = target > leafAngle;
  unsigned long full = opening ? cfg::DOOR_OPEN_MS : cfg::DOOR_CLOSE_MS;
  // Durasi sebanding sisa jarak, jadi berbalik di tengah jalan tetap pelan.
  moveDuration = max(200UL, (unsigned long)(full * distance / OPEN));
  moveFrom = leafAngle;
  moveTo = target;
  moveStart = millis();
  lastTick = 0;
  maxGap = 0;
  closeAt = 0;
  phase = opening ? DoorPhase::Opening : DoorPhase::Closing;
  Serial.printf(">>> [PINTU] %s (%lu ms)\n", opening ? "Membuka" : "Menutup", moveDuration);
}

bool isOpenOrOpening() { return phase == DoorPhase::Open || phase == DoorPhase::Opening; }

} // namespace

void doorBegin() {
  // Alokasi timer untuk ESP32PWM agar PWM berjalan stabil di semua core ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servo1.setPeriodHertz(50);
  int ch1 = servo1.attach(cfg::PIN_SERVO, cfg::SERVO_MIN_US, cfg::SERVO_MAX_US);

  servo2.setPeriodHertz(50);
  int ch2 = servo2.attach(cfg::PIN_SERVO_ALT, cfg::SERVO_MIN_US, cfg::SERVO_MAX_US);

  Serial.printf("[SERVO] Inisialisasi: D%d (Ch %d, att=%s) | D%d (Ch %d, att=%s)\n",
                cfg::PIN_SERVO, ch1, servo1.attached() ? "OK" : "GAGAL",
                cfg::PIN_SERVO_ALT, ch2, servo2.attached() ? "OK" : "GAGAL");

  writeServo(0.0f);
}

void doorTestSweep() {
  Serial.println(">>> [TEST SERVO] Memulai pengujian servo (0° -> 90° -> 180° -> 0°)...");
  writeServo(0.0f);
  delay(500);
  writeServo(HALF);
  delay(800);
  writeServo(OPEN);
  delay(800);
  writeServo(0.0f);
  delay(500);
  Serial.println(">>> [TEST SERVO] Pengujian servo selesai. Kembali ke posisi 0°.");
}

void doorUpdate(unsigned long now) {
  if (phase == DoorPhase::Opening || phase == DoorPhase::Closing) {
    if (lastTick != 0 && now - lastTick < cfg::SERVO_TICK_MS) return;
    if (lastTick != 0 && now - lastTick > maxGap) maxGap = now - lastTick;
    lastTick = now;

    float t = (float)(now - moveStart) / (float)moveDuration;
    if (t > 1.0f) t = 1.0f;
    float e = (moveTo < moveFrom) ? easeSoftClose(t) : easeInOutSine(t);
    leafAngle = moveFrom + (moveTo - moveFrom) * e;
    writeServo(leafAngle);

    if (t >= 1.0f) {
      leafAngle = moveTo;
      Serial.printf("[PINTU] Gerak selesai, jeda update terlama %lu ms (ideal %lu)\n", maxGap, cfg::SERVO_TICK_MS);
      if (moveTo > HALF) {
        onFullyOpen(now);
      } else {
        phase = DoorPhase::Closed;
        autoCycle = false;
        Serial.println(">>> [STATUS] Pintu tertutup rapat. Kembali standby.");
      }
    }
    return;
  }

  if (phase == DoorPhase::Open && closeAt != 0 && !evacuation && (long)(now - closeAt) >= 0) {
    closeAt = 0;
    startMove(0.0f);
  }
}

void doorStartCycle() {
  manualOpen = false;
  autoCycle = true;
  if (phase == DoorPhase::Open) {
    if (!evacuation) closeAt = millis() + cfg::DOOR_HOLD_MS;
  } else if (phase != DoorPhase::Opening) {
    startMove(OPEN);
  }
}

void doorOpenManual() {
  manualOpen = true;
  autoCycle = false;
  closeAt = 0;
  if (!isOpenOrOpening()) startMove(OPEN);
  Serial.println("[PINTU] Dibuka MANUAL via Web");
}

void doorCloseManual() {
  if (evacuation) {
    Serial.println("[PINTU] Tutup ditolak: evakuasi api sedang aktif");
    return;
  }
  manualOpen = false;
  autoCycle = false;
  closeAt = 0;
  if (phase != DoorPhase::Closed && phase != DoorPhase::Closing) startMove(0.0f);
  Serial.println("[PINTU] Ditutup MANUAL via Web");
}

void doorSetEvacuation(bool active) {
  if (active == evacuation) return;
  evacuation = active;
  if (active) {
    autoCycle = true;
    closeAt = 0;
    if (!isOpenOrOpening()) startMove(OPEN);
  } else if (phase == DoorPhase::Open && !manualOpen) {
    closeAt = millis() + cfg::DOOR_HOLD_MS;
  }
}

DoorPhase doorPhase() { return phase; }
bool doorIsManualOpen() { return manualOpen; }
bool doorIsAutoCycle() { return autoCycle; }
bool doorIsEvacuating() { return evacuation; }

unsigned long doorHoldRemainingMs(unsigned long now) {
  if (phase != DoorPhase::Open || closeAt == 0) return 0;
  long left = (long)(closeAt - now);
  return left > 0 ? (unsigned long)left : 0;
}

const char *doorStatusText() {
  if (phase == DoorPhase::Closed || phase == DoorPhase::Closing) return "TERTUTUP";
  return autoCycle ? "AUTO_CYCLE" : "TERBUKA";
}
