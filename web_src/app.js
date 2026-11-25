/**
 * Rumah Joglo - Smart Home IoT Controller & Hardware Abstraction Layer
 * Yogyakarta Keraton Minimalism Edition
 */

(() => {
  'use strict';

  // Hardware Abstraction Layer Configuration
  const CONFIG = {
    USE_SIMULATOR: true,
    ESP32_BASE_URL: window.location.origin || "http://127.0.0.1:8080",
    POLL_INTERVAL_MS: 250,
    ULTRASONIC_TRIGGER_CM: 10
  };

  // State Management
  let lastData = null;
  let isCountdownActive = false;
  let countdownTimer = null;
  let isAudioUnlocked = false;

  const $ = id => document.getElementById(id);

  // SVG Icons for Door States
  const SVG_DOOR_CLOSED = `
    <path d="M18 20V6a2 2 0 0 0-2-2H8a2 2 0 0 0-2 2v14"/>
    <path d="M2 20h20"/>
    <circle cx="14" cy="12" r="1" fill="currentColor"/>
  `;

  const SVG_DOOR_OPEN = `
    <path d="M14 4h4a2 2 0 0 1 2 2v14"/>
    <path d="M2 20h20"/>
    <path d="M6 20V6a2 2 0 0 1 2-2l6-2v18"/>
    <circle cx="10" cy="12" r="1" fill="currentColor"/>
  `;

  // Network Fetch with Timeout
  async function fetchEndpoint(path) {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 3500);
    try {
      const url = `${CONFIG.ESP32_BASE_URL}${path}`;
      const res = await fetch(url, { cache: 'no-store', signal: controller.signal });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      return await res.json();
    } finally {
      clearTimeout(timeout);
    }
  }

  // Polling Loop
  async function pollStatus() {
    try {
      const data = await fetchEndpoint('/api/status');
      renderDashboard(data);
    } catch (err) {
      const oledEl = $('oledStatusText');
      if (oledEl) {
        oledEl.textContent = 'Koneksi terputus \u00B7 Menunggu ESP32...';
        oledEl.classList.add('is-offline');
      }
    }
    setTimeout(pollStatus, CONFIG.POLL_INTERVAL_MS);
  }

  // Render Dashboard
  function renderDashboard(d) {
    lastData = d;
    const mode = d.doorTriggerMode;
    const isEvacuation = Boolean(d.flame);

    const oledEl = $('oledStatusText');
    if (oledEl) {
      oledEl.classList.remove('is-offline');
      if (d.oled) {
        const addr = d.oledAddress && d.oledAddress !== 'none' ? ` (${d.oledAddress})` : '';
        oledEl.textContent = d.flame
          ? `OLED: Peringatan Api${addr}`
          : `OLED: Aktif${addr}`;
      } else {
        oledEl.textContent = 'OLED: Offline';
      }
    }

    // 1. Pintu (Door Card)
    const doorBadge = $('doorIconBadge');
    const doorSvg = $('doorIconSvg');
    const statusText = $('doorStatusText');
    const subText = $('doorSubText');
    const btnAuto = $('btnDoorAuto');
    const btnAutoLabel = $('btnDoorAutoLabel');
    const btnClose = $('btnDoorClose');

    const isOpen = d.door !== 'TERTUTUP';

    if (isOpen) {
      doorSvg.innerHTML = SVG_DOOR_OPEN;
      doorBadge.classList.remove('badge-terracotta');
      doorBadge.classList.add('badge-celadon');
    } else {
      doorSvg.innerHTML = SVG_DOOR_CLOSED;
      doorBadge.classList.remove('badge-celadon');
      doorBadge.classList.add('badge-terracotta');
    }

    if (!isCountdownActive) {
      if (isEvacuation) {
        statusText.textContent = 'Evakuasi Darurat';
        subText.textContent = 'Bahaya Api';
        btnClose.disabled = true;
      } else if (d.door === 'AUTO_CYCLE') {
        statusText.textContent = 'Terbuka (Otomatis)';
        subText.textContent = 'Otomatis';
        btnClose.disabled = false;
      } else if (isOpen) {
        statusText.textContent = 'Terbuka';
        subText.textContent = 'Terbuka Terus';
        btnClose.disabled = false;
      } else {
        statusText.textContent = 'Tertutup';
        subText.textContent = 'Standby';
        btnClose.disabled = false;
        btnAuto.disabled = false;
        btnAutoLabel.textContent = 'Buka 2 detik';
      }
    }

    // 2. Api (Flame Card & Flow 2 Animation)
    const cardFlame = $('cardFlame');
    const flameBadge = $('flameBadge');
    const flameStatus = $('flameStatusText');
    const flameSub = $('flameSubText');
    const flameTelem = $('flameTelemetry');
    const flameBtn = $('btnFlameToggleLogic');
    const flameSettingDesc = $('settingFlamePolarityDesc');

    const conduit2A = $('conduit2A');
    const conduit2B = $('conduit2B');
    const flowNodeEvac = $('flowNodeEvac');

    if (d.flame) {
      cardFlame.classList.add('card-danger-active');
      flameBadge.className = 'icon-badge badge-danger';
      flameStatus.textContent = 'BAHAYA: API!';
      flameSub.textContent = 'Bahaya Api';
      if (conduit2A) conduit2A.classList.add('conduit-danger-active');
      if (conduit2B) conduit2B.classList.add('conduit-danger-active');
      if (flowNodeEvac) flowNodeEvac.classList.add('node-danger-active');
      playAlarmAudio(true);
    } else {
      cardFlame.classList.remove('card-danger-active');
      flameBadge.className = 'icon-badge badge-celadon';
      flameStatus.textContent = 'Aman';
      flameSub.textContent = 'Normal';
      if (conduit2A) conduit2A.classList.remove('conduit-danger-active');
      if (conduit2B) conduit2B.classList.remove('conduit-danger-active');
      if (flowNodeEvac) flowNodeEvac.classList.remove('node-danger-active');
      playAlarmAudio(false);
    }

    const flameModeStr = d.flameActiveHigh ? 'Active-HIGH' : 'Active-LOW';
    flameTelem.textContent = `D34: ${d.flameRaw} (${flameModeStr})`;
    flameBtn.textContent = d.flameActiveHigh ? 'Ganti: Active-LOW' : 'Ganti: Active-HIGH';
    if (flameSettingDesc) {
      flameSettingDesc.textContent = d.flameActiveHigh
        ? 'Active-HIGH (1: Api, 0: Aman)'
        : 'Active-LOW (0: Api, 1: Aman)';
    }

    // 3. Pemicu (Trigger Card & Flow 1 Animation)
    const triggerBadge = $('triggerBadge');
    const triggerMode = $('triggerModeText');
    const triggerSub = $('triggerStatusSub');
    const triggerDetail = $('triggerDetailText');
    const conduit1A = $('conduit1A');
    const conduit1B = $('conduit1B');
    const flowNodeServo = $('flowNodeServo');

    const threshold = (typeof d.maxDistance === 'number' && d.maxDistance > 0)
      ? d.maxDistance
      : CONFIG.ULTRASONIC_TRIGGER_CM;
    const isTriggered = (mode === 3 && d.distance > 0 && d.distance <= threshold);
    const flowUsThreshold = $('flowUsThreshold');
    if (flowUsThreshold) flowUsThreshold.textContent = `\u2264 ${threshold} cm`;

    triggerMode.textContent = 'Ultrasonik';
    const triggerPinInfo = $('triggerPinInfo');
    if (triggerPinInfo && typeof d.trigPin === 'number' && typeof d.echoPin === 'number') {
      triggerPinInfo.textContent = `D${d.trigPin} (Trig) \u00B7 D${d.echoPin} (Echo)`;
    }
    if (isTriggered) {
      triggerBadge.classList.add('ripple-active');
      triggerSub.textContent = 'Objek Terdeteksi';
      triggerSub.style.color = 'var(--accent-terracotta)';
      triggerDetail.textContent = `Terdeteksi ${d.distance} cm`;
      if (conduit1A) conduit1A.classList.add('conduit-triggered');
      if (conduit1B) conduit1B.classList.add('conduit-triggered');
      if (flowNodeServo) flowNodeServo.classList.add('node-triggered');
    } else {
      triggerBadge.classList.remove('ripple-active');
      triggerSub.textContent = 'Standby';
      triggerSub.style.color = 'var(--text-secondary)';
      triggerDetail.textContent = `Pemicu \u2264 ${threshold} cm`;
      if (conduit1A) conduit1A.classList.remove('conduit-triggered');
      if (conduit1B) conduit1B.classList.remove('conduit-triggered');
      if (flowNodeServo) flowNodeServo.classList.remove('node-triggered');
    }

    // 4. Jarak (Distance Card & 4-Segment Gauge)
    const distNum = $('distMetricNum');
    const distSub = $('distSubLabel');
    const s1 = $('seg1');
    const s2 = $('seg2');
    const s3 = $('seg3');
    const s4 = $('seg4');

    const dist = d.distance;
    distNum.textContent = dist > 0 ? dist : '--';

    // Reset gauge segments
    [s1, s2, s3, s4].forEach(el => {
      el.className = 'segment';
    });

    if (dist <= 0) {
      distSub.textContent = 'Di Luar Jangkauan';
    } else if (dist <= threshold) {
      s1.classList.add('active-1');
      distSub.textContent = 'Sangat Dekat';
    } else if (dist <= 35) {
      s1.classList.add('active-1');
      s2.classList.add('active-2');
      distSub.textContent = 'Mendekat';
    } else if (dist <= 65) {
      s1.classList.add('active-1');
      s2.classList.add('active-2');
      s3.classList.add('active-3');
      distSub.textContent = 'Sedang';
    } else {
      s1.classList.add('active-1');
      s2.classList.add('active-2');
      s3.classList.add('active-3');
      s4.classList.add('active-4');
      distSub.textContent = 'Standby';
    }

    // 5. Suhu Ruang & Kelembaban (DHT11 Card & Flow 3 Animation)
    const tempNum = $('tempMetricNum');
    const tempFill = $('tempProgressFill');
    const humText = $('humText');
    const conduit3A = $('conduit3A');
    const conduit3B = $('conduit3B');
    const flowNodeDhtOut = $('flowNodeDhtOut');

    if (d.tempValid && typeof d.temp === 'number') {
      tempNum.textContent = d.temp.toFixed(1);
      // Range 15C to 45C
      const percent = Math.min(100, Math.max(0, ((d.temp - 15) / 30) * 100));
      tempFill.style.width = `${percent}%`;
      humText.textContent = `Lembab ${d.humidity.toFixed(0)}%`;
      if (conduit3A) conduit3A.classList.add('conduit-triggered');
      if (conduit3B) conduit3B.classList.add('conduit-triggered');
      if (flowNodeDhtOut) flowNodeDhtOut.classList.add('node-triggered');
    } else {
      tempNum.textContent = '--.-';
      tempFill.style.width = '0%';
      humText.textContent = d.dhtStatus || 'Menunggu sensor...';
      if (conduit3A) conduit3A.classList.remove('conduit-triggered');
      if (conduit3B) conduit3B.classList.remove('conduit-triggered');
      if (flowNodeDhtOut) flowNodeDhtOut.classList.remove('node-triggered');
    }
  }

  // Door Control Actions
  $('btnDoorAuto').addEventListener('click', async () => {
    if (isCountdownActive) return;
    isCountdownActive = true;
    const btn = $('btnDoorAuto');
    const label = $('btnDoorAutoLabel');
    const statusText = $('doorStatusText');
    const subText = $('doorSubText');

    btn.disabled = true;
    statusText.textContent = 'Terbuka (2d...)';
    subText.textContent = 'Pintu Terbuka Otomatis';
    label.textContent = 'Menghitung 2s...';

    try {
      await fetchEndpoint('/api/door?action=auto');
    } catch (e) {}

    let remaining = 2;
    countdownTimer = setInterval(() => {
      remaining -= 1;
      if (remaining > 0) {
        statusText.textContent = `Terbuka (${remaining}d...)`;
        label.textContent = `Menghitung ${remaining}s...`;
      } else {
        clearInterval(countdownTimer);
        isCountdownActive = false;
        statusText.textContent = 'Menutup';
        subText.textContent = 'Kembali Tertutup Halus';
        label.textContent = 'Buka 2 detik';
        btn.disabled = false;
      }
    }, 1000);
  });

  $('btnDoorOpen').addEventListener('click', async () => {
    if (isCountdownActive) {
      clearInterval(countdownTimer);
      isCountdownActive = false;
    }
    try {
      const d = await fetchEndpoint('/api/door?action=open');
      renderDashboard(d);
    } catch (e) {}
  });

  $('btnDoorClose').addEventListener('click', async () => {
    if (isCountdownActive) {
      clearInterval(countdownTimer);
      isCountdownActive = false;
    }
    try {
      const d = await fetchEndpoint('/api/door?action=close');
      renderDashboard(d);
    } catch (e) {}
  });

  // Flame Logic Inversion
  async function toggleFlameLogic() {
    try {
      const d = await fetchEndpoint('/api/flame?toggle_logic=1');
      renderDashboard(d);
    } catch (e) {}
  }

  const btnFlameToggle = $('btnFlameToggleLogic');
  if (btnFlameToggle) btnFlameToggle.addEventListener('click', toggleFlameLogic);
  const btnSettingFlame = $('btnSettingFlameToggle');
  if (btnSettingFlame) btnSettingFlame.addEventListener('click', toggleFlameLogic);

  // Threshold Configuration
  const thresholdInput = $('settingThreshold');
  if (thresholdInput) {
    thresholdInput.addEventListener('change', e => {
      const val = parseInt(e.target.value, 10);
      if (!isNaN(val) && val >= 5 && val <= 50) {
        CONFIG.ULTRASONIC_TRIGGER_CM = val;
      }
    });
  }

  // Tab Navigation Handling
  const tabs = document.querySelectorAll('.nav-tab');
  const panels = document.querySelectorAll('.view-panel');

  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => {
        t.classList.remove('active');
        t.setAttribute('aria-selected', 'false');
      });
      panels.forEach(p => p.classList.remove('active'));

      tab.classList.add('active');
      tab.setAttribute('aria-selected', 'true');
      const targetId = tab.getAttribute('data-target');
      const targetPanel = $(targetId);
      if (targetPanel) {
        targetPanel.classList.add('active');
      }
      window.scrollTo({ top: 0, behavior: 'smooth' });
    });
  });

  // Audio Playback Handling
  const audio = $('audioAlarm');

  function unlockAudio() {
    if (!isAudioUnlocked && audio) {
      audio.play().then(() => {
        audio.pause();
        audio.currentTime = 0;
        isAudioUnlocked = true;
      }).catch(() => {});
    }
  }

  window.addEventListener('click', unlockAudio, { once: true });
  window.addEventListener('touchstart', unlockAudio, { once: true });

  function playAlarmAudio(shouldPlay) {
    if (!audio) return;
    if (shouldPlay) {
      audio.play().catch(() => {});
    } else {
      audio.pause();
      audio.currentTime = 0;
    }
  }

  const btnAlarmTest = $('btnAlarmTest');
  if (btnAlarmTest) {
    btnAlarmTest.addEventListener('click', () => {
      if (!audio) return;
      playAlarmAudio(true);
      setTimeout(() => playAlarmAudio(false), 3000);
    });
  }

  // Initialize Polling
  pollStatus();

})();
