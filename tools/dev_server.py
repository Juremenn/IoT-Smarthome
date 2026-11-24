#!/usr/bin/env python3
"""
Local Mock Server & ESP32 Hardware Simulator
Smart Home Miniatur Rumah Joglo

Menyajikan dashboard dari web_src/ dan mem-mock API firmware ESP32
secara interaktif dengan Zero-Impact Injected Simulator Drawer.
"""

import os
import sys
import json
import time
import threading
from urllib.parse import urlparse, parse_qs
from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent
WEB_SRC_DIR = BASE_DIR / "web_src"

class SmartHomeState:
    def __init__(self):
        self.lock = threading.Lock()
        self.start_time = time.time()
        
        # Pintu & Servo
        self.door = "TERTUTUP"  # TERTUTUP, AUTO_CYCLE, TERBUKA, EVAKUASI
        self.doorTriggerMode = 3  # 0: Manual, 1: Sentuh, 2: Api, 3: Ultrasonik
        self.auto_cycle_timer = None
        self.touch_timer = None
        
        # Kipas
        self.fan = False
        self.fanPin = 23
        self.fanActiveLow = True
        
        # Sensor Api
        self.flame = False
        self.flameRaw = 0
        self.flameActiveHigh = True
        
        # Sensor Sentuh D4
        self.touchRaw = 1380
        self.touchBaseline = 1380
        self.touchDetected = False
        
        # Sensor Ultrasonik
        self.trigPin = 4
        self.echoPin = 5
        self.distance = 25
        self.maxDistance = 15
        self.rawEcho = 1450
        
        # DHT11
        self.temp = 28.4
        self.humidity = 64.0
        self.tempValid = True
        self.dhtStatus = "OK"
        
        # Display & Sistem
        self.oled = True
        self.oledAddress = "0x3C"
        self.ip = "127.0.0.1:8080 (Sim)"

    def trigger_auto_door(self):
        if self.door == "EVAKUASI":
            return
        self.door = "AUTO_CYCLE"
        if self.auto_cycle_timer and self.auto_cycle_timer.is_alive():
            self.auto_cycle_timer.cancel()
        
        # Buka 2 detik lalu kembali tertutup (seperti firmware ESP32)
        def close_door():
            time.sleep(2.0)
            with self.lock:
                if self.door == "AUTO_CYCLE":
                    self.door = "TERTUTUP"
        
        self.auto_cycle_timer = threading.Thread(target=close_door, daemon=True)
        self.auto_cycle_timer.start()

    def update_automation(self):
        # Evakuasi api
        if self.doorTriggerMode == 2 and self.flame:
            self.door = "EVAKUASI"
        elif self.door == "EVAKUASI" and not self.flame:
            self.door = "TERTUTUP"
            
        # Pemicu ultrasonik
        if self.doorTriggerMode == 3 and (0 < self.distance <= self.maxDistance):
            if self.door == "TERTUTUP":
                self.trigger_auto_door()
                
        # Pemicu sentuh
        if self.doorTriggerMode == 1 and self.touchDetected:
            if self.door == "TERTUTUP":
                self.trigger_auto_door()

    def to_dict(self):
        with self.lock:
            self.update_automation()
            mode = self.doorTriggerMode
            is_touched = (mode == 1) and self.touchDetected
            uptime_sec = int(time.time() - self.start_time)
            
            return {
                "flame": bool(self.flame),
                "flameRaw": int(self.flameRaw),
                "flameActiveHigh": bool(self.flameActiveHigh),
                "fan": bool(self.fan),
                "fanPin": int(self.fanPin),
                "fanActiveLow": bool(self.fanActiveLow),
                "door": str(self.door),
                "doorTriggerMode": int(self.doorTriggerMode),
                "oled": bool(self.oled),
                "oledAddress": str(self.oledAddress),
                "touchRaw": int(self.touchRaw),
                "touchBaseline": int(self.touchBaseline),
                "touchDetected": bool(is_touched),
                "trigPin": int(self.trigPin),
                "echoPin": int(self.echoPin),
                "distance": int(self.distance),
                "maxDistance": int(self.maxDistance),
                "temp": round(float(self.temp), 1),
                "humidity": round(float(self.humidity), 1),
                "tempValid": bool(self.tempValid),
                "dhtStatus": str(self.dhtStatus),
                "rawEcho": int(self.rawEcho),
                "ip": str(self.ip),
                "uptime": uptime_sec,
            }

state = SmartHomeState()


class MockServerHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        # Mute excessive polling logs from /api/status to keep terminal clean
        if "GET /api/status" in args[0]:
            return
        super().log_message(format, *args)

    def send_cors_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_cors_headers()
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path
        query = parse_qs(parsed.query)

        # 1. Dashboard Web
        if path in ("/", "/index.html"):
            index_file = WEB_SRC_DIR / "index.html"
            if not index_file.exists():
                self.send_error(404, "web_src/index.html not found")
                return
            content = index_file.read_text(encoding="utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(content.encode("utf-8"))
            return

        elif path == "/style.css":
            css_file = WEB_SRC_DIR / "style.css"
            if not css_file.exists():
                self.send_error(404, "web_src/style.css not found")
                return
            content = css_file.read_text(encoding="utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/css; charset=utf-8")
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(content.encode("utf-8"))
            return

        elif path == "/app.js":
            js_file = WEB_SRC_DIR / "app.js"
            if not js_file.exists():
                self.send_error(404, "web_src/app.js not found")
                return
            content = js_file.read_text(encoding="utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/javascript; charset=utf-8")
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(content.encode("utf-8"))
            return

        elif path == "/alarm.mp3":
            mp3_file = WEB_SRC_DIR / "alarm.mp3"
            if mp3_file.exists():
                data = mp3_file.read_bytes()
            else:
                data = b""
            self.send_response(200)
            self.send_header("Content-Type", "audio/mpeg")
            self.send_header("Cache-Control", "max-age=86400")
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(data)
            return

        elif path == "/joglo.png":
            img_file = WEB_SRC_DIR / "joglo.png"
            if img_file.exists():
                data = img_file.read_bytes()
            else:
                data = b""
            self.send_response(200)
            self.send_header("Content-Type", "image/png")
            self.send_header("Cache-Control", "public, max-age=86400")
            self.send_cors_headers()
            self.end_headers()
            self.wfile.write(data)
            return

        # 2. ESP32 REST API
        elif path == "/api/status":
            self.send_json(state.to_dict())
            return

        elif path == "/api/door":
            action = query.get("action", [""])[0]
            with state.lock:
                if action == "auto":
                    state.trigger_auto_door()
                elif action == "open":
                    state.door = "TERBUKA"
                elif action == "close":
                    state.door = "TERTUTUP"
                elif action == "set_mode":
                    mode = query.get("mode", [""])[0]
                    if mode.isdigit():
                        state.doorTriggerMode = int(mode)
            self.send_json(state.to_dict())
            return

        elif path == "/api/fan":
            with state.lock:
                if "toggle" in query:
                    state.fan = not state.fan
                elif "state" in query:
                    val = query["state"][0]
                    state.fan = val in ("on", "1", "true")
                elif "toggle_logic" in query:
                    state.fanActiveLow = not state.fanActiveLow
            self.send_json(state.to_dict())
            return

        elif path == "/api/flame":
            with state.lock:
                if "toggle_logic" in query:
                    state.flameActiveHigh = not state.flameActiveHigh
            self.send_json(state.to_dict())
            return

        elif path == "/api/ultrasonic":
            with state.lock:
                if "swap" in query:
                    state.trigPin, state.echoPin = state.echoPin, state.trigPin
                elif "scan" in query:
                    res = {
                        "result": f"Pin HC-SR04 terdeteksi di Trig D{state.trigPin}, Echo D{state.echoPin}",
                        "trigPin": state.trigPin,
                        "echoPin": state.echoPin
                    }
                    self.send_json(res)
                    return
            self.send_json(state.to_dict())
            return

        self.send_error(404, "Not Found")

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path == "/api/simulator/update":
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode("utf-8")
            try:
                data = json.loads(body)
            except Exception:
                data = {}

            with state.lock:
                if "temp" in data:
                    state.temp = float(data["temp"])
                if "humidity" in data:
                    state.humidity = float(data["humidity"])
                if "distance" in data:
                    state.distance = int(data["distance"])
                if "flame" in data:
                    state.flame = bool(data["flame"])
                    state.flameRaw = 1 if state.flame else 0
                if "touchDetected" in data:
                    state.touchDetected = bool(data["touchDetected"])
                    state.touchRaw = 800 if state.touchDetected else 1380
                if "doorTriggerMode" in data:
                    state.doorTriggerMode = int(data["doorTriggerMode"])

            self.send_json(state.to_dict())
            return

        self.send_error(404, "Not Found")

    def send_json(self, data):
        payload = json.dumps(data)
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate")
        self.send_cors_headers()
        self.end_headers()
        self.wfile.write(payload.encode("utf-8"))

def run_server(port=8080):
    server_address = ("", port)
    httpd = HTTPServer(server_address, MockServerHandler)
    print("\n" + "=" * 58)
    print("  SMART HOME MINIATUR RUMAH JOGLO - LOCAL DEV SERVER")
    print(f"  Live Dashboard:   http://localhost:{port}")
    print(f"  Network IP Sim:   http://127.0.0.1:{port}")
    print(f"  Web Assets:       web_src/")
    print(f"  Simulator Panel:  Aktif di browser (sudut kanan bawah)")
    print("=" * 58 + "\n")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[STOP] Server dev dimatikan.")
        httpd.server_close()

if __name__ == "__main__":
    port = 8080
    if len(sys.argv) > 1 and sys.argv[1].isdigit():
        port = int(sys.argv[1])
    run_server(port)
