// LCD Worldtime — Casio-flavoured digital watchface for Pebble Time 2
// (emery, 200x228 color). Implements the Claude Design handoff spec:
//
//   header   18px  day-of-week · date · city tag · AM/PM (accent)
//   battery   6px  20-segment bar, outline = 100%, accent below 20%
//   time     76px  framed box, DSEG7 42px digits over ghost segments,
//                  blinking colon, accent 22px seconds with SEC label
//   map      46px  60x19 dot-matrix world, live day/night terminator
//   footer  ~50px  WEATHER (icon + °F + hi/lo) · STEPS (count + bar)
//
// The face is a clean solid substrate; the only texture is the hatch
// over unlit 7-segment ghosts.

#include <pebble.h>
#include "settings.h"
#include "theme.h"
#include "world_map.h"

#define SCREEN_W 200
#define SCREEN_H 228
#define PAD 6
#define INNER_X (PAD + 4)             // 10
#define INNER_W (SCREEN_W - 2 * INNER_X) // 180

#define HEADER_TOP (PAD + 4)          // 10
#define HEADER_H 18
#define BATT_TOP (HEADER_TOP + HEADER_H + 4) // 32
#define BATT_H 6
#define TIME_TOP (BATT_TOP + BATT_H + 4)     // 42
#define FOOTER_TOP 172
#define FOOTER_H (SCREEN_H - FOOTER_TOP - PAD) // 50
// The time box height hugs the digits (42px or 50px + padding) and the
// map absorbs whatever vertical space is left — see apply_layout().

#define BATTERY_LOW_PCT 20

// Weather condition codes shared with src/pkjs/index.js.
enum WeatherKind { WX_SUN = 0, WX_CLOUD = 1, WX_RAIN = 2, WX_MOON = 3 };

enum PersistKeys {
  PERSIST_WX_TEMP = 1,
  PERSIST_WX_KIND = 4,
};

static Window *s_window;
static Layer *s_bg_layer, *s_header_layer, *s_battery_layer, *s_time_layer,
             *s_map_layer, *s_footer_layer;

static GFont s_font_dseg50, s_font_dseg42, s_font_dseg28, s_font_dseg22,
             s_font_tech24, s_font_tech16, s_font_tech14, s_font_tech11,
             s_font_tech8;
static GBitmap *s_hatch;
static GColor s_hatch_palette[2];

static struct tm s_now;
static int s_battery_pct = 100;
static int s_steps = 0;
static int s_heart_bpm = 0;
// Weather defaults match the design's sample state until real data arrives.
static int s_wx_temp = 68;
static int s_wx_kind = WX_SUN;

static const Theme *T = &THEMES[THEME_POSITIVE];

static bool slot_has(uint8_t kind) {
  return g_settings.slot_left == kind || g_settings.slot_right == kind;
}

// Seconds in a footer box win over the inline column: the time box
// drops the column and the digits grow to 50px.
static bool inline_seconds(void) {
  return g_settings.show_seconds && !slot_has(SLOT_SECONDS);
}

static bool seconds_active(void) {
  return g_settings.show_seconds || slot_has(SLOT_SECONDS);
}

// Resolves the theme from settings: manual pick, or in auto mode the
// day/night theme by local hour (handles windows that wrap midnight).
static const Theme *active_theme(void) {
  uint8_t id = g_settings.theme_manual;
  if (g_settings.theme_mode == THEME_MODE_AUTO) {
    int h = s_now.tm_hour;
    int day = g_settings.day_start, night = g_settings.night_start;
    bool is_day = day <= night ? (h >= day && h < night)
                               : (h >= day || h < night);
    id = is_day ? g_settings.theme_day : g_settings.theme_night;
  }
  if (id >= THEME_COUNT) id = 0;
  return &THEMES[id];
}

