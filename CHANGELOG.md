# Changelog

## 1.0.6 — 20 October 2026

- Reconstruct today's detail HR range from recorded minute BPMs;
  read history in bounded batches, cache completed minutes and flag partial
  coverage with `BPM*`. Remove the hourly-average range fallback.

## 1.0.5 — 19 October 2026

- Add a native 25x25 red, blue and yellow De Stijl grid icon to the watch's
  Watchfaces list.
  Regenerate it with `python3 tools/create_menu_icon.py` (requires Pillow).
