#include "wifi/wifi_.hpp"
#include "system_.hpp" // Class structure and variables

#include "esp_check.h"

/* Event Callback Functions - Wifi */
void Wifi::eventHandlerWifiMarshaller(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ((Wifi *)arg)->eventHandlerWifi(event_base, event_id, event_data);
}

void Wifi::eventHandlerWifi(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // The System Event task (sys_evt) will arrive here to drive this handler.  Be sure to implement locking for any and all shared variables.

    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_SCAN_DONE: // ESP32 finish scanning AP
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_SCAN_DONE");
            lockOrUint32(&wifiEvents, _wifiEventScanDone); // Register our event in the wifiEvents flag
            break;
        }

        case WIFI_EVENT_STA_START: // ESP32 station start
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_STA_START - wifi connect");
            lockOrUint32(&wifiEvents, _wifiEventSTAStart);
            break;
        }

        case WIFI_EVENT_STA_STOP: // ESP32 station stop
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_STA_STOP");
            lockOrUint32(&wifiEvents, _wifiEventSTAStop);
            break;
        }

        case WIFI_EVENT_STA_CONNECTED: // ESP32 station connected to AP
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_STA_CONNECTED - IP address will follow...");
            lockOrUint32(&wifiEvents, _wifiEventSTAConnected);
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED: // ESP32 station disconnected from AP
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_STA_DISCONNECTED");
            lockOrUint32(&wifiEvents, _wifiEventSTADisconnected);
            break;
        }

        case WIFI_EVENT_AP_START:
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_AP_START");
            lockOrUint32(&wifiEvents, _wifiEventAPStart);
            break;
        }

        case WIFI_EVENT_AP_STOP:
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_AP_STOP");
            lockOrUint32(&wifiEvents, _wifiEventAPStop);
            break;
        }

        case WIFI_EVENT_AP_STACONNECTED:
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_AP_STACONNECTED");
            lockOrUint32(&wifiEvents, _wifiEventAPConnected);
            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_AP_STADISCONNECTED");
            lockOrUint32(&wifiEvents, _wifiEventAPDisconnected);
            break;
        }

        case WIFI_EVENT_STA_BEACON_TIMEOUT:
        {
            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_EVENT:WIFI_EVENT_STA_BEACON_TIMEOUT, Starting to lose a connection...");
            lockOrUint32(&wifiEvents, _wifiEventBeaconTimeout);
            break;
        }

        default:
        {
            routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): WIFI_EVENT:<default> event_id = " + std::to_string(event_id));
            break;
        }
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP: // ESP32 station got IP from connected AP
        {
            // routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): IP_EVENT:IP_EVENT_STA_GOT_IP:");
            lockOrUint32(&wifiEvents, _ipEventSTAGotIp); // Register our event in the wifiEvents flag
            break;
        }

        default:
        {
            routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "():IP_EVENT:<default> event_id = " + std::to_string(event_id));
            break;
        }
        }
    }
    else
        routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): Unhandled event_base of " + std::string(event_base));
    return;
}