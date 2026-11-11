#pragma once

void fanBegin();
void fanSet(bool on);
void fanToggle();
void fanSetPin(int pin);
void fanToggleLogic();
bool fanIsOn();
int fanPin();
bool fanIsActiveLow();
