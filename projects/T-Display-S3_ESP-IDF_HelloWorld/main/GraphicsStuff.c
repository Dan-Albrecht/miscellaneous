#include "GraphicsStuff.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_check.h>
#include <esp_heap_caps.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>

// BUGBUG: Disable
#include <demos/lv_demos.h>

static const char *TAG = "GraphicsStuff";
static lv_disp_draw_buf_t drawBuffer;
static lv_disp_drv_t displayDriver;

// BUGBUG: Seems awful fast...
#define TICK_PERIOD_MS 3

bool ColorTransferDone(esp_lcd_panel_io_handle_t panelIOHandle, esp_lcd_panel_io_event_data_t *pPanelEventData, void *pContext)
{
    if (pContext)
    {
        lv_disp_drv_t *pDisplayDriver = (lv_disp_drv_t *)pContext;

        if (pDisplayDriver->draw_buf)
        {
            lv_disp_flush_ready(pDisplayDriver);
        }
        else
        {
            // Need special log methods for logging in ISRs where we cannot lock
            // ROM method ets_printfis maybe another
            ESP_DRAM_LOGE(TAG, "draw_buf was null");
        }
    }
    else
    {
        ESP_DRAM_LOGE(TAG, "Context was null");
    }

    return false;
}

static void DisplayDriverFlushCallback(lv_disp_drv_t *pDisplayDriver, const lv_area_t *area, lv_color_t *colorMap)
{
    if (pDisplayDriver)
    {
        if (pDisplayDriver->user_data)
        {
            esp_lcd_panel_handle_t panelHandle = (esp_lcd_panel_handle_t)pDisplayDriver->user_data;
            int xStart = area->x1;
            int xEnd = area->x2 + 1;
            int yStart = area->y1;
            int yEnd = area->y2 + 1;

            esp_lcd_panel_draw_bitmap(panelHandle, xStart, yStart, xEnd, yEnd, colorMap);
        }
        else
        {
            ESP_DRAM_LOGE(TAG, "user_data was null");
        }
    }
    else
    {
        ESP_DRAM_LOGE(TAG, "displayDriver was null");
    }
}

static void TimerTick(void *arg)
{
    lv_tick_inc(TICK_PERIOD_MS);
}

// BUGBUG: /maybe/ only need this for psram at the most
extern int Cache_WriteBack_Addr(uint16_t *addr, uint32_t size);

void DoRawDraw(esp_lcd_panel_handle_t panelHandle)
{
    ESP_LOGI(TAG, "Filling meory with %d bytes..", MAX_TRANSFER_BYTES);
    // uint16_t *jankyBuffer = heap_caps_malloc(MAX_TRANSFER_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    uint16_t *jankyBuffer = heap_caps_malloc(MAX_TRANSFER_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    for (size_t i = 0; i < PIXEL_COUNT; i++)
    {
        jankyBuffer[i] = 0x33;
    }

    /*int i = Cache_WriteBack_Addr(jankyBuffer, MAX_TRANSFER_BYTES);
    ESP_LOGI(TAG, "Flush said %d", i);*/
    ESP_LOGI(TAG, "Rendering background..");
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panelHandle, 0, 0, HORIZONTAL_PIXELS, VERTICAL_PIXELS, &jankyBuffer));
    ESP_LOGI(TAG, "Rendering background2..");
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panelHandle, 0, 0, HORIZONTAL_PIXELS, VERTICAL_PIXELS, &jankyBuffer));
    ESP_LOGI(TAG, "Rendering background3..");
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panelHandle, 0, 0, HORIZONTAL_PIXELS, VERTICAL_PIXELS, &jankyBuffer));
    ESP_LOGI(TAG, "Waiting a second to delte..");
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Deleting it..");
    // ESP_ERROR_CHECK(esp_lcd_panel_del(panelHandle));
    DelteTheThing();
    /*
    // TOP with usb on left
    for (size_t i = 0; i < 5; i++)
    {
        size_t rowOffset = i * EXAMPLE_LCD_H_RES;

        for (size_t j = 0; j < EXAMPLE_LCD_H_RES; j++)
        {
            img[rowOffset + j] = MY_WHITE;
        }
    }

    for (size_t i = 50; i < 55; i++)
    {
        size_t rowOffset = i * EXAMPLE_LCD_H_RES;

        for (size_t j = 0; j < EXAMPLE_LCD_H_RES; j++)
        {
            img[rowOffset + j] = MY_GREEN;
        }
    }

    for (size_t i = EXAMPLE_LCD_V_RES - 5; i < EXAMPLE_LCD_V_RES; i++)
    {
        size_t rowOffset = i * EXAMPLE_LCD_H_RES;

        for (size_t j = 0; j < EXAMPLE_LCD_H_RES; j++)
        {
            img[rowOffset + j] = MY_RED;
        }
    }*/

    /*size_t len = THING;
    size_t size = THING * 2;
    uint16_t *img = heap_caps_malloc(size, MALLOC_CAP_DMA);
    assert(img);*/

    /*for (size_t i = 0; i < len; i++)
    {
        if ((i % EXAMPLE_LCD_H_RES) < (EXAMPLE_LCD_H_RES / 2))
        {
            uint16_t color_byte = esp_random() & 0xFFFF;
            img[i] = color_byte;
        }
        else
        {
            img[i] = 0xFFFF;
        }
    }*/

    /*lv_color_t buf[EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES];
    lv_color_t *buf_p = buf;*/
    /*lv_color_t *buf_p = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_DMA);
    uint16_t x, y;
    for (y = 0; y < EXAMPLE_LCD_V_RES; y++)
    {
        lv_color_t c = lv_color_mix(LV_COLOR_BLUE, LV_COLOR_RED, (y * 255) / EXAMPLE_LCD_V_RES);
        for (x = 0; x < EXAMPLE_LCD_H_RES; x++)
        {
            (*buf_p) = c;
            buf_p++;
        }
    }

    lv_area_t a;
    a.x1 = 10;
    a.y1 = 40;
    a.x2 = a.x1 + EXAMPLE_LCD_H_RES - 1;
    a.y2 = a.y1 + EXAMPLE_LCD_V_RES - 1;

    ESP_LOGI(LCD_TAG, "Rendering...");
    // ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, img));
    my_flush_cb(NULL, &a, buf_p, panel_handle);*/
}

