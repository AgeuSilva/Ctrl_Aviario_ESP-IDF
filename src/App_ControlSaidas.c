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
#include <driver/ledc.h>
/*=============================================================================================================================
    End Includes Files ESP32
=============================================================================================================================*/

/*=============================================================================================================================
    Init Include Files Controle das Saídas
=============================================================================================================================*/
#include "App_ControlSaidas.h"
#include "App_SensorDHT.h"
#include "App_Supervisorio.h"
#include "App_Azure.h"
/*=============================================================================================================================
    Init Include Files Controle das Saídas
=============================================================================================================================*/

/*=============================================================================================================================
    Init Defines
=============================================================================================================================*/
#define AQUECEDOR      17
#define UMIDIFICADOR   23

#define LIGA_AQUECEDOR()        gpio_set_level( AQUECEDOR, 0 )
#define DESLIGA_AQUECEDOR()     gpio_set_level( AQUECEDOR, 1 )

#define LIGA_UMIDIFICADOR()     gpio_set_level( UMIDIFICADOR, 0 )
#define DESLIGA_UMIDIFICADOR()  gpio_set_level( UMIDIFICADOR, 1 )
/*=============================================================================================================================
    End Defines
=============================================================================================================================*/

/*=============================================================================================================================
    Init Handle of Queue
=============================================================================================================================*/
TaskHandle_t xControlSaidas = 0;
/*=============================================================================================================================
    End Handle of Queue
=============================================================================================================================*/

Variaveis_SetPoint_Saidas_t sVariaveis_SetPoint_Saidas;

dht_t sensorReceived; //struct local para receber valor da struct dht_t sensorSent
uint8_t setPoint[2];  //variavel local para receber da fila xSerial a temperatura ou umidade

uint8_t temp = 0;     //variavel local para receber setPoint temperatura
uint8_t umid = 0;     //variavel local para receber setPoint umidade

void vConfigControlSaidas(void)
{
    gpio_pad_select_gpio(AQUECEDOR );
    gpio_set_direction(AQUECEDOR , GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(UMIDIFICADOR );
    gpio_set_direction(UMIDIFICADOR , GPIO_MODE_OUTPUT);

    DESLIGA_AQUECEDOR();
    DESLIGA_UMIDIFICADOR();
    
    //criação de fila do xSerialControl -- TaskControl
  	xSaidas_Supervisorio = xQueueCreate( 10, sizeof( Variaveis_SetPoint_Saidas_t ) );
    if(xSaidas_Supervisorio == NULL)
    {
  		ESP_LOGE("Erro", "Erro na criação da Queue.\n");
  	}

    //criação de fila do xSerialControl -- TaskControl
  	xSaidas_Azure = xQueueCreate( 10, sizeof( Variaveis_SetPoint_Saidas_t ) );
    if(xSaidas_Azure == NULL)
    {
  		ESP_LOGE("Erro", "Erro na criação da Queue.\n");
  	}
    
    xTaskCreate(&vControlSaidas, "vControlSaidas", configMINIMAL_STACK_SIZE * 40, NULL, 1, xControlSaidas);
}

void vAppControlSaidas(void)
{
    while (true)
    {
        if(xQueueReceive(xSetPoint, &setPoint, 10)) 
        {
            switch (setPoint[0]) 
            {
                case 'T':
                    temp = setPoint[1]; //valor convertido de string para int
                    sVariaveis_SetPoint_Saidas.u8_SetPointTemp = temp;
                    xQueueSend(xSaidas_Supervisorio, &sVariaveis_SetPoint_Saidas, 10);
                    xQueueSend(xSaidas_Azure, &sVariaveis_SetPoint_Saidas, 10);
                    break;

                case 'U':
                    umid = setPoint[1]; //valor convertido de string para int
                    sVariaveis_SetPoint_Saidas.u8_SetPointUmid = umid;
                    xQueueSend(xSaidas_Supervisorio, &sVariaveis_SetPoint_Saidas, 10);
                    xQueueSend(xSaidas_Azure, &sVariaveis_SetPoint_Saidas, 10);
                    break;
            }
        }
        
        else if (xQueueReceive(xSensor_Saidas, &sensorReceived, 10)) 
        {
            sVariaveis_SetPoint_Saidas.u16_Temperatura = sensorReceived.u16_Temperatura;
            sVariaveis_SetPoint_Saidas.u16_Umidade = sensorReceived.u16_Umidade;

            if (sensorReceived.u16_Temperatura < temp) 
            {
                /*desligar GPIO Aquecedor*/
                LIGA_AQUECEDOR();
                /*envia ON Aquecedor : xQueueSend para a fila xSerial */
                sVariaveis_SetPoint_Saidas.b_StatusSaidaAquecedor = true;
                xQueueSend(xSaidas_Supervisorio, &sVariaveis_SetPoint_Saidas, 10); 
                xQueueSend(xSaidas_Azure, &sVariaveis_SetPoint_Saidas, 10);
            }
            else if (sensorReceived.u16_Temperatura > temp) 
            {
                /*ligar GPIO Aquecedor*/
                DESLIGA_AQUECEDOR();
                /*envia OFF Aquecedor : xQueueSend para a fila xSerial */
                sVariaveis_SetPoint_Saidas.b_StatusSaidaAquecedor = false;
                xQueueSend(xSaidas_Supervisorio, &sVariaveis_SetPoint_Saidas, 10);
                xQueueSend(xSaidas_Azure, &sVariaveis_SetPoint_Saidas, 10);
            }

            if (sensorReceived.u16_Umidade < umid) 
            {
                /*ligar GPIO umidificador*/
                LIGA_UMIDIFICADOR();
                /*envia ON Umidificador : xQueueSend para a fila xSerial */
                sVariaveis_SetPoint_Saidas.b_StatusSaidaUmidificador = true;
                xQueueSend(xSaidas_Supervisorio, &sVariaveis_SetPoint_Saidas, 10); 
                xQueueSend(xSaidas_Azure, &sVariaveis_SetPoint_Saidas, 10);
            }
            else if (sensorReceived.u16_Umidade > umid)
            {
                /*desligar GPIO umidificador*/
                DESLIGA_UMIDIFICADOR();
                /*envia OFF Umidificador : xQueueSend para a fila xSerial */
                sVariaveis_SetPoint_Saidas.b_StatusSaidaUmidificador = false;
                xQueueSend(xSaidas_Supervisorio, &sVariaveis_SetPoint_Saidas, 10);
                xQueueSend(xSaidas_Azure, &sVariaveis_SetPoint_Saidas, 10);
            }
        }
    }
}

/*=============================================================================================================================
    Init Tasks FreeRTOS
=============================================================================================================================*/
void vControlSaidas(void *pvParameter)
{
    (void) pvParameter;
    vAppControlSaidas();
}
/*=============================================================================================================================
    End Tasks FreeRTOS
=============================================================================================================================*/