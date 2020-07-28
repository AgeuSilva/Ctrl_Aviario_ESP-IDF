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
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_event_loop.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
/*=============================================================================================================================
    End Includes Files ESP32
=============================================================================================================================*/

/*=============================================================================================================================
    Init Includes
=============================================================================================================================*/
#include "iothub_client.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothubtransportmqtt.h"
#include "iothub_client_options.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "constants.h"

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES
/*=============================================================================================================================
    End Includes
=============================================================================================================================*/

/*=============================================================================================================================
    Init Include Files Controle dos Dados do Supervisório
=============================================================================================================================*/
#include "App_Azure.h"
#include "App_Supervisorio.h"
#include "App_ControlSaidas.h"
/*=============================================================================================================================
    End Include Files Controle dos Dados do Supervisório
=============================================================================================================================*/

/*=============================================================================================================================
    Init Defines
=============================================================================================================================*/
#define WIFI_MAXIMUM_RETRY 10

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP
/*=============================================================================================================================
    End Includes Files ESP32
=============================================================================================================================*/

/*=============================================================================================================================
    Init Handle of Tasks
=============================================================================================================================*/
TaskHandle_t xAzure = 0;
/*=============================================================================================================================
    End Handle of Tasks
=============================================================================================================================*/

/*=============================================================================================================================
    Init Handles
=============================================================================================================================*/
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
/*=============================================================================================================================
    End Handle
=============================================================================================================================*/

/*=============================================================================================================================
    Init Variáveis
=============================================================================================================================*/
static const char *TAG = "esp32-azure";
static int s_retry_num = 0;

static const char *connectionString = AZURE_IOT_CONNECTION_STRING;
static int callbackCounter;
static char msgText[1024];

typedef struct EVENT_INSTANCE_TAG
{
    IOTHUB_MESSAGE_HANDLE messageHandle;
    size_t messageTrackingId; // For tracking the messages within the user callback.
} EVENT_INSTANCE;
/*=============================================================================================================================
    End Variáveis
=============================================================================================================================*/

/*=============================================================================================================================
    Init Function IoT
=============================================================================================================================*/
static IOTHUBMESSAGE_DISPOSITION_RESULT ReceiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
    int *counter = (int *)userContextCallback;
    const char *buffer;
    size_t size;
    MAP_HANDLE mapProperties;
    const char *messageId;
    const char *correlationId;

    // Message properties
    if ((messageId = IoTHubMessage_GetMessageId(message)) == NULL)
    {
        messageId = "<null>";
    }

    if ((correlationId = IoTHubMessage_GetCorrelationId(message)) == NULL)
    {
        correlationId = "<null>";
    }

    if (IoTHubMessage_GetByteArray(message, (const unsigned char **)&buffer, &size) != IOTHUB_MESSAGE_OK)
    {
        printf("Unable to retrieve the message data\r\n");
    }
    else
    {
        printf("Received message [%d]\r\nMessage ID: %s\r\nCorrelation ID: %s\r\nData: <<<%.*s>>> & Size=%d\r\n", *counter, messageId, correlationId, (int)size, buffer, (int)size);
    
        if(strncmp((char*)buffer, (char*)"[SETTEMP=", strlen((char*)"[SETTEMP=")) == false)
        {
            uint8_t cSetTemp[2];
            cSetTemp[0] = 'T';
            cSetTemp[1] = ((buffer[9] - '0') * 10) + (buffer[10] - '0');
            xQueueSend(xSetPoint, &cSetTemp, 10);
        }

        if(strncmp((char*)buffer, (char*)"[SETUMID=", strlen((char*)"[SETUMID=")) == false)
        {
            uint8_t cSetUmi[2];                    
            cSetUmi[0] = 'U';
            cSetUmi[1] = ((buffer[9] - '0') * 10) + (buffer[10] - '0');
            xQueueSend(xSetPoint, &cSetUmi, 10);
        }
    }

    mapProperties = IoTHubMessage_Properties(message);
    if (mapProperties != NULL)
    {
        const char *const *keys;
        const char *const *values;
        size_t propertyCount = 0;
        if (Map_GetInternals(mapProperties, &keys, &values, &propertyCount) == MAP_OK)
        {
            if (propertyCount > 0)
            {
                size_t index;

                printf(" Message Properties:\r\n");
                for (index = 0; index < propertyCount; index++)
                {
                    printf("\tKey: %s Value: %s\r\n", keys[index], values[index]);
                }
                printf("\r\n");
            }
        }
    }

    (*counter)++;
    return IOTHUBMESSAGE_ACCEPTED;
}