// Swaps T if the resolved theme changed; refreshes the hatch palette
// (the bitmap blits through it) and redraws everything.
static void refresh_theme(void) {
  const Theme *next = active_theme();
  if (next == T) return;
  T = next;
  s_hatch_palette[1] = T->bg;
  if (!s_bg_layer) return; // before window_load — nothing to redraw yet
  layer_mark_dirty(s_bg_layer);
  layer_mark_dirty(s_header_layer);
  layer_mark_dirty(s_battery_layer);
  layer_mark_dirty(s_time_layer);
  layer_mark_dirty(s_map_layer);
  layer_mark_dirty(s_footer_layer);
}

// ---------------------------------------------------------------- helpers

static GSize text_size(const char *text, GFont font) {
  return graphics_text_layout_get_content_size(
      text, font, GRect(0, 0, SCREEN_W, SCREEN_H),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
}

static void draw_text(GContext *ctx, const char *text, GFont font,
                      GColor color, GRect box) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, font, box, GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
}

// Share Tech Mono has no bold cut — double-striking 1px apart reads as
// bold on the physical panel, where the single-weight strokes wash out.
static void draw_text_bold(GContext *ctx, const char *text, GFont font,
                           GColor color, GRect box) {
  draw_text(ctx, text, font, color, box);
  box.origin.x += 1;
  draw_text(ctx, text, font, color, box);
}

// ------------------------------------------------------------- background

// Unlit 7-segment "ghosts" get a checkerboard of bg-colored dots blitted
// over them — halves their intensity with an LCD-like hatch texture.
// One 8x8 tile, tiled across the target rect by the renderer.
static void hatch_create(void) {
  s_hatch_palette[0] = GColorClear;
  s_hatch_palette[1] = T->bg;
  s_hatch = gbitmap_create_blank_with_palette(
      GSize(8, 8), GBitmapFormat1BitPalette, s_hatch_palette, false);
  if (!s_hatch) return;
  uint8_t *data = gbitmap_get_data(s_hatch);
  uint16_t stride = gbitmap_get_bytes_per_row(s_hatch);
  for (int y = 0; y < 8; y++) {
    // 75% coverage: ghost pixels survive only 1-in-4 — unlit segments
    // read as a faint texture instead of competing with lit digits.
    data[y * stride] = (y % 2 == 0) ? 0xAA : 0xFF;
  }
}

static void hatch_rect(GContext *ctx, GRect r) {
  if (!s_hatch) return;
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_hatch, r);
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, T->bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Outer LCD frame line (inset 3).
  graphics_context_set_stroke_color(ctx, T->mute);
  graphics_draw_rect(ctx, grect_inset(bounds, GEdgeInsets(3)));
}

// ----------------------------------------------------------------- header

