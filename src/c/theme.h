#pragma once

#include <pebble.h>

// Theme palettes from the design's four LCD variants. The prototype's hex
// values don't all survive Pebble's 64-color (2-bit/channel) palette, so
// each entry is hand-picked to the nearest color that keeps the intent:
//
//   positive   tan substrate / dark ink / rust accent   (#c9c4ae -> Brass)
//   negative   dark face / warm ink / amber accent
//   mono       pure ink on paper, no accent
//   accent     dark face / teal accent
typedef struct {
  GColor bg;      // face substrate
  GColor ink;     // primary foreground
  GColor mute;    // secondary foreground (55%-ink in the prototype)
  GColor accent;  // single accent: AM/PM, sun marker, step bar, low battery
  GColor ghost;   // unlit 7-segment shadow behind the digits
  GColor paper;   // map background
  GColor frame;   // box/frame lines
} Theme;

typedef enum {
  THEME_POSITIVE = 0,
  THEME_NEGATIVE,
  THEME_MONO,
  THEME_ACCENT,
  THEME_COUNT,
} ThemeId;

static const Theme THEMES[THEME_COUNT] = {
  [THEME_POSITIVE] = {
    .bg     = {GColorBrassARGB8},        // #AAAA55, warm LCD tan
    .ink    = {GColorBlackARGB8},
    .mute   = {GColorDarkGrayARGB8},
    .accent = {GColorWindsorTanARGB8},   // #AA5500, rust
    .ghost  = {GColorLightGrayARGB8},
    .paper  = {GColorBrassARGB8},
    .frame  = {GColorBlackARGB8},
  },
  [THEME_NEGATIVE] = {
    .bg     = {GColorBlackARGB8},
    .ink    = {GColorLightGrayARGB8},
    .mute   = {GColorDarkGrayARGB8},
    .accent = {GColorRajahARGB8},        // #FFAA55, amber
    .ghost  = {GColorDarkGrayARGB8},
    .paper  = {GColorBlackARGB8},
    .frame  = {GColorLightGrayARGB8},
  },
  [THEME_MONO] = {
    .bg     = {GColorWhiteARGB8},
    .ink    = {GColorBlackARGB8},
    .mute   = {GColorDarkGrayARGB8},
    .accent = {GColorBlackARGB8},        // mono: accent == ink
    .ghost  = {GColorLightGrayARGB8},
    .paper  = {GColorWhiteARGB8},
    .frame  = {GColorBlackARGB8},
  },
  [THEME_ACCENT] = {
    .bg     = {GColorBlackARGB8},
    .ink    = {GColorLightGrayARGB8},
    .mute   = {GColorDarkGrayARGB8},
    .accent = {GColorCadetBlueARGB8},    // #55AAAA, teal
    .ghost  = {GColorDarkGrayARGB8},
    .paper  = {GColorBlackARGB8},
    .frame  = {GColorCadetBlueARGB8},
  },
};

// Active theme for the build. Swap to any ThemeId above.
#define ACTIVE_THEME THEME_POSITIVE
