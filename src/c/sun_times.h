#pragma once

#include <pebble.h>

// Sunrise / sunset for a location on a given day (NOAA solar-position
// approximation, accurate to a couple of minutes). Pure math on the watch —
// no network. Results are minutes since local midnight.
typedef enum {
  SUN_NORMAL = 0,
  SUN_ALWAYS_UP,   // polar day
  SUN_ALWAYS_DOWN, // polar night
} SunKind;

// `now` is local time (supplies the date and UTC offset); lat/lon are in
// centi-degrees (+N/+E). Results are cached per day and location.
SunKind sun_times_get(const struct tm *now, int32_t lat_x100,
                      int32_t lon_x100, int *sunrise_min, int *sunset_min);
