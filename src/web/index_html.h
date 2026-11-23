#pragma once
#include <pgmspace.h>

// Halaman dashboard. Disajikan di "/" oleh web_server.cpp.
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <meta name="theme-color" content="#F6F2EA">
  <title>Rumah Joglo - Smart Home IoT</title>
  <link rel="icon" href="data:,">
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Montserrat:wght@500;600;700;800&family=Poppins:wght@400;500;600&family=JetBrains+Mono:wght@400;500;600&display=swap" rel="stylesheet">
  <link rel="stylesheet" href="/style.css">
</head>
<body>

<!-- Batik Kawung Background Texture -->
<div class="kawung-texture" aria-hidden="true"></div>

<!-- Gunungan Corner Silhouettes -->
<svg class="gunungan-frame gunungan-left" viewBox="0 0 100 150" fill="none" stroke="#C8B08A" stroke-width="1.2" aria-hidden="true">
  <path d="M50 5 C50 5, 20 50, 10 90 C5 110, 10 135, 20 145 L80 145 C90 135, 95 110, 90 90 C80 50, 50 5, 50 5 Z"/>
  <path d="M50 15 L50 145"/>
  <path d="M50 45 C35 60, 25 75, 25 95"/>
  <path d="M50 45 C65 60, 75 75, 75 95"/>
  <path d="M50 75 C30 90, 20 105, 22 125"/>
  <path d="M50 75 C70 90, 80 105, 78 125"/>
  <rect x="35" y="115" width="30" height="30" rx="3"/>
  <circle cx="50" cy="30" r="4"/>
</svg>

<svg class="gunungan-frame gunungan-right" viewBox="0 0 100 150" fill="none" stroke="#C8B08A" stroke-width="1.2" aria-hidden="true">
  <path d="M50 5 C50 5, 20 50, 10 90 C5 110, 10 135, 20 145 L80 145 C90 135, 95 110, 90 90 C80 50, 50 5, 50 5 Z"/>
  <path d="M50 15 L50 145"/>
  <path d="M50 45 C35 60, 25 75, 25 95"/>
  <path d="M50 45 C65 60, 75 75, 75 95"/>
  <path d="M50 75 C30 90, 20 105, 22 125"/>
  <path d="M50 75 C70 90, 80 105, 78 125"/>
  <rect x="35" y="115" width="30" height="30" rx="3"/>
  <circle cx="50" cy="30" r="4"/>
</svg>

