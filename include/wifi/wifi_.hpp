#pragma once
#include "wifi/wifi_defs.hpp" // Local definitions, structs, and enumerations

#include <cstring> // Standard libraries

#include "freertos/FreeRTOS.h" // RTOS Libraries
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/projdefs.h"

#include "esp_log.h" // ESP libraries

#include "esp_wifi.h"
#include "esp_netif_types.h"
#include "esp_wifi_types.h"

#include "system_.hpp"
#include "nvs/nvs_.hpp"
#include "sntp/sntp_.hpp"

/* Forward Declarations */
class System;
class NVS;
class SNTP;

/* External Semaphores */
extern SemaphoreHandle_t semSysEntry;

extern "C"
{
    class Wifi
    {
    public:
        Wifi();
        ~Wifi();

        TaskHandle_t getRunTaskHandle(void);
        QueueHandle_t getCmdRequestQueue(void); // Outside objects must ask for the CmdQueue

        /* Wifi_Diagnostics */
        void printTaskInfoByColumns(void);

    private:
        Wifi(const Wifi &) = delete;           // Disable copy constructor
        void operator=(Wifi const &) = delete; // Disable assignment operator

        char TAG[6] = "_wifi";

        /* Object References */
        System *sys = nullptr;
        NVS *nvs = nullptr;

        TaskHandle_t taskHandleSystemRun = nullptr;

        uint8_t show = 0;
        uint8_t showWifi = 0;

        QueueHandle_t queueEvents = nullptr;

        bool haveIPAddress = false;

        uint8_t wifiDirectives = 0;

        void setShowFlags(void); // Pre-Task Functions
        void setLogLevels(void);
        void createSemaphores(void);
        void destroySemaphores(void);
        void createQueues(void);
        void destroyQueues(void);

        uint8_t runStackSizeK = 14; // Default/Minimum stacksize
        TaskHandle_t taskHandleRun = nullptr;

        /* SNTP */
        SNTP *sntp = nullptr;

        /* Wifi_Diagnostic */
        void logTaskInfo(void);

        /* Wifi_Events */
        uint32_t wifiEvents = 0;
        esp_event_handler_instance_t instanceHandlerWifiEventAnyId = nullptr; // Event Registration handles
        esp_event_handler_instance_t instanceHandlerIpEventGotIp = nullptr;

        static void eventHandlerWifiMarshaller(void *, esp_event_base_t, int32_t, void *);
        void eventHandlerWifi(esp_event_base_t, int32_t, void *);

        /* Wifi_Logging */
        std::string errMsg = "";
        void routeLogByRef(LOG_TYPE, std::string *);
        void routeLogByValue(LOG_TYPE, std::string);

        /* Wifi_NVS */
        void restoreVariablesFromNVS(void);
        void saveVariablesToNVS(void);

        /* Wifi_Run */
        TaskHandle_t taskHandleWIFIRun = nullptr;

        esp_netif_t *defaultSTANetif = nullptr; // Default STA
        wifi_config_t staConfig;

        uint8_t hostStatus = 0;           // A host can be a Modem OR and Router as either have SSIDs and run DHCP.
        std::string ssidPri = "empty";    //
        std::string ssidPwdPri = "empty"; //

        bool wifiHostTimeOut = false; // Timeout flags
        bool wifiIPAddressTimeOut = false;
        bool wifiNoValidTimeTimeOut = false;
        bool autoConnect = true; // Normal state is true unless manually disconnecting

        uint8_t noHostSecsToRestartMax = 15; // Seconds to wait before we restart our connection to the host.
        uint8_t noIPAddressSecToRestartMax = 15;
        uint8_t noValidTimeSecToRestartMax = 30; // We allow 1 full rotation through all 4 SNTP servers before resetting connection

        QueueHandle_t queueCmdRequests = nullptr; // WIFI <-- ?? (Request Queue is here)
        WIFI_CmdRequest *ptrWifiCmdRequest = nullptr;
        std::string strCmdPayload = "";

        static void runMarshaller(void *);
        void run(void);
        void runEvents();

        WIFI_OP wifiOP = WIFI_OP::Idle;                                 // Object States
        WIFI_CONN_STATE wifiConnState = WIFI_CONN_STATE::NONE;          //
        WIFI_INIT wifiInitStep = WIFI_INIT::Finished;                   //
        WIFI_DIRECTIVES wifiDirectivesStep = WIFI_DIRECTIVES::Finished; //
        WIFI_CONN wifiConnStep = WIFI_CONN::Finished;                   //
        WIFI_DISC wifiDiscStep = WIFI_DISC::Finished;                   //
        WIFI_SHUTDOWN wifiShdnStep = WIFI_SHUTDOWN::Finished;           //
    };
}