# Shared kit migration — 20 October 2026

Migrated to [pebble-watchface-kit](https://github.com/LiamCordelle/pebble-watchface-kit)
2.0.0, revision `42dd429169de83fcd45821b425ab8bd4bc759d81`.

The copied kit implementations and generic regression tests now live in that
repository. C and phone code paths link into the `shared-kit` submodule;
settings headings/namespaces and optional UI are consumer configuration.
The UUID, message-key numbering, phone storage keys and persisted settings/weather
formats are retained. The watchface version is unchanged by this source migration.

Validation:

- Shared native health/history/protocol/DST tests and phone configuration/bootstrap
  tests pass; timezone coverage includes 143,192 independent comparisons.
- Real Git fixtures verify automatic latest-kit updates, preservation of dirty
  edits/unpublished commits, and stopping after fetch failure.
- Release tests verify live defaults, clean-before-build ordering and a receipt
  containing the exact kit revision and PBW SHA-256.
- Both consumers pass isolated candidate-kit SDK builds and render coverage.
- All 36 Mondrian and 62 Underground render scenes match their pre-migration
  images pixel for pixel.
- The complete `npm run release -- --render` command succeeds against the public
  kit repository for each face, producing a checked PBW and provenance receipt.
- A fresh temporary clone of each staged migration fetches its recorded kit
  from GitHub; source links resolve and consumer configuration checks pass.

The per-face QEMU results below use a temporary clean Emery profile, verify the
active app UUID, exercise the recorded-minute history scan, capture face/detail/
timeout views and restore the existing SDK profile before stopping the emulator.
Artifacts are local build outputs, not new store screenshots. No watchface store
release is published by this migration. Signal Deck has not been migrated.

Mondrian QEMU: history scan completed without app faults; frames 4–13ms.
Screenshots and log: `build/kit-migration/`.

## 8,000-step default — 22 October 2026

Updated to shared kit 2.1.2 (`71a935d`). Fresh watch/phone settings and
face fallbacks use 8,000 steps. Valid saved goals are preserved. Protocol keys,
persistence layouts and explicit screenshot fixtures are unchanged. Shared and
consumer tests, all five render suites and fresh-profile QEMU checks cover the
new default and a saved 12,500-step goal across explicit stop/start.

## Shared behaviour extraction — kit 2.2.0

Moon/live-data assembly, temperature/event clocks, bounded forecast queries,
daylight state and activity estimates now use the shared kit. Drawing and
scenario fixtures remain here. Underground Fahrenheit now rounds consistently;
unknown daylight stays unknown and activity math uses 64-bit intermediates.
The thin tools/emulator_smoke.py entrypoint uses shared profile/transport/
fixture/log utilities. Existing face-specific visual checkers remain available.

Final dependency: `c737afcfadf8cdd62dec58941cf8718fed0d6017` (kit 2.2.0). Shared/native/phone
tests, all 271 cross-consumer scenes and the local shared emulator smoke pass.
Runtime evidence is in shots/emulator-smoke/result.json; fresh 8,000 and saved
12,500 goals, cold weather-cache restoration, detail timeout and clean exits
are verified. The original SDK profile was restored.
