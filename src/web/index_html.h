#pragma once
#include <pgmspace.h>

// Halaman dashboard. Disajikan di "/" oleh web_server.cpp.
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#F4F5F7">
<title>Rumah Joglo</title>
<link rel="icon" href="data:,">
<link rel="stylesheet" href="/style.css">
</head>
<body>
<header class="bar">
  <h1>Rumah Joglo</h1>
  <p class="conn" id="conn"><span id="connText">Menghubungkan</span></p>
</header>

<main class="main">
  <section class="view" id="v-dev" role="tabpanel" aria-labelledby="t-dev">
    <h2 class="sr">Perangkat</h2>
    <div class="grid">
      <div class="tile wide" id="tDoor">
        <div class="hit" role="group" aria-labelledby="doorTitle doorVal doorSub">
          <span class="t-title" id="doorTitle">Pintu</span>
          <span class="ic" aria-hidden="true">
            <svg viewBox="0 0 24 24"><path id="doorPath" d="M5 21V4a1 1 0 0 1 1-1h12a1 1 0 0 1 1 1v17M3 21h18M15 12h.01"/></svg>
          </span>
          <span class="t-val" id="doorVal">Memuat</span>
          <span class="t-sub" id="doorSub"></span>
          <span class="acts">
            <button type="button" class="btn" id="bAuto">Buka 2 detik</button>
            <button type="button" class="btn" id="bOpen">Buka terus</button>
            <button type="button" class="btn" id="bClose">Tutup</button>
          </span>
        </div>
      </div>

      <div class="tile" id="tMode">
        <button type="button" class="hit" id="modeHit" aria-haspopup="dialog">
          <span class="t-title">Pemicu</span>
          <span class="ic" aria-hidden="true">
            <svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M2 12h2M20 12h2"/></svg>
          </span>
          <span class="t-val" id="modeVal">Memuat</span>
          <span class="t-sub" id="modeSub"></span>
        </button>
      </div>

      <div class="tile" id="tFlame">
        <div class="hit" role="group" aria-labelledby="flameTitle flameVal flameSub">
          <span class="t-title" id="flameTitle">Api</span>
          <span class="ic" aria-hidden="true">
            <svg viewBox="0 0 24 24"><path d="M12 3c.8 3 4.5 5.2 4.5 9.8a4.5 4.5 0 0 1-9 0c0-2 .9-3.3 1.8-4.2.2 1.5 1 2.4 1.9 2.8 0-3.1-.4-5.8.8-8.4z"/></svg>
          </span>
          <span class="t-val" id="flameVal">Memuat</span>
          <span class="t-sub" id="flameSub"></span>
          <button type="button" class="btn alarm-btn" id="bAlarm" aria-label="Matikan alarm" hidden>Matikan</button>
        </div>
      </div>

      <div class="tile" id="tDist">
        <div class="hit" role="group" aria-labelledby="distTitle distNum distUnit distSub">
          <span class="t-title" id="distTitle">Jarak</span>
          <span class="num"><span id="distNum">--</span><small id="distUnit">cm</small></span>
          <span class="gauge" aria-hidden="true"><span class="fill" id="distFill"></span><span class="tick" id="distTick"></span></span>
          <span class="t-sub" id="distSub"></span>
        </div>
      </div>
    </div>
  </section>

  <section class="view" id="v-set" role="tabpanel" aria-labelledby="t-set" hidden>
    <h2 class="v-title">Pengaturan</h2>
    <ul class="list">
      <li><span><b>Logika api</b><small id="sFlame">--</small></span><button type="button" class="btn" id="bFlame">Balik</button></li>
      <li><span><b>Pin ultrasonik</b><small id="sUs">--</small></span><button type="button" class="btn" id="bSwap">Tukar</button></li>
      <li><span><b>Cari pin</b><small>HC-SR04</small></span><button type="button" class="btn" id="bScan">Pindai</button></li>
      <li><span><b>Suara alarm</b><small>Saat ada api</small></span><button type="button" class="btn" id="bAlarmTest">Tes</button></li>
    </ul>
  </section>

  <section class="view" id="v-info" role="tabpanel" aria-labelledby="t-info" hidden>
    <h2 class="v-title">Info</h2>
    <dl class="list">
      <div><dt>Alamat IP</dt><dd id="iIp">--</dd></div>
      <div><dt>Layar OLED</dt><dd id="iOled">--</dd></div>
      <div><dt>Menyala</dt><dd id="iUp">--</dd></div>
      <div><dt>Servo pintu</dt><dd>D13</dd></div>
      <div><dt>Sensor api</dt><dd id="iFlame">--</dd></div>
      <div><dt>Sentuh D4</dt><dd id="iTouch">--</dd></div>
      <div><dt>Echo terakhir</dt><dd id="iEcho">--</dd></div>
      <div><dt>Batas jarak</dt><dd id="iMax">--</dd></div>
    </dl>
  </section>
</main>

<p class="toast" id="toast" role="status" aria-live="polite"></p>

<nav class="tabs" role="tablist" aria-label="Menu">
  <button type="button" role="tab" id="t-dev" aria-controls="v-dev" aria-selected="true">
    <svg viewBox="0 0 24 24" aria-hidden="true"><rect x="4" y="4" width="7" height="7" rx="1.5"/><rect x="13" y="4" width="7" height="7" rx="1.5"/><rect x="4" y="13" width="7" height="7" rx="1.5"/><rect x="13" y="13" width="7" height="7" rx="1.5"/></svg>
    <span>Perangkat</span>
  </button>
  <button type="button" role="tab" id="t-set" aria-controls="v-set" aria-selected="false" tabindex="-1">
    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 7h10M18 7h2M4 17h2M10 17h10"/><circle cx="16" cy="7" r="2"/><circle cx="8" cy="17" r="2"/></svg>
    <span>Pengaturan</span>
  </button>
  <button type="button" role="tab" id="t-info" aria-controls="v-info" aria-selected="false" tabindex="-1">
    <svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="9"/><path d="M12 11v5M12 8h.01"/></svg>
    <span>Info</span>
  </button>
</nav>

<dialog class="sheet" id="sheetMode" aria-labelledby="smTitle">
  <h2 id="smTitle">Pemicu pintu</h2>
  <div class="opts" role="radiogroup" aria-labelledby="smTitle">
    <label class="row"><span>Sentuh (D4)</span><input type="radio" name="mode" value="1"></label>
    <label class="row"><span>Sensor api</span><input type="radio" name="mode" value="2"></label>
    <label class="row"><span>Ultrasonik</span><input type="radio" name="mode" value="3"></label>
    <label class="row"><span>Manual</span><input type="radio" name="mode" value="0"></label>
  </div>
  <button type="button" class="row cancel" data-close>Batal</button>
</dialog>

<script src="/app.js" defer></script>
</body>
</html>
)rawliteral";
