#include "settings.h"
#include "theme.h"
#include <stdlib.h>

#define PERSIST_SETTINGS 100

// Clay sends select values as strings ("3"), sliders and toggles as
// ints — reading value->int32 on a cstring yields ASCII garbage, so
// every numeric key goes through this.
static int32_t tuple_int(const Tuple *t) {
  if (t->type == TUPLE_CSTRING) return atoi(t->value->cstring);
  return t->value->int32;
}

Settings g_settings;

static void settings_defaults(void) {
  g_settings = (Settings){
    .loc_lat_x100 = 3777,    // San Francisco
    .loc_lon_x100 = -12242,
    .show_dot = true,
    .map_style = 0, // MAP_STYLE_DOTS
    .wx_gps = true,
    .theme_mode = THEME_MODE_AUTO,
    .theme_manual = THEME_POSITIVE,
    .theme_day = THEME_POSITIVE,
    .theme_night = THEME_NEGATIVE,
    .day_start = 7,
    .night_start = 20,
    .show_seconds = true,
    .lead_zero = true,
    .use_24h = false,
    .date_order = 0, // DOW date MONTH
    .date_lead_zero = true,
    .slot_left = SLOT_WEATHER,
    .slot_right = SLOT_STEPS,
    .use_celsius = false,
    .step_goal = 10000,
  };
}

static void settings_sanitize(void) {
  if (g_settings.loc_lat_x100 < -9000) g_settings.loc_lat_x100 = -9000;
  if (g_settings.loc_lat_x100 > 9000) g_settings.loc_lat_x100 = 9000;
  if (g_settings.loc_lon_x100 < -18000) g_settings.loc_lon_x100 = -18000;
  if (g_settings.loc_lon_x100 > 18000) g_settings.loc_lon_x100 = 18000;
  if (g_settings.theme_mode > THEME_MODE_AUTO)
    g_settings.theme_mode = THEME_MODE_MANUAL;
  if (g_settings.theme_manual >= THEME_COUNT) g_settings.theme_manual = 0;
  if (g_settings.theme_day >= THEME_COUNT) g_settings.theme_day = 0;
  if (g_settings.theme_night >= THEME_COUNT) g_settings.theme_night = 0;
  g_settings.day_start %= 24;
  g_settings.night_start %= 24;
  if (g_settings.map_style > 1) g_settings.map_style = 0;
  if (g_settings.date_order > 5) g_settings.date_order = 0;
  if (g_settings.slot_left > SLOT_NONE) g_settings.slot_left = SLOT_WEATHER;
  if (g_settings.slot_right > SLOT_NONE) g_settings.slot_right = SLOT_STEPS;
  if (g_settings.step_goal < 100) g_settings.step_goal = 10000;
}

void settings_load(void) {
  settings_defaults();
  if (persist_exists(PERSIST_SETTINGS)) {
    Settings stored;
    int read = persist_read_data(PERSIST_SETTINGS, &stored, sizeof(stored));
    // A size mismatch means the struct changed shape across an app
    // update — fall back to defaults rather than reading garbage.
    if (read == (int)sizeof(stored)) g_settings = stored;
  }
  settings_sanitize();
}

void settings_save(void) {
  persist_write_data(PERSIST_SETTINGS, &g_settings, sizeof(g_settings));
}

bool settings_apply_message(DictionaryIterator *iter) {
  bool seen = false;
  Tuple *t;

  if ((t = dict_find(iter, MESSAGE_KEY_LOC_LAT))) {
    g_settings.loc_lat_x100 = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_LOC_LON))) {
    g_settings.loc_lon_x100 = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_SHOW_DOT))) {
    g_settings.show_dot = tuple_int(t) != 0;
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_MAP_STYLE))) {
    g_settings.map_style = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_WX_GPS))) {
    g_settings.wx_gps = tuple_int(t) != 0;
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_THEME_MODE))) {
    g_settings.theme_mode = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_THEME_SEL))) {
    g_settings.theme_manual = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_THEME_DAY))) {
    g_settings.theme_day = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_THEME_NIGHT))) {
    g_settings.theme_night = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_DAY_START))) {
    g_settings.day_start = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_NIGHT_START))) {
    g_settings.night_start = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_SHOW_SECONDS))) {
    g_settings.show_seconds = tuple_int(t) != 0;
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_LEAD_ZERO))) {
    g_settings.lead_zero = tuple_int(t) != 0;
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_USE_24H))) {
    g_settings.use_24h = tuple_int(t) != 0;
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_DATE_ORDER))) {
    g_settings.date_order = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_DATE_LZ))) {
    g_settings.date_lead_zero = tuple_int(t) != 0;
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_SLOT_LEFT))) {
    g_settings.slot_left = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_SLOT_RIGHT))) {
    g_settings.slot_right = tuple_int(t);
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_USE_CELSIUS))) {
    g_settings.use_celsius = tuple_int(t) != 0;
    seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_STEP_GOAL))) {
    g_settings.step_goal = tuple_int(t);
    seen = true;
  }

  if (seen) {
    settings_sanitize();
    settings_save();
  }
  return seen;
}
