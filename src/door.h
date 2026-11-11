#pragma once
#include <stdint.h>

enum class DoorPhase : uint8_t { Closed, Opening, Open, Closing };

void doorBegin();
void doorUpdate(unsigned long now);

// Buka, tahan 2 detik, tutup. Dipanggil lagi saat pintu terbuka = waktu tahan diperpanjang,
// saat pintu sedang menutup = pintu berbalik membuka (seperti pintu otomatis di mal).
void doorStartCycle();
void doorOpenManual();
void doorCloseManual();  // diabaikan selama evakuasi api
void doorSetEvacuation(bool active);
void doorTestSweep();

DoorPhase doorPhase();
bool doorIsManualOpen();
bool doorIsAutoCycle();
bool doorIsEvacuating();
unsigned long doorHoldRemainingMs(unsigned long now);

// Nilai "door" di /api/status: TERTUTUP, TERBUKA, atau AUTO_CYCLE
const char *doorStatusText();
