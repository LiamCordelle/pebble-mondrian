#include "lab.h"

// Mondrian: a De Stijl composition where every rectangle is a metric.
//
// The grid lines are 6 px, the stem width of LECO 42, so the clock is built
// from the same black bars as the painting. Two lines are driven by data:
//   - steps: the blue plane rises through the bottom-right plane; the white
//     remainder above it is the distance still to walk.
//   - battery: the grey plane in the bottom-left corner shrinks with charge
//     (red below 10%).
// The sun cell is yellow while the sun is up and black after sunset.

#define LW 6    // grid line width
#define PAD 6   // content inset inside every cell

#define COL_RED GColorFromHEX(0xff0000)
#define COL_BLUE GColorFromHEX(0x0000ff)
#define COL_YELLOW GColorFromHEX(0xffff00)
#define COL_GREY GColorFromHEX(0xaaaaaa)

// --- composition (200 x 228) --------------------------------------------------

#define TOP_H 102                   // top band height
#define MID_Y TOP_H                 // full-width line under the top band
#define BOT_Y (TOP_H + LW)          // bottom band top (108)
#define V_TOP_X 138                 // line between time plane and right column
#define V_BOT_X 80                  // line between left column and steps plane
#define DATE_H 44                   // date cell; the red heart plane takes the rest
#define WX_H 49                     // weather cell
#define SUN_H 43                    // sun cell; the battery strip takes the rest
#define SUN_Y (BOT_Y + WX_H + LW)
#define BAT_Y (SUN_Y + SUN_H + LW)
#define RIGHT_X (V_TOP_X + LW)
#define RIGHT_W (200 - RIGHT_X)

static const GRect R_TIME = {{0, 0}, {V_TOP_X, TOP_H}};
static const GRect R_DATE = {{RIGHT_X, 0}, {RIGHT_W, DATE_H}};
static const GRect R_HEART = {{RIGHT_X, DATE_H + LW}, {RIGHT_W, TOP_H - DATE_H - LW}};
static const GRect R_WEATHER = {{0, BOT_Y}, {V_BOT_X, WX_H}};
static const GRect R_SUN = {{0, SUN_Y}, {V_BOT_X, SUN_H}};
static const GRect R_BATTERY = {{0, BAT_Y}, {V_BOT_X, 228 - BAT_Y}};
static const GRect R_STEPS = {{V_BOT_X + LW, BOT_Y}, {200 - V_BOT_X - LW, 228 - BOT_Y}};

// --- type -----------------------------------------------------------------------

// A font plus the measured geometry of its glyphs: `top` is the gap from the
// text box top to the glyph tops, `h` the digit/cap height.
typedef enum { FACE_L42, FACE_L26, FACE_L20, FACE_PX8, FACE_G14, FACE_G18 } FaceId;

typedef struct {
  GFont font;
  int8_t top;
  int8_t h;
} Face;

static Face s_face[6];
static bool s_large_labels;
static FaceId caption(void) { return s_large_labels ? FACE_G14 : FACE_PX8; }
static int display_temp(const LabData *d, int c) {
  return d->temp_fahrenheit ? (c * 9 + (c >= 0 ? 2 : -2)) / 5 + 32 : c;
}

static void load(void) {
  s_face[FACE_L42] = (Face){fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS), 13, 29};
  s_face[FACE_L26] = (Face){fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM), 8, 18};
  s_face[FACE_L20] = (Face){fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS), 6, 14};
  s_face[FACE_PX8] = (Face){NULL, 3, 5};
  s_face[FACE_G14] = (Face){fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), 5, 9};
  s_face[FACE_G18] = (Face){fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), 7, 11};
}

static GFont face_font(FaceId f) {
  return f == FACE_PX8 ? lab_font_pixel8() : s_face[f].font;
}

// Side bearings measured on Emery (glyph ink vs. its advance box).
static int bearing_left(FaceId f, char c) {
  switch (f) {
    case FACE_L42: return c == '7' ? 1 : 3;
    case FACE_L26: return (c == '1' || c == '7') ? 1 : 2;
    default: return 1;
  }
}

static int bearing_right(FaceId f, char c) {
  switch (f) {
    case FACE_L42: return c == '1' ? 1 : 3;
    case FACE_L26: return c == '-' ? 2 : 1;
    case FACE_L20: return (c == '1' || c == '7' || c == '-') ? 1 : 2;
    default: return 1;
  }
}

static int advance_w(const char *s, FaceId f) {
  return lab_text_size(s, face_font(f)).w;
}

