#pragma once
#include "sntp/sntp_defs.hpp" // Local definitions, structs, and enumerations
#include "system_.hpp"        // Class structure and variables

#include <string> // Standard libraries

#include "esp_netif_sntp.h"

#include "esp_sntp.h"
#include "esp_netif_types.h"
#include "esp_netif_sntp.h"

extern "C"
{
    class SNTP
    {
    public:
        SNTP();
        ~SNTP();

        bool timeValid = false;

        bool timeOut = false;                    // Violation value
        bool waitingForSNTPNotification = false; // Waiting for value

        uint8_t waitSecsCount = 0;    // Seconds count in progress
        uint8_t waitSecsTimeOut = 20; // Seconds to exire

        std::string serverName = "";

        /* SNTP_Run */
        SNTP_OP sntpOP = SNTP_OP::Idle;           // Object States
        SNTP_CONN connStep = SNTP_CONN::Idle;     //
        SNTP_DISC discStep = SNTP_DISC::Finished; //

        /* SNTP_Run */
        void run(void);
        void runEvents(void);

    private:
        char TAG[6] = "_sntp";

        SNTP(const SNTP &) = delete;           // Disable copy constructor
        void operator=(SNTP const &) = delete; // Disable assignment operator

        /* Object References */
        System *sys = nullptr;
        NVS *nvs = nullptr;

        uint8_t show = 0;
        uint8_t showSNTP = 0;

        void setShowFlags(void);
        void setLogLevels(void);
        void createSemaphores(void);
        void destroySemaphores(void);

        /* Working Variables */
        std::string timeZone = "HKT-8"; // Our default time zone selection

        esp_sntp_config_t config;
        uint8_t serverIndex = 0;

        /* SNTP_Events */
        uint8_t sntpEvents = 0;
        const uint8_t syncEventTimeOutSecs = 7; // We time out in 7 seconds waiting for a SNTP server response
        uint8_t sntpSyncEventCountDown = 0;

        static void eventHandlerSNTPMarshaller(struct timeval *);
        void eventHandlerSNTP(struct timeval *);

        /* SNTP_Logging */
        std::string errMsg = "";
        void routeLogByRef(LOG_TYPE, std::string *);
        void routeLogByValue(LOG_TYPE, std::string);

        /* SNTP_NVS */
        void restoreVariablesFromNVS(void);
        void saveVariablesToNVS(void);

        /* SNTP_Utilities */
        void lockOrUint8(uint8_t *, uint8_t);
        void lockAndUint8(uint8_t *, uint8_t);
        uint8_t lockGetUint8(uint8_t *); // NOTE: Don't need a lockSet() function
    };
}