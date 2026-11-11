#pragma once
#include <Arduino.h>

// Coba WiFi rumah dulu, kalau gagal jadi Access Point sendiri.
void networkBegin();
void networkMaintain(unsigned long now);
const String &networkIp();
bool networkIsAccessPoint();
