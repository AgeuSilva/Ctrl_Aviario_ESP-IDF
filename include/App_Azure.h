#ifndef APP_AZURE__h
#define APP_AZURE__h

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

void vConfigAzure(void);
void vAppAzure(void);
void vAzure(void *pvParameter);

#endif