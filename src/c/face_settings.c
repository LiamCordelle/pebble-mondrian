#include "face_settings.h"
#include <string.h>
#ifdef LAB_HOST_RENDER
#include <stdlib.h>
bool face_dark_mode(void) {
  const char *value = getenv("MONDRIAN_DARK_MODE");
  return value && strcmp(value, "1") == 0;
}
#else
#include "kit/kit.h"
#define DARK_MODE_KEY 2000
static bool s_dark_mode = false;
bool face_dark_mode(void) { return s_dark_mode; }
void kit_consumer_settings_init(void) {
  if (persist_get_size(DARK_MODE_KEY) == sizeof(int32_t)) {
    int value = persist_read_int(DARK_MODE_KEY);
    if (value == 0 || value == 1) s_dark_mode = value;
  }
}
bool kit_consumer_settings_receive(DictionaryIterator *iter) {
  int32_t value;
  if (!kit_tuple_int(iter, MESSAGE_KEY_DARK_MODE, &value) || value < 0 || value > 1)
    return false;
  if (s_dark_mode == (bool)value) return false;
  s_dark_mode = value;
  if (persist_write_int(DARK_MODE_KEY, value) != sizeof(int32_t))
    APP_LOG(APP_LOG_LEVEL_WARNING, "Dark mode could not be saved");
  APP_LOG(APP_LOG_LEVEL_INFO, "Mondrian dark mode: %ld", (long)value);
  return true;
}
#endif