static void header_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  static const char *DOW[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  static const char *MON[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                              "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
  static const char *MONF[] = {"JANUARY", "FEBRUARY", "MARCH", "APRIL",
                               "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER",
                               "OCTOBER", "NOVEMBER", "DECEMBER"};

  const char *dow = DOW[s_now.tm_wday];
  const char *ampm = s_now.tm_hour >= 12 ? "PM" : "AM";

  // Right: AM/PM in accent at the far edge.
  GSize ampm_sz = text_size(ampm, s_font_tech14);
  int ampm_y = (b.size.h - ampm_sz.h) / 2 - 1;
  int ampm_x = b.size.w - ampm_sz.w - 1;
  draw_text_bold(ctx, ampm, s_font_tech14, T->accent,
                 GRect(ampm_x, ampm_y, ampm_sz.w + 2, ampm_sz.h + 2));

  int right_limit = ampm_x - 8;

  // Left: DOW (ink) + date (mute) at 16px, double-struck for weight,
  // with the full month name when it fits (SEPTEMBER etc. fall back
  // to 3 letters).
  GSize row_sz = text_size("00", s_font_tech16);
  int text_y = (b.size.h - row_sz.h) / 2 - 1;
  GSize dow_sz = text_size(dow, s_font_tech16);
  draw_text_bold(ctx, dow, s_font_tech16, T->ink,
                 GRect(0, text_y, dow_sz.w + 3, row_sz.h + 2));

  int date_x = dow_sz.w + 7;
  char date_buf[16];
  snprintf(date_buf, sizeof(date_buf), "%02d %s", s_now.tm_mday,
           MONF[s_now.tm_mon]);
  if (date_x + text_size(date_buf, s_font_tech16).w + 1 > right_limit) {
    snprintf(date_buf, sizeof(date_buf), "%02d %s", s_now.tm_mday,
             MON[s_now.tm_mon]);
  }
  draw_text_bold(ctx, date_buf, s_font_tech16, T->mute,
                 GRect(date_x, text_y, right_limit - date_x, row_sz.h + 2));
}

// ---------------------------------------------------------------- battery

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  // 10 blocks = one per 10%, each simply lit or empty — halfway fills
  // never read right on the panel.
  const int segs = 10;
  int filled = (s_battery_pct + 5) / 10;
  GColor fill = s_battery_pct <= BATTERY_LOW_PCT ? T->accent : T->ink;

  graphics_context_set_stroke_color(ctx, T->ink);
  graphics_draw_rect(ctx, b);

  int inner_w = b.size.w - 2;
  graphics_context_set_fill_color(ctx, fill);
  for (int i = 0; i < filled; i++) {
    int xs = 1 + i * inner_w / segs;
    int xe = 1 + (i + 1) * inner_w / segs;
    graphics_fill_rect(ctx, GRect(xs + 1, 2, xe - xs - 2, b.size.h - 4),
                       0, GCornerNone);
  }
}

// ------------------------------------------------------------------- time

static void time_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);

  // Framed box with a solid background.
  graphics_context_set_fill_color(ctx, T->bg);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, T->frame);
  graphics_draw_rect(ctx, b);

  int hour12 = s_now.tm_hour % 12;
  if (hour12 == 0) hour12 = 12;
  char hh[4], mm[4], ss[4];
  snprintf(hh, sizeof(hh), "%02d", hour12);
  snprintf(mm, sizeof(mm), "%02d", s_now.tm_min);
  snprintf(ss, sizeof(ss), "%02d", s_now.tm_sec);

  // With the seconds column inline the digits max out at 42px; once the
  // seconds move to a footer box (or off), HH:MM alone grows to 50px.
  // The box height hugs whichever size is active (see apply_layout).
  bool inline_sec = inline_seconds();
  GFont tf = inline_sec ? s_font_dseg42 : s_font_dseg50;

  // Measure once per draw; DSEG7 is fixed-width so "88" == any digits.
  GSize d2 = text_size("88", tf);                  // HH or MM block
  GSize colon = text_size(":", tf);
  GSize sec2 = text_size("88", s_font_dseg22);
  GSize sec_label = text_size("SEC", s_font_tech8);

  int total_w = d2.w * 2 + colon.w;
  if (inline_sec) total_w += 2 + sec2.w;
  int x = (b.size.w - total_w) / 2;
  if (x < 2) x = 2;
  int y = (b.size.h - d2.h) / 2;

  int sec_x = x + d2.w * 2 + colon.w + 2;
  int sec_y = y + d2.h - sec2.h - 2;

  // Ghost (unlit) segments first, then the hatch dims them all at once,
  // then the lit digits go on top untouched.
  draw_text(ctx, "88", tf, T->ghost, GRect(x, y, d2.w + 2, d2.h + 2));
  draw_text(ctx, ":", tf, T->ghost,
            GRect(x + d2.w, y, colon.w + 2, d2.h + 2));
  draw_text(ctx, "88", tf, T->ghost,
            GRect(x + d2.w + colon.w, y, d2.w + 2, d2.h + 2));
  if (inline_sec) {
    draw_text(ctx, "88", s_font_dseg22, T->ghost,
              GRect(sec_x, sec_y, sec2.w + 2, sec2.h + 2));
  }
  hatch_rect(ctx, GRect(x, y, total_w + 2, d2.h + 2));

  // Lit digits. Colon blinks at the design's 0.5 Hz cycle whenever
  // seconds tick somewhere (steady on minute-only wakeups).
  draw_text(ctx, hh, tf, T->ink, GRect(x, y, d2.w + 2, d2.h + 2));
  if (!seconds_active() || s_now.tm_sec % 2 == 0) {
    draw_text(ctx, ":", tf, T->ink,
              GRect(x + d2.w, y, colon.w + 2, d2.h + 2));
  }
  draw_text(ctx, mm, tf, T->ink,
            GRect(x + d2.w + colon.w, y, d2.w + 2, d2.h + 2));

  if (!inline_sec) return;

  // Seconds column: SEC label over accent digits, bottom-aligned with HH:MM.
  draw_text(ctx, "SEC", s_font_tech8, T->mute,
            GRect(sec_x, sec_y - sec_label.h - 2, sec_label.w + 2,
                  sec_label.h + 2));
  draw_text(ctx, ss, s_font_dseg22, T->accent,
            GRect(sec_x, sec_y, sec2.w + 2, sec2.h + 2));
}

