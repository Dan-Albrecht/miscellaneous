#include "LcdStuff.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_check.h>
#include <esp_err.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <driver/gpio.h>

// https://github.com/Xinyuan-LilyGO/T-Display-S3
// https://raw.githubusercontent.com/Xinyuan-LilyGO/T-Display-S3/main/image/T-DISPLAY-S3.jpg

#define PIN_RESET 5
#define PIN_CS 6
#define PIN_DC 7
#define PIN_WR 8
// i8080 read, active low
#define PIN_RD 9
#define PIN_POWER 15
#define PIN_BACKLIGHT 38
#define PIN_D0 39
#define PIN_D1 40
#define PIN_D2 41
#define PIN_D3 42
#define PIN_D4 45
#define PIN_D5 46
#define PIN_D6 47
#define PIN_D7 48

#define PIXEL_SIZE_BITS (PIXEL_SIZE_BYTES * 8)
#define SWAP_COLORS 1

#define X_GAP 0
#define Y_GAP 35
#define INVERT_COLOR true
#define SWAP_XY true
#define MIRROR_X false
#define MIRROR_Y true

//  BUGBUG: Slowig down for debugging / need config for PSRAM as it needs a slow clock too
#define CLOCK_HZ (2 * 1000 * 1000)
#define TRANSFER_DEPTH 10

// Supported alignment: 16, 32, 64. A higher alignment can enables higher burst transfer size, thus a higher i80 bus throughput.
#define PSRAM_ALIGNMENT 64

#define SRAM_ALIGNMENT 4

// Aparanetly this is sometimes inverted
#define BACKLIGHT_ON_LEVEL 1
#define BACKLIGHT_OFF_LEVEL !BACKLIGHT_ON_LEVEL
#define DC_IDLE_LEVEL 0
#define DC_CMD_LEVEL 0
#define DC_DUMMY_LEVEL 0
#define DC_DATA_LEVEL 1
#define CMD_BITS 8
#define PARAM_BITS 8

// Pins we need to explicitly set, bus stuff can handle initilization of the others
/*#define ALL_OUTPUT_PINS_MASK \
    BIT64(PIN_RESET)         \
    | BIT64(PIN_CS) | BIT64(PIN_DC) | BIT64(PIN_WR) | BIT64(PIN_RD) | BIT64(PIN_POWER) | BIT64(PIN_BACKLIGHT) | BIT64(PIN_D0) | BIT64(PIN_D1) | BIT64(PIN_D2) | BIT64(PIN_D3) | BIT64(PIN_D4) | BIT64(PIN_D5) | BIT64(PIN_D6) | BIT64(PIN_D7)*/

#define ALL_OUTPUT_PINS_MASK \
    BIT64(PIN_RD)         \
    | BIT64(PIN_BACKLIGHT)

typedef struct
{
    uint8_t cmd;
    uint8_t data[14];
    uint8_t len;
} lcd_cmd_t;