static int ink_w(const char *s, FaceId f) {
  size_t n = strlen(s);
  if (n == 0) {
    return 0;
  }
  return advance_w(s, f) - bearing_left(f, s[0]) - bearing_right(f, s[n - 1]);
}

// Draws text with its ink's top-left corner at (x, y).
static void ink_text(GContext *ctx, const char *s, FaceId f, int x, int y, GColor c) {
  int bx = x - bearing_left(f, s[0]);
  GRect box = GRect(bx, y - s_face[f].top, advance_w(s, f) + 8, s_face[f].top + 40);
  lab_text(ctx, s, face_font(f), box, GTextAlignmentLeft, c);
}

static void fill(GContext *ctx, GRect r, GColor c) {
  if (r.size.w <= 0 || r.size.h <= 0) {
    return;
  }
  graphics_context_set_fill_color(ctx, c);
  graphics_fill_rect(ctx, r, 0, GCornerNone);
}

// Cell furniture: a small caps label at the top (optionally a second one
// flush right), a LECO value at the bottom.
static void cell_label(GContext *ctx, GRect c, const char *s, GColor ink) {
  ink_text(ctx, s, caption(), c.origin.x + PAD, c.origin.y + PAD, ink);
}

static void cell_label_right(GContext *ctx, GRect c, const char *s, GColor ink) {
  int x = c.origin.x + c.size.w - PAD - ink_w(s, caption());
  ink_text(ctx, s, caption(), x, c.origin.y + PAD, ink);
}

static void cell_value(GContext *ctx, GRect c, const char *s, FaceId f, GColor ink) {
  ink_text(ctx, s, f, c.origin.x + PAD, c.origin.y + c.size.h - PAD - s_face[f].h, ink);
}

// A De Stijl degree sign: a small hollow square.
static void degree(GContext *ctx, int x, int y, GColor ink, GColor bg) {
  fill(ctx, GRect(x, y, 6, 6), ink);
  fill(ctx, GRect(x + 2, y + 2, 2, 2), bg);
}

// --- weather icons: rectangles only, 20 x 16 (partly cloudy overhangs 2 px
// to the left) ------------------------------------------------------------------

typedef struct {
  int8_t x, y, w, h;
  uint8_t ink;  // 0 black, 1 white, 2 yellow, 3 blue
} IconRect;

// A stepped cloud with a 1 px white halo, so it reads cleanly over a sun.
static const IconRect ICON_CLOUD[] = {
  {-1, 3, 20, 10, 1}, {3, -1, 11, 7, 1},
  {0, 4, 18, 8, 0}, {4, 0, 9, 6, 0}, {2, 6, 14, 4, 1}, {6, 2, 5, 4, 1},
};

static GColor icon_color(uint8_t ink) {
  switch (ink) {
    case 1: return GColorWhite;
    case 2: return COL_YELLOW;
    case 3: return COL_BLUE;
    default: return GColorBlack;
  }
}

static void icon_rects(GContext *ctx, GPoint o, const IconRect *r, int n) {
  for (int i = 0; i < n; i++) {
    fill(ctx, GRect(o.x + r[i].x, o.y + r[i].y, r[i].w, r[i].h), icon_color(r[i].ink));
  }
}

static void draw_icon(GContext *ctx, LabWxKind kind, GPoint o) {
  static const IconRect SUN[] = {
    {5, 3, 10, 10, 0}, {7, 5, 6, 6, 2},
    {9, 0, 2, 2, 0}, {9, 14, 2, 2, 0}, {2, 7, 2, 2, 0}, {16, 7, 2, 2, 0},
  };
  static const IconRect PARTLY_SUN[] = {{8, 0, 12, 12, 0}, {10, 2, 8, 8, 2}};
  static const IconRect RAIN[] = {{3, 13, 2, 3, 3}, {8, 13, 2, 3, 3}, {13, 13, 2, 3, 3}};
  static const IconRect SNOW[] = {{3, 13, 2, 2, 0}, {8, 14, 2, 2, 0}, {13, 13, 2, 2, 0}};
  static const IconRect BOLT[] = {{9, 12, 2, 2, 0}, {7, 14, 2, 2, 0}};
  static const IconRect FOG[] = {{0, 3, 20, 2, 0}, {2, 7, 16, 2, 0}, {0, 11, 20, 2, 0}};
  const int n_cloud = (int)ARRAY_LENGTH(ICON_CLOUD);
  switch (kind) {
    case LabWxClear:
      icon_rects(ctx, o, SUN, ARRAY_LENGTH(SUN));
      break;
    case LabWxPartly:
      icon_rects(ctx, o, PARTLY_SUN, ARRAY_LENGTH(PARTLY_SUN));
      icon_rects(ctx, GPoint(o.x - 2, o.y + 4), ICON_CLOUD, n_cloud);
      break;
    case LabWxCloudy:
      icon_rects(ctx, GPoint(o.x + 1, o.y + 2), ICON_CLOUD, n_cloud);
      break;
    case LabWxRain:
      icon_rects(ctx, GPoint(o.x + 1, o.y), ICON_CLOUD, n_cloud);
      icon_rects(ctx, o, RAIN, ARRAY_LENGTH(RAIN));
      break;
    case LabWxSnow:
      icon_rects(ctx, GPoint(o.x + 1, o.y), ICON_CLOUD, n_cloud);
      icon_rects(ctx, o, SNOW, ARRAY_LENGTH(SNOW));
      break;
    case LabWxStorm:
      icon_rects(ctx, GPoint(o.x + 1, o.y), ICON_CLOUD, n_cloud);
      icon_rects(ctx, o, BOLT, ARRAY_LENGTH(BOLT));
      break;
    case LabWxFog:
      icon_rects(ctx, o, FOG, ARRAY_LENGTH(FOG));
      break;
    default:
      break;
  }
}