// -------------------------------------------------------------------- map

static void map_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  time_t now = time(NULL);
  struct tm utc = *gmtime(&now);
  world_map_draw(ctx, b, &utc, T, g_settings.show_dot,
                 g_settings.loc_lat_x100, g_settings.loc_lon_x100);
}

// ----------------------------------------------------------------- footer

static void draw_weather_icon(GContext *ctx, GPoint c, int kind) {
  switch (kind) {
    case WX_SUN: {
      graphics_context_set_fill_color(ctx, T->accent);
      graphics_fill_circle(ctx, c, 4);
      graphics_context_set_stroke_color(ctx, T->ink);
      for (int i = 0; i < 8; i++) {
        int32_t a = i * TRIG_MAX_ANGLE / 8;
        int32_t dx = cos_lookup(a), dy = sin_lookup(a);
        graphics_draw_line(
            ctx,
            GPoint(c.x + dx * 6 / TRIG_MAX_RATIO, c.y + dy * 6 / TRIG_MAX_RATIO),
            GPoint(c.x + dx * 9 / TRIG_MAX_RATIO, c.y + dy * 9 / TRIG_MAX_RATIO));
      }
      break;
    }
    case WX_CLOUD:
    case WX_RAIN: {
      int cy = kind == WX_RAIN ? c.y - 3 : c.y;
      graphics_context_set_fill_color(ctx, T->ink);
      graphics_fill_circle(ctx, GPoint(c.x - 4, cy + 1), 4);
      graphics_fill_circle(ctx, GPoint(c.x + 1, cy - 2), 5);
      graphics_fill_circle(ctx, GPoint(c.x + 5, cy + 1), 4);
      graphics_fill_rect(ctx, GRect(c.x - 4, cy + 1, 10, 4), 0, GCornerNone);
      if (kind == WX_RAIN) {
        graphics_context_set_stroke_color(ctx, T->accent);
        for (int i = -1; i <= 1; i++) {
          graphics_draw_line(ctx, GPoint(c.x + i * 4, cy + 7),
                             GPoint(c.x + i * 4 - 1, cy + 10));
        }
      }
      break;
    }
    case WX_MOON: {
      graphics_context_set_fill_color(ctx, T->accent);
      graphics_fill_circle(ctx, c, 5);
      graphics_context_set_fill_color(ctx, T->bg);
      graphics_fill_circle(ctx, GPoint(c.x + 3, c.y - 2), 4);
      break;
    }
  }
}

// Each footer section is unlabeled — just the information, centered in
// `area` — so it works in a half column (both boxes on) or full width.
static void draw_weather_section(GContext *ctx, GRect area) {
  char temp_buf[16];
  snprintf(temp_buf, sizeof(temp_buf), "%d°", s_wx_temp);
  GSize temp_sz = text_size(temp_buf, s_font_tech24);
  int group_w = 18 + 6 + temp_sz.w;
  int gx = area.origin.x + (area.size.w - group_w) / 2;
  int gy = area.origin.y + (area.size.h - temp_sz.h) / 2;
  draw_weather_icon(ctx, GPoint(gx + 9, gy + temp_sz.h / 2), s_wx_kind);
  draw_text(ctx, temp_buf, s_font_tech24, T->ink,
            GRect(gx + 24, gy, temp_sz.w + 2, temp_sz.h + 2));
}

