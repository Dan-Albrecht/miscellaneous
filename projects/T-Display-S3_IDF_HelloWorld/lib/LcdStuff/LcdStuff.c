#include "LcdStuff.h"
#define LV_COLOR_BLUE (lv_color_t) LV_COLOR_MAKE(0x00, 0x00, 0xFF)
#define LV_COLOR_RED (lv_color_t) LV_COLOR_MAKE(0xFF, 0x00, 0x00)

static const char *LCD_TAG = "LCD_Stuff";

typedef struct
{
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    // {0x3A, {0X06}, 1},
    {0xB2, {0X0B, 0X0B, 0X00, 0X33, 0X33}, 5},
    {0xB7, {0X75}, 1},
    {0xBB, {0X28}, 1},
    {0xC0, {0X2C}, 1},
    {0xC2, {0X01}, 1},
    {0xC3, {0X1F}, 1},
    {0xC6, {0X13}, 1},
    {0xD0, {0XA7}, 1},
    {0xD0, {0XA4, 0XA1}, 2},
    {0xD6, {0XA1}, 1},
    {0xE0, {0XF0, 0X05, 0X0A, 0X06, 0X06, 0X03, 0X2B, 0X32, 0X43, 0X36, 0X11, 0X10, 0X2B, 0X32}, 14},
    {0xE1, {0XF0, 0X08, 0X0C, 0X0B, 0X09, 0X24, 0X2B, 0X22, 0X43, 0X38, 0X15, 0X16, 0X2F, 0X37}, 14},

};

// https://github.com/espressif/esp-idf/tree/v4.4.3/examples/peripherals/lcd/lvgl

// Warning! Couldn't extract the list of installed Python packages.
// https://community.platformio.org/t/remove-warning-couldnt-extract-the-list-of-installed-python-packages/28918/2

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    if (user_ctx)
    {
        lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
        if (disp_driver->draw_buf)
        {
            lv_disp_flush_ready(disp_driver);
        }
    }

    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
static lv_disp_drv_t disp_drv;      // contains callback functions

esp_lcd_panel_handle_t StartLcdDriver(void)
{
    ESP_LOGI(LCD_TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL);

    ESP_LOGI(LCD_TAG, "Turn on LCD power");
    ESP_ERROR_CHECK(gpio_set_direction(LCD_PIN_POWER, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_POWER, 1));

    ESP_LOGI(LCD_TAG, "Turn on whatever the RD pin is");
    ESP_ERROR_CHECK(gpio_set_direction(LCD_PIN_RD, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(LCD_PIN_RD, 1));

    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_DC,
        .wr_gpio_num = LCD_PIN_WR,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
        },
        .bus_width = 8,
        .max_transfer_bytes = LCD_BUFFER_BYTES,
    };

    ESP_LOGI(LCD_TAG, "Initialize Intel 8080 bus");
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = EXAMPLE_PIN_NUM_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        //.trans_queue_depth = 10,
        .trans_queue_depth = 20,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
            .swap_color_bytes = !LV_COLOR_16_SWAP,
        },
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
    };

    ESP_LOGI(LCD_TAG, "Initialize Intel 8080 panel");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        // This seems to control endian on later versions of IDF and fixes our wrong color problems
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        //.color_space = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 16,
    };

    ESP_LOGI(LCD_TAG, "Install LCD driver of st7789");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));

    // the gap is LCD panel specific, even panels with the same driver IC, can have different gap value
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, 0, 35));

    // BUGBUG: Need to figure out what this magic is
    /*for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++)
    {
        esp_lcd_panel_io_tx_param(io_handle, lcd_st7789v[i].cmd, lcd_st7789v[i].data, lcd_st7789v[i].len & 0x7f);
        if (lcd_st7789v[i].len & 0x80)
        {
            // delay(120);
            vTaskDelay(pdMS_TO_TICKS(120));
        }
    }*/

    ESP_LOGI(LCD_TAG, "Turn on LCD backlight");
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL));

    return panel_handle;
}

void DoLvgl(esp_lcd_panel_handle_t panel_handle)
{
    ESP_LOGI(LCD_TAG, "Initialize LVGL library");
    lv_init();
    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    /*lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 20);*/

    lv_color_t *lv_disp_buf = (lv_color_t *)heap_caps_malloc(LCD_BUFFER_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    lv_disp_draw_buf_init(&disp_buf, lv_disp_buf, NULL, LCD_BUFFER_LENGTH);

    ESP_LOGI(LCD_TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(LCD_TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(LCD_TAG, "Display LVGL animation");
    ////lv_obj_t *scr = lv_disp_get_scr_act(disp);
    lv_disp_t *def = lv_disp_get_default();

    if (disp == def)
    {
        ESP_LOGI(LCD_TAG, "Default is what we expect");
    }
    else
    {
        ESP_LOGI(LCD_TAG, "Shit is fucked");
    }

    //lv_demo_benchmark();
    lv_demo_widgets();

    while (1)
    {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lv_timer_handler();
    }
}

#define MY_WHITE 0xFFFF
#define MY_RED 0xF800
#define MY_GREEN 0x07E0
#define MY_BLUE 0x001F

void DoRawDraw(esp_lcd_panel_handle_t panel_handle)
{
    // green == red
    // white == white
    // red == blue
    // blue == green

    // endian switch
    // red == red
    // green == blue
    // blue == green

    // ST7789 wrong color
    // https://esp32.com/viewtopic.php?t=31304

    ESP_LOGI(LCD_TAG, "Filling random meory...");
    uint16_t *img = heap_caps_malloc(LCD_BUFFER_BYTES, MALLOC_CAP_DMA);
    for (size_t i = 0; i < LCD_BUFFER_LENGTH; i++)
    {
        img[i] = MY_WHITE;
    }

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
    }

    ESP_LOGI(LCD_TAG, "Rendering...");
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, img));



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

void DoEverything(void)
{
    ESP_LOGI(LCD_TAG, "Starting LCD driver...");
    esp_lcd_panel_handle_t panel_handle = StartLcdDriver();

    ESP_LOGI(LCD_TAG, "Doing LVGL...");
    DoLvgl(panel_handle);
}