lcd_cmd_t lcd_st7789v[] = {
    {0x11, {0}, 0 | 0x80},
    {0x3A, {0X05}, 1},
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

static const char *LCD_TAG = "LCD_Stuff";
static esp_lcd_i80_bus_handle_t busHandle = NULL;
static esp_lcd_panel_handle_t panelHandle = NULL;
static esp_lcd_panel_io_handle_t panelIOHandle = NULL;

void DelteTheThing(void)
{
    // Want to see a call to esp_lcd_i80_bus_handle_t::panel_io_i80_del
    // That is the thing returned from esp_lcd_new_panel_io_i80

    // Can't call below it is caled through
    // ???
    // which is registered as
    // st7789->base.del = panel_st7789_del;
    // ESP_LOGI(LCD_TAG, "panel_st7789_del");
    // ESP_ERROR_CHECK(panel_st7789_del(panelHandle));

    ESP_LOGI(LCD_TAG, "esp_lcd_panel_io_del");
    ESP_ERROR_CHECK(esp_lcd_panel_io_del(panelIOHandle));

    ESP_LOGI(LCD_TAG, "Panel del");
    ESP_ERROR_CHECK(esp_lcd_panel_del(panelHandle));

    ESP_LOGI(LCD_TAG, "I80 del");
    ESP_ERROR_CHECK(esp_lcd_del_i80_bus(busHandle));
}

void SetBacklightOn(bool on)
{
    if (on)
    {
        ESP_LOGI(LCD_TAG, "Turning on backlight");
        ESP_ERROR_CHECK(gpio_set_level(PIN_BACKLIGHT, BACKLIGHT_ON_LEVEL));
    }
    else
    {
        ESP_LOGI(LCD_TAG, "Turning off backlight");
        ESP_ERROR_CHECK(gpio_set_level(PIN_BACKLIGHT, BACKLIGHT_OFF_LEVEL));
    }
}

esp_err_t BulkSet(uint64_t mask, uint32_t level)
{
    uint32_t pinNum = 0;

    do
    {
        if ((mask >> pinNum) & BIT64(0))
        {
            ESP_RETURN_ON_ERROR(gpio_set_level(pinNum, level), LCD_TAG, "Fail to set pin");
        }

        pinNum++;
    } while (pinNum < GPIO_PIN_COUNT);

    return ESP_OK;
}

esp_lcd_panel_handle_t InitLcd(esp_lcd_panel_io_color_trans_done_cb_t colorTransferDone, void *colorTransferDoneContext)
{
    ESP_LOGI(LCD_TAG, "Enabling all output pins");
    gpio_config_t pinConfig = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = ALL_OUTPUT_PINS_MASK};
    ESP_ERROR_CHECK(gpio_config(&pinConfig));

    ESP_LOGI(LCD_TAG, "Turning off backlight");
    ESP_ERROR_CHECK(gpio_set_level(PIN_BACKLIGHT, BACKLIGHT_OFF_LEVEL));

    /*ESP_LOGI(LCD_TAG, "Powering up LCD");
    ESP_ERROR_CHECK(gpio_set_level(PIN_POWER, 1));*/

    ESP_LOGI(LCD_TAG, "Powering up RD");
    ESP_ERROR_CHECK(gpio_set_level(PIN_RD, 1));

    // BUGBUG: Verify already null
    busHandle = NULL;
    esp_lcd_i80_bus_config_t busConfig = {
        .dc_gpio_num = PIN_DC,
        .wr_gpio_num = PIN_WR,
        .data_gpio_nums = {
            PIN_D0,
            PIN_D1,
            PIN_D2,
            PIN_D3,
            PIN_D4,
            PIN_D5,
            PIN_D6,
            PIN_D7,
        },
        .bus_width = 8,
        // BUGBUG: Seems it wants an extra byte or something??
        // assert(color_size <= (bus->num_dma_nodes * DMA_DESCRIPTOR_BUFFER_MAX_SIZE) && "color bytes too long, enlarge max_transfer_bytes");
        .max_transfer_bytes = MAX_TRANSFER_BYTES, 
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .psram_trans_align = PSRAM_ALIGNMENT,
        .sram_trans_align = SRAM_ALIGNMENT,
    };

    ESP_LOGI(LCD_TAG, "Transfer bytes is %d", MAX_TRANSFER_BYTES);
    ESP_LOGI(LCD_TAG, "Initializing the Intel 8080 LCD bus");
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&busConfig, &busHandle));

    esp_lcd_panel_io_i80_config_t panelIOConfig = {
        .cs_gpio_num = PIN_CS,
        .pclk_hz = CLOCK_HZ,
        .trans_queue_depth = TRANSFER_DEPTH,
        .dc_levels = {
            .dc_idle_level = DC_IDLE_LEVEL,
            .dc_cmd_level = DC_CMD_LEVEL,
            .dc_dummy_level = DC_DUMMY_LEVEL,
            .dc_data_level = DC_DATA_LEVEL,
        },
        .flags = {
            // BUGBUG: Need to do a build time assert about this being the oposite of LVGL
            .swap_color_bytes = SWAP_COLORS,
        },
        .on_color_trans_done = colorTransferDone,
        .user_ctx = colorTransferDoneContext,
        .lcd_cmd_bits = CMD_BITS,
        .lcd_param_bits = PARAM_BITS,
    };

    ESP_LOGI(LCD_TAG, "Initializing the Intel 8080 LCD panel");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(busHandle, &panelIOConfig, &panelIOHandle));

    panelHandle = NULL;
    esp_lcd_panel_dev_config_t panelConfig = {
        .reset_gpio_num = PIN_RESET,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = PIXEL_SIZE_BITS};

    ESP_LOGI(LCD_TAG, "Initializing the ST7789 driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(panelIOHandle, &panelConfig, &panelHandle));
    ESP_LOGI(LCD_TAG, "Created the ST7789 driver at %p", panelHandle);

    ESP_LOGI(LCD_TAG, "Resetting the panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panelHandle));

    ESP_LOGI(LCD_TAG, "Initializing the panel");
    ESP_ERROR_CHECK(esp_lcd_panel_init(panelHandle));

    ESP_LOGI(LCD_TAG, "Inverting panel colors");
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panelHandle, INVERT_COLOR));

    ESP_LOGI(LCD_TAG, "Swapping X & Y");
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panelHandle, SWAP_XY));

    ESP_LOGI(LCD_TAG, "Mirroring panel");
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panelHandle, MIRROR_X, MIRROR_Y));

    ESP_LOGI(LCD_TAG, "Setting panel gap");
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panelHandle, X_GAP, Y_GAP));

    ESP_LOGI(LCD_TAG, "Turnning on panel");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panelHandle, true));

    // BUGBUG: Don't think we actually need this. Maybe its for the touch version?
    // https://github.com/Xinyuan-LilyGO/T-Display-S3/issues/87
    /*for (uint8_t i = 0; i < (sizeof(lcd_st7789v) / sizeof(lcd_cmd_t)); i++)
    {
        esp_lcd_panel_io_tx_param(panelIOHandle, lcd_st7789v[i].cmd, lcd_st7789v[i].data, lcd_st7789v[i].len & 0x7f);
        if (lcd_st7789v[i].len & 0x80)
            vTaskDelay(pdMS_TO_TICKS(100));
    }*/

    return panelHandle;
}