// --- data-driven split ------------------------------------------------------

// Splits a plane of length `len` at `permille` (0..1000). Returns the length
// of the coloured part; *divider is true when a black line separates it from
// the white remainder. Slivers never appear: a sliver of colour grows to 4 px
// and a remainder thinner than 4 px snaps to full.
static int split_len(int len, int permille, bool *divider) {
  permille = lab_clamp(permille, 0, 1000);
  int n = (len * permille + 500) / 1000;
  *divider = false;
  if (permille == 0) {
    return 0;
  }
  if (n >= len - LW - 4) {
    return len;
  }
  *divider = true;
  return n < 4 ? 4 : n;
}

// --- cells --------------------------------------------------------------------

static void draw_time(GContext *ctx, const LabData *d) {
  char hh[4];
  char mm[4];
  int hour = d->now.tm_hour;
  if (!d->is_24h) {
    hour %= 12;
    if (hour == 0) {
      hour = 12;
    }
  }
  snprintf(hh, sizeof(hh), d->is_24h ? "%02d" : "%d", hour);
  snprintf(mm, sizeof(mm), "%02d", d->now.tm_min);
  const int gap = 7;  // ink gap either side of the colon
  const int dot = 6;  // colon squares match the 6 px stems
  int wh = ink_w(hh, FACE_L42);
  int wm = ink_w(mm, FACE_L42);
  int total = wh + gap + dot + gap + wm;
  int x = R_TIME.origin.x + (R_TIME.size.w - total) / 2;
  int y = R_TIME.origin.y + (R_TIME.size.h - s_face[FACE_L42].h) / 2;
  ink_text(ctx, hh, FACE_L42, x, y, GColorBlack);
  int cx = x + wh + gap;
  fill(ctx, GRect(cx, y + 5, dot, dot), GColorBlack);
  fill(ctx, GRect(cx, y + 18, dot, dot), GColorBlack);
  ink_text(ctx, mm, FACE_L42, cx + dot + gap, y, GColorBlack);

  // Second time zone, signed in the corner like Mondrian's "PM".
  char t[8];
  char nz[16];
  lab_fmt_time(t, sizeof(t), d->world_hour, d->world_min, d->is_24h);
  snprintf(nz, sizeof(nz), "%s %s", d->world_label, t);
  int right = R_TIME.origin.x + R_TIME.size.w - PAD;
  FaceId nz_font = s_large_labels ? FACE_G18 : FACE_PX8;
  ink_text(ctx, nz, nz_font, right - ink_w(nz, nz_font),
           R_TIME.origin.y + R_TIME.size.h - PAD - s_face[nz_font].h, GColorBlack);
  if (!d->bt_connected) {
    // Exception-only phone plane; the calm clock plane stays open when connected.
    fill(ctx, GRect(PAD, R_TIME.size.h - 23, 17, 17), GColorBlack);
    fill(ctx, GRect(PAD + 5, R_TIME.size.h - 20, 7, 11), GColorWhite);
    fill(ctx, GRect(PAD + 6, R_TIME.size.h - 19, 5, 7), GColorBlack);
    fill(ctx, GRect(PAD + 2, R_TIME.size.h - 15, 13, 3), COL_RED);
  }
}

