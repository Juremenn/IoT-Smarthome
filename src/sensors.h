#pragma once
#include <Arduino.h>

// ---- Sensor api 5-way (D34) ----
void flameBegin();
void flameUpdate(unsigned long now);
bool flameDetected();
int flameRaw();
bool flameActiveHigh();
void flameToggleLogic();

// ---- Sensor sentuh (Touch0, D4) ----
void touchBegin();
void touchUpdate(unsigned long now, bool enabled);
int touchRawValue();
int touchBaselineValue();
bool touchIsTouched();

// ---- Ultrasonik HC-SR04 ----
void ultrasonicBegin();
void ultrasonicUpdate(unsigned long now, bool enabled);
int ultrasonicDistance();  // 0 = tidak ada objek / tidak ada echo
long ultrasonicLastEcho();
int ultrasonicTrigPin();
int ultrasonicEchoPin();
void ultrasonicSwapPins();
String ultrasonicScanPins(int fanPin);

// ---- Sensor Suhu & Kelembaban DHT11 (D27) ----
void dhtBegin();
void dhtUpdate(unsigned long now);
float dhtTemperature();
float dhtHumidity();
bool dhtIsValid();
const char *dhtStatusMessage();
