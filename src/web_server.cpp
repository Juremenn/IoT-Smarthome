#include "web_server.h"

#include <WebServer.h>

#include "automation.h"
#include "config.h"
#include "door.h"
#include "app_network.h"
#include "oled.h"
#include "sensors.h"
#include "web/alarm_mp3.h"
#include "web/app_js.h"
#include "web/index_html.h"
#include "web/style_css.h"

namespace {
WebServer server(80);

const char *boolStr(bool v) { return v ? "true" : "false"; }

// Format JSON efisien & ringan untuk polling cepat (300ms)
void handleStatus() {
  int mode = triggerMode();
  int distance = ultrasonicDistance();
  bool isTouched = (mode == MODE_TOUCH) && touchIsTouched();
  bool oled = oledIsActive();

  String json;
  json.reserve(320);
  json += "{\"flame\":";
  json += boolStr(flameDetected());
  json += ",\"flameRaw\":";
  json += flameRaw();
  json += ",\"flameActiveHigh\":";
  json += boolStr(flameActiveHigh());
  json += ",\"door\":\"";
  json += doorStatusText();
  json += "\",\"doorTriggerMode\":";
  json += mode;
  json += ",\"oled\":";
  json += boolStr(oled);
  json += ",\"oledAddress\":\"";
  json += oled ? ("0x" + String(oledAddress(), HEX)) : "none";
  json += "\",\"touchRaw\":";
  json += touchRawValue();
  json += ",\"touchBaseline\":";
  json += touchBaselineValue();
  json += ",\"touchDetected\":";
  json += boolStr(isTouched);
  json += ",\"trigPin\":";
  json += ultrasonicTrigPin();
  json += ",\"echoPin\":";
  json += ultrasonicEchoPin();
  json += ",\"distance\":";
  json += distance;
  json += ",\"maxDistance\":";
  json += cfg::MAX_DISTANCE_CM;
  json += ",\"temp\":";
  json += String(dhtTemperature(), 1);
  json += ",\"humidity\":";
  json += String(dhtHumidity(), 1);
  json += ",\"tempValid\":";
  json += boolStr(dhtIsValid());
  json += ",\"dhtStatus\":\"";
  json += dhtStatusMessage();
  json += "\",\"rawEcho\":";
  json += ultrasonicLastEcho();
  json += ",\"ip\":\"";
  json += networkIp();
  json += "\",\"uptime\":";
  json += (millis() / 1000);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.send(200, "application/json", json);
}

void sendAsset(const char *type, const char *body) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, type, body);
}

void handleFlame() {
  if (server.hasArg("toggle_logic")) flameToggleLogic();
  handleStatus();
}

void handleDoor() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
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

void handleUltrasonic() {
  if (server.hasArg("swap")) {
    ultrasonicSwapPins();
  } else if (server.hasArg("scan")) {
    String res = ultrasonicScanPins();
    String json = "{\"result\":\"" + res + "\",\"trigPin\":" + String(ultrasonicTrigPin()) +
                  ",\"echoPin\":" + String(ultrasonicEchoPin()) + "}";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
    return;
  }
  handleStatus();
}
} // namespace

void webBegin() {
  server.on("/", HTTP_GET, [] { sendAsset("text/html; charset=utf-8", INDEX_HTML); });
  server.on("/style.css", HTTP_GET, [] { sendAsset("text/css; charset=utf-8", STYLE_CSS); });
  server.on("/app.js", HTTP_GET,
            [] { sendAsset("application/javascript; charset=utf-8", APP_JS); });
  server.on("/alarm.mp3", HTTP_GET, [] {
    server.sendHeader("Cache-Control", "max-age=86400");
    server.send_P(200, "audio/mpeg", (PGM_P)ALARM_MP3, ALARM_MP3_LEN);
  });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/flame", HTTP_GET, handleFlame);
  server.on("/api/door", HTTP_GET, handleDoor);
  server.on("/api/ultrasonic", HTTP_GET, handleUltrasonic);
  server.begin();
  Serial.println("[WEB] Web Server aktif di port 80.");
}

void webHandle() { server.handleClient(); }
