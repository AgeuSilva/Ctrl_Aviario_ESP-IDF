/*=============================================================================================================================
    Init Includes Files ANSI C
=============================================================================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "_ansi.h"
/*=============================================================================================================================
    End Includes Files ANSI C
=============================================================================================================================*/

/*=============================================================================================================================
    Init Includes Files ESP32
=============================================================================================================================*/
#include <driver/i2c.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include "driver/i2c.h"
#include "sdkconfig.h"
/*=============================================================================================================================
    End Includes Files ESP32
=============================================================================================================================*/

/*=============================================================================================================================
    Init Include Files Controle do Display
=============================================================================================================================*/
#include "App_Display.h"
#include "Lib_HD44780.h"
/*=============================================================================================================================
    End Include Files Controle do Display
=============================================================================================================================*/

/*=============================================================================================================================
    Init Defines
=============================================================================================================================*/
#define LCD_ADDR 0x7E
#define SDA_PIN  21
#define SCL_PIN  22
#define LCD_COLS 16
#define LCD_ROWS 2
/*=============================================================================================================================
    End Defines
=============================================================================================================================*/

/*=============================================================================================================================
    Init Handle of Tasks
=============================================================================================================================*/
TaskHandle_t xDisplay = 0;
/*=============================================================================================================================
    End Handle of Tasks
=============================================================================================================================*/

void vConfigDisplay(void)
{
    //LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
    //LCD_setCursor(1, 1);
    //LCD_writeStr("Teste LCD I2C");
    xTaskCreate(&vDisplay, "vDisplay", configMINIMAL_STACK_SIZE * 15, NULL, 1, xDisplay);
}

void vAppDisplay(void)
{
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(10)); /* Delay de 1 mili segundos */
    }
}

/*=============================================================================================================================
    Init Tasks FreeRTOS
=============================================================================================================================*/
void vDisplay(void *pvParameter)
{
    (void) pvParameter;
    vAppDisplay();
}
/*=============================================================================================================================
    End Tasks FreeRTOS
=============================================================================================================================*/