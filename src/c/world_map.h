#pragma once

#include <pebble.h>

// Dot-matrix world map with live day/night terminator and subsolar sun
// marker, ported from the design prototype's mercator-map.jsx.
//
// Draws into `bounds` of the given graphics context. `utc` must be the
// current UTC broken-down time (gmtime), used for the solar geometry.
void world_map_draw(GContext *ctx, GRect bounds, const struct tm *utc,
                    GColor ink, GColor mute, GColor accent);
