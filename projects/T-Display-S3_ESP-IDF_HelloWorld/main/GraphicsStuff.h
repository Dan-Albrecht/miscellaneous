#pragma once

#include <assert.h>
#include <lvgl.h>

#include "LcdStuff.h"

static_assert(sizeof(lv_color_t) == PIXEL_SIZE_BYTES, "Panel driver disagrees with Graphics driver on pixel size");

void InitGraphics();
