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

## Design → device deviations

- **Colors** are quantized to Pebble's 64-color palette. Positive LCD's
  `#c9c4ae` tan becomes `GColorBrass`, the rust accent `#7a3a18` becomes
  `GColorWindsorTan`, and the prototype's alpha-based ghost/stipple/mute
  tones map to solid Light/DarkGray. All four themes are in
  `src/c/theme.h` — change `ACTIVE_THEME` to switch.
- **Ghost segments** render as a solid light color rather than 8 % alpha.
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
  location, every 30 minutes; last reading is persisted on the watch.
  Conditions map to sun / cloud / rain / moon icons.
- **City tag** is the `CITY_TAG` define in `src/c/main.c` (default `SF`).

## Building

Requires the [Core Devices pebble-tool](https://github.com/coredevices) ≥ 5.0
with SDK core 4.4 and an ARM toolchain:

```sh
pip install pebble-tool   # or: uv tool install pebble-tool
pebble sdk install latest # needs access to sdk.repebble.com
pebble build
```

The built `build/watchface-emery.pbw` (a copy is committed in `dist/`) can
be sideloaded with `pebble install --phone <ip>` or through the Pebble
mobile app. Note the current SDK's QEMU cannot emulate emery yet
("Robert platform not yet ported"), so testing needs real hardware or a
newer emulator.

Toolchain compatibility flags for GCC 14 (builtin `__FILE_NAME__`,
`strftime` prototype, `time_t` via `sys/types.h`) live in `wscript`.

## Fonts

- [DSEG7 Classic Bold](https://github.com/keshikan/DSEG) v0.46 — digits
  (40 px time, 18 px seconds). SIL OFL 1.1, see
  `resources/fonts/DSEG-LICENSE.txt`.
- [Share Tech Mono](https://fonts.google.com/specimen/Share+Tech+Mono) —
  labels (18/11/8 px). SIL OFL 1.1, see
  `resources/fonts/ShareTechMono-OFL.txt`.
