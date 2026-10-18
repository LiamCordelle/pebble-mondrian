# 05 · Mondrian

## Pitch

A De Stijl painting where every rectangle is a metric. A black 6 px grid holds
white, red, yellow, blue and grey planes, and your day repaints it: the blue
plane rises as you walk, the grey plane shrinks as the battery drains, and the
yellow sun cell turns black after sunset. The grid is 6 px because LECO 42's
stems are 6 px, so the clock looks built from the same bars as the painting.

## Composition (200 x 228)

```
x 0                 137|144        199
  +-------------------+-------------+  y 0
  |                   | SAT     OCT |  date (white, 44 px)
  |      10:09        |=============|
  |   (time plane)    | BPM   (red) |  heart (52 px)
  |          NZ 22:09 |  64         |
  +===================+=============+  y 102..107 full-width line
  | RAIN 20%   |  STEPS 68%         |  steps remainder (white)
  | 14°  [wx]  |=====(moves)========|
  |============|                    |
  | SET   UV 3 |   (blue, rising)   |
  | 18:03 (yel)|                    |
  |============|                    |
  | BAT 76%|   |  6842              |
  +------------+--------------------+  y 227
x 0         79|86                 199
```

Every cell follows one rule: a Silkscreen 8 label (or two, flush left and flush
right) at the top, a LECO value at the bottom, and a 6 px inset on every side.
Label and number ink share an exact left edge (checked by pixel measurement).

## Metric → element

| Metric | Element |
|---|---|
| Time | Largest white plane. LECO 42, HH and MM drawn separately with a hand-placed colon of two 6x6 squares; centred to the pixel. |
| NZ time | The painting's "signature": a Silkscreen caption in the time plane's bottom-right corner, like Mondrian's "PM 30". |
| Date | White cell: `SAT` left, `OCT` right, `17` in LECO 26. |
| Heart rate | Red plane: `BPM` + value in white. 3 digits up to `199` fit LECO 26 (`148` in stress); `200`+ falls back to LECO 20. |
| Weather | White cell: `RAIN 20%` label, temperature with a hollow-square degree sign, rectangle-only condition icon. |
| Sun | **Data-driven colour:** yellow plane while the sun is up (`SET` + `UV n` + sunset time), black plane after sunset (`RISE` + sunrise time), white when unknown. |
| Battery | **Data-driven plane:** grey fill shrinking with charge, a black divider and a white remainder; red below 10%. Label `BAT 76%` / `CHG 100%`, falling back to `76%` when the full label fits neither part. |
| Steps | **Data-driven line:** the blue plane rises from the bottom of the bottom-right plane to the goal; the white remainder above it is what's left to walk. `STEPS 68%` label + count. Full blue at or above the goal. |

### Data-driven geometry rules (so it always stays balanced)

- `split_len()` never draws slivers: colour under 4 px grows to 4 px, and a
  remainder under 4 px snaps to full (no divider).
- The steps divider **snaps onto the left column's grid lines** when it lands
  within 6 px, so it continues an existing line instead of near-missing it.
- Steps text moves with the line: the label sits in the white remainder until
  it gets too thin, then moves into the blue. The count sits at the foot of the
  blue until the blue is too short, then sits just above the divider in black.
- Steps split vertically (rising) on purpose. A left-to-right split of the
  114 px plane has a dead zone around 31–63% where a 61 px number fits neither
  part. The rising split always has the full width.

## Detail view (tap for 8 s)

The same painting, re-divided. The face's full-width line stays at y=102, so
the steps plane below it becomes the week. Above it, the planes split into what
the face drops. Rendered with `tools/render.py --detail` (`shots/detail*.png`).

```
x 0          71|78        145|152      199
  +------------+-------------+-----------+  y 0
  | BT ON      | RAIN 60%    | HI 17     |  time · rain (blue) · hi/lo (yellow)
  | 10:09      | 15:00       | LO 9      |
  +============+=============+===========+  y 56..61
  |    |####### RED ########|           |  heart band: edges at min/max bpm
  |    | BPM 52       141   |           |
  +==================+=====+===========+  y 102..107 (the face's line)
  |    |  |    |  |  |    |  |         |  week: seven rising planes,
  |    |  |    |  |  |    | ## today   |  today in red
  | S  |M |  T |W |T | F  | S          |
  +--------------------------------------+  y 227
```

| Plane | Shows |
|---|---|
| Time cell (white) | `BT ON` / `BT OFF`, then the time in LECO 20. |
| Rain cell | The highest rain chance left today and its hour (`RAIN 60%` / `15:00`). **Data-driven colour:** blue when rain is likely (50% or more), white otherwise. |
| Hi/lo cell (yellow) | Read like a thermometer: `HI 17` at the top edge, `LO 9` at the bottom. |
| Heart band | Today's range as a red plane whose edges are grid lines at the lowest and highest bpm on a fixed 20–220 axis (exactly 1 px per bpm). `BPM`, the low and the high sit inside. A narrow range (e.g. 55–70 just after waking) moves to `55-70` in the wider white plane. White planes under 10 px join the red. Falls back to the range of `hr_by_hour` when `hr_min`/`hr_max` are unknown. |
| Week | Seven equal planes, oldest on the left, each rising blue to that day's share of the goal. Full means the goal was reached. A day short of the goal always keeps at least 4 px of white, so 98.7% never passes for a goal. Today is red. Day initials at the foot, white on colour or black on white when the plane is too low. |

