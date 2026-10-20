# Changelog

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
