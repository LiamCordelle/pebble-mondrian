# Watchface kit 1.0

Established by `pebble-mondrian` on 18 October 2026. No earlier sibling kit
existed. This is a copied-source kit, with no runtime dependency on the lab,
Signal Deck, another watchface, or a server of our own.

## Integration

Copy `src/c/kit/` and `src/pkjs/kit/` into the next face. Keep the lab contract
(`lab.h`, `lab_common.c`, scenario definitions in `lab_data.c`, `lab_designs.h`).
Kit additions to `LabData` must be copied too: temperature unit, optional label
size, tomorrow/next sun fields, maximum UV, rolling temperature/rain/code,
rolling local hour/day labels and `rolling_count`.

`lab_main.c` calls `kit_shell_run(&your_design)`. The live branch of
`lab_data_refresh` sets local time/device state and calls `kit_fill`; fixed lab
scenarios bypass the kit. Use `src/pkjs/index.js` as the small app bootstrap.
It wires Clay and the kit modules; change the app heading and storage namespace.
Add `@rebble/clay`, health/location/configurable capabilities and the message keys
below to package.json. Build live by default (`LAB_SCENARIO=3`).

The host renderer defines `LAB_HOST_RENDER`, compiles C recursively and excludes
`lab_main.c`. All watch-only kit modules are guarded accordingly. `kit_core.c`
is pure C and also runs in native tests. Keep `LabDesign.draw`/`draw_detail`
unchanged. No button handlers are needed for a watchface.

## Files

| File | Responsibility |
|---|---|
| `kit.h` | watch-side integration API and persisted settings |
| `kit.c` | single AppMessage dispatcher; registers before opening buffers |
| `kit_core.c/.h` | versioned weather decoder, validation, calendar, DST and retry state |
| `kit_health.c` | permission-aware health, event coalescing, daily/hourly history caches |
| `kit_settings.c` | staged settings validation, persistence and world-clock projection |
| `kit_weather.c` | request correlation, watchdog/retries, cache and LabData projection |
| `kit_shell.c` | window, minute tick, touch/flick detail view and frame timing |
| `pkjs/kit/config.js` | Clay schema, including optional larger captions |
| `pkjs/kit/settings.js` | settings normalization and phone storage |
| `pkjs/kit/queue.js` | one native acknowledged send at a time, FIFO, three attempts |
| `pkjs/kit/weather.js` | serialized geolocation/HTTP, Open-Meteo validation and wire encoding |

## Defaults and keys

Watch persistent key **1000**: 12-byte versioned settings. Key **1001**: 204-byte
weather wire record. Both are private to each app UUID. Preserve these within an
app; bump the record version before changing layouts.

