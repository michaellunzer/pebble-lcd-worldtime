# LCD Worldtime — Pebble Time 2 watchface

A Casio-flavoured LCD digital watchface for the **Pebble Time 2** (platform
`emery`, 200 × 228 color), implemented from the Claude Design handoff in
`../project/`. The print spec (`Pebble Watchface-print.html`) is the source
of truth for the layout.

```
┌──────────────────────────────┐
│ THU 14 MAY        [SF]  PM   │  header 18px (16px type, full month)
│ ▮▮▮▮▮▮▮▮▮▮▮▮▮▮▯▯▯▯▯▯         │  battery bar 6px (20 segments + ghost)
│ ┌──────────────────────────┐ │
│ │  02:32  ³²SEC            │ │  time box hugs digits: 42px (+22px
│ └──────────────────────────┘ │  seconds) or 50px when seconds move
│ ▓▓░ dot-matrix world map ░▓▓ │  green/blue map, day/night shading
│  WEATHER      │      STEPS   │  two slots: weather/steps/heart/sec
└──────────────────────────────┘
```

## Layout (native pixels)

| Strip | Y | Height | Content |
|---|---|---|---|
| Header | 10 | 18 | day-of-week + date (16px double-struck bold, full month when it fits), AM/PM (accent) |
| Battery | 32 | 6 | 10 blocks, lit or empty (one per 10 %), accent ≤ 20 % |
| Time | 42 | hugs digits | framed box (full 188px width), DSEG7 42px digits + 22px seconds — or 50px digits alone when seconds are off/moved to a slot |
| Map | below time | remainder | 60 × 19 grid from Natural Earth 110m coastlines — green land / blue sea in day/night shades, yellow sun marker, accent home dot |
| Footer | 172 | 50 | two unlabeled slots: weather (icon + temp) · steps · heart rate · seconds · empty |

Screen padding 6 px; every strip shares the same 188 px column
(x = 6) so the boxes align down the face. The LCD stipple texture
(prebuilt 1-bit bitmap, one blit per frame) covers the face except the
time box and the map, which sit on solid backing.

## Settings

All user options live in a Clay configuration page (open the watchface's
settings in the Pebble mobile app):

- **Location** — home-dot latitude / longitude (drawn on the map), and
  whether weather uses the phone's GPS or those fixed coordinates.
- **Theme** — one of the four LCD variants, either fixed or in **auto
  day/night mode**: pick a day theme and a night theme plus the hours
  they switch (defaults: positive 07:00–20:00, negative overnight).
- **Display** — seconds beside the time, leading zero on the hour, and
  what each bottom box shows:
  weather, steps, heart rate, seconds, or empty (a solo box takes the
  full footer width). Putting seconds in a box hides the inline column
  and grows the time to 50 px. With no seconds anywhere the face wakes
  once a minute and saves battery. Plus °F/°C and the step goal.

Settings are pushed over AppMessage (`src/c/settings.c`) and persisted
on the watch.

## Design → device deviations

- **Colors** are quantized to Pebble's 64-color palette and pushed
  saturated for the physical panel (the prototype's soft tones washed
  out on-device): dark themes use white ink with ChromeYellow / Cyan
  accents; positive runs pure black text/accents on Brass. All four
  themes are in `src/c/theme.h`; the active one is at runtime from
  settings.
- **Ghost segments** render in a solid light color knocked back by a
  75 % bg-colored hatch, approximating the prototype's 8 % alpha.
- **Time digits are 42 px with inline 22 px seconds, 50 px without**
  (spec says 44/18): the time box spans the full 188 px screen padding
  (the other strips keep the 180 px column) — the largest sizes DSEG7's
  advance allows on the pixel grid. The box height hugs the digits and
  the map absorbs the leftover space.
- **The sun marker has its own color** (yellow, per-theme `sun` entry)
  and is larger than the accent home-location dot, so the two map
  markers never read as the same thing.
- **Header text is 16 px** with the full month name, falling back to the
  3-letter month when a long month plus the city box won't fit.
- Font resource names must not contain digits before the size — the SDK's
  fontgen takes the *first* number in the name as the pixel height
  (hence `FONT_LCD_40`, not `FONT_DSEG7_40`).
- The colon blinks on a 1 s on / 1 s off cycle (the spec's 0.5 Hz).

## Data sources

- **Battery / steps**: on-watch battery and Health services.
- **Weather**: PebbleKit JS (`src/pkjs/index.js`) fetches from
  [Open-Meteo](https://open-meteo.com/) (no API key) using the phone's
  location (or the configured coordinates), every 30 minutes; last
  reading is persisted on the watch. Conditions map to sun / cloud /
  rain / moon icons.

## Building

Requires the [Core Devices pebble-tool](https://github.com/coredevices) ≥ 5.0
with SDK core 4.4 and an ARM toolchain:

```sh
pip install pebble-tool   # or: uv tool install pebble-tool
pebble sdk install latest # needs access to sdk.repebble.com
pebble build
```

The built `build/pebble-lcd-worldtime.pbw` (a copy is committed in
`dist/`) can be sideloaded with `pebble install --phone <ip>` or through
the Pebble mobile app. SDK core ≥ 4.9 emulates emery in QEMU
(`pebble install --emulator emery`).

Toolchain compatibility flags for GCC 14 (builtin `__FILE_NAME__`,
`strftime` prototype) live in `wscript`; `time_t` comes from the SDK's
own `-Dtime_t=long`.

## Fonts

- [DSEG7 Classic Bold](https://github.com/keshikan/DSEG) v0.46 — digits
  (42 px time, 22 px seconds). SIL OFL 1.1, see
  `resources/fonts/DSEG-LICENSE.txt`.
- [Share Tech Mono](https://fonts.google.com/specimen/Share+Tech+Mono) —
  labels (18/14/11/8 px). SIL OFL 1.1, see
  `resources/fonts/ShareTechMono-OFL.txt`.
