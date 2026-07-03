# LCD Worldtime — Pebble Time 2 watchface

A Casio-flavoured LCD digital watchface for the **Pebble Time 2** (platform
`emery`, 200 × 228 color), implemented from the Claude Design handoff in
`../project/`. The print spec (`Pebble Watchface-print.html`) is the source
of truth for the layout.

```
┌──────────────────────────────┐
│ THU 14 MAY        [SF]  PM   │  header 18px
│ ▮▮▮▮▮▮▮▮▮▮▮▮▮▮▯▯▯▯▯▯         │  battery bar 6px (20 segments)
│ ┌──────────────────────────┐ │
│ │  02:32  ³²SEC            │ │  time box 76px, DSEG7 44px + ghost
│ └──────────────────────────┘ │
│ ▓▓░ dot-matrix world map ░▓▓ │  46px, live day/night terminator
│  WEATHER      │      STEPS   │  footer: icon+°F+H/L · count+bar
└──────────────────────────────┘
```

## Layout (native pixels)

| Strip | Y | Height | Content |
|---|---|---|---|
| Header | 10 | 18 | day-of-week, date, city tag, AM/PM (accent) |
| Battery | 32 | 6 | 20 segments, outline = 100 %, accent ≤ 20 % |
| Time | 42 | 76 | framed box, DSEG7 digits over ghost segments, blinking colon, accent seconds |
| Map | 122 | 46 | 60 × 19 dot grid, solar terminator, subsolar marker |
| Footer | 172 | 50 | WEATHER · STEPS with progress bar |

Screen padding 6 px; content inset to x = 10, width 180. The LCD stipple
texture (prebuilt 1-bit bitmap, one blit per frame) covers the face and
stops at the time box edges, per the design.

## Settings

All user options live in a Clay configuration page (open the watchface's
settings in the Pebble mobile app):

- **Location** — city code shown in the header box, home-dot latitude /
  longitude (drawn on the map), and whether weather uses the phone's GPS
  or those fixed coordinates.
- **Theme** — one of the four LCD variants, either fixed or in **auto
  day/night mode**: pick a day theme and a night theme plus the hours
  they switch (defaults: positive 07:00–20:00, negative overnight).
- **Display** — show/hide seconds (hiding them drops the face to
  once-a-minute wakeups and saves battery), show/hide the weather and
  steps sections (a solo section takes the full footer width), °F/°C,
  and the step goal for the progress bar.

Settings are pushed over AppMessage (`src/c/settings.c`) and persisted
on the watch.

## Design → device deviations

- **Colors** are quantized to Pebble's 64-color palette. Positive LCD's
  `#c9c4ae` tan becomes `GColorBrass`, the rust accent `#7a3a18` becomes
  `GColorWindsorTan`, and the prototype's alpha-based ghost/stipple/mute
  tones map to solid Light/DarkGray. All four themes are in
  `src/c/theme.h`; the active one is chosen at runtime from settings.
- **Ghost segments** render as a solid light color rather than 8 % alpha.
- **Header text is 14 px** (spec says 11): the prototype's CSS pixels
  read larger than real pixels on the 200 px-wide LCD, so the header row
  is bumped for legibility (city-tag box 11 px).
- **Time digits are 40 px** (spec says 44): DSEG7's real advance is 0.82 em,
  so 44 px digits plus the seconds column overflow the 180 px inner width
  on the actual pixel grid. 40 px is the largest size that fits.
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
  (40 px time, 18 px seconds). SIL OFL 1.1, see
  `resources/fonts/DSEG-LICENSE.txt`.
- [Share Tech Mono](https://fonts.google.com/specimen/Share+Tech+Mono) —
  labels (18/11/8 px). SIL OFL 1.1, see
  `resources/fonts/ShareTechMono-OFL.txt`.