static void draw_steps_section(GContext *ctx, GRect area) {
  char steps_buf[16];
  if (s_steps >= 1000) {
    snprintf(steps_buf, sizeof(steps_buf), "%d,%03d", s_steps / 1000,
             s_steps % 1000);
  } else {
    snprintf(steps_buf, sizeof(steps_buf), "%d", s_steps);
  }
  GSize st_sz = text_size(steps_buf, s_font_tech24);
  int sy = area.origin.y + (area.size.h - st_sz.h - 6) / 2;
  draw_text(ctx, steps_buf, s_font_tech24, T->ink,
            GRect(area.origin.x + (area.size.w - st_sz.w) / 2, sy,
                  st_sz.w + 2, st_sz.h + 2));

  // Progress bar, 85% of the section width (capped so a full-width
  // solo section keeps the design's half-width proportions), 4px tall.
  int bar_w = area.size.w * 85 / 100;
  if (bar_w > 90) bar_w = 90;
  int bx = area.origin.x + (area.size.w - bar_w) / 2;
  int by = sy + st_sz.h + 2;
  graphics_context_set_stroke_color(ctx, T->frame);
  graphics_draw_rect(ctx, GRect(bx, by, bar_w, 4));
  int goal = g_settings.step_goal > 0 ? g_settings.step_goal : 1;
  int fill_w = s_steps >= goal ? bar_w - 2 : (bar_w - 2) * s_steps / goal;
  graphics_context_set_fill_color(ctx, T->accent);
  graphics_fill_rect(ctx, GRect(bx + 1, by + 1, fill_w, 2), 0, GCornerNone);
}

static void draw_heart_section(GContext *ctx, GRect area) {
  char buf[16];
  if (s_heart_bpm > 0) {
    snprintf(buf, sizeof(buf), "%d", s_heart_bpm);
  } else {
    strcpy(buf, "--");
  }
  GSize v_sz = text_size(buf, s_font_tech24);
  GSize u_sz = text_size("BPM", s_font_tech11);
  int vy = area.origin.y + (area.size.h - v_sz.h - u_sz.h - 1) / 2;
  draw_text(ctx, buf, s_font_tech24, T->ink,
            GRect(area.origin.x + (area.size.w - v_sz.w) / 2, vy,
                  v_sz.w + 2, v_sz.h + 2));
  draw_text(ctx, "BPM", s_font_tech11, T->accent,
            GRect(area.origin.x + (area.size.w - u_sz.w) / 2,
                  vy + v_sz.h + 1, u_sz.w + 2, u_sz.h + 2));
}

static void draw_seconds_section(GContext *ctx, GRect area) {
  char ss[4];
  snprintf(ss, sizeof(ss), "%02d", s_now.tm_sec);
  GSize sec2 = text_size("88", s_font_dseg28);
  int sx = area.origin.x + (area.size.w - sec2.w) / 2;
  int sy = area.origin.y + (area.size.h - sec2.h) / 2;
  draw_text(ctx, "88", s_font_dseg28, T->ghost,
            GRect(sx, sy, sec2.w + 2, sec2.h + 2));
  hatch_rect(ctx, GRect(sx, sy, sec2.w + 2, sec2.h + 2));
  draw_text(ctx, ss, s_font_dseg28, T->accent,
            GRect(sx, sy, sec2.w + 2, sec2.h + 2));
}