static void draw_date(GContext *ctx, const LabData *d) {
  char day[4];
  snprintf(day, sizeof(day), "%s", lab_day_abbr(d->now.tm_wday));
  lab_upper(day);
  char mon[4];
  snprintf(mon, sizeof(mon), "%s", lab_month_abbr(d->now.tm_mon));
  lab_upper(mon);
  if (s_large_labels) {
    ink_text(ctx, day, FACE_G14, R_DATE.origin.x + 2, PAD, GColorBlack);
    ink_text(ctx, mon, FACE_G14, R_DATE.origin.x + R_DATE.size.w - 2 - ink_w(mon, FACE_G14), PAD, GColorBlack);
  } else {
    cell_label(ctx, R_DATE, day, GColorBlack);
    cell_label_right(ctx, R_DATE, mon, GColorBlack);
  }
  char num[12];
  snprintf(num, sizeof(num), "%d", d->now.tm_mday);
  cell_value(ctx, R_DATE, num, FACE_L26, GColorBlack);
}

static void draw_heart(GContext *ctx, const LabData *d) {
  GRect c = R_HEART;
  fill(ctx, c, COL_RED);
  cell_label(ctx, c, "BPM", GColorWhite);
  char v[12];
  if (d->hr_ok) {
    snprintf(v, sizeof(v), "%d", d->heart_rate);
  } else {
    snprintf(v, sizeof(v), "--");
  }
  FaceId f = ink_w(v, FACE_L26) <= c.size.w - 2 * PAD ? FACE_L26 : FACE_L20;
  cell_value(ctx, c, v, f, GColorWhite);
}

static void draw_weather(GContext *ctx, const LabData *d) {
  GRect c = R_WEATHER;
  char rain[24];
  if (d->weather_ok) {
    snprintf(rain, sizeof(rain), "%d%%", d->rain_pct);
  } else {
    snprintf(rain, sizeof(rain), "--");
  }
  if (s_large_labels) {
    cell_label(ctx, c, "RAIN", GColorBlack);
    ink_text(ctx, rain, FACE_G18, c.origin.x + c.size.w - PAD - ink_w(rain, FACE_G18),
             c.origin.y + PAD - 1, GColorBlack);
  } else {
    char line[32]; snprintf(line,sizeof(line),"RAIN %s",rain);
    cell_label(ctx,c,line,GColorBlack);
  }
  if (!d->weather_ok) {
    cell_value(ctx, c, "--", FACE_L26, GColorBlack);
    return;
  }
  char t[12];
  int temp = display_temp(d,d->temp);
  snprintf(t, sizeof(t), "%d", temp);
  const int icon_w = 20;
  // Temperature + degree square must clear the icon (and its 2 px overhang).
  const int room = c.size.w - 2 * PAD - (icon_w + 2) - 4 - 9;
  FaceId f = ink_w(t, FACE_L26) <= room ? FACE_L26 : FACE_L20;
  cell_value(ctx, c, t, f, GColorBlack);
  int vy = c.origin.y + c.size.h - PAD - s_face[f].h;
  degree(ctx, c.origin.x + PAD + ink_w(t, f) + 3, vy, GColorBlack, GColorWhite);
  draw_icon(ctx, lab_weather_kind(d->weather_code),
            GPoint(c.origin.x + c.size.w - PAD - icon_w, c.origin.y + c.size.h - PAD - 16));
}

// The sun cell is the daylight itself: a yellow plane while the sun is up, a
// black plane after sunset, white when the sun times are unknown.
static void draw_sun(GContext *ctx, const LabData *d) {
  GRect c = R_SUN;
  bool day = d->weather_ok && d->minute_of_day >= d->sunrise_min &&
             d->minute_of_day < d->sunset_min;
  GColor ink = GColorBlack;
  if (d->weather_ok) {
    fill(ctx, c, day ? COL_YELLOW : GColorBlack);
    ink = day ? GColorBlack : GColorWhite;
  }
  cell_label(ctx, c, d->weather_ok && !d->next_sun_is_set ? "RISE" : "SET", ink);
  if (s_large_labels) {
    char uv[16];
    if (d->weather_ok) snprintf(uv, sizeof(uv), "%d", (d->uv_x10 + 5) / 10);
    else snprintf(uv, sizeof(uv), "--");
    int vx = c.origin.x + c.size.w - PAD - ink_w(uv, FACE_G18);
    ink_text(ctx, "UV", FACE_G14, vx - 3 - ink_w("UV", FACE_G14), c.origin.y + PAD, ink);
    ink_text(ctx, uv, FACE_G18, vx, c.origin.y + PAD - 1, ink);
  } else if (day) {
    char uv[16]; snprintf(uv,sizeof(uv),"UV %d",(d->uv_x10+5)/10);
    cell_label_right(ctx,c,uv,ink);
  }
  char v[8];
  if (d->weather_ok && d->next_sun_min >= 0) {
    lab_fmt_minute_of_day(v, sizeof(v), d->next_sun_min, d->is_24h);
  } else {
    snprintf(v, sizeof(v), "--");
  }
  cell_value(ctx, c, v, FACE_L26, ink);
}

