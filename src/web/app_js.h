#pragma once
#include <pgmspace.h>

// Logika dashboard. Disajikan di "/app.js".
const char APP_JS[] PROGMEM = R"rawliteral(
(() => {
'use strict';
const POLL_MS = 600, FAIL_LIMIT = 3, TIMEOUT_MS = 3000, SCALE_CM = 40;
const DOOR_CLOSE_MS = 7000; // sama dengan DOOR_CLOSE_MS di include/config.h
const DOOR_CLOSED = 'M5 21V4a1 1 0 0 1 1-1h12a1 1 0 0 1 1 1v17M3 21h18M15 12h.01';
const DOOR_OPEN = 'M14 4h4a1 1 0 0 1 1 1v16M3 21h18M5 21V5.5L14 3v18M11 12h.01';
const MODE_NAME = { 0: 'Manual', 1: 'Sentuh', 2: 'Sensor api', 3: 'Ultrasonik' };
const $ = id => document.getElementById(id);

/* Tulis ke DOM hanya bila nilainya berubah */
const memo = new Map();
const put = (k, v, fn) => { if (memo.get(k) !== v) { memo.set(k, v); fn(v); } };
const text = (id, v) => put(id + ':t', v, x => { $(id).textContent = x; });
const flag = (id, c, on) => put(id + ':' + c, on, x => { $(id).classList.toggle(c, x); });
const attr = (id, a, v) => put(id + ':' + a, v, x => { $(id).setAttribute(a, x); });

let last = null, fails = 0, online = false, wake = null, closingUntil = 0;
const busy = new Set();

/* Jaringan: satu loop polling, request baru setelah yang lama selesai */
async function get(url) {
  const ctl = new AbortController();
  const t = setTimeout(() => ctl.abort(), TIMEOUT_MS);
  try {
    const r = await fetch(url, { cache: 'no-store', signal: ctl.signal });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    return await r.json();
  } finally { clearTimeout(t); }
}
const sleep = ms => new Promise(res => { wake = res; setTimeout(res, ms); });

async function pollLoop() {
  for (;;) {
    try {
      const d = await get('/api/status');
      fails = 0;
      setOnline(true);
      render(d);
    } catch (e) {
      if (++fails >= FAIL_LIMIT) setOnline(false);
    }
    await sleep(POLL_MS);
  }
}

function setOnline(on) {
  online = on;
  flag('conn', 'ok', on);
  flag('conn', 'bad', !on);
  if (!on) text('connText', 'Terputus');
  put('body:off', !on, x => document.body.classList.toggle('offline', x));
  if (!on) ['doorSub', 'modeSub', 'flameSub', 'distSub', 'tempSub'].forEach(id => text(id, 'Offline'));
  sync();
}

const evacuating = d => !!d && d.doorTriggerMode === 2 && d.flame;
const CONTROLS = ['bAuto', 'bOpen', 'bClose', 'modeHit', 'bFlame', 'bSwap', 'bScan'];
function sync() {
  for (const id of CONTROLS) {
    let off = !online || !last || busy.has(id);
    if (id === 'bClose' && evacuating(last)) off = true;
    put(id + ':d', off, x => { $(id).disabled = x; });
  }
}

/* Toast konfirmasi */
let toastTimer;
function toast(msg) {
  const el = $('toast');
  el.textContent = msg;
  el.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.remove('show'), 2400);
}

async function act(id, url, ok, tileId) {
  busy.add(id);
  if (tileId) $(tileId).setAttribute('aria-busy', 'true');
  sync();
  try {
    const d = await get(url);
    if (d && 'door' in d) render(d);
    toast(typeof ok === 'function' ? ok(d) : ok);
  } catch (e) {
    toast('Gagal terkirim');
  } finally {
    busy.delete(id);
    if (tileId) $(tileId).removeAttribute('aria-busy');
    sync();
  }
}

function uptime(s) {
  const h = Math.floor(s / 3600), m = Math.floor(s % 3600 / 60);
  return h ? h + ' j ' + m + ' m' : m ? m + ' m ' + (s % 60) + ' d' : s + ' d';
}

/* Render data /api/status */
function render(d) {
  const prev = last;
  last = d;
  const mode = d.doorTriggerMode, evac = evacuating(d);

  // Koneksi: tampilkan alamat perangkat, bukan kata "Online"
  text('connText', d.ip);
  attr('conn', 'aria-label', 'Terhubung ke ' + d.ip);

  // Pintu
  const open = d.door !== 'TERTUTUP', auto = d.door === 'AUTO_CYCLE';
  // Firmware melapor TERTUTUP begitu pintu mulai menutup; UI menampilkan "Menutup" selama geraknya.
  if (!open && prev && prev.door !== 'TERTUTUP') closingUntil = Date.now() + DOOR_CLOSE_MS;
  if (open) closingUntil = 0;
  const closing = !open && Date.now() < closingUntil;
  const doorVal = evac ? 'Evakuasi' : auto ? 'Membuka' : open ? 'Terbuka' : closing ? 'Menutup' : 'Tertutup';
  flag('tDoor', 'on', open || closing);
  attr('doorPath', 'd', open || closing ? DOOR_OPEN : DOOR_CLOSED);
  text('doorVal', doorVal);
  flag('doorSub', 'alert', evac);
  text('doorSub', evac ? 'Terkunci' : auto ? 'Tutup otomatis' : open ? 'Buka terus' : '');
  text('bClose', evac ? 'Terkunci' : 'Tutup');

  // Pemicu
  const dist = d.distance, hit = mode === 3 && dist > 0 && dist <= d.maxDistance;
  let sub = 'Nonaktif', active = false;
  if (mode === 1) { active = d.touchDetected; sub = active ? 'Tersentuh' : 'Siap (' + d.touchRaw + ')'; }
  else if (mode === 2) { sub = d.flame ? 'Ada api' : 'Memantau'; }
  else if (mode === 3) { active = hit; sub = hit ? 'Terpicu' : 'Standby'; }
  flag('tMode', 'on', active);
  flag('modeSub', 'alert', mode === 2 && d.flame);
  text('modeVal', MODE_NAME[mode] || 'Manual');
  text('modeSub', sub);
  attr('modeHit', 'aria-label', 'Pemicu pintu, ' + (MODE_NAME[mode] || 'Manual') + ', ' + sub.toLowerCase() + '. Ganti');
  const r = document.querySelector('input[name=mode][value="' + mode + '"]');
  if (r && !r.checked) r.checked = true;

  // Api
  flag('tFlame', 'danger', d.flame);
  if (d.flame && !(prev && prev.flame)) {
    const t = $('tFlame'); t.classList.remove('pulse'); void t.offsetWidth; t.classList.add('pulse');
  }
  text('flameVal', d.flame ? 'Bahaya' : 'Aman');
  text('flameSub', 'D34: ' + d.flameRaw);
  alarmUpdate(d.flame);

  // Jarak
  const on3 = mode === 3;
  flag('tDist', 'on', hit);
  text('distNum', !on3 ? '--' : dist > 0 ? String(dist) : SCALE_CM + '+');
  text('distSub', !on3 ? 'Nonaktif' : hit ? 'Terpicu' : 'Standby');
  put('fill', on3 && dist > 0 ? Math.min(dist, SCALE_CM) : 0, x => { $('distFill').style.width = (x / SCALE_CM * 100) + '%'; });
  put('tick', d.maxDistance, x => { $('distTick').style.left = Math.min(100, x / SCALE_CM * 100) + '%'; });

  // Suhu & Kelembaban (DHT11)
  const hasTemp = d.tempValid && typeof d.temp === 'number';
  text('tempNum', hasTemp ? d.temp.toFixed(1) : '--');
  text('tempSub', hasTemp ? ('Lembab: ' + d.humidity.toFixed(0) + '%') : (d.dhtStatus || 'Menunggu sensor...'));
  put('tempFill', hasTemp ? Math.min(Math.max(d.temp, 0), 50) : 0, x => { $('tempFill').style.width = (x / 50 * 100) + '%'; });

  // Pengaturan
  text('sFlame', d.flameActiveHigh ? 'Active-HIGH' : 'Active-LOW');
  text('sUs', 'Trig D' + d.trigPin + ', Echo D' + d.echoPin);

  // Info
  text('iIp', d.ip);
  text('iOled', d.oled ? 'Online (' + d.oledAddress.toUpperCase().replace('0X', '0x') + ')' : 'Offline');
  text('iUp', uptime(d.uptime));
  text('iFlame', d.flameRaw + (d.flameRaw === 1 ? ' (HIGH)' : ' (LOW)'));
  text('iTouch', d.touchRaw + ' / ' + d.touchBaseline);
  text('iEcho', d.rawEcho + ' µs');
  text('iMax', '≤ ' + d.maxDistance + ' cm');
  text('iTemp', hasTemp ? (d.temp.toFixed(1) + ' °C') : (d.dhtStatus || 'Tidak terbaca'));
  text('iHum', hasTemp ? (d.humidity.toFixed(0) + ' %') : (d.dhtStatus || 'Tidak terbaca'));

  sync();
}

/* Bottom sheet */
function openSheet(id) { const s = $(id); if (!s.open) s.showModal(); }
document.querySelectorAll('dialog').forEach(s => {
  s.addEventListener('click', e => { if (e.target === s || e.target.hasAttribute('data-close')) s.close(); });
});

/* Aksi */
$('bAuto').addEventListener('click', () => act('bAuto', '/api/door?action=auto', 'Pintu dibuka 2 detik', 'tDoor'));
$('bOpen').addEventListener('click', () => act('bOpen', '/api/door?action=open', 'Pintu terbuka', 'tDoor'));
$('bClose').addEventListener('click', () => act('bClose', '/api/door?action=close', 'Pintu menutup', 'tDoor'));
$('modeHit').addEventListener('click', () => openSheet('sheetMode'));
document.querySelectorAll('input[name=mode]').forEach(r => r.addEventListener('change', () => {
  $('sheetMode').close();
  act('modeHit', '/api/door?action=set_mode&mode=' + r.value, 'Pemicu: ' + MODE_NAME[r.value], 'tMode');
}));
$('bFlame').addEventListener('click', () => act('bFlame', '/api/flame?toggle_logic=1', d => 'Logika api: ' + (d.flameActiveHigh ? 'Active-HIGH' : 'Active-LOW')));
$('bSwap').addEventListener('click', () => act('bSwap', '/api/ultrasonic?swap=1', d => 'Trig D' + d.trigPin + ', Echo D' + d.echoPin));
$('bScan').addEventListener('click', () => act('bScan', '/api/ultrasonic?scan=1', d => d.result));

/* Navigasi bawah (tab, panah kiri/kanan) */
const tabs = [...document.querySelectorAll('[role=tab]')];
function selectTab(tab, focus) {
  tabs.forEach(t => {
    const sel = t === tab;
    t.setAttribute('aria-selected', String(sel));
    t.tabIndex = sel ? 0 : -1;
    $(t.getAttribute('aria-controls')).hidden = !sel;
  });
  if (focus) tab.focus();
  window.scrollTo(0, 0);
}
tabs.forEach((t, i) => {
  t.addEventListener('click', () => selectTab(t));
  t.addEventListener('keydown', e => {
    const k = { ArrowRight: i + 1, ArrowLeft: i - 1, Home: 0, End: tabs.length - 1 }[e.key];
    if (k === undefined) return;
    e.preventDefault();
    selectTab(tabs[(k + tabs.length) % tabs.length], true);
  });
});

/* Alarm suara saat api terdeteksi.
   Browser baru boleh memutar suara setelah halaman disentuh, jadi sentuhan pertama
   dipakai untuk membuka kunci audio. Tombol di tile Api bisa mematikan bunyinya. */
const alarm = new Audio('/alarm.mp3');
alarm.loop = true;
alarm.preload = 'auto';
let alarmUnlocked = false, alarmMuted = false, alarmFire = false, testTimer = 0;

function unlockAudio() {
  if (alarmUnlocked) return;
  alarm.muted = true;
  alarm.play().then(() => {
    if (!alarmFire) { alarm.pause(); alarm.currentTime = 0; }
    alarm.muted = false;
    alarmUnlocked = true;
  }).catch(() => { alarm.muted = false; });
}
['pointerdown', 'keydown'].forEach(ev => document.addEventListener(ev, unlockAudio, true));

function alarmLabel() {
  text('bAlarm', alarm.paused ? 'Bunyikan' : 'Matikan');
  attr('bAlarm', 'aria-label', alarm.paused ? 'Bunyikan alarm' : 'Matikan alarm');
}
alarm.addEventListener('play', alarmLabel);
alarm.addEventListener('pause', alarmLabel);

function alarmUpdate(fire) {
  alarmFire = fire;
  put('bAlarm:h', fire, x => { $('bAlarm').hidden = !x; });
  if (fire && !alarmMuted) {
    if (alarm.paused) { alarm.loop = true; alarm.play().catch(alarmLabel); }
  } else if (!fire) {
    alarmMuted = false;
    if (!alarm.paused && !testTimer) { alarm.pause(); alarm.currentTime = 0; }
  }
  alarmLabel();
}

$('bAlarm').addEventListener('click', () => {
  if (alarm.paused) {
    alarmMuted = false;
    alarm.loop = true;
    alarm.play().catch(() => toast('Suara diblokir browser'));
  } else {
    alarmMuted = true;
    alarm.pause();
    toast('Alarm dimatikan');
  }
});

$('bAlarmTest').addEventListener('click', () => {
  if (alarmFire) return;
  clearTimeout(testTimer);
  alarm.currentTime = 0;
  alarm.loop = false;
  alarm.play().then(() => {
    testTimer = setTimeout(() => { testTimer = 0; if (!alarmFire) { alarm.pause(); alarm.currentTime = 0; } }, 4200);
  }).catch(() => toast('Suara diblokir browser'));
});

sync();
pollLoop();
})();
)rawliteral";
