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
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "sdkconfig.h"
/*=============================================================================================================================
    End Includes Files ESP32
=============================================================================================================================*/

/*=============================================================================================================================
    Init Include Files Controle dos Dados do Supervisório
=============================================================================================================================*/
#include "App_Supervisorio.h"
#include "App_SensorDHT.h"
#include "App_ControlSaidas.h"
#include "App_Azure.h"
/*=============================================================================================================================
    End Include Files Controle dos Dados do Supervisório
=============================================================================================================================*/

/*=============================================================================================================================
    Init Defines
=============================================================================================================================*/
#define BUF_SIZE        (1024)

#define UART_USER UART_NUM_0
/*=============================================================================================================================
    End Defines
=============================================================================================================================*/

/*=============================================================================================================================
    Init Handles
=============================================================================================================================*/
TaskHandle_t xSupervisorio = 0;

SemaphoreHandle_t xMutex = 0;
/*=============================================================================================================================
    End Handles
=============================================================================================================================*/

/*=============================================================================================================================
    Init Variables
=============================================================================================================================*/
dht_t sensorReceive;
Variaveis_SetPoint_Saidas_t sVariaveis_SetPoint_Saidas;
uint8_t cStatusSaidas[3];
uint8_t setPoint[2];

static uint8_t cBufferRx_USB[BUF_SIZE]; //= (uint8_t *) malloc(BUF_SIZE);
static uint8_t cBufferTx_USB[BUF_SIZE];
static uint8_t cBufferRx_USB_Comp[BUF_SIZE];
static int len = 0;

uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
};
/*=============================================================================================================================
    End Variables
=============================================================================================================================*/

void vConfigSupervisorio(void)
{
    uart_param_config(UART_USER, &uart_config);
    uart_set_pin(UART_USER, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_USER, BUF_SIZE * 2, 0, 0, NULL, 0);

    /* Cria semaforo binário */
    xMutex = xSemaphoreCreateMutex();
    /* comando necessário pois o semáforo começa em zero */
    xSemaphoreGive( xMutex );

    //criação de fila do xSerialControl -- TaskControl
  	xSetPoint = xQueueCreate( 10, sizeof( uint8_t[2]) );
    if(xSetPoint == NULL)
    {
  		ESP_LOGE("Erro", "Erro na criação da Queue.\n");
  	}
    
    xTaskCreate(&vSupervisorio, "vSupervisorio", configMINIMAL_STACK_SIZE * 10, NULL, 1, xSupervisorio);
}

