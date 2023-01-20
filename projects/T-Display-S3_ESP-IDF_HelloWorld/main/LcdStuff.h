#pragma once

#include <esp_lcd_types.h>
#include <esp_lcd_panel_io.h>

#define HORIZONTAL_PIXELS 320
#define VERTICAL_PIXELS 170
// RGB565 https://chrishewett.com/blog/true-rgb565-colour-picker/
#define PIXEL_SIZE_BYTES sizeof(uint16_t)
#define PIXEL_COUNT (HORIZONTAL_PIXELS * VERTICAL_PIXELS)

// BUGBUG: Research / tweak these more
#define MAX_TRANSFER_BYTES (PIXEL_COUNT * PIXEL_SIZE_BYTES)
//#define MAX_TRANSFER_BYTES (100 * HORIZONTAL_PIXELS * PIXEL_SIZE_BYTES)

esp_lcd_panel_handle_t InitLcd(esp_lcd_panel_io_color_trans_done_cb_t colorTransferDone, void *colorTransferDoneContext);

void SetBacklightOn(bool on);
void DelteTheThing(void);
