#include <esp_check.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "BoardStuff.h"
#include "GraphicsStuff.h"

static const char *TAG = "Main";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting up");
    DumpSomeBoardStuff();
    InitGraphics();
    uint8_t counter = 0;

    for (;;)
    {
        if (!counter++)
        {
            ESP_LOGI(TAG, "Havne't crashed, but nothing else to do");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