static void draw_battery(GContext *ctx, const LabData *d) {
  GRect c = R_BATTERY;
  bool divider;
  int w = split_len(c.size.w, d->battery_pct * 10, &divider);
  bool critical = d->battery_pct < 10;
  GColor col = d->battery_pct < 40 ? lab_battery_color(d->battery_pct) : COL_GREY;
  fill(ctx, GRect(c.origin.x, c.origin.y, w, c.size.h), col);
  if (divider) {
    fill(ctx, GRect(c.origin.x + w, c.origin.y, LW, c.size.h), GColorBlack);
  }
  // The label goes in whichever part holds it: the full "BAT 76%" where it
  // fits, otherwise just "76%" (one of the two parts always fits that).
  int rest_x = w > 0 ? c.origin.x + w + LW : c.origin.x;
  int rest_w = w > 0 ? (divider ? c.size.w - w - LW : 0) : c.size.w;
  char full[24];
  char pct[16];
  snprintf(full, sizeof(full), "%s %d%%", d->charging ? "CHG" : "BAT", d->battery_pct);
  snprintf(pct, sizeof(pct), "%d%%", d->battery_pct);
  int y = c.origin.y + (c.size.h - s_face[s_large_labels ? FACE_G18 : FACE_PX8].h) / 2;
  GColor on_fill = critical ? GColorWhite : GColorBlack;
  const char *const texts[] = {full, d->charging ? "CHG" : pct};
  for (unsigned i = 0; i < ARRAY_LENGTH(texts); i++) {
    FaceId f = s_large_labels ? (i == 0 ? FACE_G14 : FACE_G18) : FACE_PX8;
    int inset = s_large_labels ? 3 : PAD;
    int need = ink_w(texts[i], f) + 2 * inset;
    if (w >= need) {
      ink_text(ctx, texts[i], f, c.origin.x + inset, y, on_fill);
      return;
    }
    if (rest_w >= need) {
      ink_text(ctx, texts[i], f, rest_x + inset, y, GColorBlack);
      return;
    }
  }
}

static void draw_steps(GContext *ctx, const LabData *d) {
  GRect c = R_STEPS;
  int32_t goal = d->step_goal > 0 ? d->step_goal : 10000;
  int permille = d->steps_ok ? (int)((d->steps * 1000) / goal) : 0;
  bool divider;
  int h = split_len(c.size.h, permille, &divider);
  int bottom = c.origin.y + c.size.h;
  if (divider) {
    // Continue a line of the left column when the divider lands close to it.
    const int snaps[] = {SUN_Y - LW, BAT_Y - LW};
    for (unsigned i = 0; i < ARRAY_LENGTH(snaps); i++) {
      if (abs(bottom - h - LW - snaps[i]) <= 6) {
        h = bottom - snaps[i] - LW;
      }
    }
  }
  int blue_y = bottom - h;
  fill(ctx, GRect(c.origin.x, blue_y, c.size.w, h), COL_BLUE);
  if (divider) {
    fill(ctx, GRect(c.origin.x, blue_y - LW, c.size.w, LW), GColorBlack);
  }
  int white_h = c.size.h - h - (divider ? LW : 0);

  char count[12];
  char lab[24];
  if (d->steps_ok) {
    snprintf(count, sizeof(count), "%ld", (long)d->steps);
    if (s_large_labels) snprintf(lab, sizeof(lab), "STEPS");
    else snprintf(lab,sizeof(lab),"STEPS %d%%",permille/10);
  } else {
    snprintf(count, sizeof(count), "--");
    snprintf(lab, sizeof(lab), "STEPS");
  }
  int x = c.origin.x + PAD;
  // Label: top of the white remainder, or top of the blue once the remainder
  // is too thin to hold it.
  if (white_h >= 2 * PAD + s_face[caption()].h) {
    ink_text(ctx, lab, caption(), x, c.origin.y + PAD, GColorBlack);
  } else {
    ink_text(ctx, lab, caption(), x, blue_y + PAD, GColorWhite);
  }
  // Count: foot of the blue, or just above the divider while the blue is low.
  const int count_h = s_face[FACE_L26].h;
  if (h >= 2 * PAD + count_h) {
    ink_text(ctx, count, FACE_L26, x, bottom - PAD - count_h, GColorWhite);
  } else if (divider) {
    ink_text(ctx, count, FACE_L26, x, blue_y - LW - PAD - count_h, GColorBlack);
  } else {
    ink_text(ctx, count, FACE_L26, x, bottom - PAD - count_h, GColorBlack);
  }
}

