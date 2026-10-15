# Changelog

## Unreleased

### Added
- **Henry's Theme** — Mono's white face with a colored (green land / blue sea) map; brighter than the tan Positive face on dim panels.
- **Sunrise / sunset theme switching** — new "Auto: day / night at sunrise / sunset" mode. Times are calculated on the watch from the home latitude/longitude, no network needed.
- **Set-time theme switching** now uses a time picker (to the minute) instead of an hour slider. Defaults stay 07:00 / 20:00.

### Changed
- Settings page shows only the options relevant to the selected theme mode.
- Saved day/night times are now stored as minutes since midnight, so watches reset to default settings once after updating.

### Fixed
- Sunrise/sunset mode was being reset to single-theme mode, and set times wrapped past 00:23 (`settings.c`).
