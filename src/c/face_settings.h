#pragma once
#include "lab.h"
typedef enum { FACE_LIGHT, FACE_DARK, FACE_AUTOMATIC } FaceAppearance;
typedef struct { uint8_t appearance, white_grid; } FaceSettings;
FaceSettings face_settings_get(void);
bool face_dark_mode(const LabData *data);