static void draw_grid(GContext *ctx) {
  fill(ctx, GRect(0, MID_Y, 200, LW), GColorBlack);
  fill(ctx, GRect(V_TOP_X, 0, LW, TOP_H), GColorBlack);
  fill(ctx, GRect(RIGHT_X, DATE_H, RIGHT_W, LW), GColorBlack);
  fill(ctx, GRect(V_BOT_X, BOT_Y, LW, 228 - BOT_Y), GColorBlack);
  fill(ctx, GRect(0, SUN_Y - LW, V_BOT_X, LW), GColorBlack);
  fill(ctx, GRect(0, BAT_Y - LW, V_BOT_X, LW), GColorBlack);
}

static void draw(GContext *ctx, GRect bounds, const LabData *d) {
  s_large_labels = d->large_labels;
  graphics_context_set_antialiased(ctx, false);
  fill(ctx, bounds, GColorWhite);
  draw_heart(ctx, d);
  draw_steps(ctx, d);
  draw_battery(ctx, d);
  draw_grid(ctx);
  draw_time(ctx, d);
  draw_date(ctx, d);
  draw_weather(ctx, d);
  draw_sun(ctx, d);
}

// --- detail view: the same painting, re-divided -------------------------------
//
// Tap for detail. The full-width line stays where it is on the face; above it
// the time, rain and temperature planes and a heart band whose red plane spans
// today's lowest to highest bpm; below it the week of steps as seven rising
// blue planes (today in red).

#define DT_ROW_H 56                         // top row of cells
#define DT_TIME_W 72
#define DT_RAIN_W 68
#define DT_HEART_Y (DT_ROW_H + LW)
#define BPM_AXIS_LO 20                      // heart band axis: x = bpm - 20,
#define BPM_AXIS_HI 220                     // so 1 px per bpm across 200 px

static const GRect D_TIME = {{0, 0}, {DT_TIME_W, DT_ROW_H}};
static const GRect D_RAIN = {{DT_TIME_W + LW, 0}, {DT_RAIN_W, DT_ROW_H}};
static const GRect D_TEMP = {{DT_TIME_W + DT_RAIN_W + 2 * LW, 0},
                             {200 - DT_TIME_W - DT_RAIN_W - 2 * LW, DT_ROW_H}};
static const GRect D_HEART = {{0, DT_HEART_Y}, {200, MID_Y - DT_HEART_Y}};
static const GRect D_WEEK = {{0, BOT_Y}, {200, 228 - BOT_Y}};

static void detail_time(GContext *ctx, const LabData *d) {
  cell_label(ctx, D_TIME, d->bt_connected ? "BT ON" : "BT OFF", GColorBlack);
  char t[8];
  lab_fmt_time(t, sizeof(t), d->now.tm_hour, d->now.tm_min, d->is_24h);
  cell_value(ctx, D_TIME, t, FACE_L20, GColorBlack);
}

// Rain: the highest chance left today and its hour. The plane turns blue when
// rain is likely (50% or more), like the face's sun cell turns with the day.
static void detail_rain(GContext *ctx, const LabData *d) {
  GRect c = D_RAIN;
  int peak = -1;
  int peak_hour = 0;
  int peak_day = 0;
  if (d->weather_ok) {
    for (int i = 0; i < d->rolling_count; i++) {
      if (d->rolling_rain[i] > peak) {
        peak = d->rolling_rain[i];
        peak_hour = d->rolling_hour[i];
        peak_day = d->rolling_day[i];
      }
    }
  }
  char lab[16];
  char when[8];
  if (peak < 0) {
    snprintf(lab, sizeof(lab), "RAIN --");
  } else {
    snprintf(lab, sizeof(lab), "RAIN %d%%", peak);
  }
  if (peak > 0) {
    lab_fmt_time(when, sizeof(when), peak_hour, 0, d->is_24h);
  } else {
    snprintf(when, sizeof(when), "--");
  }
  bool likely = peak >= 50;
  if (likely) {
    fill(ctx, c, COL_BLUE);
  }
  GColor ink = likely ? GColorWhite : GColorBlack;
  if (s_large_labels) {
    cell_label(ctx,c,peak_day ? "RAIN +1" : "RAIN",ink);
    char pct[16];
    if (peak<0)snprintf(pct,sizeof(pct),"--");
    else snprintf(pct,sizeof(pct),"%d%%",peak);
    ink_text(ctx,pct,FACE_L20,c.origin.x+PAD,c.origin.y+20,ink);
  } else {
    cell_label(ctx,c,lab,ink);
    if(peak_day)ink_text(ctx,"NEXT DAY",FACE_PX8,c.origin.x+PAD,c.origin.y+18,ink);
  }
  cell_value(ctx, c, when, FACE_L20, ink);
}