void vAppSupervisorio(void)
{
    while (true)
    {
        len = 0;
        len = uart_read_bytes(UART_USER, cBufferRx_USB, BUF_SIZE, 20 / portTICK_RATE_MS);

        if(len > 0)
        {
            strcpy((char*)cBufferRx_USB_Comp, (char*)cBufferRx_USB);

            if(strncmp((char*)cBufferRx_USB_Comp, (char*)"[SETTEMP=", strlen((char*)"[SETTEMP=")) == false)
            {
                uint8_t cSetTemp[2];
                cSetTemp[0] = 'T';
                cSetTemp[1] = ((cBufferRx_USB_Comp[9] - '0') * 10) + (cBufferRx_USB_Comp[10] - '0');
                xQueueSend(xSetPoint, &cSetTemp, 10);
            }

            else if(strncmp((char*)cBufferRx_USB_Comp, (char*)"[SETUMID=", strlen((char*)"[SETUMID=")) == false)
            {
                uint8_t cSetUmi[2];                    
                cSetUmi[0] = 'U';
                cSetUmi[1] = ((cBufferRx_USB_Comp[9] - '0') * 10) + (cBufferRx_USB_Comp[10] - '0');
                xQueueSend(xSetPoint, &cSetUmi, 10);
            }

            else{
                memset((char*)cBufferRx_USB_Comp, '\0', strlen((char*)cBufferRx_USB_Comp));
            }
        }

        if(xQueueReceive(xSaidas_Supervisorio, &sVariaveis_SetPoint_Saidas, 10))
        {  
            xSemaphoreTake( xMutex, portMAX_DELAY );
            sprintf((char*)cBufferTx_USB, "[TEMPERATURA=%d]\r\n", sVariaveis_SetPoint_Saidas.u16_Temperatura);
            uart_write_bytes(UART_USER, (char*)cBufferTx_USB, strlen((char*)cBufferTx_USB));
            memset((char*)cBufferRx_USB_Comp, '\0', strlen((char*)cBufferRx_USB_Comp));
            
            sprintf((char*)cBufferTx_USB, "[UMIDADE=%d]\r\n", sVariaveis_SetPoint_Saidas.u16_Umidade);
            uart_write_bytes(UART_USER, (char*)cBufferTx_USB, strlen((char*)cBufferTx_USB));
            memset((char*)cBufferRx_USB_Comp, '\0', strlen((char*)cBufferRx_USB_Comp));
            xSemaphoreGive( xMutex );

            xSemaphoreTake( xMutex, portMAX_DELAY );
            sprintf((char*)cBufferTx_USB, "[SETTEMP=%d]\r\n", sVariaveis_SetPoint_Saidas.u8_SetPointTemp);
            uart_write_bytes(UART_USER, (char*)cBufferTx_USB, strlen((char*)cBufferTx_USB));
            xSemaphoreGive( xMutex );

            xSemaphoreTake( xMutex, portMAX_DELAY );
            sprintf((char*)cBufferTx_USB, "[SETUMID=%d]\r\n", sVariaveis_SetPoint_Saidas.u8_SetPointUmid);
            uart_write_bytes(UART_USER, (char*)cBufferTx_USB, strlen((char*)cBufferTx_USB));
            xSemaphoreGive( xMutex );

            if(sVariaveis_SetPoint_Saidas.b_StatusSaidaAquecedor == true)
            {               
                xSemaphoreTake( xMutex, portMAX_DELAY );
                uart_write_bytes(UART_USER, (char*)"[AQUECEDOR=LIGADO]\r\n", strlen((char*)"[AQUECEDOR=LIGADO]\r\n"));
                xSemaphoreGive( xMutex );
            }
            else if(sVariaveis_SetPoint_Saidas.b_StatusSaidaAquecedor == false)
            {               
                xSemaphoreTake( xMutex, portMAX_DELAY );
                uart_write_bytes(UART_USER, (char*)"[AQUECEDOR=DESLIGADO]\r\n", strlen((char*)"[AQUECEDOR=DESLIGADO]\r\n"));
                xSemaphoreGive( xMutex );
            }

            if(sVariaveis_SetPoint_Saidas.b_StatusSaidaUmidificador == true)
            {               
                xSemaphoreTake( xMutex, portMAX_DELAY );
                uart_write_bytes(UART_USER, (char*)"[UMIDIFICADOR=LIGADO]\r\n", strlen((char*)"[UMIDIFICADOR=LIGADO]\r\n"));
                xSemaphoreGive( xMutex );
            }
            else if(sVariaveis_SetPoint_Saidas.b_StatusSaidaUmidificador == false)
            {               
                xSemaphoreTake( xMutex, portMAX_DELAY );
                uart_write_bytes(UART_USER, (char*)"[UMIDIFICADOR=DESLIGADO]\r\n", strlen((char*)"[UMIDIFICADOR=DESLIGADO]\r\n"));
                xSemaphoreGive( xMutex );
            }
        }
    }
}

/*=============================================================================================================================
    Init Tasks FreeRTOS
=============================================================================================================================*/
void vSupervisorio(void *pvParameter)
{
    (void) pvParameter;
    vAppSupervisorio();
}
/*=============================================================================================================================
    End Tasks FreeRTOS
=============================================================================================================================*/