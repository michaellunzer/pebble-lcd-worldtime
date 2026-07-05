#include "world_map.h"

// World bitmap (60 cols x 19 rows). X = land, . = sea.
//
// Generated from Natural Earth 110m land polygons: each cell is 3x3
// point-in-polygon supersampled at the row's latitude band (land when
// >= 3 of 9 hits), so coastlines land where the linear-longitude math
// expects them. Column c covers lon -180 + c*6 .. -174 + c*6.
#define MAP_COLS 60
#define MAP_ROWS 19

static const char *const WORLD[MAP_ROWS] = {
  /* 75N */ ".........XXX.XXXX..XXXXXXXX............X....XXXXXXX..X......",
  /* 67N */ "X.XXXXXXXXXXXXXX.XX..XXXXXX.....XXXXXXXXXXXXXXXXXXXXXXXXXXXX",
  /* 60N */ "..XXXXXXXXXXXXX..XXX..X........XX.XXXXXXXXXXXXXXXXXXXXXXXXX.",
  /* 53N */ "........XXXXXXXXXXXXX.......XXXXXXXXXXXXXXXXXXXXXXXXXX..X...",
  /* 47N */ ".........XXXXXXXXXXX.........XXXXXXXXXXXXXXXXXXXXXXXX.......",
  /* 41N */ ".........XXXXXXXXX..........XX.XXXXXXXXXXXXXXXXXXXXX.X......",
  /* 35N */ "..........XXXXXXX............XXX...XXXXXXXXXXXXXXX.XX.......",
  /* 28N */ "...........XXXX.X...........XXXXXXXXXXXXXXXXXXXXXX..........",
  /* 21N */ "............XX.X...........XXXXXXXXXXXXX.XXXXXXXX...........",
  /* 14N */ "..............XX...........XXXXXXXXXXXX...XX..XX..X.........",
  /* 7N  */ ".................XXXX.......XXXXXXXXXX........X.............",
  /* 0   */ ".................XXXXX.........XXXXXX.........XXXX..........",
  /* 7S  */ "................XXXXXXXX........XXXXX................XX.....",
  /* 14S */ ".................XXXXXXX........XXXXX..............XXX......",
  /* 22S */ "..................XXXXX.........XXXX.X...........XXXXXX.....",
  /* 30S */ "..................XXXX...........XX..............XXXXXXX....",
  /* 38S */ "..................XX.................................XX.....",
  /* 47S */ ".................XX.........................................",
  /* 70S */ "..................XX........XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX..",
};

// Latitude at the centre of each row, used by the day/night calc.
static const int8_t WORLD_LATS[MAP_ROWS] = {
  75, 67, 60, 53, 47, 41, 35, 28, 21, 14, 7, 0,
  -7, -14, -22, -30, -38, -47, -70,
};

// High-res land mask for MAP_STYLE_SOLID (94 cols x 35 rows, bit-packed
// MSB-first), generated from Natural Earth 110m with the same nonlinear
// latitude bands as the dot map so markers land in the same place.
#define HI_COLS 94
#define HI_ROWS 35