static void draw_slot(GContext *ctx, uint8_t kind, GRect area) {
  switch (kind) {
    case SLOT_WEATHER: draw_weather_section(ctx, area); break;
    case SLOT_STEPS:   draw_steps_section(ctx, area); break;
    case SLOT_HEART:   draw_heart_section(ctx, area); break;
    case SLOT_SECONDS: draw_seconds_section(ctx, area); break;
    default: break; // SLOT_NONE
  }
}

static void footer_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  uint8_t l = g_settings.slot_left, r = g_settings.slot_right;
  if (l == SLOT_NONE && r == SLOT_NONE) return;

  int half = b.size.w / 2;
  if (l != SLOT_NONE && r != SLOT_NONE) {
    graphics_context_set_stroke_color(ctx, T->mute);
    graphics_draw_line(ctx, GPoint(half, 2), GPoint(half, b.size.h - 2));
    draw_slot(ctx, l, GRect(0, 0, half, b.size.h));
    draw_slot(ctx, r, GRect(half, 0, half, b.size.h));
  } else {
    draw_slot(ctx, l != SLOT_NONE ? l : r, GRect(0, 0, b.size.w, b.size.h));
  }
}

// --------------------------------------------------------------- services

// The time box hugs its digits (42px inline-seconds / 50px without) and
// the map absorbs the leftover vertical space between it and the footer.
static void apply_layout(void) {
  GFont tf = inline_seconds() ? s_font_dseg42 : s_font_dseg50;
  int time_h = text_size("88", tf).h + 12;
  layer_set_frame(s_time_layer,
                  GRect(PAD, TIME_TOP, SCREEN_W - 2 * PAD, time_h));
  int map_top = TIME_TOP + time_h + 4;
  layer_set_frame(s_map_layer,
                  GRect(INNER_X, map_top, INNER_W, FOOTER_TOP - 4 - map_top));
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_now = *tick_time;
  layer_mark_dirty(s_time_layer);
  if (slot_has(SLOT_SECONDS)) layer_mark_dirty(s_footer_layer);
  if (units_changed & MINUTE_UNIT) {
    layer_mark_dirty(s_header_layer);
    layer_mark_dirty(s_map_layer);
    refresh_theme(); // auto mode may cross a day/night boundary
  }
}

// Any visible seconds need SECOND_UNIT; otherwise a once-a-minute wake
// is enough (and much kinder to the battery).
static void update_tick_subscription(void) {
  tick_timer_service_subscribe(
      seconds_active() ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
}

static void battery_handler(BatteryChargeState state) {
  s_battery_pct = state.charge_percent;
  layer_mark_dirty(s_battery_layer);
}

#if defined(PBL_HEALTH)
static void update_steps(void) {
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  if (health_service_metric_accessible(metric, start, end) &
      HealthServiceAccessibilityMaskAvailable) {
    s_steps = (int)health_service_sum_today(metric);
    layer_mark_dirty(s_footer_layer);
  }
}

static void update_heart(void) {
  time_t now = time(NULL);
  if (health_service_metric_accessible(HealthMetricHeartRateBPM, now, now) &
      HealthServiceAccessibilityMaskAvailable) {
    s_heart_bpm = (int)health_service_peek_current_value(
        HealthMetricHeartRateBPM);
    layer_mark_dirty(s_footer_layer);
  }
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventSignificantUpdate ||
      event == HealthEventMovementUpdate) {
    update_steps();
  }
  if (event == HealthEventHeartRateUpdate ||
      event == HealthEventSignificantUpdate) {
    update_heart();
  }
}
#endif

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t;
  bool wx_seen = false;
  if ((t = dict_find(iter, MESSAGE_KEY_TEMPERATURE))) {
    s_wx_temp = t->value->int32;
    wx_seen = true;
  }
  if ((t = dict_find(iter, MESSAGE_KEY_CONDITION))) s_wx_kind = t->value->int32;
  if (wx_seen) {
    persist_write_int(PERSIST_WX_TEMP, s_wx_temp);
    persist_write_int(PERSIST_WX_KIND, s_wx_kind);
    layer_mark_dirty(s_footer_layer);
  }

  // Settings pushed from the Clay config page.
  if (settings_apply_message(iter)) {
    update_tick_subscription();
    refresh_theme();
    apply_layout(); // seconds may have moved: time/map frames change
    layer_mark_dirty(s_header_layer);
    layer_mark_dirty(s_time_layer);
    layer_mark_dirty(s_map_layer);
    layer_mark_dirty(s_footer_layer);
  }
}