void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void *userContextCallback)
{
    printf("\n\nConnection Status result:%s, Connection Status reason: %s\n\n", ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, result),
           ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    EVENT_INSTANCE *eventInstance = (EVENT_INSTANCE *)userContextCallback;
    size_t id = eventInstance->messageTrackingId;

    if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
    {
        (void)printf("Confirmation[%d] received for message tracking id = %d with result = %s\r\n", callbackCounter, (int)id, ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
        /* Some device specific action code goes here... */
        callbackCounter++;
    }
    IoTHubMessage_Destroy(eventInstance->messageHandle);
}

void iothub_client_sample_mqtt_run(void)
{
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
    EVENT_INSTANCE message;

    srand((unsigned int)time(NULL));
    callbackCounter = 0;

    int receiveContext = 0;

    if (platform_init() != 0)
    {
        printf("Failed to initialize the platform\r\n");
    }
    else
    {
        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol)) == NULL)
        {
            printf("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            bool traceOn = true;
            IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

            IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientHandle, connection_status_callback, NULL);
            // Setting the Trusted Certificate.  This is only necessary on system with without
            // built in certificate stores.
            #ifdef SET_TRUSTED_CERT_IN_SAMPLES
                IoTHubDeviceClient_LL_SetOption(iotHubClientHandle, OPTION_TRUSTED_CERT, certificates);
            #endif // SET_TRUSTED_CERT_IN_SAMPLES

            // Set callback for handling cloud-to-device messages
            if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, ReceiveMessageCallback, &receiveContext) != IOTHUB_CLIENT_OK)
            {
                printf("ERROR: IoTHubClient_LL_SetMessageCallback..........FAILED!\r\n");
            }
            else
            {

                int iterator = 0;
                time_t sent_time = 0;
                time_t current_time = 0;

                Variaveis_SetPoint_Saidas_t sVariaveis_SetPoint_Saidas;
                
                uint16_t u16_Temperatura = 0;
                uint16_t u16_Umidade = 0;
                uint8_t u8_SetPointUmid = 0;
                uint8_t u8_SetPointTemp = 0;
                bool b_StatusSaidaUmidificador = 0;
                bool b_StatusSaidaAquecedor = 0;

                while (1)
                {
                    if(xQueueReceive(xSaidas_Azure, &sVariaveis_SetPoint_Saidas, 10)) 
                    {
                        u16_Temperatura = sVariaveis_SetPoint_Saidas.u16_Temperatura;
                        u16_Umidade = sVariaveis_SetPoint_Saidas.u16_Umidade;
                        u8_SetPointTemp = sVariaveis_SetPoint_Saidas.u8_SetPointTemp;
                        u8_SetPointUmid = sVariaveis_SetPoint_Saidas.u8_SetPointUmid;
                        b_StatusSaidaAquecedor = sVariaveis_SetPoint_Saidas.b_StatusSaidaAquecedor;
                        b_StatusSaidaUmidificador = sVariaveis_SetPoint_Saidas.b_StatusSaidaUmidificador;
                    }

                    time(&current_time);

                    if (difftime(current_time, sent_time) > TX_INTERVAL_SECOND)
                    {

                        sprintf_s(msgText, sizeof(msgText), "{\"Temperatura\":%d, \"Umidade\":%d, \"Set-Poit Temperatura\":%d, \"Set-Poit Umidade\":%d, \"Saida Aquecedor\":%d, \"Saida Umidificador\":%d}", u16_Temperatura, u16_Umidade, u8_SetPointTemp, u8_SetPointUmid, b_StatusSaidaAquecedor, b_StatusSaidaUmidificador);

                        if ((message.messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)msgText, strlen(msgText))) == NULL)
                        {
                            printf("ERROR: iotHubMessageHandle is NULL!\r\n");
                        }
                        else
                        {
                            message.messageTrackingId = iterator;

                            if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, message.messageHandle, SendConfirmationCallback, &message) != IOTHUB_CLIENT_OK)
                            {
                                (void)printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");
                            }
                            else
                            {
                                time(&sent_time);
                                (void)printf("IoTHubClient_LL_SendEventAsync accepted message [%d] for transmission to IoT Hub.\r\n", (int)iterator);
                            }
                        }

                        iterator++;
                    }

                    IoTHubClient_LL_DoWork(iotHubClientHandle);
                    ThreadAPI_Sleep(10);
                }

                for (int i = 0; i < 3; i++)
                {
                    IoTHubClient_LL_DoWork(iotHubClientHandle);
                    ThreadAPI_Sleep(1);
                }
            }
            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }
        platform_deinit();
    }
}


static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < WIFI_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry connect to the access point");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }

        ESP_LOGI(TAG, "Failed connecting to access point");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        s_retry_num = 0;

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: %s", ip4addr_ntoa(&event->ip_info.ip));

        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSPHRASE},
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished");
}
/*=============================================================================================================================
    End Function IoT
=============================================================================================================================*/

/*=============================================================================================================================
    Init Config Azure
=============================================================================================================================*/
void vConfigAzure(void)
{
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    xTaskCreate(&vAzure, "vAzure", configMINIMAL_STACK_SIZE * 20, NULL, 1, xAzure);
}
/*=============================================================================================================================
    End Config Azure
=============================================================================================================================*/

/*=============================================================================================================================
    Init Function Azure
=============================================================================================================================*/
void vAppAzure(void)
{
     /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to access point: %s", WIFI_SSID);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to access point: %s", WIFI_SSID);
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "Unexpected event!");
    }

    while (true)
    {
        iothub_client_sample_mqtt_run();
        vTaskDelay(pdMS_TO_TICKS(10)); /* Delay de 1 mili segundos */
    }
}
/*=============================================================================================================================
    End Function Azure
=============================================================================================================================*/

/*=============================================================================================================================
    Init Task FreeRTOS
=============================================================================================================================*/
void vAzure(void *pvParameter)
{
    (void) pvParameter;
    vAppAzure();
    vTaskDelete(NULL);
}
/*=============================================================================================================================
    End Task FreeRTOS
=============================================================================================================================*/