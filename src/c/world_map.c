#include "world_map.h"

// World bitmap (hand-authored, 60 cols x 19 rows, mercator-ish).
// X = land, . = sea. Same art as the design prototype.
#define MAP_COLS 60
#define MAP_ROWS 19

static const char *const WORLD[MAP_ROWS] = {
  /* 75N */ ".........................XX.................XXXXX.........",
  /* 67N */ "....XXX..................XXXX............XXXXXXXXXXXXXXX...",
  /* 60N */ "...XXXXXXXX............XXXXX..XX.........XXXXXXXXXXXXXXX...",
  /* 53N */ "..XXXXXXXXXXX...........XX...XXXX.X......XXXXXXXXXXXXXXXX..",
  /* 47N */ "..XXXXXXXXXXXX..............XXXXXXXXXX...XXXXXXXXXXXXXXX...",
  /* 41N */ "...XXXXXXXXXX................XXXX.XXXXXXXXXXXXXX.XXXXXX....",
  /* 35N */ "....XXXXXXXX.................XXXXXXXXXXXXXXX..XXXXXXX......",
  /* 28N */ ".....XXXXXX...............XXXXXXX.XXXXXXXXXX..XXX.XXX......",
  /* 21N */ "......XXXX................XXXXXXXX.XXXXX..XXX....XX........",
  /* 14N */ "........XX................XXXXXXX...XXX...XXX..............",
  /* 7N  */ ".........XX................XXXX.........XXXXX.X............",
  /* 0   */ "..........XXXX.............XXX...........XXXXXX.X..........",
  /* 7S  */ "..........XXXX..............XX...........XXXX...XX.........",
  /* 14S */ "..........XXXXX..............X.............XXX..XXXX.......",
  /* 22S */ "...........XXX...............XX..X............XXXXXXX......",
  /* 30S */ "...........XXX................XX................XXXX.X.....",
  /* 38S */ "...........XX....................................X..XX.....",
  /* 47S */ "............X...............................................",
  /* 60S */ "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
};

// Latitude at the centre of each row, used by the day/night calc.
static const int8_t WORLD_LATS[MAP_ROWS] = {
  75, 67, 60, 53, 47, 41, 35, 28, 21, 14, 7, 0,
  -7, -14, -22, -30, -38, -47, -60,
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
                    GColor ink, GColor mute, GColor accent, GColor bg,
                    bool show_home, int32_t home_lat_x100,
                    int32_t home_lon_x100) {
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

      if (land) {
        graphics_context_set_fill_color(ctx, ink);
        graphics_fill_rect(ctx, GRect(cx, cy, dot_w, dot_h), 0, GCornerNone);
      } else if (night) {
        // Night ocean: small muted dot — reads as the shaded hemisphere.
        graphics_context_set_fill_color(ctx, mute);
        graphics_fill_rect(ctx, GRect(cx + dot_w / 2, cy + dot_h / 2, 1, 1),
                           0, GCornerNone);
      }
    }
  }

  // --- Home-location dot: bg ring under an accent disc ---
  // (The sun's position shows only through the day/night shading — a
  // second marker on the map read as clutter.)
  if (show_home) {
    int32_t hx = x0 + (int32_t)(((int64_t)(home_lon_x100 + 18000) * w) / 36000);
    int hy = lat_to_y(home_lat_x100 / 100, y0, h);
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_circle(ctx, GPoint(hx, hy), 3);
    graphics_context_set_fill_color(ctx, accent);
    graphics_fill_circle(ctx, GPoint(hx, hy), 2);
  }

  // Frame
  graphics_context_set_stroke_color(ctx, mute);
  graphics_draw_rect(ctx, bounds);
}
