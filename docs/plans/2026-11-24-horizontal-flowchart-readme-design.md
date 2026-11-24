# Desain Alur Horizontal Pipeline Flowchart (flowchart LR) pada README.md

Tanggal: 2026-11-24  
Status: Disetujui  
Target: [README.md](file:///C:/Users/Nirvana/Documents/Script/IoT-Smarthome/README.md)

---

## 1. Latar Belakang & Masalah
Diagram sistem utama pada `README.md` sebelumnya menggunakan konfigurasi Mermaid `flowchart TD` (Top-to-Down). Pada tampilan GitHub Desktop, layout ini merender 6 subgraph secara vertikal memanjang ke bawah, menghasilkan bentuk pita sempit yang sangat panjang dan sulit dibaca tanpa melakukan scrolling vertikal yang berlebihan.

## 2. Tujuan & Sasaran Desain
1. Mengubah orientasi diagram dari `flowchart TD` menjadi `flowchart LR` (Left-to-Right) agar menyamping horizontal (landscape widescreen).
2. Membagi alur sistem menjadi 6 kolom tahapan horizontal:
   - `SG_BOOT`: Boot & Hardware Initialization
   - `SG_NET`: Wi-Fi AP & HTTP Server Handling
   - `SG_ACQ`: Main Loop & Sensor Acquisition
   - `SG_LOGIC`: Priority Engine & Decision Logic
   - `SG_ACT`: Actuators (Servo, Fan) & Display (OLED)
   - `SG_SYNC`: Loop Sync, Serial Telemetry & Watchdog Guard
3. Mempertahankan kepatuhan simbol standar ISO-5807:
   - Terminator kapsul (`TERM_`)
   - Preparation heksagon (`PREP_`)
   - Input jajaran genjang biru (`IO_`)
   - Output jajaran genjang oranye (`IO_`)
   - Process persegi abu-abu (`PROC_`)
   - Decision belah ketupat kuning (`DEC_`)
   - Subroutine kotak garis ganda ungu (`SUB_`)
   - Emergency kotak/panah merah tebal (`==>`)
   - Connector lingkaran (`CONN_`)
4. Mengoptimalkan teks node agar ringkas, padat, dan tidak membengkak tinggi dalam kolom horizontal.

---

## 3. Struktur Spesifikasi Mermaid (flowchart LR)

```mermaid
flowchart LR
    %% Pengaturan Tema dan Styling Bersih
    classDef terminator fill:#DCFCE7,stroke:#16A34A,stroke-width:2.5px,color:#0F172A,font-weight:bold
    classDef preparation fill:#E0E7FF,stroke:#4F46E5,stroke-width:2px,color:#0F172A
    classDef io_input fill:#E0F2FE,stroke:#0284C7,stroke-width:2px,color:#0F172A
    classDef io_output fill:#FFEDD5,stroke:#EA580C,stroke-width:2px,color:#0F172A
    classDef process fill:#F8FAFC,stroke:#475569,stroke-width:1.8px,color:#0F172A
    classDef decision fill:#FEF9C3,stroke:#CA8A04,stroke-width:2px,color:#0F172A,font-weight:bold
    classDef subroutine fill:#F3E8FF,stroke:#9333EA,stroke-width:2px,color:#0F172A
    classDef emergency fill:#FEE2E2,stroke:#DC2626,stroke-width:2.5px,color:#7F1D1D,font-weight:bold
    classDef connector fill:#F1F5F9,stroke:#334155,stroke-width:2px,color:#0F172A,font-weight:bold
```

### 3.1 Detail Kolom Pipeline
- **Kolom 1: Boot & Setup**
  - `TERM_START`: Power ON ESP32
  - `PREP_BOOT`: Inisialisasi Serial 115200, Servo D13/D12, Kipas D23, OLED D21/D22, Sensor D4/D5/D27/D34
  - `PREP_NET`: Inisialisasi AP `RumahJoglo-SmartHome` & WebServer Port 80
  - Lanjut ke konektor loop utama `CONN_LOOP_IN`.
- **Kolom 2: AP Network & Web Server**
  - `PROC_MILLIS`: Update waktu `now = millis()`
  - `SUB_WEB`: Eksekusi `webHandle()`
  - `DEC_HTTP`: Cek request masuk. Jika ya, proses REST API dan balas JSON status HTTP 200.
- **Kolom 3: Sensor Input Acquisition**
  - `DEC_DHT`: Baca DHT11 tiap 2100ms jika pintu tidak sedang bergerak.
  - `IO_FLAME`: Sampling sensor api D34 (filter 15ms).
  - `DEC_TRIGGER`: Sampling Ultrasonik HC-SR04 atau Touch0 D4 sesuai mode.
- **Kolom 4: Decision Engine & Prioritas**
  - `DEC_EVAC`: Prioritas 1 darurat api. Jika ya, picu evakuasi penuh.
  - `DEC_MAN`: Prioritas 2 buka manual web.
  - `DEC_AUTO`: Prioritas 3 & 4 pemicu otomatis buka pintu 5000ms & tahan 2000ms.
- **Kolom 5: Aktuator & Feedback Display**
  - `SUB_SERVO`: Update non-blocking servo 20ms (`easeInOutSine` buka, `easeSoftClose` tutup).
  - `IO_FAN`: Kendali relay kipas D23 Active-LOW.
  - `SUB_OLED`: Refresh OLED 50ms (animasi wayang atau peringatan bahaya api).
- **Kolom 6: Return Loop & Sync**
  - `IO_SERIAL`: Log periodik serial tiap 3000ms.
  - `PROC_GUARD`: `delay(1)` untuk stabilitas watchdog.
  - `CONN_LOOP_OUT`: Loop kembali ke `CONN_LOOP_IN` di Kolom 2.

---

## 4. Kriteria Keberhasilan
- File `README.md` berhasil diperbarui dan dirender oleh engine Mermaid GitHub tanpa error syntax.
- Diagram tampil proporsional melebar horizontal di monitor desktop tanpa tumpukan vertikal yang sempit.
- Seluruh teks dan logika sistem ESP32 tetap akurat 100% sesuai implementasi firmware.
