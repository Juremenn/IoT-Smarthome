#pragma once
#include <pgmspace.h>
#include <stddef.h>
#include <stdint.h>

// Deklarasi ringan aset web PROGMEM agar web_server.cpp cepat di-compile ulang.
extern const uint8_t INDEX_HTML_GZ[] PROGMEM;
extern const size_t INDEX_HTML_GZ_LEN;

extern const uint8_t STYLE_CSS_GZ[] PROGMEM;
extern const size_t STYLE_CSS_GZ_LEN;

extern const uint8_t APP_JS_GZ[] PROGMEM;
extern const size_t APP_JS_GZ_LEN;

extern const uint8_t JOGLO_PNG[] PROGMEM;
extern const size_t JOGLO_PNG_LEN;

extern const uint8_t ALARM_MP3[] PROGMEM;
extern const size_t ALARM_MP3_LEN;
