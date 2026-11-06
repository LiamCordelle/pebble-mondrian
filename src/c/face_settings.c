#include "face_settings.h"
#include "kit/kit_calc.h"

#ifdef LAB_HOST_RENDER
#include <stdlib.h>
FaceSettings face_settings_get(void) {
  const char *mode = getenv("MONDRIAN_DARK_MODE");
  const char *grid = getenv("MONDRIAN_DARK_GRID");
  return (FaceSettings){mode ? atoi(mode) : FACE_LIGHT, grid ? atoi(grid) : 0};
}
#else
#include "kit/kit.h"
// Keep the original 4-byte 0/1 theme record; 2 opts into automatic appearance.
#define DARK_MODE_KEY 2000
#define DARK_GRID_KEY 2001
static FaceSettings s_face;
FaceSettings face_settings_get(void) { return s_face; }
void kit_consumer_settings_init(void) {
  s_face = (FaceSettings){FACE_LIGHT, 0};
  if (persist_get_size(DARK_MODE_KEY) == sizeof(int32_t)) {
    int value = persist_read_int(DARK_MODE_KEY);
    if (value >= FACE_LIGHT && value <= FACE_AUTOMATIC) s_face.appearance = value;
  }
  if (persist_get_size(DARK_GRID_KEY) == sizeof(int32_t)) {
    int value = persist_read_int(DARK_GRID_KEY);
    if (value == 0 || value == 1) s_face.white_grid = value;
  }
}
bool kit_consumer_settings_receive(DictionaryIterator *iter) {
  FaceSettings next = s_face;
  int32_t value;
  if (dict_find(iter, MESSAGE_KEY_DARK_MODE)) {
    if (!kit_tuple_int(iter, MESSAGE_KEY_DARK_MODE, &value) ||
        value < FACE_LIGHT || value > FACE_AUTOMATIC) return false;
    next.appearance = value;
  }
  if (dict_find(iter, MESSAGE_KEY_DARK_GRID)) {
    if (!kit_tuple_int(iter, MESSAGE_KEY_DARK_GRID, &value) || value < 0 || value > 1)
      return false;
    next.white_grid = value;
  }
  if (next.appearance == s_face.appearance && next.white_grid == s_face.white_grid)
    return false;
  s_face = next;
  if (persist_write_int(DARK_MODE_KEY, next.appearance) != sizeof(int32_t) ||
      persist_write_int(DARK_GRID_KEY, next.white_grid) != sizeof(int32_t))
    APP_LOG(APP_LOG_LEVEL_WARNING, "Appearance settings could not be saved");
  APP_LOG(APP_LOG_LEVEL_INFO, "Mondrian appearance: %u grid: %u", next.appearance, next.white_grid);
  return true;
}
#endif

bool face_dark_mode(const LabData *data) {
  FaceSettings settings = face_settings_get();
  if (settings.appearance == FACE_DARK) return true;
  // The same shared daylight decision drives the face's yellow/black sun cell.
  return settings.appearance == FACE_AUTOMATIC &&
    kit_daylight_state(data->weather_ok, data->minute_of_day,
                       data->sunrise_min, data->sunset_min) == KitDaylightNight;
}
