/**
 * Rumah Joglo - Smart Home Controller
 * Optimistic UI + Command Priority (Zero-Delay Control)
 */

(() => {
  'use strict';

  const CONFIG = {
    ESP32_BASE_URL: window.location.origin || 'http://127.0.0.1:8080',
    POLL_INTERVAL_MS: 400,
    ULTRASONIC_TRIGGER_CM: 10
  };

  let lastData = null;
  let pollTimer = null;
  let pollController = null;
  let stateVersion = 0;
  let isAudioUnlocked = false;

  const $ = id => document.getElementById(id);

  function hapticTap(ms = 12) {
    if (navigator && typeof navigator.vibrate === 'function') {
      try {
        navigator.vibrate(ms);
      } catch (_e) {}
    }
  }

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

  async function fetchJson(path, externalController) {
    const controller = externalController || new AbortController();
    const timeout = setTimeout(() => controller.abort(), 3000);
    try {
      const res = await fetch(`${CONFIG.ESP32_BASE_URL}${path}`, {
        cache: 'no-store',
        signal: controller.signal
      });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      return await res.json();
    } finally {
      clearTimeout(timeout);
    }
  }

  function showConnectionBanner(message, isError) {
    const banner = $('connBanner');
    const msg = $('connMessage');
    const retryBtn = $('btnRetryConn');
    if (!banner || !msg) return;

    banner.hidden = false;
    banner.classList.toggle('is-error', Boolean(isError));
    msg.textContent = message;
    if (retryBtn) retryBtn.hidden = !isError;
  }

  function hideConnectionBanner() {
    const banner = $('connBanner');
    if (banner) banner.hidden = true;
  }

  function schedulePoll(delayMs = CONFIG.POLL_INTERVAL_MS) {
    if (pollTimer) clearTimeout(pollTimer);
    pollTimer = setTimeout(pollStatus, delayMs);
  }

  function abortActivePoll() {
    if (pollTimer) {
      clearTimeout(pollTimer);
      pollTimer = null;
    }
    if (pollController) {
      pollController.abort();
      pollController = null;
    }
  }

  async function pollStatus() {
    pollTimer = null;
    const myVersion = stateVersion;
    const ctrl = new AbortController();
    pollController = ctrl;
    try {
      const data = await fetchJson('/api/status', ctrl);
      if (myVersion === stateVersion) {
        hideConnectionBanner();
        renderDashboard(data);
      }
    } catch (err) {
      if (err && err.name === 'AbortError') return;
      if (myVersion === stateVersion) {
        showConnectionBanner('Koneksi ESP32 terputus. Periksa Wi-Fi perangkat.', true);
      }
    } finally {
      if (pollController === ctrl) pollController = null;
      if (!pollTimer) schedulePoll();
    }
  }

  function applyDoorVisual(doorMode, isEvacuation) {
    const doorBadge = $('doorIconBadge');
    const doorSvg = $('doorIconSvg');
    const doorState = $('doorStatusText');
    const doorHint = $('doorHint');
    const btnAuto = $('btnDoorAuto');
    const btnOpen = $('btnDoorOpen');
    const btnClose = $('btnDoorClose');

    const isOpen = isEvacuation || doorMode !== 'TERTUTUP';
    doorSvg.innerHTML = isOpen ? SVG_DOOR_OPEN : SVG_DOOR_CLOSED;

    if (isEvacuation) {
      doorBadge.className = 'icon-box icon-danger';
      doorState.textContent = 'Evakuasi';
      doorHint.textContent = 'Darurat api';
      doorHint.className = 'state-tag is-danger';
      doorHint.hidden = false;
      btnAuto.disabled = true;
      btnOpen.disabled = true;
      btnClose.disabled = true;
      return;
    }

    btnAuto.disabled = false;
    btnOpen.disabled = false;
    btnClose.disabled = false;

    if (doorMode === 'AUTO_CYCLE') {
      doorBadge.className = 'icon-box icon-safe';
      doorState.textContent = 'Terbuka';
      doorHint.textContent = 'Otomatis';
      doorHint.className = 'state-tag';
      doorHint.hidden = false;
    } else if (isOpen) {
      doorBadge.className = 'icon-box icon-safe';
      doorState.textContent = 'Terbuka';
      doorHint.textContent = 'Manual';
      doorHint.className = 'state-tag';
      doorHint.hidden = false;
    } else {
      doorBadge.className = 'icon-box icon-terracotta';
      doorState.textContent = 'Tertutup';
      doorHint.hidden = true;
    }
  }

  function renderDashboard(d) {
    lastData = d;
    const isEvacuation = Boolean(d.flame);

    // 1. Pintu
    applyDoorVisual(d.door, isEvacuation);

    // 2. Sensor Api
    const cardFlame = $('cardFlame');
    const flameBadge = $('flameBadge');
    const flameStatus = $('flameStatusText');
    const flameBtn = $('btnFlameToggleLogic');

    if (d.flame) {
      dismissWelcome(true);
      endTour();
      cardFlame.classList.add('is-danger');
      flameBadge.className = 'icon-box icon-danger';
      flameStatus.textContent = 'Bahaya api!';
      flameStatus.className = 'flame-state state-danger';
      playAlarmAudio(true);
    } else {
      cardFlame.classList.remove('is-danger');
      flameBadge.className = 'icon-box icon-safe';
      flameStatus.textContent = 'Aman';
      flameStatus.className = 'flame-state state-safe';
      playAlarmAudio(false);
    }

    const currentLogic = d.flameActiveHigh ? 'Active-HIGH' : 'Active-LOW';
    const nextLogic = d.flameActiveHigh ? 'Active-LOW' : 'Active-HIGH';
    flameBtn.textContent = currentLogic;
    flameBtn.setAttribute('aria-label', `Logika sensor api: ${currentLogic}. Tekan untuk ganti ke ${nextLogic}`);
    flameBtn.title = `Ganti ke ${nextLogic}`;

    // 3. Jarak (Ultrasonik + Pemicu Pintu)
    const threshold = (typeof d.maxDistance === 'number' && d.maxDistance > 0)
      ? d.maxDistance
      : CONFIG.ULTRASONIC_TRIGGER_CM;
    const dist = d.distance;
    const distNum = $('distMetricNum');
    const distSub = $('distSubLabel');
    const segs = [$('seg1'), $('seg2'), $('seg3'), $('seg4')];

    distNum.textContent = dist > 0 ? dist : '--';
    segs.forEach(el => {
      if (el) el.className = 'segment';
    });

    const isTriggered = (d.doorTriggerMode === 3 && dist > 0 && dist <= threshold);
    distSub.classList.toggle('is-triggered', isTriggered);

    if (dist <= 0) {
      distSub.textContent = 'Di luar jangkauan';
    } else if (dist <= threshold) {
      segs[0].classList.add('active-1');
      distSub.textContent = isTriggered ? 'Pintu dipicu' : 'Sangat dekat';
    } else if (dist <= 35) {
      segs[0].classList.add('active-1');
      segs[1].classList.add('active-2');
      distSub.textContent = `Auto \u2264 ${threshold} cm`;
    } else if (dist <= 65) {
      segs[0].classList.add('active-1');
      segs[1].classList.add('active-2');
      segs[2].classList.add('active-3');
      distSub.textContent = `Auto \u2264 ${threshold} cm`;
    } else {
      segs[0].classList.add('active-1');
      segs[1].classList.add('active-2');
      segs[2].classList.add('active-3');
      segs[3].classList.add('active-4');
      distSub.textContent = `Auto \u2264 ${threshold} cm`;
    }

    const infoRuleDoor = $('infoRuleDoor');
    if (infoRuleDoor) {
      infoRuleDoor.textContent = `Jarak \u2264 ${threshold} cm buka halus 7 dtk, tahan 2 dtk`;
    }
    const infoUltrasonicPins = $('infoUltrasonicPins');
    if (infoUltrasonicPins && typeof d.trigPin === 'number' && typeof d.echoPin === 'number') {
      infoUltrasonicPins.textContent = `D${d.trigPin} Trig, D${d.echoPin} Echo`;
    }

    // 4. Suhu & Kelembaban (DHT11)
    const tempNum = $('tempMetricNum');
    const tempFill = $('tempProgressFill');
    const humText = $('humText');

    if (d.tempValid && typeof d.temp === 'number') {
      tempNum.textContent = d.temp.toFixed(1);
      const percent = Math.min(100, Math.max(0, ((d.temp - 15) / 30) * 100));
      tempFill.style.width = `${percent}%`;
      humText.textContent = `Lembab ${d.humidity.toFixed(0)}%`;
    } else {
      tempNum.textContent = '--.-';
      tempFill.style.width = '0%';
      humText.textContent = d.dhtStatus || 'Menunggu sensor...';
    }
  }

  // Kirim perintah prioritas: hentikan poll yang sedang berjalan, render optimistik, lalu sinkronkan
  async function sendCommand(path, optimisticFn, errorMessage) {
    abortActivePoll();
    const myVersion = ++stateVersion;
    if (optimisticFn) optimisticFn();

    try {
      const d = await fetchJson(path);
      if (myVersion === stateVersion) {
        hideConnectionBanner();
        renderDashboard(d);
      }
    } catch (_err) {
      if (myVersion === stateVersion) {
        showConnectionBanner(errorMessage, true);
      }
    } finally {
      if (myVersion === stateVersion) {
        schedulePoll(CONFIG.POLL_INTERVAL_MS);
      }
    }
  }

  // Door Controls (Zero-Delay Optimistic UI + Haptic Tap)
  $('btnDoorAuto').addEventListener('click', () => {
    if (lastData && lastData.flame) return;
    hapticTap();
    sendCommand(
      '/api/door?action=auto',
      () => applyDoorVisual('AUTO_CYCLE', false),
      'Gagal membuka pintu otomatis. Periksa koneksi ESP32.'
    );
  });

  $('btnDoorOpen').addEventListener('click', () => {
    if (lastData && lastData.flame) return;
    hapticTap();
    sendCommand(
      '/api/door?action=open',
      () => applyDoorVisual('TERBUKA', false),
      'Gagal membuka pintu. Periksa koneksi ESP32.'
    );
  });

  $('btnDoorClose').addEventListener('click', () => {
    if (lastData && lastData.flame) return;
    hapticTap();
    sendCommand(
      '/api/door?action=close',
      () => applyDoorVisual('TERTUTUP', false),
      'Gagal menutup pintu. Periksa koneksi ESP32.'
    );
  });

  // Flame Logic Toggle (Optimistic UI)
  $('btnFlameToggleLogic').addEventListener('click', () => {
    hapticTap();
    sendCommand(
      '/api/flame?toggle_logic=1',
      () => {
        const btn = $('btnFlameToggleLogic');
        const nextHigh = btn.textContent.trim() !== 'Active-HIGH';
        btn.textContent = nextHigh ? 'Active-HIGH' : 'Active-LOW';
      },
      'Gagal mengubah logika sensor api.'
    );
  });

  // Connection Retry
  const btnRetry = $('btnRetryConn');
  if (btnRetry) {
    btnRetry.addEventListener('click', () => {
      abortActivePoll();
      showConnectionBanner('Menghubungkan ulang...', false);
      pollStatus();
    });
  }

  // Tab Navigation (Click + Arrow Keys per WCAG APG)
  const tabs = Array.from(document.querySelectorAll('.nav-tab'));
  const panels = Array.from(document.querySelectorAll('.view-panel'));

  function activateTab(tab, moveFocus) {
    tabs.forEach(t => {
      const selected = t === tab;
      t.classList.toggle('active', selected);
      t.setAttribute('aria-selected', selected ? 'true' : 'false');
      t.tabIndex = selected ? 0 : -1;
    });
    panels.forEach(p => {
      p.hidden = p.id !== tab.getAttribute('data-target');
    });
    if (moveFocus) tab.focus();
    window.scrollTo({ top: 0, behavior: 'auto' });
  }

  tabs.forEach((tab, index) => {
    tab.tabIndex = tab.classList.contains('active') ? 0 : -1;
    tab.addEventListener('click', () => activateTab(tab, false));
    tab.addEventListener('keydown', e => {
      if (e.key === 'ArrowRight' || e.key === 'ArrowLeft') {
        e.preventDefault();
        const nextIndex = (index + (e.key === 'ArrowRight' ? 1 : -1) + tabs.length) % tabs.length;
        activateTab(tabs[nextIndex], true);
      }
    });
  });

  // Audio Alarm
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

  // Welcome Popup, Voice Over Jawa Tengah & Interactive Tour Guide (Kirana Dewi)
  const welcomeOverlay = $('welcomeOverlay');
  const btnWelcomeClose = $('btnWelcomeClose');
  const btnWelcomeVoice = $('btnWelcomeVoice');
  const welcomeVoiceLabel = $('welcomeVoiceLabel');
  const btnStartTour = $('btnStartTour');
  const btnWelcomeEnter = $('btnWelcomeEnter');
  const btnReplayWelcome = $('btnReplayWelcome');
  const audioWelcome = $('audioWelcome');

  const tourDock = $('tourDock');
  const tourStepBadge = $('tourStepBadge');
  const tourTitle = $('tourTitle');
  const tourText = $('tourText');
  const btnTourPrev = $('btnTourPrev');
  const btnTourNext = $('btnTourNext');
  const btnTourClose = $('btnTourClose');

  const WELCOME_SPEECH_TEXT = 'Halo, Sugeng Rawuh! Kenalin, aku Kirana Dewi, pemandu Rumah Joglo pintar ini. Monggo, mari aku pandu mengenal fitur-fiturnya.';
  let welcomeHideTimer = null;
  let isVoicePlaying = false;
  let tourIndex = -1;
  let activeSpotlightEl = null;

  const TOUR_STEPS = [
    {
      tab: 'tab-perangkat',
      target: null,
      title: 'Sugeng Rawuh di Rumah Joglo!',
      text: 'Halo, saya Kirana Dewi! Mari saya pandu mengenal fitur kendali pintar miniatur Rumah Joglo berbasis ESP32 ini.'
    },
    {
      tab: 'tab-perangkat',
      target: 'cardDoor',
      title: 'Kontrol Pintu Servo Halus',
      text: 'Atur pintu Joglo secara manual: pilih "Buka 2 detik" untuk siklus otomatis, "Buka terus", atau "Tutup" dengan gerak servo lembut.'
    },
    {
      tab: 'tab-perangkat',
      target: 'cardFlame',
      title: 'Sensor Api & Evakuasi Darurat',
      text: 'Jika api terdeteksi (15 ms), pintu otomatis terbuka penuh untuk evakuasi dan alarm berbunyi. Tombol kanan untuk ubah logika sensor.'
    },
    {
      tab: 'tab-perangkat',
      target: 'cardDist',
      title: 'Sensor Jarak Ultrasonik',
      text: 'Memantau jarak objek di depan pintu. Saat jarak 10 cm atau kurang, pintu otomatis membuka dan menahan 2 detik.'
    },
    {
      tab: 'tab-perangkat',
      target: 'cardTemp',
      title: 'Pemantauan Suhu & Kelembaban',
      text: 'Menampilkan suhu ruangan (°C) dan tingkat kelembaban (%) secara langsung dari sensor DHT11 di dalam miniatur.'
    },
    {
      tab: 'tab-info',
      target: 'view-info',
      title: 'Informasi Otomasi & Pinout ESP32',
      text: 'Di tab Pinout ini kamu bisa melihat ringkasan aturan otomasi serta daftar pin GPIO ESP32 dan nama Wi-Fi AP. Selamat mencoba!'
    }
  ];

  function setVoiceUi(playing, customLabel) {
    isVoicePlaying = Boolean(playing);
    if (!btnWelcomeVoice || !welcomeVoiceLabel) return;
    btnWelcomeVoice.classList.toggle('is-speaking', isVoicePlaying);
    if (customLabel) {
      welcomeVoiceLabel.textContent = customLabel;
    } else {
      welcomeVoiceLabel.textContent = isVoicePlaying ? 'Memutar Sambutan...' : 'Putar Suara Sambutan';
    }
  }

  function stopWelcomeVoice() {
    if (audioWelcome) {
      audioWelcome.pause();
      audioWelcome.currentTime = 0;
    }
    if ('speechSynthesis' in window) {
      try {
        window.speechSynthesis.cancel();
      } catch (_e) {}
    }
    setVoiceUi(false);
  }

  function speakFallback() {
    if (!('speechSynthesis' in window)) {
      setVoiceUi(false);
      return;
    }
    try {
      window.speechSynthesis.cancel();
      const utter = new SpeechSynthesisUtterance(WELCOME_SPEECH_TEXT);
      utter.lang = 'id-ID';
      utter.rate = 0.94;
      utter.pitch = 1.08;
      const voices = window.speechSynthesis.getVoices();
      const idFemale = voices.find(v => /jv|id/i.test(v.lang) && /female|gadis|siti|damayanti|google/i.test(v.name))
        || voices.find(v => /id/i.test(v.lang));
      if (idFemale) utter.voice = idFemale;
      utter.onstart = () => setVoiceUi(true);
      utter.onend = () => setVoiceUi(false);
      utter.onerror = () => setVoiceUi(false);
      window.speechSynthesis.speak(utter);
    } catch (_e) {
      setVoiceUi(false);
    }
  }

  function playWelcomeVoice(isUserGesture) {
    if (lastData && lastData.flame) return;
    stopWelcomeVoice();

    if (audioWelcome) {
      audioWelcome.currentTime = 0;
      const p = audioWelcome.play();
      if (p && typeof p.then === 'function') {
        p.then(() => {
          setVoiceUi(true);
        }).catch(() => {
          if (isUserGesture) {
            speakFallback();
          } else {
            setVoiceUi(false, 'Ketuk untuk Suara Sambutan');
          }
        });
      }
    } else if (isUserGesture) {
      speakFallback();
    }
  }

  if (audioWelcome) {
    audioWelcome.addEventListener('ended', () => setVoiceUi(false));
    audioWelcome.addEventListener('error', () => setVoiceUi(false));
  }

  function clearSpotlight() {
    if (activeSpotlightEl) {
      activeSpotlightEl.classList.remove('tour-spotlight');
      activeSpotlightEl = null;
    }
  }

  function renderTourStep(index) {
    if (!tourDock || index < 0 || index >= TOUR_STEPS.length) return;
    tourIndex = index;
    const step = TOUR_STEPS[index];

    if (step.tab) {
      const targetTab = $(step.tab);
      if (targetTab && !targetTab.classList.contains('active')) {
        activateTab(targetTab, false);
      }
    }

    clearSpotlight();
    if (step.target) {
      const el = $(step.target);
      if (el) {
        el.classList.add('tour-spotlight');
        activeSpotlightEl = el;
        el.scrollIntoView({ behavior: 'smooth', block: 'center' });
      }
    }

    tourStepBadge.textContent = `${index + 1} / ${TOUR_STEPS.length}`;
    tourTitle.textContent = step.title;
    tourText.textContent = step.text;
    btnTourPrev.hidden = index === 0;
    btnTourNext.textContent = index === TOUR_STEPS.length - 1 ? 'Selesai' : 'Lanjut';
    tourDock.hidden = false;
    document.body.classList.add('tour-active');
  }

  function startTour() {
    if (lastData && lastData.flame) return;
    hapticTap();
    dismissWelcome(false);
    renderTourStep(0);
  }

  function endTour() {
    clearSpotlight();
    tourIndex = -1;
    document.body.classList.remove('tour-active');
    if (tourDock) tourDock.hidden = true;
  }

  function dismissWelcome(immediate) {
    stopWelcomeVoice();
    if (!welcomeOverlay || welcomeOverlay.classList.contains('is-hidden')) return;
    if (welcomeHideTimer) {
      clearTimeout(welcomeHideTimer);
      welcomeHideTimer = null;
    }

    if (immediate) {
      welcomeOverlay.classList.add('is-hidden');
      welcomeOverlay.classList.remove('is-leaving');
      return;
    }

    welcomeOverlay.classList.add('is-leaving');
    welcomeHideTimer = setTimeout(() => {
      welcomeOverlay.classList.add('is-hidden');
      welcomeHideTimer = null;
    }, 360);
  }

  function showWelcome() {
    if (!welcomeOverlay) return;
    endTour();
    if (welcomeHideTimer) {
      clearTimeout(welcomeHideTimer);
      welcomeHideTimer = null;
    }
    welcomeOverlay.classList.remove('is-hidden', 'is-leaving');
    playWelcomeVoice(false);
  }

  if (welcomeOverlay) {
    showWelcome();

    if (btnWelcomeVoice) {
      btnWelcomeVoice.addEventListener('click', () => {
        unlockAudio();
        if (isVoicePlaying) {
          stopWelcomeVoice();
        } else {
          playWelcomeVoice(true);
        }
      });
    }

    if (btnStartTour) {
      btnStartTour.addEventListener('click', () => {
        unlockAudio();
        startTour();
      });
    }

    if (btnWelcomeEnter) {
      btnWelcomeEnter.addEventListener('click', () => {
        unlockAudio();
        dismissWelcome(false);
      });
    }

    if (btnWelcomeClose) {
      btnWelcomeClose.addEventListener('click', () => {
        unlockAudio();
        dismissWelcome(false);
      });
    }

    if (btnReplayWelcome) {
      btnReplayWelcome.addEventListener('click', () => {
        unlockAudio();
        if (tourDock && !tourDock.hidden) {
          endTour();
        } else {
          startTour();
        }
      });
    }

    welcomeOverlay.addEventListener('click', e => {
      if (e.target === welcomeOverlay) {
        unlockAudio();
        dismissWelcome(false);
      }
    });

    if (btnTourPrev) {
      btnTourPrev.addEventListener('click', () => {
        hapticTap();
        if (tourIndex > 0) renderTourStep(tourIndex - 1);
      });
    }

    if (btnTourNext) {
      btnTourNext.addEventListener('click', () => {
        hapticTap();
        if (tourIndex < TOUR_STEPS.length - 1) {
          renderTourStep(tourIndex + 1);
        } else {
          endTour();
          const firstTab = $('tab-perangkat');
          if (firstTab) activateTab(firstTab, false);
        }
      });
    }

    if (btnTourClose) {
      btnTourClose.addEventListener('click', () => {
        hapticTap();
        endTour();
      });
    }

    // Touch swipe gesture on tour card for mobile navigation
    if (tourDock) {
      let touchStartX = 0;
      tourDock.addEventListener('touchstart', e => {
        if (e.touches && e.touches.length === 1) {
          touchStartX = e.touches[0].clientX;
        }
      }, { passive: true });
      tourDock.addEventListener('touchend', e => {
        if (!e.changedTouches || e.changedTouches.length !== 1) return;
        const dx = e.changedTouches[0].clientX - touchStartX;
        if (Math.abs(dx) < 48) return;
        hapticTap();
        if (dx < 0) {
          if (tourIndex < TOUR_STEPS.length - 1) renderTourStep(tourIndex + 1);
          else endTour();
        } else if (tourIndex > 0) {
          renderTourStep(tourIndex - 1);
        }
      }, { passive: true });
    }

    window.addEventListener('keydown', e => {
      if (e.key === 'Escape') {
        if (!welcomeOverlay.classList.contains('is-hidden')) {
          dismissWelcome(false);
        } else if (tourDock && !tourDock.hidden) {
          endTour();
        }
      }
    });
  }

  // Start Polling
  pollStatus();
})();
