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
                    const Theme *theme, bool show_home,
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
      } else {
        graphics_context_set_fill_color(
            ctx, night ? theme->sea_night : theme->sea_day);
        graphics_fill_rect(ctx, GRect(cx + dot_w / 2, cy + dot_h / 2, 1, 1),
                           0, GCornerNone);
      }
    }
  }

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
