#pragma once
#include "../../shared-kit/tests/settings-stubs/pebble.h"
#define MESSAGE_KEY_DARK_MODE 12
#define MESSAGE_KEY_DARK_GRID 13
int32_t persist_read_int(uint32_t);
int persist_write_int(uint32_t, int32_t);
