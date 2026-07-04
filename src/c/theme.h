#pragma once

#include <pebble.h>

// Theme palettes from the design's four LCD variants, quantized to
// Pebble's 64-color palette and saturated for the physical panel:
//
//   positive   tan substrate, pure black text/accent (pale tones wash out)
//   negative   black face / white ink / amber accent
//   mono       pure ink on paper, no accent
//   accent     black face / white ink / cyan accent
typedef struct {
  GColor bg;         // face substrate
  GColor ink;        // primary foreground
  GColor mute;       // secondary foreground (labels, date)
  GColor accent;     // AM/PM, seconds, home dot, step bar, low battery
  GColor ghost;      // unlit 7-segment shadow behind the digits
  GColor frame;      // box/frame lines
  GColor sun;        // subsolar sun marker — distinct from accent
  GColor land_day;   // map land cells in daylight
  GColor land_night; // map land cells in darkness
  GColor sea_day;    // map ocean dots in daylight
  GColor sea_night;  // map ocean dots in darkness
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
    .bg         = {GColorBrassARGB8},        // #AAAA55, warm LCD tan
    .ink        = {GColorBlackARGB8},
    .mute       = {GColorBlackARGB8},        // gray reads too pale on tan
    .accent     = {GColorBlackARGB8},        // ditto for orange seconds
    .ghost      = {GColorLightGrayARGB8},
    .frame      = {GColorBlackARGB8},
    .sun        = {GColorChromeYellowARGB8}, // #FFAA00
    .land_day   = {GColorIslamicGreenARGB8}, // #00AA00
    .land_night = {GColorDarkGreenARGB8},    // #005500
    .sea_day    = {GColorVividCeruleanARGB8},// #00AAFF
    .sea_night  = {GColorDukeBlueARGB8},     // #0000AA
  },
  // Dark faces run full-contrast: white ink and saturated accents —
  // gray-on-black washes out badly on the real panel.
  [THEME_NEGATIVE] = {
    .bg         = {GColorBlackARGB8},
    .ink        = {GColorWhiteARGB8},
    .mute       = {GColorLightGrayARGB8},
    .accent     = {GColorChromeYellowARGB8}, // #FFAA00, saturated amber
    .ghost      = {GColorDarkGrayARGB8},
    .frame      = {GColorWhiteARGB8},
    .sun        = {GColorYellowARGB8},       // #FFFF00
    .land_day   = {GColorGreenARGB8},        // #00FF00
    .land_night = {GColorDarkGreenARGB8},    // #005500
    .sea_day    = {GColorVividCeruleanARGB8},// #00AAFF
    .sea_night  = {GColorDukeBlueARGB8},     // #0000AA
  },
  [THEME_MONO] = {
    .bg         = {GColorWhiteARGB8},
    .ink        = {GColorBlackARGB8},
    .mute       = {GColorDarkGrayARGB8},
    .accent     = {GColorBlackARGB8},        // mono: accent == ink
    .ghost      = {GColorLightGrayARGB8},
    .frame      = {GColorBlackARGB8},
    .sun        = {GColorBlackARGB8},        // mono stays ink-only
    .land_day   = {GColorBlackARGB8},
    .land_night = {GColorBlackARGB8},
    .sea_day    = {GColorLightGrayARGB8},
    .sea_night  = {GColorDarkGrayARGB8},
  },
  [THEME_ACCENT] = {
    .bg         = {GColorBlackARGB8},
    .ink        = {GColorWhiteARGB8},
    .mute       = {GColorLightGrayARGB8},
    .accent     = {GColorCyanARGB8},         // #00FFFF, electric teal
    .ghost      = {GColorDarkGrayARGB8},
    .frame      = {GColorCyanARGB8},
    .sun        = {GColorYellowARGB8},       // #FFFF00
    .land_day   = {GColorGreenARGB8},        // #00FF00
    .land_night = {GColorDarkGreenARGB8},    // #005500
    .sea_day    = {GColorVividCeruleanARGB8},// #00AAFF
    .sea_night  = {GColorDukeBlueARGB8},     // #0000AA
  },
};

// The active theme is chosen at runtime from user settings — see
// active_theme() in main.c (manual pick, or day/night auto switching).
