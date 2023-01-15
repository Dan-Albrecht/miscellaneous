#include <stdio.h>
#include <inttypes.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"



#include "ChipStuff.h"
#include "LcdStuff.h"
#include "WifiStuff.h"

void doSomeWifi(void);

void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(3000));
    printf("Hello world!\n");

    InitLcd();
    DoWifi();

    while (1)
    {
        printf("Nothing else to do, but still running.\n");

        for (int i = 0; i < 1000; i++)
        {
            TickLcd();
        }
    }
}