Settings are one complete dictionary: `TIME_MODE` (0 watch, 1 twelve-hour,
2 twenty-four-hour), `TEMP_UNIT` (0 C, 1 F), `STEP_GOAL` (1000–50000, default
10000), `WORLD` (0 NZ, 1 UTC, 2 LON, 3 NYC, 4 LA, 5 TKY, 6 SYD),
`WEATHER_ENABLED` (default 1), `DEMO_FALLBACK` (default 0), `LARGE_LABELS`
(default 0 for Mondrian, preserving Liam's preferred original artwork).
Partial/invalid dictionaries are ignored. Settings apply immediately and persist.
The phone sends this snapshot with `READY=1` before weather work.

Watch → phone: `REQUEST_WEATHER` is a positive request ID. Phone → watch:
`WEATHER_ID` plus either `WEATHER` (byte array) or `WEATHER_ERROR=1`.
Only replies for the active request can affect the watch. AppMessage buffers are
512 bytes inbound and 64 outbound; weather and settings are separate messages.
SDK message keys are extern values, so arrays of them cannot be static C
initializers. Clean/rebuild when the SDK caches an old message-key schema.

## Weather format and reliability

All multibyte values are little endian. The 204-byte record fits Pebble's
256-byte limit per persisted value. It is decoded into a temporary snapshot
and committed only after every field and the FNV-1a checksum pass.

| Bytes | Contents |
|---|---|
| 0–3 | `MK`, version 1, hourly count (24–48) |
| 4–11 | observation epoch, hourly start epoch |
| 12–19 | two local forecast dates, YYYYMMDD |
| 20–35 | day 0 rise/set, day 1 rise/set, UTC epochs |
| 36–44 | signed Celsius temperature, WMO code, humidity, UV×10, wind km/h, direction |
| 45–52 | two signed daily highs/lows, two maximum UV×10 values |
| 53–100 | 48 signed Celsius hourly temperatures |
| 101–148 | 48 hourly rain percentages |
| 149–196 | 48 hourly WMO codes |
| 197–199 | reserved, zero |
| 200–203 | FNV-1a of bytes 0–199 |

Open-Meteo requests `forecast_days=2`, `timezone=auto`, `timeformat=unixtime`.
Current UV comes from the corresponding hourly UV sample; maximum UV comes from
the daily response. Nulls are rejected rather than converted into zero. Polar
sun events may be an explicit pair of zero values. Dates apply the API's UTC
offset as documented; sun/hourly timestamps remain UTC.

The watch alone schedules: startup after settings (five-second fallback), then
30 minutes after success. One 45-second watchdog covers location/HTTP/delivery.
Failures retry after 1, 5 and 15 minutes; a fourth failure returns to 30 minutes.
Each timer/request is cancelled or made irrelevant when weather is disabled.
The phone uses 15-second location and HTTP limits. Native AppMessage ACK/NACK,
not a JS timeout, ends a send; otherwise retries can overlap a still-active send.

The last valid observation persists through failure/restart for at most three
hours. Expiry clears every projected weather field together. At midnight, daily
high/low/maximum UV switch to day 1; future sun epochs select tomorrow's sunrise.
Rolling arrays use actual chronological hours. `rolling_count` can fall below 24
near the end of an older cache, so never wrap or repeat missing forecast data.
Use `rolling_hour[i]`/`rolling_day[i]` for labels across DST transitions.

## Health, timing and touch

Health subscriptions never request a faster heart-rate sampling period. Movement,
heart-rate and sleep events coalesce to one sample per minute; no health event
marks the canvas dirty. A significant update invalidates cached historical days.
Completed hourly bins refresh once an hour; current-hour values refresh once a
minute. Permission loss clears data, and demo fallback is explicitly opt-in.

Pebble `mktime` does not resolve `tm_isdst=-1`: it uses the supplied UTC offset.
Historical local boundaries therefore resolve the target offset with `localtime`
instead of reusing today's `tm_gmtoff`. Repeated hours share their wall-hour bin;
skipped boundaries normalize forward. The kit's world clock uses UTC transition
instants for current NZ/LON/US/SYD rules, not date-only DST flags.

Minute ticks only; no animation. Touch or wrist flick toggles detail, with an
eight-second timeout. Default touch subscription is continuous to preserve direct
tap behavior. `KIT_TOUCH_WINDOW_MS=10000 pebble build` provides an A/B hardware
build subscribing for ten seconds after startup/flick. **Touch power cost has not
been measured on a physical watch.** Decide between these policies using real
hardware battery measurements, not QEMU CPU usage.

The shell logs `Frame face/detail Nms`. Emulator results and remaining hardware
checks are recorded in the app README. No static bitmap cache is needed for this
rectangle-only design unless physical-watch timing demonstrates otherwise.

Battery/connection callbacks must apply their supplied state after refreshing
other data. A `*_peek()` made inside the callback can still report the previous
state, making exception indicators lag one event. The final QEMU checks exposed
this; `kit_shell.c` uses callback values directly.
ConnectionService also deliberately hides disconnects for 25 seconds. Wait at
least 27 seconds in QEMU before checking the phone-lost marker; reconnecting
earlier cancels the notification. This is firmware debounce, not a rendering lag.

## Verification and changelog

1.0: first kit, established in Mondrian. Tests are `python3 tests/run.py`:
cross-language weather encoding/decoding, atomic malformed-payload rejection,
expiry boundaries, retry/watchdog scheduling, phone serialization, persisted
settings normalization and exact DST transition tests. Independent Python
zoneinfo comparisons cover every six hours in 2024–2037 for all seven presets.
`tools/render_scenes.py` covers layout/units/status/weather variants without QEMU.

Final Emery check passed: live settings via the Clay response path, correlated
weather transport/cache, restart, injected steps/heart sample, detail toggle and
eight-second timeout, amber/charging states. Observed draw times were 2–11 ms;
the exit log reported zero remaining allocations. Test logs are under `shots/`.
The default demo is pixel-identical to the original proposal. Liam preferred
that composition to the readability redesign, so larger captions are optional.
The final battery/charging/phone-lost regions also match their host-rendered
counterparts pixel for pixel. `tools/verify_emulator.py` verifies those captures,
restored connectivity and the weather cache after restart without another reply.

Emulator gotcha: a running C face can have stale phonesim lifecycle state even
when installation succeeds. Sending `AppRunStateRequest` refreshes it and starts
its companion JS. A synthetic config response also requires `AppConfigSetup`
first, so phonesim has a pending configuration callback. The test harness uses
both handshakes. It restores the original settings and stops its log process.
Phonesim's native GeoIP lookup can block configuration callbacks in this SDK;
the injection test keeps phone weather disabled until restoring defaults, while
exercising the watch request/response path separately. It injects only the first
weather reply, so the restarted face must use its persisted cache.

Future faces: copy the highest-version sibling kit, update only your copy, bump
this version and record fixes here. Keep Signal Deck read-only.

Release baseline: Mondrian 1.0.2 is the initial public release with corrected
package metadata. Publishing metadata changes leave the copied-source kit at
version 1.0.

Release gotcha: after changing package.json version, use a clean build before
uploading and inspect appinfo.json inside the PBW. An incremental SDK build
can regenerate build/appinfo.json while leaving the previous PBW unchanged.
