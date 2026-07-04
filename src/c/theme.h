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
  GColor accent;  // single accent: AM/PM, home dot, step bar, low battery
  GColor ghost;   // unlit 7-segment shadow behind the digits
  GColor paper;   // map background
  GColor frame;   // box/frame lines
  GColor sun;     // subsolar sun marker — distinct from accent so the
                  // sun and the home dot never read as the same thing
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
    .accent = {GColorOrangeARGB8},       // #FF5500, saturated rust
    .ghost  = {GColorLightGrayARGB8},
    .paper  = {GColorBrassARGB8},
    .frame  = {GColorBlackARGB8},
    .sun    = {GColorChromeYellowARGB8}, // #FFAA00, pops on the tan face
  },
  // Dark faces run full-contrast: white ink and saturated accents —
  // gray-on-black washes out badly on the real panel.
  [THEME_NEGATIVE] = {
    .bg     = {GColorBlackARGB8},
    .ink    = {GColorWhiteARGB8},
    .mute   = {GColorLightGrayARGB8},
    .accent = {GColorChromeYellowARGB8}, // #FFAA00, saturated amber
    .ghost  = {GColorDarkGrayARGB8},
    .paper  = {GColorBlackARGB8},
    .frame  = {GColorWhiteARGB8},
    .sun    = {GColorYellowARGB8},       // #FFFF00, bright on black
  },
  [THEME_MONO] = {
    .bg     = {GColorWhiteARGB8},
    .ink    = {GColorBlackARGB8},
    .mute   = {GColorDarkGrayARGB8},
    .accent = {GColorBlackARGB8},        // mono: accent == ink
    .ghost  = {GColorLightGrayARGB8},
    .paper  = {GColorWhiteARGB8},
    .frame  = {GColorBlackARGB8},
    .sun    = {GColorBlackARGB8},        // mono stays ink-only
  },
  [THEME_ACCENT] = {
    .bg     = {GColorBlackARGB8},
    .ink    = {GColorWhiteARGB8},
    .mute   = {GColorLightGrayARGB8},
    .accent = {GColorCyanARGB8},         // #00FFFF, electric teal
    .ghost  = {GColorDarkGrayARGB8},
    .paper  = {GColorBlackARGB8},
    .frame  = {GColorCyanARGB8},
    .sun    = {GColorYellowARGB8},       // #FFFF00, bright on black
  },
};

// The active theme is chosen at runtime from user settings — see
// active_theme() in main.c (manual pick, or day/night auto switching).
