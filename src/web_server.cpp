#include "web_server.h"

#include <WebServer.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_network.h"
#include "automation.h"
#include "config.h"
#include "door.h"
#include "oled.h"
#include "sensors.h"
#include "web/web_assets.h"

namespace {
WebServer server(80);
char jsonBuf[448];

const char *boolStr(bool v) { return v ? "true" : "false"; }

bool parseFloatArg(const char *name, float &outVal) {
  if (!server.hasArg(name)) return false;
  const String &raw = server.arg(name);
  if (raw.length() == 0) return false;
  char *endPtr = nullptr;
  float v = strtof(raw.c_str(), &endPtr);
  if (endPtr == raw.c_str()) return false;
  outVal = v;
  return true;
}

// Format JSON zero-heap (tanpa String) untuk respons instan < 5 ms
void handleStatus() {
  bool oled = oledIsActive();
  char oledAddr[8];
  if (oled) {
    snprintf(oledAddr, sizeof(oledAddr), "0x%02X", oledAddress());
  } else {
    snprintf(oledAddr, sizeof(oledAddr), "none");
  }

  snprintf(
      jsonBuf, sizeof(jsonBuf),
      "{\"flame\":%s,\"flameRaw\":%d,\"flameActiveHigh\":%s,"
      "\"door\":\"%s\",\"doorTriggerMode\":%d,"
      "\"oled\":%s,\"oledAddress\":\"%s\","
      "\"trigPin\":%d,\"echoPin\":%d,\"distance\":%d,\"maxDistance\":%d,"
      "\"temp\":%.1f,\"humidity\":%.1f,\"tempValid\":%s,\"dhtStatus\":\"%s\","
      "\"dhtCalibrated\":%s,\"tempOffset\":%.1f,\"humOffset\":%.1f,"
      "\"rawEcho\":%ld,\"ip\":\"%s\",\"uptime\":%lu}",
      boolStr(flameDetected()), flameRaw(), boolStr(flameActiveHigh()),
      doorStatusText(), triggerMode(),
      boolStr(oled), oledAddr,
      ultrasonicTrigPin(), ultrasonicEchoPin(), ultrasonicDistance(), cfg::MAX_DISTANCE_CM,
      dhtTemperature(), dhtHumidity(), boolStr(dhtIsValid()), dhtStatusMessage(),
      boolStr(dhtIsCalibrated()), dhtTempOffset(), dhtHumOffset(),
      ultrasonicLastEcho(), networkIp().c_str(), millis() / 1000UL);

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "application/json", jsonBuf);
}

void sendGzipAsset(const char *type, const uint8_t *data, size_t len, const char *cacheControl) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Cache-Control", cacheControl);
  server.send_P(200, type, (PGM_P)data, len);
}

void handleFlame() {
  if (server.hasArg("toggle_logic")) flameToggleLogic();
  handleStatus();
}

void handleDoor() {
  if (server.hasArg("action")) {
    const String &action = server.arg("action");
    if (action == "auto") {
      doorStartCycle();
    } else if (action == "open") {
      doorOpenManual();
    } else if (action == "close") {
      doorCloseManual();
    } else if (action == "set_mode" && server.hasArg("mode")) {
      setTriggerMode(server.arg("mode").toInt());
    }
  }
  handleStatus();
}

// Endpoint kalibrasi live + simpan NVS:
//   GET /api/calibrate?temp_offset=-7.0&hum_offset=-25.0
//   GET /api/calibrate?reset=1
void handleCalibrate() {
  if (server.hasArg("reset")) {
    dhtResetCalibration();
    handleStatus();
    return;
  }

  float tOff = dhtTempOffset();
  float hOff = dhtHumOffset();
  bool changed = false;
  if (parseFloatArg("temp_offset", tOff)) changed = true;
  if (parseFloatArg("hum_offset", hOff)) changed = true;
  if (changed) {
    dhtSetCalibration(tOff, hOff);
  }
  handleStatus();
}
}  // namespace

void webBegin() {
  // Matikan Nagle algorithm (TCP_NODELAY) agar respons kontrol tombol langsung terkirim
  server.enableDelay(false);

  server.on("/", HTTP_GET, [] {
    sendGzipAsset("text/html; charset=utf-8", INDEX_HTML_GZ, INDEX_HTML_GZ_LEN, "no-cache");
  });
  server.on("/style.css", HTTP_GET, [] {
    sendGzipAsset("text/css; charset=utf-8", STYLE_CSS_GZ, STYLE_CSS_GZ_LEN, "public, max-age=3600");
  });
  server.on("/app.js", HTTP_GET, [] {
    sendGzipAsset("application/javascript; charset=utf-8", APP_JS_GZ, APP_JS_GZ_LEN,
                  "public, max-age=3600");
  });
  server.on("/alarm.mp3", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.send_P(200, "audio/mpeg", (PGM_P)ALARM_MP3, ALARM_MP3_LEN);
  });
  server.on("/welcome.mp3", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.send_P(200, "audio/mpeg", (PGM_P)WELCOME_MP3, WELCOME_MP3_LEN);
  });
  server.on("/joglo.png", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.send_P(200, "image/png", (PGM_P)JOGLO_PNG, JOGLO_PNG_LEN);
  });
  server.on("/guide-female.png", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "public, max-age=86400");
    server.send_P(200, "image/png", (PGM_P)GUIDE_FEMALE_PNG, GUIDE_FEMALE_PNG_LEN);
  });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/flame", HTTP_GET, handleFlame);
  server.on("/api/door", HTTP_GET, handleDoor);
  server.on("/api/calibrate", HTTP_GET, handleCalibrate);
  server.begin();
}

void webHandle() { server.handleClient(); }
