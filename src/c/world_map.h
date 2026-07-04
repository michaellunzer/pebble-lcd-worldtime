#pragma once

#include <pebble.h>

// Dot-matrix world map with live day/night terminator and subsolar sun
// marker, ported from the design prototype's mercator-map.jsx.
//
// Draws into `bounds` of the given graphics context. `utc` must be the
// current UTC broken-down time (gmtime), used for the solar geometry.
//
// The subsolar sun marker is drawn in `sun` (disc + crosshair rays).
// When `show_home` is set, a home-location dot is drawn in `accent` at
// the given coordinates (centi-degrees, +N/+E), ringed in bg so it
// reads against both land and sea.
void world_map_draw(GContext *ctx, GRect bounds, const struct tm *utc,
                    GColor ink, GColor mute, GColor accent, GColor bg,
                    GColor sun, bool show_home, int32_t home_lat_x100,
                    int32_t home_lon_x100);
