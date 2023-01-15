#pragma once

// https://docs.espressif.com/projects/esp-idf/en/v4.4.3/esp32s3/api-reference/peripherals/lcd.html
// https://github.com/espressif/esp-idf/tree/v4.4.3/examples/peripherals/lcd/lvgl

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "demos/lv_demos.h"

lv_disp_t * InitLcd(void);
void TickLcd(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_DATA0          39
#define EXAMPLE_PIN_NUM_DATA1          40
#define EXAMPLE_PIN_NUM_DATA2          41
#define EXAMPLE_PIN_NUM_DATA3          42
#define EXAMPLE_PIN_NUM_DATA4          45
#define EXAMPLE_PIN_NUM_DATA5          46
#define EXAMPLE_PIN_NUM_DATA6          47
#define EXAMPLE_PIN_NUM_DATA7          48
#define EXAMPLE_PIN_NUM_CS             6
#define EXAMPLE_PIN_NUM_DC             7
#define EXAMPLE_PIN_NUM_RST            5
#define EXAMPLE_PIN_NUM_BK_LIGHT       38

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              320
#define EXAMPLE_LCD_V_RES              170
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_TICK_PERIOD_MS   3

#define LCD_PIN_POWER 15
#define LCD_PIN_RD 9
#define LCD_PIN_WR 8

#define LCD_BUFFER_LENGTH (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES)
#define LCD_BUFFER_BYTES  (LCD_BUFFER_LENGTH * sizeof(uint16_t))
