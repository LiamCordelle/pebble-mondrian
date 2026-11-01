# Changelog

## 1.1.0 — 1 November 2026

- Add a searchable second-timezone picker for all 597 IANA identifiers and aliases,
  including Mumbai, Abu Dhabi and Moscow. The main and flick clocks show the
  selected location with automatic offline daylight-saving transitions.
- Include half-hour and quarter-hour offsets. Keep existing timezone selections,
  saved step goals and message keys intact.
- Start with common choices; search by city, region or IANA name, then select
  a match. Preserve the selected timezone while filtering or clearing search.
- Use shared kit 2.4.1 with bundled IANA tzdata 2026f.

## 1.0.10 — 1 November 2026

- Show the configured second timezone and its label in the flick-view clock.
- Fill the step counter blue to 100%, yellow over blue to 200%, green over yellow
  to 300%, then repeat the palette for further goals. Keep text readable on
  each colour, and retain the total percentage in compact captions.
- Use shared kit 2.3.0 for overflow-safe repeated-goal progress. Existing saved
  step goals and timezone settings are preserved.

## 1.0.7 — 20 October 2026

- Build from the shared watchface kit 2.0.0 so future shared improvements are
  picked up by release builds. Preserve the existing appearance, settings,
  weather reliability and recorded-minute heart-rate history behaviour.

## 1.0.6 — 20 October 2026

- Reconstruct today's detail HR range from recorded minute BPMs;
  read history in bounded batches, cache completed minutes and flag partial
  coverage with `BPM*`. Remove the hourly-average range fallback.

## 1.0.5 — 19 October 2026

- Add a native 25x25 red, blue and yellow De Stijl grid icon to the watch's
  Watchfaces list.
  Regenerate it with `python3 tools/create_menu_icon.py` (requires Pillow).
