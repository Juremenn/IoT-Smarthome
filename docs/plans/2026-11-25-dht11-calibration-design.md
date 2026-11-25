# Desain Kalibrasi & Stabilisasi Sensor DHT11 (Pin D27)

**Tanggal:** 2026-11-25  
**Proyek:** Smart Home Miniatur Rumah Joglo (ESP32)  
**Status:** Disetujui Pengguna

## 1. Perbaikan Masalah Pembacaan DHT11
- **Anti Self-Heating (`3000 ms` di Core 0)**: Interval pembacaan dinaikkan dari `2100 ms` ke `3000 ms` pada FreeRTOS `dhtTask` di Core 0 (zero blocking ke loop servo, ultrasonik ISR, dan WebServer di Core 1).
- **Validasi & Outlier Rejection**:
  - Menangani `isnan()` dan batas fisik sensor; mempertahankan nilai valid terakhir hingga `3x` gagal berturut-turut, lalu menandai status tidak valid jika `> 3x` gagal.
  - Menolak lonjakan tiba-tiba `|dT| > 3.0 C` atau `|dH| > 15%` per siklus baca (maks 2x penolakan beruntun agar perpindahan ruangan fisik tetap diterima pada bacaan ke-3 yang konsisten).
- **Engine Kalibrasi (2-Point Linear + Offset)**:
  - Konstanta kalibrasi terpisah rapi di `include/config.h` (`DHT_USE_TWO_POINT_CAL`, `DHT_RAW_LOW/HIGH`, `DHT_REF_LOW/HIGH`, `DHT_TEMP_OFFSET_DEFAULT = 0.0f`, `DHT_HUM_OFFSET_DEFAULT = 0.0f`).
  - Kelembapan dikompensasi offset lalu dibatasi `constrain(val, 0.0f, 100.0f)`.
- **Filter EMA ($\alpha = 0.2$)**:
  - Bacaan valid pertama langsung menginisialisasi EMA (tanpa merangkak dari 0 C), pembacaan berikutnya dihaluskan dengan `0.2 * cal + 0.8 * ema`.
- **NVS Persisten (`Preferences`) & Live Endpoint**:
  - Offset disimpan di NVS namespace `"dhtcal"` dan dapat disetel langsung melalui `GET /api/calibrate?temp_offset=-7.0&hum_offset=-25.0` atau direset lewat `GET /api/calibrate?reset=1`.
  - Nilai terkalibrasi yang sama persis dipakai seragam oleh Serial TUI (menampilkan `DHT11 OK (CAL)` saat aktif), OLED SSD1306, dan Web Dashboard.
