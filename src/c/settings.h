#pragma once

#include <pebble.h>

// User settings, configured from the phone via Clay (src/pkjs/config.js)
// and delivered over AppMessage. Persisted as one blob on the watch.

enum ThemeMode {
  THEME_MODE_MANUAL = 0,
  THEME_MODE_AUTO = 1, // day/night themes switch on the hours below
};

// What a footer box (slot) displays.
enum SlotKind {
  SLOT_WEATHER = 0,
  SLOT_STEPS = 1,
  SLOT_HEART = 2,
  SLOT_SECONDS = 3,
  SLOT_NONE = 4,
};

typedef struct {
  int32_t loc_lat_x100;  // home dot, centi-degrees (+N)
  int32_t loc_lon_x100;  // centi-degrees (+E)
  bool show_dot;         // draw the home dot on the map
  bool wx_gps;           // weather from phone GPS (else the coords above)
  uint8_t theme_mode;    // ThemeMode
  uint8_t theme_manual;  // ThemeId when manual
  uint8_t theme_day;     // ThemeId in auto mode, day hours
  uint8_t theme_night;   // ThemeId in auto mode, night hours
  uint8_t day_start;     // hour the day theme takes over (0-23)
  uint8_t night_start;   // hour the night theme takes over (0-23)
  bool show_seconds;     // seconds column beside the time (SECOND_UNIT)
  bool lead_zero;        // "02:32" vs "2:32"
  uint8_t slot_left;     // SlotKind for the bottom-left box
  uint8_t slot_right;    // SlotKind for the bottom-right box
  bool use_celsius;      // display unit; pkjs fetches accordingly
  int32_t step_goal;
} Settings;

extern Settings g_settings;

void settings_load(void);
void settings_save(void);

// Copies any settings keys present in the message into g_settings and
// persists. Returns true if a settings key was seen (vs pure weather).
bool settings_apply_message(DictionaryIterator *iter);
