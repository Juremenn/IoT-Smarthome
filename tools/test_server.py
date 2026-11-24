import urllib.request
import urllib.parse
import json
import time

BASE_URL = "http://127.0.0.1:8080"

def get(path):
    req = urllib.request.Request(BASE_URL + path)
    with urllib.request.urlopen(req, timeout=3) as res:
        return res.status, res.headers.get("Content-Type"), res.read()

def post(path, data):
    body = json.dumps(data).encode("utf-8")
    req = urllib.request.Request(BASE_URL + path, data=body, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=3) as res:
        return res.status, res.headers.get("Content-Type"), res.read()

def main():
    print("Testing Mock Dev Server Endpoints...")
    
    # 1. Test HTML
    st, ct, body = get("/")
    assert st == 200, f"Expected 200, got {st}"
    assert "text/html" in ct, f"Expected text/html, got {ct}"
    html_str = body.decode("utf-8")
    assert "Rumah Joglo" in html_str, "HTML must contain title"
    assert "ESP32 Simulator" not in html_str, "Simulator drawer must be removed"
    assert "connPill" not in html_str, "Connection pill must be removed"
    print(" [PASS] GET / (pure dashboard without AI slop pills)")

    # 1b. Test GET /joglo.png
    st, ct, png_bytes = get("/joglo.png")
    assert st == 200 and "image/png" in ct
    assert png_bytes[:4] == b"\x89PNG", "Valid PNG magic header"
    print(" [PASS] GET /joglo.png")

    # 2. Test CSS & JS
    st, ct, _ = get("/style.css")
    assert st == 200 and "text/css" in ct
    print(" [PASS] GET /style.css")

    st, ct, _ = get("/app.js")
    assert st == 200 and "javascript" in ct
    print(" [PASS] GET /app.js")

    # 3. Test /api/status
    st, ct, body = get("/api/status")
    assert st == 200
    data = json.loads(body.decode("utf-8"))
    assert "flame" in data and "door" in data and "temp" in data
    print(f" [PASS] GET /api/status (door={data['door']}, temp={data['temp']}C)")

    # 4. Test /api/door action=auto
    st, ct, body = get("/api/door?action=auto")
    data = json.loads(body.decode("utf-8"))
    assert data["door"] in ("AUTO_CYCLE", "TERBUKA")
    print(f" [PASS] GET /api/door?action=auto -> door: {data['door']}")

    # 5. Test /api/fan toggle
    st, ct, body = get("/api/status")
    prev_fan = json.loads(body.decode("utf-8"))["fan"]
    st, ct, body = get("/api/fan?toggle=1")
    data = json.loads(body.decode("utf-8"))
    assert data["fan"] != prev_fan
    print(f" [PASS] GET /api/fan?toggle=1 -> fan changed from {prev_fan} to {data['fan']}")

    # 6. Test /api/simulator/update with flame evacuation priority
    st, ct, body = post("/api/simulator/update", {"temp": 35.5, "flame": True})
    data = json.loads(body.decode("utf-8"))
    assert data["temp"] == 35.5
    assert data["flame"] is True
    assert data["door"] == "EVAKUASI", f"Door must be EVAKUASI during fire, got {data['door']}"
    print(f" [PASS] POST /api/simulator/update -> temp: {data['temp']}, flame: {data['flame']}, door: {data['door']}")

    # 6b. Reset flame to False
    st, ct, body = post("/api/simulator/update", {"flame": False})
    data = json.loads(body.decode("utf-8"))
    assert data["flame"] is False
    assert data["door"] == "TERTUTUP"
    print(" [PASS] Fire cleared -> door returned to TERTUTUP")

    print("\nALL DEV SERVER TESTS PASSED SUCCESSFULLY!")

if __name__ == "__main__":
    main()