// Today's high and low: a yellow plane read like a thermometer, high at the
// top edge and low at the bottom.
static void detail_temp(GContext *ctx, const LabData *d) {
  GRect c = D_TEMP;
  fill(ctx, c, COL_YELLOW);
  char hi[12];
  char lo[12];
  if (d->weather_ok) {
    snprintf(hi, sizeof(hi), "HI %d", display_temp(d,d->temp_hi));
    snprintf(lo, sizeof(lo), "LO %d", display_temp(d,d->temp_lo));
  } else {
    snprintf(hi, sizeof(hi), "HI --");
    snprintf(lo, sizeof(lo), "LO --");
  }
  FaceId f=s_large_labels ? FACE_G18 : FACE_PX8;
  int inset=s_large_labels ? 3 : PAD;
  if(ink_w(hi,f)>c.size.w-2*inset)memmove(hi+1,hi+2,strlen(hi+2)+1);
  if(ink_w(lo,f)>c.size.w-2*inset)memmove(lo+1,lo+2,strlen(lo+2)+1);
  ink_text(ctx,hi,f,c.origin.x+inset,c.origin.y+PAD,GColorBlack);
  ink_text(ctx,lo,f,c.origin.x+inset,c.origin.y+c.size.h-PAD-s_face[f].h,GColorBlack);
}

static int bpm_x(int bpm) {
  return lab_clamp((bpm - BPM_AXIS_LO) * 200 / (BPM_AXIS_HI - BPM_AXIS_LO), 0, 200);
}

// Today's heart range: the red plane's edges are real grid lines placed at the
// lowest and highest bpm on a fixed 30-200 axis.
static void detail_heart(GContext *ctx, const LabData *d) {
  GRect c = D_HEART;
  int lo = d->hr_min;
  int hi = d->hr_max;
  if (lo <= 0 || hi <= 0) {
    // Fall back to the hourly averages.
    lo = 0;
    hi = 0;
    for (int h = 0; h < 24; h++) {
      int v = d->hr_by_hour[h];
      if (v > 0) {
        lo = (lo == 0 || v < lo) ? v : lo;
        hi = v > hi ? v : hi;
      }
    }
  }
  int top = c.origin.y;
  int bottom = top + c.size.h;
  const int vh = s_face[FACE_L20].h;
  if (lo <= 0 || hi <= 0) {
    cell_label(ctx, c, "BPM", GColorBlack);
    cell_value(ctx, c, "--", FACE_L20, GColorBlack);
    return;
  }
  int xa = bpm_x(lo);
  int xb = bpm_x(hi);
  if (xb - xa < 8) {
    xa=lab_clamp(xa,0,192);
    xb=xa+8;
  }
  // No slivers of white at the edges: a white plane under 10 px joins the red.
  if (xa - LW < 10) {
    xa = 0;
  }
  if (200 - xb - LW < 10) {
    xb = 200;
  }
  fill(ctx, GRect(xa, top, xb - xa, c.size.h), COL_RED);
  if (xa > 0) {
    fill(ctx, GRect(xa - LW, top, LW, c.size.h), GColorBlack);
  }
  if (xb < 200) {
    fill(ctx, GRect(xb, top, LW, c.size.h), GColorBlack);
  }
  char lo_s[12];
  char hi_s[12];
  snprintf(lo_s, sizeof(lo_s), "%d", lo);
  snprintf(hi_s, sizeof(hi_s), "%d", hi);
  int wl = ink_w(lo_s, FACE_L20);
  int wh = ink_w(hi_s, FACE_L20);
  int vy = bottom - PAD - vh;
  if (wl + wh + 2 * PAD + 8 <= xb - xa) {
    // Both ends inside the red plane, at its edges.
    ink_text(ctx, "BPM", caption(), xa + PAD, top + PAD, GColorWhite);
    ink_text(ctx, lo_s, FACE_L20, xa + PAD, vy, GColorWhite);
    ink_text(ctx, hi_s, FACE_L20, xb - PAD - wh, vy, GColorWhite);
  } else {
    // A narrow range: "55-70" stands in the wider white plane beside it.
    char range[24];
    snprintf(range, sizeof(range), "%d-%d", lo, hi);
    int left=xa>0 ? xa-LW : 0, right=xb<200 ? 200-xb-LW : 0;
    int room=left>right ? left : right;
    int x=right>=left ? xb+LW+PAD : PAD;
    if(ink_w(range,FACE_L20)+2*PAD<=room) {
      ink_text(ctx,"BPM",caption(),x,top+PAD,GColorBlack);
      ink_text(ctx,range,FACE_L20,x,vy,GColorBlack);
    } else {
      // A middle-sized range can fit neither combined label; put its endpoints
      // in the two surrounding white planes instead of crossing black rules.
      ink_text(ctx,"BPM",caption(),xa+PAD,top+PAD,GColorWhite);
      ink_text(ctx,lo_s,FACE_L20,PAD,vy,GColorBlack);
      ink_text(ctx,hi_s,FACE_L20,xb+LW+PAD,vy,GColorBlack);
    }
  }
}