// ----------------------------------------------------------------- window

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);

  s_font_dseg50 = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_LCD_50));
  s_font_dseg42 = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_LCD_42));
  s_font_dseg28 = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_LCD_28));
  s_font_dseg22 = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_LCD_22));
  s_font_tech24 = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_TECH_24));
  s_font_tech16 = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_TECH_16));
  s_font_tech14 = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_TECH_14));
  s_font_tech11 = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_TECH_11));
  s_font_tech8 = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_TECH_8));

  hatch_create();

  s_bg_layer = layer_create(GRect(0, 0, SCREEN_W, SCREEN_H));
  layer_set_update_proc(s_bg_layer, bg_update_proc);
  layer_add_child(root, s_bg_layer);

  s_header_layer = layer_create(GRect(INNER_X, HEADER_TOP, INNER_W, HEADER_H));
  layer_set_update_proc(s_header_layer, header_update_proc);
  layer_add_child(root, s_header_layer);

  s_battery_layer = layer_create(GRect(INNER_X, BATT_TOP, INNER_W, BATT_H));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(root, s_battery_layer);

  // Full-bleed to the screen padding (188px vs the 180px content column)
  // to buy the extra width the one-line time + seconds layout needs.
  // Frames for the time and map layers are set by apply_layout().
  s_time_layer = layer_create(GRect(PAD, TIME_TOP, SCREEN_W - 2 * PAD, 10));
  layer_set_update_proc(s_time_layer, time_update_proc);
  layer_add_child(root, s_time_layer);

  s_map_layer = layer_create(GRect(INNER_X, TIME_TOP, INNER_W, 10));
  layer_set_update_proc(s_map_layer, map_update_proc);
  layer_add_child(root, s_map_layer);

  s_footer_layer = layer_create(GRect(INNER_X, FOOTER_TOP, INNER_W, FOOTER_H));
  layer_set_update_proc(s_footer_layer, footer_update_proc);
  layer_add_child(root, s_footer_layer);

  apply_layout();
}

static void window_unload(Window *window) {
  layer_destroy(s_bg_layer);
  layer_destroy(s_header_layer);
  layer_destroy(s_battery_layer);
  layer_destroy(s_time_layer);
  layer_destroy(s_map_layer);
  layer_destroy(s_footer_layer);
  fonts_unload_custom_font(s_font_dseg50);
  fonts_unload_custom_font(s_font_dseg42);
  fonts_unload_custom_font(s_font_dseg28);
  fonts_unload_custom_font(s_font_dseg22);
  fonts_unload_custom_font(s_font_tech24);
  fonts_unload_custom_font(s_font_tech16);
  fonts_unload_custom_font(s_font_tech14);
  fonts_unload_custom_font(s_font_tech11);
  fonts_unload_custom_font(s_font_tech8);
  if (s_hatch) gbitmap_destroy(s_hatch);
}

static void init(void) {
  time_t now = time(NULL);
  s_now = *localtime(&now);

  settings_load();
  refresh_theme();

  if (persist_exists(PERSIST_WX_TEMP)) {
    s_wx_temp = persist_read_int(PERSIST_WX_TEMP);
    s_wx_kind = persist_read_int(PERSIST_WX_KIND);
  }

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  update_tick_subscription();

  battery_state_service_subscribe(battery_handler);
  s_battery_pct = battery_state_service_peek().charge_percent;

#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
  update_steps();
  update_heart();
#endif

  app_message_register_inbox_received(inbox_received);
  // Inbox must fit a full Clay settings dictionary, not just weather.
  app_message_open(512, 32);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
