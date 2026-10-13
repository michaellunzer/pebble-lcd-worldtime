#include "sun_times.h"
#include <math.h>

#define DEG2RAD 0.017453292519943295
// Sun's apparent edge: 90.833° zenith (refraction + solar radius).
#define SUNRISE_ZENITH_COS (-0.014543897651582656) // cos(90.833°)

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

  const double lat = lat_x100 / 100.0 * DEG2RAD;
  const double lon = lon_x100 / 100.0;
  // Fractional year, using local noon of this date.
  const double g = 2.0 * 3.141592653589793 / 365.0 * (now->tm_yday + 0.5);

  const double eqtime =
      229.18 * (0.000075 + 0.001868 * cos(g) - 0.032077 * sin(g) -
                0.014615 * cos(2 * g) - 0.040849 * sin(2 * g));
  const double decl = 0.006918 - 0.399912 * cos(g) + 0.070257 * sin(g) -
                      0.006758 * cos(2 * g) + 0.000907 * sin(2 * g) -
                      0.002697 * cos(3 * g) + 0.00148 * sin(3 * g);

  const double denom = cos(lat) * cos(decl);
  SunKind kind = SUN_NORMAL;
  int rise = 0, set = 0;
  if (denom < 1e-9) {
    // Pole itself: sun is up when its declination is on the same side.
    kind = (lat * decl > 0) ? SUN_ALWAYS_UP : SUN_ALWAYS_DOWN;
  } else {
    const double c = SUNRISE_ZENITH_COS / denom - tan(lat) * tan(decl);
    if (c > 1.0) {
      kind = SUN_ALWAYS_DOWN;
    } else if (c < -1.0) {
      kind = SUN_ALWAYS_UP;
    } else {
      const double ha = acos(c) / DEG2RAD; // degrees
      const double off_min = off / 60.0;
      double r = 720.0 - 4.0 * (lon + ha) - eqtime + off_min;
      double s = 720.0 - 4.0 * (lon - ha) - eqtime + off_min;
      rise = ((int)floor(r + 0.5) % 1440 + 1440) % 1440;
      set = ((int)floor(s + 0.5) % 1440 + 1440) % 1440;
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
