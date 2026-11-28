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
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent
WEB_SRC_DIR = BASE_DIR / "web_src"

class SmartHomeState:
    def __init__(self):
        self.lock = threading.Lock()
        self.start_time = time.time()
        
        # Pintu & Servo (D13)
        self.door = "TERTUTUP"  # TERTUTUP, AUTO_CYCLE, TERBUKA, EVAKUASI
        self.doorTriggerMode = 3  # 0: Manual (Standby / Kontrol Web), 3: Ultrasonik
        self.auto_cycle_timer = None
        
        # Sensor Api (D34)
        self.flame = False
        self.flameRaw = 0
        self.flameActiveHigh = True
        
        # Sensor Ultrasonik (D25/D26)
        self.trigPin = 25
        self.echoPin = 26
        self.distance = 25
        self.maxDistance = 10
        self.rawEcho = 1450
        
        # DHT11
        self.temp = 28.4
        self.humidity = 64.0
        self.tempOffset = 0.0
        self.humOffset = 0.0
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
        # Evakuasi api (Prioritas Darurat Global)
        if self.flame:
            self.door = "EVAKUASI"
        elif self.door == "EVAKUASI" and not self.flame:
            self.door = "TERTUTUP"
            
        # Pemicu ultrasonik (<= 10 cm)
        if self.doorTriggerMode == 3 and (0 < self.distance <= self.maxDistance):
            if self.door == "TERTUTUP":
                self.trigger_auto_door()

    def to_dict(self):
        with self.lock:
            self.update_automation()
            uptime_sec = int(time.time() - self.start_time)
            
            return {
                "flame": bool(self.flame),
                "flameRaw": int(self.flameRaw),
                "flameActiveHigh": bool(self.flameActiveHigh),
                "door": str(self.door),
                "doorTriggerMode": int(self.doorTriggerMode),
                "oled": bool(self.oled),
                "oledAddress": str(self.oledAddress),
                "trigPin": int(self.trigPin),
                "echoPin": int(self.echoPin),
                "distance": int(self.distance),
                "maxDistance": int(self.maxDistance),
                "temp": round(float(self.temp + self.tempOffset), 1),
                "humidity": round(max(0.0, min(100.0, float(self.humidity + self.humOffset))), 1),
                "tempValid": bool(self.tempValid),
                "dhtStatus": str(self.dhtStatus),
                "dhtCalibrated": bool(abs(self.tempOffset) > 0.001 or abs(self.humOffset) > 0.001),
                "tempOffset": round(float(self.tempOffset), 1),
                "humOffset": round(float(self.humOffset), 1),
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

        elif path in ("/alarm.mp3", "/welcome.mp3"):
            mp3_file = WEB_SRC_DIR / path.lstrip("/")
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

        elif path in ("/joglo.png", "/guide-female.png", "/guide-male.png"):
            img_file = WEB_SRC_DIR / path.lstrip("/")
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

        elif path == "/api/flame":
            with state.lock:
                if "toggle_logic" in query:
                    state.flameActiveHigh = not state.flameActiveHigh
            self.send_json(state.to_dict())
            return

        elif path == "/api/calibrate":
            with state.lock:
                if "reset" in query:
                    state.tempOffset = 0.0
                    state.humOffset = 0.0
                else:
                    if "temp_offset" in query:
                        try:
                            state.tempOffset = max(-30.0, min(30.0, float(query["temp_offset"][0])))
                        except ValueError:
                            pass
                    if "hum_offset" in query:
                        try:
                            state.humOffset = max(-50.0, min(50.0, float(query["hum_offset"][0])))
                        except ValueError:
                            pass
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
                if "doorTriggerMode" in data:
                    mode_val = int(data["doorTriggerMode"])
                    if mode_val in (0, 3):
                        state.doorTriggerMode = mode_val

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
    httpd = ThreadingHTTPServer(server_address, MockServerHandler)
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
