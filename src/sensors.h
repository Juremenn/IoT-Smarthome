#pragma once
#include <Arduino.h>

// ---- Sensor api 5-way (D34) ----
void flameBegin();
void flameUpdate(unsigned long now);
bool flameDetected();
int flameRaw();
bool flameActiveHigh();
void flameToggleLogic();

// ---- Ultrasonik HC-SR04 (Trig D25, Echo D26, ISR non-blocking + median-3) ----
void ultrasonicBegin();
void ultrasonicUpdate(unsigned long now, bool enabled, bool shortTimeout = false);
int ultrasonicDistance();    // 0 = tidak ada objek / tidak ada echo
bool ultrasonicTriggered();  // true setelah konfirmasi 2x berturut-turut <= MAX_DISTANCE_CM
long ultrasonicLastEcho();
int ultrasonicTrigPin();
int ultrasonicEchoPin();

// ---- Sensor Suhu & Kelembaban DHT11 (D27, Core 0 + kalibrasi + EMA + NVS) ----
void dhtBegin();
void dhtUpdate(unsigned long now);
float dhtTemperature();
float dhtHumidity();
float dhtRawTemperature();
float dhtRawHumidity();
float dhtTempOffset();
float dhtHumOffset();
bool dhtIsValid();
bool dhtIsCalibrated();
const char *dhtStatusMessage();
void dhtSetCalibration(float tempOffset, float humOffset);
void dhtResetCalibration();
