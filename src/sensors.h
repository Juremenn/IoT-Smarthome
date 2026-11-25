#pragma once
#include <Arduino.h>

// ---- Sensor api 5-way (D34) ----
void flameBegin();
void flameUpdate(unsigned long now);
bool flameDetected();
int flameRaw();
bool flameActiveHigh();
void flameToggleLogic();

// ---- Ultrasonik HC-SR04 (Trig D25, Echo D26) ----
void ultrasonicBegin();
void ultrasonicUpdate(unsigned long now, bool enabled, bool shortTimeout = false);
int ultrasonicDistance();  // 0 = tidak ada objek / tidak ada echo
long ultrasonicLastEcho();
int ultrasonicTrigPin();
int ultrasonicEchoPin();

// ---- Sensor Suhu & Kelembaban DHT11 (D27) ----
void dhtBegin();
void dhtUpdate(unsigned long now);
float dhtTemperature();
float dhtHumidity();
bool dhtIsValid();
const char *dhtStatusMessage();
