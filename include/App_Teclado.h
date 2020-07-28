#ifndef APP_TECLADO__h
#define APP_TECLADO__h

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

void vConfigTeclado(void);
void vAppTeclado(void);
void vTeclado(void *pvParameter);

#endif