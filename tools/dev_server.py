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

SIMULATOR_DRAWER_HTML = """
<!-- SIMULATOR DRAWER (Injected purely in dev server - Zero ESP32 overhead) -->
<style>
#sim-toggle {
  position: fixed;
  bottom: 20px;
  right: 20px;
  z-index: 99999;
  background: #2B1D16;
  color: #F6F2EA;
  border: 1px solid rgba(200, 176, 138, 0.6);
  border-radius: 30px;
  padding: 10px 18px;
  font-family: 'Poppins', system-ui, -apple-system, sans-serif;
  font-size: 13px;
  font-weight: 600;
  box-shadow: 0 4px 14px rgba(43, 29, 22, 0.3);
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 8px;
  transition: transform 0.15s ease, background 0.2s ease, border-color 0.2s ease;
}
#sim-toggle:hover { background: #3B281E; border-color: #C8B08A; transform: scale(1.02); }
#sim-toggle .sim-dot {
  width: 9px;
  height: 9px;
  background: #10B981;
  border-radius: 50%;
  box-shadow: 0 0 8px #10B981;
}

#sim-panel {
  position: fixed;
  bottom: 74px;
  right: 20px;
  width: 330px;
  max-width: calc(100vw - 40px);
  max-height: 80vh;
  overflow-y: auto;
  background: #FCFBF7;
  color: #3B281E;
  border: 1px solid #C8B08A;
  border-radius: 12px;
  box-shadow: 0 10px 30px rgba(59, 40, 30, 0.15);
  font-family: 'Poppins', system-ui, -apple-system, sans-serif;
  padding: 18px;
  z-index: 99999;
  display: none;
}
#sim-panel.show { display: block; animation: simFadeIn 0.18s ease-out; }
@keyframes simFadeIn { from { opacity: 0; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }

.sim-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-bottom: 10px;
  border-bottom: 1px solid #eaeef2;
  margin-bottom: 12px;
}
.sim-header h3 { font-size: 14px; font-weight: 700; margin: 0; display: flex; align-items: center; gap: 6px; }
.sim-close { background: none; border: none; font-size: 18px; cursor: pointer; color: #57606a; padding: 2px 6px; }

.sim-group { margin-bottom: 12px; }
.sim-label { display: flex; justify-content: space-between; font-size: 12px; font-weight: 600; color: #57606a; margin-bottom: 4px; }
.sim-slider { width: 100%; height: 6px; accent-color: #0969da; cursor: pointer; }

.sim-btn-row { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 10px; }
.sim-btn {
  padding: 8px 10px;
  border-radius: 8px;
  border: 1px solid #d0d7de;
  background: #f6f8fa;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.15s ease;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 5px;
}
.sim-btn:hover { background: #eaeef2; }
.sim-btn.danger-on { background: #d73a49; color: #fff; border-color: #cb2431; box-shadow: 0 0 8px rgba(215,58,73,0.5); }
.sim-btn.active-touch { background: #2ea44f; color: #fff; border-color: #2c974b; }

.sim-status-box {
  background: #f6f8fa;
  border-radius: 8px;
  padding: 8px 10px;
  font-size: 11px;
  margin-top: 12px;
  border: 1px solid #e1e4e8;
  color: #444d56;
}
.sim-status-box span { font-weight: 600; }
</style>

<button type="button" id="sim-toggle" title="ESP32 Simulator Controls">
  <span class="sim-dot"></span>
  <span>ESP32 Simulator</span>
</button>

<div id="sim-panel">
  <div class="sim-header">
    <h3>⚙️ Kontrol Sensor Hardware</h3>
    <button type="button" class="sim-close" id="sim-close-btn">&times;</button>
  </div>

  <div class="sim-group">
    <div class="sim-label"><span>🌡️ Suhu Ruang (DHT11)</span><span id="sim-temp-val">28.4 °C</span></div>
    <input type="range" class="sim-slider" id="sim-temp-range" min="15" max="50" step="0.5" value="28.4">
  </div>

  <div class="sim-group">
    <div class="sim-label"><span>💧 Kelembaban</span><span id="sim-hum-val">64 %</span></div>
    <input type="range" class="sim-slider" id="sim-hum-range" min="20" max="95" step="1" value="64">
  </div>

  <div class="sim-group">
    <div class="sim-label"><span>📏 Jarak Ultrasonik</span><span id="sim-dist-val">25 cm</span></div>
    <input type="range" class="sim-slider" id="sim-dist-range" min="1" max="40" step="1" value="25">
    <div style="font-size: 10px; color: #8c959f; margin-top: 2px;">Trigger buka pintu saat &le; 10 cm (Mode 3)</div>
  </div>

  <div class="sim-btn-row">
    <button type="button" class="sim-btn" id="sim-flame-btn">🔥 Api: Aman</button>
    <button type="button" class="sim-btn" id="sim-touch-btn">👆 Sentuh D4</button>
  </div>

  <div class="sim-status-box" id="sim-status-readout">
    <div>Servo: <span id="sim-read-door">TERTUTUP</span> | Kipas: <span id="sim-read-fan">OFF</span></div>
    <div>Pemicu Aktif: <span id="sim-read-mode">Ultrasonik</span></div>
  </div>
</div>

<script>
(() => {
  const toggle = document.getElementById('sim-toggle');
  const panel = document.getElementById('sim-panel');
  const closeBtn = document.getElementById('sim-close-btn');
  
  const tempRange = document.getElementById('sim-temp-range');
  const tempVal = document.getElementById('sim-temp-val');
  const humRange = document.getElementById('sim-hum-range');
  const humVal = document.getElementById('sim-hum-val');
  const distRange = document.getElementById('sim-dist-range');
  const distVal = document.getElementById('sim-dist-val');
  const flameBtn = document.getElementById('sim-flame-btn');
  const touchBtn = document.getElementById('sim-touch-btn');
  
  const readDoor = document.getElementById('sim-read-door');
  const readFan = document.getElementById('sim-read-fan');
  const readMode = document.getElementById('sim-read-mode');

  let isFlame = false;
  let isTouch = false;

  toggle.addEventListener('click', () => panel.classList.toggle('show'));
  closeBtn.addEventListener('click', () => panel.classList.remove('show'));

  function updateServer(data) {
    fetch('/api/simulator/update', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data)
    }).catch(() => {});
  }

  tempRange.addEventListener('input', e => {
    tempVal.textContent = e.target.value + ' °C';
    updateServer({ temp: parseFloat(e.target.value) });
  });

  humRange.addEventListener('input', e => {
    humVal.textContent = e.target.value + ' %';
    updateServer({ humidity: parseFloat(e.target.value) });
  });

  distRange.addEventListener('input', e => {
    distVal.textContent = e.target.value + ' cm';
    updateServer({ distance: parseInt(e.target.value) });
  });

  flameBtn.addEventListener('click', () => {
    isFlame = !isFlame;
    flameBtn.textContent = isFlame ? '🔥 API BAHAYA!' : '🔥 Api: Aman';
    flameBtn.classList.toggle('danger-on', isFlame);
    updateServer({ flame: isFlame });
  });

  // Sentuh D4 bisa momentary atau click
  touchBtn.addEventListener('mousedown', () => {
    isTouch = true;
    touchBtn.classList.add('active-touch');
    updateServer({ touchDetected: true });
  });
  window.addEventListener('mouseup', () => {
    if (isTouch) {
      isTouch = false;
      touchBtn.classList.remove('active-touch');
      updateServer({ touchDetected: false });
    }
  });

  // Sync simulator status view with /api/status periodically
  setInterval(async () => {
    try {
      const res = await fetch('/api/status');
      if (res.ok) {
        const d = await res.json();
        readDoor.textContent = d.door;
        readFan.textContent = d.fan ? 'ON' : 'OFF';
        const modes = {0: 'Manual', 1: 'Sentuh', 2: 'Api', 3: 'Ultrasonik'};
        readMode.textContent = modes[d.doorTriggerMode] || 'Unknown';
      }
    } catch (e) {}
  }, 1000);
})();
</script>
"""

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
            # Inject simulator drawer right before </body>
            if "</body>" in content:
                content = content.replace("</body>", f"{SIMULATOR_DRAWER_HTML}\n</body>")
            else:
                content += SIMULATOR_DRAWER_HTML
            
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
