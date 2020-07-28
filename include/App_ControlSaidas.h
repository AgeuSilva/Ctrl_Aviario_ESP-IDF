#ifndef APP_CONTROLSAIDAS__h
#define APP_CONTROLSAIDAS__h

/*=============================================================================================================================
    Init Includes Files FreeRTOS
=============================================================================================================================*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
/*=============================================================================================================================
    End Includes Files FreeRTOS
=============================================================================================================================*/

/*=============================================================================================================================
    Init Handle of Task
=============================================================================================================================*/
QueueHandle_t xSaidas_Supervisorio;
QueueHandle_t xSaidas_Azure;
/*=============================================================================================================================
    End Handle of Task
=============================================================================================================================*/

typedef struct Variaveis_SetPoint_Saidas
{
    uint16_t u16_Temperatura;
    uint16_t u16_Umidade;
    uint8_t u8_SetPointUmid;
    uint8_t u8_SetPointTemp;
    bool b_StatusSaidaUmidificador;
    bool b_StatusSaidaAquecedor;
}Variaveis_SetPoint_Saidas_t;

void vConfigControlSaidas(void);
void vAppControlSaidas(void);
void vControlSaidas(void *pvParameter);

#endif