static const uint8_t WORLD_HI[35][12] = {
  {0x00, 0x03, 0xF6, 0xC0, 0x7F, 0x80, 0x00, 0x04, 0x4F, 0xFF, 0x80, 0x00},
  {0x8F, 0xFF, 0xFF, 0xDC, 0x7F, 0x00, 0x1F, 0x8B, 0xFF, 0xFF, 0xFF, 0xFC},
  {0x5F, 0xFF, 0xFF, 0x8C, 0x78, 0x60, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC},
  {0x0F, 0xFF, 0xFE, 0x38, 0x10, 0x00, 0xF7, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0},
  {0x04, 0x0F, 0xFF, 0x1E, 0x00, 0x04, 0x67, 0xFF, 0xFF, 0xFF, 0xE0, 0xC0},
  {0x00, 0x07, 0xFF, 0xFF, 0x00, 0x0E, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x80},
  {0x00, 0x03, 0xFF, 0xFE, 0x80, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xF8, 0x00},
  {0x00, 0x01, 0xFF, 0xFC, 0x00, 0x03, 0xFF, 0xF7, 0xFF, 0xFF, 0xE0, 0x00},
  {0x00, 0x01, 0xFF, 0xF8, 0x00, 0x07, 0xFC, 0x7F, 0xFF, 0xFF, 0xC8, 0x00},
  {0x00, 0x01, 0xFF, 0xF0, 0x00, 0x06, 0x0F, 0xF7, 0xFF, 0xFF, 0x80, 0x00},
  {0x00, 0x01, 0xFF, 0xE0, 0x00, 0x06, 0xC3, 0xFF, 0xFF, 0xFC, 0x90, 0x00},
  {0x00, 0x00, 0xFF, 0xC0, 0x00, 0x07, 0xC0, 0xFF, 0xFF, 0xFE, 0x40, 0x00},
  {0x00, 0x00, 0x7F, 0x40, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00},
  {0x00, 0x00, 0x3C, 0x00, 0x00, 0x1F, 0xFF, 0xF3, 0xFF, 0xFC, 0x00, 0x00},
  {0x00, 0x00, 0x1C, 0x00, 0x00, 0x1F, 0xFF, 0xFE, 0x7F, 0xF8, 0x00, 0x00},
  {0x00, 0x00, 0x0F, 0x00, 0x00, 0x1F, 0xFF, 0xBC, 0x39, 0xE2, 0x00, 0x00},
  {0x00, 0x00, 0x01, 0x80, 0x00, 0x1F, 0xFF, 0xF0, 0x30, 0x70, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x5E, 0x00, 0x1F, 0xFF, 0xF0, 0x10, 0x20, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x1F, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x41, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x3F, 0xC0, 0x00, 0x7F, 0xE0, 0x00, 0xCC, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x3F, 0xE0, 0x00, 0x7F, 0xC0, 0x00, 0x5C, 0x40, 0x00},
  {0x00, 0x00, 0x00, 0x3F, 0xF8, 0x00, 0x3F, 0x80, 0x00, 0x20, 0x38, 0x00},
  {0x00, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x3F, 0x80, 0x00, 0x00, 0x0C, 0x00},
  {0x00, 0x00, 0x00, 0x1F, 0xF8, 0x00, 0x3F, 0xC0, 0x00, 0x00, 0x60, 0x00},
  {0x00, 0x00, 0x00, 0x0F, 0xF8, 0x00, 0x3F, 0x90, 0x00, 0x01, 0xE8, 0x00},
  {0x00, 0x00, 0x00, 0x07, 0xF8, 0x00, 0x3F, 0x30, 0x00, 0x07, 0xFC, 0x00},
  {0x00, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x1F, 0x00, 0x00, 0x07, 0xFE, 0x00},
  {0x00, 0x00, 0x00, 0x0F, 0xC0, 0x00, 0x1E, 0x00, 0x00, 0x07, 0xFE, 0x00},
  {0x00, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x0C, 0x00, 0x00, 0x06, 0x3E, 0x00},
  {0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x08},
  {0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00},
  {0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  {0x00, 0x00, 0x08, 0x0E, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE0},
  {0x03, 0xFF, 0xFF, 0xF0, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0},
};

static const int8_t WORLD_HI_LATS[35] = {
  73, 68, 64, 61, 57, 53, 50, 47, 43, 40, 37, 33, 30, 26, 22, 18, 14, 10, 7, 3, -1, -5, -9, -12, -16, -21, -25, -29, -34, -38, -43, -49, -62, -72, -77,
};

// Degrees -> Pebble trig-angle units.
static inline int32_t deg_to_angle(int32_t deg_x100) {
  return (int32_t)((int64_t)deg_x100 * TRIG_MAX_ANGLE / 36000);
}

// Piecewise-linear latitude -> pixel y over the map's row latitudes.
static int lat_to_y(int lat_deg, int16_t y0, int16_t h) {
  if (lat_deg >= WORLD_LATS[0]) return y0;
  for (int i = 0; i < MAP_ROWS - 1; i++) {
    if (lat_deg <= WORLD_LATS[i] && lat_deg >= WORLD_LATS[i + 1]) {
      int span = WORLD_LATS[i] - WORLD_LATS[i + 1];
      int frac_num = WORLD_LATS[i] - lat_deg;
      return y0 + (i * h + frac_num * h / span) / MAP_ROWS;
    }
  }
  return y0 + h - 1;
}

void world_map_draw(GContext *ctx, GRect bounds, const struct tm *utc,
                    const Theme *theme, uint8_t style, bool show_home,
                    int32_t home_lat_x100, int32_t home_lon_x100) {
  const int16_t w = bounds.size.w;
  const int16_t h = bounds.size.h;
  const int16_t x0 = bounds.origin.x;
  const int16_t y0 = bounds.origin.y;

  // --- Solar geometry (all integer / Pebble fixed-point trig) ---
  // Declination: 23.45deg * sin(360/365 * (dayOfYear - 81))
  int32_t doy = utc->tm_yday + 1;
  int32_t season = ((doy - 81) % 365 + 365) % 365;
  int32_t season_angle = (int32_t)((int64_t)season * TRIG_MAX_ANGLE / 365);
  // declination in centi-degrees: sin() is in [-65536, 65536]
  int32_t decl_x100 = (int32_t)((int64_t)sin_lookup(season_angle) * 2345 / TRIG_MAX_RATIO);
  int32_t decl_angle = deg_to_angle(decl_x100);
  int32_t sin_decl = sin_lookup(decl_angle);
  int32_t cos_decl = cos_lookup(decl_angle);

  // Subsolar longitude in centi-degrees: -(utcHours - 12) * 15
  int32_t utc_min = utc->tm_hour * 60 + utc->tm_min;
  int32_t sublon_x100 = -(utc_min - 720) * 25; // *15deg/60min*100

  if (style == MAP_STYLE_SOLID) {
    // Filled continents on a solid ocean, 94x35 cells (~2px each) —
    // every cell painted edge to edge, day/night via the shade pairs.
    for (int r = 0; r < HI_ROWS; r++) {
      const int16_t cy = y0 + (int16_t)(r * h / HI_ROWS);
      const int16_t cy1 = y0 + (int16_t)((r + 1) * h / HI_ROWS);
      if (cy1 <= cy) continue;

      int32_t lat_angle = deg_to_angle((int32_t)WORLD_HI_LATS[r] * 100);
      int32_t ss = (int32_t)((int64_t)sin_lookup(lat_angle) * sin_decl /
                             TRIG_MAX_RATIO);
      int32_t cc = (int32_t)((int64_t)cos_lookup(lat_angle) * cos_decl /
                             TRIG_MAX_RATIO);

      const uint8_t *row = WORLD_HI[r];
      for (int c = 0; c < HI_COLS; c++) {
        const int16_t cx = x0 + (int16_t)(c * w / HI_COLS);
        const int16_t cx1 = x0 + (int16_t)((c + 1) * w / HI_COLS);
        if (cx1 <= cx) continue;

        int32_t lon_x100 = -18000 + (2 * c + 1) * 18000 / HI_COLS;
        int32_t dlon_angle = deg_to_angle(lon_x100 - sublon_x100);
        int64_t cos_z = (int64_t)ss + (int64_t)cc *
                        cos_lookup(dlon_angle) / TRIG_MAX_RATIO;
        bool night = cos_z < 0;
        bool land = row[c / 8] & (0x80 >> (c % 8));

        graphics_context_set_fill_color(
            ctx, land ? (night ? theme->land_night : theme->land_day)
                      : (night ? theme->sea_night : theme->sea_day));
        graphics_fill_rect(ctx, GRect(cx, cy, cx1 - cx, cy1 - cy), 0,
                           GCornerNone);
      }
    }
    goto markers;
  }

  // Per-row lat trig; per-cell only one cos_lookup for delta-longitude.
  // Cell geometry: integer edges so the grid divides evenly.
  for (int r = 0; r < MAP_ROWS; r++) {
    const int16_t cy = y0 + (int16_t)(r * h / MAP_ROWS);
    const int16_t cy1 = y0 + (int16_t)((r + 1) * h / MAP_ROWS);
    const int16_t dot_h = (cy1 - cy - 1) > 0 ? (cy1 - cy - 1) : 1;

    int32_t lat_angle = deg_to_angle((int32_t)WORLD_LATS[r] * 100);
    int32_t sin_lat = sin_lookup(lat_angle);
    int32_t cos_lat = cos_lookup(lat_angle);
    // sinφ·sinδ and cosφ·cosδ, both scaled back to TRIG_MAX_RATIO
    int32_t ss = (int32_t)((int64_t)sin_lat * sin_decl / TRIG_MAX_RATIO);
    int32_t cc = (int32_t)((int64_t)cos_lat * cos_decl / TRIG_MAX_RATIO);

    const char *row = WORLD[r];
    for (int c = 0; c < MAP_COLS; c++) {
      const int16_t cx = x0 + (int16_t)(c * w / MAP_COLS);
      const int16_t cx1 = x0 + (int16_t)((c + 1) * w / MAP_COLS);
      const int16_t dot_w = (cx1 - cx - 1) > 0 ? (cx1 - cx - 1) : 1;

      // Cell-centre longitude in centi-degrees.
      int32_t lon_x100 = -18000 + (2 * c + 1) * 18000 / MAP_COLS;
      int32_t dlon_angle = deg_to_angle(lon_x100 - sublon_x100);
      // cos of solar zenith: sinφsinδ + cosφcosδcos(Δλ) — sign only
      int64_t cos_z = (int64_t)ss +
                      (int64_t)cc * cos_lookup(dlon_angle) / TRIG_MAX_RATIO;
      bool night = cos_z < 0;
      bool land = row[c] == 'X';

      // Green land, blue sea; the terminator reads through the
      // day/night shades of each.
      if (land) {
        graphics_context_set_fill_color(
            ctx, night ? theme->land_night : theme->land_day);
        graphics_fill_rect(ctx, GRect(cx, cy, dot_w, dot_h), 0, GCornerNone);
      } else if ((r + c) % 2 == 0) {
        // Sea dots on a checkerboard — every cell matched the stipple's
        // pitch and made the map read as noise.
        graphics_context_set_fill_color(
            ctx, night ? theme->sea_night : theme->sea_day);
        graphics_fill_rect(ctx, GRect(cx + dot_w / 2, cy + dot_h / 2, 1, 1),
                           0, GCornerNone);
      }
    }
  }

markers:
  // --- Subsolar sun marker: bright disc + crosshair rays ---
  // Column from longitude, row from declination. Drawn in its own sun
  // color and bigger than the home dot so the two never get confused.
  {
    int32_t sun_x =
        x0 + (int32_t)(((int64_t)(sublon_x100 + 18000) * w) / 36000);
    int sun_y = lat_to_y(decl_x100 / 100, y0, h);
    graphics_context_set_stroke_color(ctx, theme->ink);
    graphics_draw_line(ctx, GPoint(sun_x - 7, sun_y), GPoint(sun_x + 7, sun_y));
    graphics_draw_line(ctx, GPoint(sun_x, sun_y - 7), GPoint(sun_x, sun_y + 7));
    graphics_context_set_fill_color(ctx, theme->sun);
    graphics_fill_circle(ctx, GPoint(sun_x, sun_y), 4);
    graphics_context_set_stroke_color(ctx, theme->ink);
    graphics_draw_circle(ctx, GPoint(sun_x, sun_y), 4);
  }

  // --- Home-location dot: bg ring under an accent disc ---
  if (show_home) {
    int32_t hx = x0 + (int32_t)(((int64_t)(home_lon_x100 + 18000) * w) / 36000);
    int hy = lat_to_y(home_lat_x100 / 100, y0, h);
    graphics_context_set_fill_color(ctx, theme->bg);
    graphics_fill_circle(ctx, GPoint(hx, hy), 3);
    graphics_context_set_fill_color(ctx, theme->accent);
    graphics_fill_circle(ctx, GPoint(hx, hy), 2);
  }

  // Frame
  graphics_context_set_stroke_color(ctx, theme->mute);
  graphics_draw_rect(ctx, bounds);
}