// The week: seven equal planes, oldest on the left, each rising to that day's
// share of the goal. Full means the goal was reached.
static void detail_week(GContext *ctx, const LabData *d) {
  static const char LETTERS[] = "SMTWTFS";
  GRect c = D_WEEK;
  int32_t goal = d->step_goal > 0 ? d->step_goal : 10000;
  const int inner = c.size.w - 6 * LW;
  int bottom = c.origin.y + c.size.h;
  for (int i = 0; i < 7; i++) {
    int k = 6 - i;  // days ago
    int x0 = c.origin.x + (inner * i) / 7 + i * LW;
    int w = (inner * (i + 1)) / 7 - (inner * i) / 7;
    int32_t steps = k == 0 && d->steps_ok ? d->steps : d->steps_by_day[k];
    int permille = (int)((steps * 1000) / goal);
    int h;
    if (steps <= 0) {
      h = 0;
    } else if (permille >= 1000) {
      h = c.size.h;
    } else {
      // Short of the goal always leaves a visible white remainder.
      h = (permille * (c.size.h - LW) + 500) / 1000;
      h = lab_clamp(h, 4, c.size.h - LW - 4);
    }
    GColor col = k == 0 ? COL_RED : COL_BLUE;
    fill(ctx, GRect(x0, bottom - h, w, h), col);
    if (h > 0 && h < c.size.h) {
      fill(ctx, GRect(x0, bottom - h - LW, w, LW), GColorBlack);
    }
    if (i < 6) {
      fill(ctx, GRect(x0 + w, c.origin.y, LW, c.size.h), GColorBlack);
    }
    char letter[2] = {LETTERS[(d->now.tm_wday - k + 7) % 7], 0};
    FaceId day_font=s_large_labels ? FACE_G18 : FACE_PX8;
    int lx = x0 + (w - ink_w(letter, day_font)) / 2;
    const int lh = s_face[day_font].h;
    if (h >= 2 * PAD + lh) {
      ink_text(ctx, letter, day_font, lx, bottom - PAD - lh, GColorWhite);
    } else {
      ink_text(ctx, letter, day_font, lx, bottom - h - (h > 0 ? LW : 0) - PAD - lh,
               GColorBlack);
    }
  }
}

static void draw_detail(GContext *ctx, GRect bounds, const LabData *d) {
  s_large_labels = d->large_labels;
  graphics_context_set_antialiased(ctx, false);
  fill(ctx, bounds, GColorWhite);
  detail_week(ctx, d);
  detail_heart(ctx, d);
  detail_rain(ctx, d);
  detail_temp(ctx, d);
  // Grid.
  fill(ctx, GRect(0, MID_Y, 200, LW), GColorBlack);
  fill(ctx, GRect(0, DT_ROW_H, 200, LW), GColorBlack);
  fill(ctx, GRect(D_RAIN.origin.x - LW, 0, LW, DT_ROW_H), GColorBlack);
  fill(ctx, GRect(D_TEMP.origin.x - LW, 0, LW, DT_ROW_H), GColorBlack);
  detail_time(ctx, d);
}

const LabDesign design_mondrian = {"Mondrian", load, NULL, draw, draw_detail};
