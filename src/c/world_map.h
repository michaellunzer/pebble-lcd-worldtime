#pragma once

#include <pebble.h>
#include "theme.h"

// World map with live day/night terminator, colored land and sea
// (per-theme day/night shades), a subsolar sun marker, and an optional
// home-location dot. Two styles:
enum MapStyle {
  MAP_STYLE_DOTS = 0,  // 60x19 dot-matrix cells, checkerboard sea
  MAP_STYLE_SOLID = 1, // 94x35 filled cells — solid continents + ocean
};
//
// Draws into `bounds` of the given graphics context. `utc` must be the
// current UTC broken-down time (gmtime), used for the solar geometry.
// When `show_home` is set, the home dot is drawn in the theme accent at
// the given coordinates (centi-degrees, +N/+E), ringed in bg so it
// reads against both land and sea.
void world_map_draw(GContext *ctx, GRect bounds, const struct tm *utc,
                    const Theme *theme, uint8_t style, bool show_home,
                    int32_t home_lat_x100, int32_t home_lon_x100);