void InitGraphics()
{
    ESP_LOGI(TAG, "Bringing up the panel");
    // BUGBUG: This thing is static, so why pass it alone?
    esp_lcd_panel_handle_t panel = InitLcd(ColorTransferDone, &displayDriver);
    ESP_LOGI(TAG, "Panel init did: %p", panel);

    /*DoRawDraw(panel);
    while (1)
        vTaskDelay(pdMS_TO_TICKS(100));*/

    // BUGBUG: turn on at some point or otherwise sort out

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    // Docs say buffer should be at least 1/10 screen size. Let's go full screen for now...
    // BUGBUG: One version of our code also set MALLOC_CAP_INTERNAL, figure that out
    ESP_LOGI(TAG, "Allocating buffer 1");
    lv_color_t *buffer1 = heap_caps_malloc(MAX_TRANSFER_BYTES, MALLOC_CAP_DMA);
    assert(buffer1);

    ESP_LOGI(TAG, "Allocating buffer 2");
    lv_color_t *buffer2 = heap_caps_malloc(MAX_TRANSFER_BYTES, MALLOC_CAP_DMA);
    assert(buffer2);

    ESP_LOGI(TAG, "Initalizing buffers");
    lv_disp_draw_buf_init(&drawBuffer, buffer1, buffer2, MAX_TRANSFER_BYTES);

    ESP_LOGI(TAG, "Initalizing display driver");
    lv_disp_drv_init(&displayDriver);
    displayDriver.hor_res = HORIZONTAL_PIXELS;
    displayDriver.ver_res = VERTICAL_PIXELS;
    displayDriver.flush_cb = DisplayDriverFlushCallback;
    displayDriver.draw_buf = &drawBuffer;
    displayDriver.user_data = panel;

    ESP_LOGI(TAG, "Registering display driver");
    lv_disp_t *pDisplay = lv_disp_drv_register(&displayDriver);

    ESP_LOGI(TAG, "Setting up tick timer");
    const esp_timer_create_args_t timerArgs = {
        .callback = &TimerTick,
        .name = "lvgl_tick"};
    esp_timer_handle_t timerHandle = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &timerHandle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timerHandle, TICK_PERIOD_MS * 1000));

    SetBacklightOn(true);
    // lv_demo_widgets();
    // lv_demo_benchmark();
    lv_demo_stress();
    uint32_t nextTickTime = 10;

    for (;;)
    {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(nextTickTime));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        nextTickTime = lv_timer_handler();
    }
}