Notes:
- Data positions real grid lines (the heart band's edges, the week's dividers),
  so each tap shows a slightly different Mondrian, and it still reads as one.
- The rain cell first used a rising fill like the face's steps. In a 56 px cell
  the divider collided with the label or value at several levels, so it uses a
  colour state instead (same idea as the face's day/night sun cell).
- Empty data is calm: a blank week grid, a white heart band with `BPM --`,
  `RAIN --`, `HI --` / `LO --` and `BT OFF`.
- Integer rectangles only, AA off, so no anti-aliasing edges to inspect.

## Palette

| Role | Specified | After colour correction |
|---|---|---|
| Lines, night sun cell | `000000` | `000000` |
| Planes, paper | `FFFFFF` | `FFFFFF` |
| Red: heart, critical battery | `FF0000` | `E35462` |
| Blue: steps, rain in icon | `0000FF` | `0068CA` |
| Yellow: day sun cell, icon sun | `FFFF00` | `FFEEAB` |
| Grey: battery | `AAAAAA` | `ABABAB` |

## Fonts

- System `LECO_42_NUMBERS` for the time.
- System `LECO_26_BOLD_NUMBERS_AM_PM` for all cell values.
- System `LECO_20_BOLD_NUMBERS` as a fallback for wide values (`-12°`, 200+ bpm)
  and for every value in the detail view.
- `lab_font_pixel8()` (Silkscreen Bold 8) for all labels.
- No bundled fonts or other resources added. LECO's rectangular glyphs echo
  Van Doesburg's rectilinear De Stijl lettering.

## Emery rendering lessons

- **Measured side bearings** (calibration shot): LECO 42 digits have a 3 px left
  bearing (`7` has 1). LECO 26 has 2 (`1` and `7` have 1), LECO 20 has 1, and
  Silkscreen 8 has 1. Right bearings: LECO 42 3 (`1` 1), LECO 26 1, LECO 20 2
  (`1`/`7` 1). `graphics_text_layout_get_content_size` returns the advance sum,
  so ink width = advance − left bearing of the first glyph − right bearing of the last.
  Positioning by ink, not by box, is what makes labels and numbers line up exactly.
- **Glyph top offsets** (box top → ink top): LECO 42 13 px, LECO 26 8, LECO 20 6,
  Silkscreen 8 3 (caps are 5 px tall). Digit heights: 29 / 18 / 14.
- **LECO has `%` and `-` at every size** (verified for 20/26/42). The `%` has a
  stair-step diagonal.
- **`--:--` in LECO 26 looks like a division sign** (the colon dots straddle the
  dashes). Use `--` as the time placeholder.
- **Corrected reds:** `F00` → coral `E35462` still reads as Mondrian red. `F50` reads
  salmon, `A00` maroon, `F05` pink (compared in place on the screenshot).
- **Yellow only survives as a plane.** `FFFF00` → `FFEEAB` disappears as a thin
  strip but reads as yellow in a large cell bounded by black lines. That is why
  yellow became the 80x43 sun cell rather than the battery strip.
- `AAAAAA` grey is a good neutral plane (Mondrian's 1920s greys) with black text.
- White Silkscreen 8 on the corrected red is legible at 1x (about 3.7:1).

## Ideas worth stealing

1. **Grid weight = type weight.** Match line width to the clock font's stem
   (6 px for LECO 42) and draw the colon as stem-sized squares.
2. **Remainder as metric.** A plane split by a moving black line: colour is
   done, white is what's left. Add sliver rules and snap the line to nearby grid
   lines so data never makes the composition look accidental.
3. **Day/night cell.** One cell whose colour is the sky (yellow by day, black
   by night) and whose label is the next sun event.
4. **Second time zone as a signature** tucked into the hero plane's corner.
5. **Range plane** (detail view): a coloured plane whose edges are grid lines
   at today's min and max on a fixed 1 px-per-unit axis. A range becomes part
   of the composition, not a chart.

## Known limitations and next steps

- The yellow is pale on Emery by nature. Check it on a real watch in daylight.
- The face shows the current hour's rain only; the peak, hi/lo and Bluetooth
  live in the detail view. UV shows by day only, as an integer.
- Signal Deck's amber battery state (<40%) isn't shown: grey until 10%, then red.
- Three-character temperatures (`-12`) drop to LECO 20 so they clear the icon.
- `SET` / `RISE` are abbreviated so `UV n` doesn't read as part of the label.
- 12h mode logic is in place but not screenshot-tested (face or detail).
- Detail view: no distance, active minutes, sleep or "vs usual" yet. Today's
  red week column could carry `+1722 vs usual` if it earns its place.
- Next: let the user choose which metric each cell holds, and consider
  snapping the battery divider too (it currently has no nearby lines).
