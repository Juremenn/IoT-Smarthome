#include "web_server.h"

#include <WebServer.h>

#include "automation.h"
#include "config.h"
#include "door.h"
#include "fan.h"
#include "network.h"
#include "oled.h"
#include "sensors.h"
#include "web/alarm_mp3.h"
#include "web/app_js.h"
#include "web/index_html.h"
#include "web/style_css.h"

namespace {
WebServer server(80);

const char *boolStr(bool v) { return v ? "true" : "false"; }

// Format JSON identik dengan versi sebelum refactor (urutan dan nama field sama).
void handleStatus() {
  int mode = triggerMode();
  int distance = ultrasonicDistance();
  bool isHand = (mode == MODE_ULTRASONIC) && (distance > 0 && distance <= cfg::MAX_DISTANCE_CM);
  bool isTouched = (mode == MODE_TOUCH) && touchIsTouched();
  bool oled = oledIsActive();

  String json = "{";
  json += "\"flame\":" + String(boolStr(flameDetected())) + ",";
  json += "\"flameRaw\":" + String(flameRaw()) + ",";
  json += "\"flameActiveHigh\":" + String(boolStr(flameActiveHigh())) + ",";
  json += "\"fan\":" + String(boolStr(fanIsOn())) + ",";
  json += "\"fanPin\":" + String(fanPin()) + ",";
  json += "\"fanActiveLow\":" + String(boolStr(fanIsActiveLow())) + ",";
  json += "\"isScanning\":false,";
  json += "\"scannedPin\":-1,";
  json += "\"door\":\"" + String(doorStatusText()) + "\",";
  json += "\"doorTriggerMode\":" + String(mode) + ",";
  json += "\"oled\":" + String(boolStr(oled)) + ",";
  json += "\"oledAddress\":\"" + (oled ? String("0x") + String(oledAddress(), HEX) : "none") + "\",";
  json += "\"touchRaw\":" + String(touchRawValue()) + ",";
  json += "\"touchBaseline\":" + String(touchBaselineValue()) + ",";
  json += "\"touchDetected\":" + String(boolStr(isTouched)) + ",";
  json += "\"autoDoorUltrasonic\":" + String(boolStr(mode == MODE_ULTRASONIC)) + ",";
  json += "\"autoServo\":" + String(boolStr(mode != MODE_MANUAL)) + ",";
  json += "\"trigPin\":" + String(ultrasonicTrigPin()) + ",";
  json += "\"echoPin\":" + String(ultrasonicEchoPin()) + ",";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"maxDistance\":" + String(cfg::MAX_DISTANCE_CM) + ",";
  json += "\"handDetected\":" + String(boolStr(isHand || isTouched)) + ",";
  json += "\"rawEcho\":" + String(ultrasonicLastEcho()) + ",";
  json += "\"ip\":\"" + networkIp() + "\",";
  json += "\"uptime\":" + String(millis() / 1000);
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

void handleFan() {
  if (server.hasArg("set")) {
    fanSet(server.arg("set").toInt() == 1);
  } else if (server.hasArg("toggle")) {
    fanToggle();
  } else if (server.hasArg("pin")) {
    fanSetPin(server.arg("pin").toInt());
  } else if (server.hasArg("toggle_logic")) {
    fanToggleLogic();
  }
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
    } else if (action == "test") {
      doorTestSweep();
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
    String res = ultrasonicScanPins(fanPin());
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
  server.on("/api/fan", HTTP_GET, handleFan);
  server.on("/api/door", HTTP_GET, handleDoor);
  server.on("/api/ultrasonic", HTTP_GET, handleUltrasonic);
  server.begin();
  Serial.println("[WEB] Web Server aktif di port 80.");
}

void webHandle() { server.handleClient(); }
