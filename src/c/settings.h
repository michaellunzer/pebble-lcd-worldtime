#pragma once

#include <pebble.h>

// User settings, configured from the phone via Clay (src/pkjs/config.js)
// and delivered over AppMessage. Persisted as one blob on the watch.

enum ThemeMode {
  THEME_MODE_MANUAL = 0,
  THEME_MODE_AUTO = 1, // day/night themes switch on the hours below
};

typedef struct {
  char city_tag[4];      // 1-3 chars shown in the header box
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
  bool show_seconds;     // seconds column + blinking colon (SECOND_UNIT)
  bool show_weather;
  bool show_steps;
  bool use_celsius;      // display unit; pkjs fetches accordingly
  int32_t step_goal;
} Settings;

extern Settings g_settings;

void settings_load(void);
void settings_save(void);

// Copies any settings keys present in the message into g_settings and
// persists. Returns true if a settings key was seen (vs pure weather).
bool settings_apply_message(DictionaryIterator *iter);