<div class="app-container">

  <!-- Header Bar -->
  <header class="header-bar">
    <div class="brand-wrapper">
      <div class="brand-icon" aria-hidden="true">
        <!-- Minimalist Yogyakarta Joglo Roof SVG -->
        <svg viewBox="0 0 36 36" width="36" height="36" fill="none" stroke="#111827" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">
          <path d="M4 28 L11 20 L15 9 L21 9 L25 20 L32 28 Z"/>
          <path d="M11 20 L25 20"/>
          <path d="M15 9 L21 9"/>
          <path d="M14 28 L14 20"/>
          <path d="M22 28 L22 20"/>
          <path d="M2 28 L34 28"/>
        </svg>
      </div>
      <div>
        <h1 class="brand-title">Rumah Joglo</h1>
        <p class="brand-subtitle">Smart Home Yogyakarta Keraton</p>
      </div>
    </div>

    <div class="conn-pill" id="connPill" role="status">
      <span class="conn-indicator" id="connDot" aria-hidden="true"></span>
      <span id="connLabel">127.0.0.1:8080 (Sim)</span>
    </div>
  </header>

  <!-- View 1: Perangkat (5-Card Symmetrical Bento Grid) -->
  <main class="view-panel active" id="view-perangkat" role="tabpanel" aria-labelledby="tab-perangkat">
    <div class="bento-grid">

      <!-- Row 1, Card 1: Pintu (Col-Span-2) -->
      <section class="card col-span-2" id="cardDoor" aria-labelledby="doorLabel">
        <div class="card-header">
          <span class="card-label" id="doorLabel">Pintu Utama</span>
          <span class="card-sub-icon" aria-hidden="true">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><rect x="3" y="3" width="18" height="18" rx="2"/><path d="M9 3v18"/></svg>
          </span>
        </div>

        <div class="centerpiece">
          <div class="icon-badge badge-terracotta" id="doorIconBadge">
            <svg id="doorIconSvg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
              <path id="doorPath1" d="M18 20V6a2 2 0 0 0-2-2H8a2 2 0 0 0-2 2v14"/>
              <path id="doorPath2" d="M2 20h20"/>
              <circle id="doorPath3" cx="14" cy="12" r="1" fill="currentColor"/>
            </svg>
          </div>
          <div class="status-meta">
            <span class="status-main" id="doorStatusText">Tertutup</span>
            <span class="status-sub" id="doorSubText">Mode Otomasi Aktif</span>
            <span class="telemetry-mono" id="doorTelemetry">Servo GPIO D13: 180°</span>
          </div>
        </div>

        <div class="card-actions">
          <button type="button" class="btn-primary" id="btnDoorAuto">
            <span id="btnDoorAutoLabel">Buka 2 detik</span>
          </button>
          <div class="btn-group-2">
            <button type="button" class="btn-secondary" id="btnDoorOpen">Buka terus</button>
            <button type="button" class="btn-secondary" id="btnDoorClose">Tutup</button>
          </div>
        </div>
      </section>

      <!-- Row 1, Card 2: Api (Col-Span-1) -->
      <section class="card col-span-1" id="cardFlame" aria-labelledby="flameLabel">
        <div class="card-header">
          <span class="card-label" id="flameLabel">Sensor Api</span>
          <span class="card-sub-icon" aria-hidden="true">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M12 2v4M12 18v4M4.93 4.93l2.83 2.83M16.24 16.24l2.83 2.83M2 12h4M18 12h4M4.93 19.07l2.83-2.83M16.24 7.76l2.83-2.83"/></svg>
          </span>
        </div>

        <div class="centerpiece">
          <div class="icon-badge badge-celadon" id="flameBadge">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
              <path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 2.5z"/>
            </svg>
          </div>
          <div class="status-meta">
            <span class="status-main" id="flameStatusText">Aman</span>
            <span class="status-sub" id="flameSubText">Kondisi Normal</span>
            <span class="telemetry-mono" id="flameTelemetry">D34: 0 (Active-HIGH)</span>
          </div>
        </div>

        <div>
          <button type="button" class="btn-pill" id="btnFlameToggleLogic">
            Ganti: Active-LOW
          </button>
        </div>
      </section>

      <!-- Row 2, Card 3: Pemicu (Ultrasonik) -->
      <section class="card col-span-1" id="cardTrigger" aria-labelledby="triggerLabel">
        <div class="card-header">
          <span class="card-label" id="triggerLabel">Pemicu Pintu</span>
          <span class="card-sub-icon" aria-hidden="true">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><circle cx="12" cy="12" r="10"/><path d="m4.93 4.93 4.24 4.24M14.83 9.17l4.24-4.24M14.83 14.83l4.24 4.24M9.17 14.83l-4.24 4.24"/></svg>
          </span>
        </div>

        <div class="centerpiece">
          <div class="icon-badge badge-muted ripple-indicator" id="triggerBadge">
            <!-- Lucide Waves / Radar Icon (No target/bullseye) -->
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
              <path d="M2 10c3-3 7-3 10 0s7 3 10 0"/>
              <path d="M2 14c3-3 7-3 10 0s7 3 10 0"/>
              <path d="M2 18c3-3 7-3 10 0s7 3 10 0"/>
            </svg>
          </div>
          <div class="status-meta">
            <span class="status-main" id="triggerModeText">Ultrasonik</span>
            <span class="status-sub" id="triggerStatusSub">Standby</span>
            <span class="telemetry-mono" id="triggerPinInfo">Trig D4, Echo D5</span>
          </div>
        </div>

        <div class="status-sub" id="triggerDetailText" style="margin-top: 10px;">
          Pintu terbuka saat terdeteksi jarak ≤ 15 cm
        </div>
      </section>

      <!-- Row 2, Card 4: Jarak -->
      <section class="card col-span-1" id="cardDist" aria-labelledby="distLabel">
        <div class="card-header">
          <span class="card-label" id="distLabel">Jarak Halangan</span>
          <span class="card-sub-icon" aria-hidden="true">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3Z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
          </span>
        </div>

        <div class="metric-display">
          <span class="metric-num" id="distMetricNum">25</span>
          <span class="metric-unit">cm</span>
        </div>

        <!-- 4-Segment Horizontal Gauge -->
        <div class="gauge-segments" aria-hidden="true">
          <div class="segment" id="seg1"></div>
          <div class="segment" id="seg2"></div>
          <div class="segment" id="seg3"></div>
          <div class="segment" id="seg4"></div>
        </div>

        <div class="status-sub" id="distSubLabel">
          Status: Standby
        </div>
      </section>

      <!-- Row 2, Card 5: Suhu Ruang -->
      <section class="card col-span-1" id="cardTemp" aria-labelledby="tempLabel">
        <div class="card-header">
          <span class="card-label" id="tempLabel">Suhu Ruang</span>
          <span class="card-sub-icon" aria-hidden="true">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/></svg>
          </span>
        </div>

        <div class="metric-display">
          <span class="metric-num" id="tempMetricNum">28.4</span>
          <span class="metric-unit">°C</span>
        </div>

        <!-- Celadon Dual-Tone Progress Bar -->
        <div class="progress-bar-track" aria-hidden="true">
          <div class="progress-bar-fill" id="tempProgressFill" style="width: 44%;"></div>
        </div>

        <div class="status-sub" id="humSubLabel" style="display: flex; align-items: center; gap: 4px;">
          <svg viewBox="0 0 24 24" width="14" height="14" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"><path d="M12 22a7 7 0 0 0 7-7c0-2-1-3.9-3-5.5s-3.5-4-4-6.5c-.5 2.5-2 4.9-4 6.5C6 11.1 5 13 5 15a7 7 0 0 0 7 7z"/></svg>
          <span id="humText">Lembab: 64%</span>
        </div>
      </section>

    </div>
  </main>

  <!-- View 2: Info & Pinout Map -->
  <section class="view-panel" id="view-info" role="tabpanel" aria-labelledby="tab-info">
    <div class="info-grid">
      <table class="pin-table">
        <thead>
          <tr>
            <th>Modul Perangkat</th>
            <th>Pin GPIO</th>
            <th>Antarmuka / Sinyal</th>
            <th>Keterangan</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td><strong>Servo Pintu Utama</strong></td>
            <td><code>GPIO 13</code></td>
            <td>PWM 50 Hz (500-2400 µs)</td>
            <td>Buka 0°, Tutup 180° (Gerak Halus)</td>
          </tr>
          <tr>
            <td><strong>Servo Pintu Alternatif</strong></td>
            <td><code>GPIO 12</code></td>
            <td>PWM 50 Hz Paralel</td>
            <td>Siklus gerak sinkron dengan pintu utama</td>
          </tr>
          <tr>
            <td><strong>Sensor Api 5-Way</strong></td>
            <td><code>GPIO 34</code></td>
            <td>Digital Input (3.3V)</td>
            <td>Evakuasi darurat, prioritas keselamatan</td>
          </tr>
          <tr>
            <td><strong>Sensor Jarak Ultrasonik</strong></td>
            <td><code>Trig D4, Echo D5</code></td>
            <td>Pulse Input / Output</td>
            <td>Deteksi halangan pengunjung mendekat</td>
          </tr>
          <tr>
            <td><strong>Sensor Suhu DHT11</strong></td>
            <td><code>GPIO 27</code></td>
            <td>Single-Wire Bus</td>
            <td>Suhu & Kelembaban ruang</td>
          </tr>
          <tr>
            <td><strong>Layar OLED SSD1306</strong></td>
            <td><code>GPIO 21 (SDA), 22 (SCL)</code></td>
            <td>I2C (0x3C / 0x3D)</td>
            <td>Visual wayang & telemetri sistem</td>
          </tr>
          <tr>
            <td><strong>Kipas Pendingin</strong></td>
            <td><code>Hardwired VCC / GND</code></td>
            <td>Suplai Daya Langsung</td>
            <td>Menyala kontinu tanpa modulasi GPIO</td>
          </tr>
        </tbody>
      </table>
    </div>
  </section>

</div>

<!-- Bottom Navigation Bar -->
<nav class="bottom-nav" aria-label="Menu Navigasi">
  <div class="nav-inner">
    <button type="button" class="nav-tab active" id="tab-perangkat" data-target="view-perangkat">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round">
        <rect x="3" y="3" width="7" height="7" rx="1.5"/>
        <rect x="14" y="3" width="7" height="7" rx="1.5"/>
        <rect x="3" y="14" width="7" height="7" rx="1.5"/>
        <rect x="14" y="14" width="7" height="7" rx="1.5"/>
      </svg>
      <span>Perangkat</span>
    </button>

    <button type="button" class="nav-tab" id="tab-info" data-target="view-info">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round">
        <circle cx="12" cy="12" r="10"/>
        <line x1="12" y1="16" x2="12" y2="12"/>
        <line x1="12" y1="8" x2="12.01" y2="8"/>
      </svg>
      <span>Info Pinout</span>
    </button>
  </div>
</nav>

<audio id="audioAlarm" src="/alarm.mp3" preload="auto" loop></audio>
<script src="/app.js" defer></script>
</body>
</html>

)rawliteral";
