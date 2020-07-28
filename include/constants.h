#ifndef ESP32_AZURE_IOT_HUB_CONSTANTS_H
#define ESP32_AZURE_IOT_HUB_CONSTANTS_H

// TODO: provide correct SSID for WiFi
#define WIFI_SSID "TP-LINK_4F5C"

// TODO: provide correct password for WiFi
#define WIFI_PASSPHRASE "50704767"

// TODO: provide correct connectiong string for Azure IoT Hub connection
#define AZURE_IOT_CONNECTION_STRING "HostName=ageusilva.azure-devices.net;DeviceId=esp32-ageusilva-iot;SharedAccessKey=ZEfXLHFPEM4ULMXFSkv0n86NJDnDx+xKpQa8mECfG3g="

// Interval of Device to Cloud (D2C) publishing in seconds
#define TX_INTERVAL_SECOND (60 * 60)

#endif