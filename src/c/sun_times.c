#include "sun_times.h"

// Integer-only NOAA sunrise equation on the watch's sin/cos lookup tables.
// (The libm double version pulled in soft-float trig and large stack
// frames; this keeps everything in int32/int64.)
//
// Angles are in Pebble units: TRIG_MAX_ANGLE (65536) = 360°, trig results
// in TRIG_MAX_RATIO (65536) = 1.0.

#define R TRIG_MAX_RATIO
#define A TRIG_MAX_ANGLE
// cos(90.833°) — the sun's apparent edge (refraction + solar radius).
#define COS_ZENITH (-953) // -0.0145439 * 65536

static int32_t trig_sin(int64_t angle) {
  return sin_lookup((int32_t)(((angle % A) + A) % A));
}
static int32_t trig_cos(int64_t angle) {
  return cos_lookup((int32_t)(((angle % A) + A) % A));
}

SunKind sun_times_get(const struct tm *now, int32_t lat_x100,
                      int32_t lon_x100, int *sunrise_min, int *sunset_min) {
  static int s_yday = -1, s_year, s_rise, s_set;
  static int32_t s_lat, s_lon;
  static long s_off;
  static SunKind s_kind;

  const long off = now->tm_gmtoff;
  if (s_yday == now->tm_yday && s_year == now->tm_year &&
      s_lat == lat_x100 && s_lon == lon_x100 && s_off == off) {
    *sunrise_min = s_rise;
    *sunset_min = s_set;
    return s_kind;
  }

  // Fractional year g = 2π(yday + 0.5)/365 as an angle.
  const int64_t g = (int64_t)A * (2 * now->tm_yday + 1) / 730;
  const int64_t sg = trig_sin(g), cg = trig_cos(g);
  const int64_t s2 = trig_sin(2 * g), c2 = trig_cos(2 * g);
  const int64_t s3 = trig_sin(3 * g), c3 = trig_cos(3 * g);

  // Equation of time, minutes x100 (coefficients x1e6).
  const int64_t eq_scaled = 75 * (int64_t)R + 1868 * cg - 32077 * sg -
                            14615 * c2 - 40849 * s2;
  const int32_t eq_x100 = (int32_t)(eq_scaled * 22918 / (1000000LL * R));

  // Solar declination as an angle (coefficients x1e6, radians -> angle:
  // /(2π·1e6) with the R factor cancelling).
  const int64_t decl_scaled = 6918 * (int64_t)R - 399912 * cg + 70257 * sg -
                              6758 * c2 + 907 * s2 - 2697 * c3 + 1480 * s3;
  const int64_t decl_angle = decl_scaled / 6283185;

  const int64_t lat_angle = (int64_t)lat_x100 * A / 36000;
  const int64_t sin_sin =
      (int64_t)trig_sin(lat_angle) * trig_sin(decl_angle) / R;
  const int64_t den = (int64_t)trig_cos(lat_angle) * trig_cos(decl_angle) / R;

  SunKind kind = SUN_NORMAL;
  int rise = 0, set = 0;
  if (den <= 0) {
    // At a pole the sun is up when declination is on the same side.
    kind = sin_sin > 0 ? SUN_ALWAYS_UP : SUN_ALWAYS_DOWN;
  } else {
    // cos(hour angle) = (cos(zenith) - sinφ sinδ) / (cosφ cosδ), in R units
    const int64_t c = (COS_ZENITH - sin_sin) * R / den;
    if (c > R) {
      kind = SUN_ALWAYS_DOWN;
    } else if (c < -R) {
      kind = SUN_ALWAYS_UP;
    } else {
      // acos by bisection: cos_lookup falls monotonically over 0..180°.
      int32_t lo = 0, hi = A / 2;
      while (hi - lo > 1) {
        int32_t mid = (lo + hi) / 2;
        if (cos_lookup(mid) > c) lo = mid; else hi = mid;
      }
      const int32_t ha_x100 = (int32_t)((int64_t)hi * 36000 / A); // degrees x100
      const int32_t off_x100 = (int32_t)(off * 100 / 60);
      const int32_t base = 72000 - eq_x100 + off_x100; // minutes x100
      int32_t r = base - 4 * (lon_x100 + ha_x100);
      int32_t s = base - 4 * (lon_x100 - ha_x100);
      rise = (((r + 50) / 100) % 1440 + 1440) % 1440;
      set = (((s + 50) / 100) % 1440 + 1440) % 1440;
    }
  }

  s_yday = now->tm_yday;
  s_year = now->tm_year;
  s_lat = lat_x100;
  s_lon = lon_x100;
  s_off = off;
  s_rise = rise;
  s_set = set;
  s_kind = kind;
  *sunrise_min = rise;
  *sunset_min = set;
  return kind;
}
