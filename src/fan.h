#pragma once

// Kontrol relay kipas manual di GPIO D23 (mendukung Active-LOW dan Active-HIGH).
void fanBegin();
void fanSetState(bool on);
void fanToggle();
bool fanIsOn();
bool fanActiveLow();
void fanToggleLogic();
