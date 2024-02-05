#include "sntp/sntp_.hpp"
#include "system_.hpp"

#include "esp_check.h"

//
// NOTE: The run function is not called by a local Task so it behaves a bit differently.  We enter here at a fairly slow cadence.
//       Upon entering, we process any events that may have occurred first.   Then we (re)enter any state processing that may be in progress.
//       This appears to be the right solution for an instance which has no task and is owned ('has as' relationship) by a parent object.
//
void SNTP::run(void)
{
    esp_err_t ret = ESP_OK;

    /* Event Processing */
    runEvents();

    switch (sntpOP)
    {
        // This particular object is simple enough that it doesn't require an initialization process which is typically located here.  Construction was enough to initialize.

        // NOTE:  There is no RTOS processing done at all here.  NO Task Notification, NO Command Queue.  This is because the Wifi parent can control all the variables without
        //        Marshalling between tasks - as SNTP has NO task.

    case SNTP_OP::Connect:
    {
        switch (connStep)
        {
        case SNTP_CONN::Start:
        {
            if (showSNTP & _showSNTPConnSteps)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): SNTP_CONN::Start");

            connStep = SNTP_CONN::Set_Time_Zone;
            [[fallthrough]];
        }

        case SNTP_CONN::Set_Time_Zone:
        {
            if (showSNTP & _showSNTPConnSteps)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): SNTP_CONN::Set_Time_Zone - Step " + std::to_string((int)SNTP_CONN::Set_Time_Zone));

            // https://sites.google.com/a/usapiens.com/opnode/time-zones
            // setenv("TZ", "CEST-2", 1);  // This is the time zone for Europe/Berlin
            // setenv("TZ", "EST5EDT", 1); // This is US EST
            // setenv("TZ", "HKT-8", 1);   // This is the Phillippines

            std::string temp = CONFIG_SNTP_TIME_ZONE;

            if (temp.compare(timeZone) != 0)
            {
                timeZone = CONFIG_SNTP_TIME_ZONE; // Right now, favor the Config setting over value in nvs
                if (showSNTP & _showSNTPConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): SNTP_CONN::Set_Time_Zone: New time zone setting is " + timeZone);
                saveVariablesToNVS();
            }

            if (showSNTP & _showSNTPConnSteps)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): timeZone            is " + timeZone);

            // On start-up, the time zone variable is always empty.
            // NOTE: You can not read the Time Zone unless one has been commited to memory first.
            setenv("TZ", timeZone.c_str(), 1);

            if (showSNTP & _showSNTPConnSteps)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): SNTP_CONN::Set_Time_Zone: Setting time zone to " + timeZone);

            tzset();
            connStep = SNTP_CONN::Configure;
            [[fallthrough]];
        }

        case SNTP_CONN::Configure:
        {
            if (showSNTP & _showSNTPConnSteps)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): SNTP_CONN::Configure - Step " + std::to_string((int)SNTP_CONN::Configure));

            esp_sntp_stop(); // We can crash of we try to set an operating mode while the client is running.... stop first to be sure all will be ok.
            esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);

            serverName = "time" + std::to_string(serverIndex) + ".google.com";

            if (showSNTP & _showSNTPConnSteps)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): SNTP_CONN::Configure: SNTP Server set to " + serverName);

            config = {
                false,                      // smooth_sync
                false,                      // server_from_dhcp
                true,                       // wait_for_sync
                true,                       // start
                eventHandlerSNTPMarshaller, // sync_cb
                false,                      // renew_servers_after_new_IP
                IP_EVENT_STA_GOT_IP,        // ip_event_to_renew
                0,                          // index_of_first_server
                1,                          // num_of_servers
                {serverName.c_str()},       // servers -- We set our server manually, but there is an automatic method.
            };

            // NOTE: IDF v5 impliments a method to look for multiple SNTP servers.  We did this previously in v4 and have carried
            // our method over so we don't use the IDF's method at this time.

            connStep = SNTP_CONN::Init;
            break;
        }

        case SNTP_CONN::Init:
        {
            if (showSNTP & _showSNTPConnSteps)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): SNTP_CONN::Init - Step " + std::to_string((int)SNTP_CONN::Init));

            timeValid = false; // At every new Wifi connection to a host, we consider SNTP to be invalid.

            ESP_GOTO_ON_ERROR(esp_netif_sntp_init(&config), sntp_Init_err, TAG, "esp_netif_sntp_init(&config) failed");

            waitingForSNTPNotification = true;
            sntpSyncEventCountDown = syncEventTimeOutSecs; // Start the Wait counter
            connStep = SNTP_CONN::Waiting_For_Response;

            if (showSNTP & _showSNTPConnSteps)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): SNTP_CONN::Waiting_For_Response - Step " + std::to_string((int)SNTP_CONN::Waiting_For_Response));
            break;

        sntp_Init_err:
            errMsg = std::string(__func__) + "(): SNTP_CONN::SNTP_Init: error: " + esp_err_to_name(ret);
            connStep = SNTP_CONN::Error; // If we have any failures here, then exit out of our process.
            break;
        }

        case SNTP_CONN::Waiting_For_Response:
        {
            if (timeValid) // Did our time synchronization arrive?  If so, then we are done waitingForSNTPNotification.
            {
                waitingForSNTPNotification = false; // Reset our timeout variables.
                waitSecsCount = 0;                  //
                connStep = SNTP_CONN::Idle;         // Our SNTP process is over, go to an idle state.
                saveVariablesToNVS();               // There is a possibility that SNTP server index changed during that process -- call on save.
                break;
            }

            if (sntpSyncEventCountDown < 4)
            {
                if (showSNTP & _showSNTPConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Waiting response from " + serverName + ": " + std::to_string(sntpSyncEventCountDown) + " Secs remain before server rotation...");
            }

            if (--sntpSyncEventCountDown < 1)
            {
                serverIndex++;
                if (serverIndex > 4) // 4 servers to rotate through
                    serverIndex = 1;

                if (showSNTP & _showSNTPConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Changing server to " + std::to_string(serverIndex));

                connStep = SNTP_CONN::Configure;
                break;
            }
            break;
        }

        case SNTP_CONN::Error:
        {
            connStep = SNTP_CONN::Idle; // Park this state
            sntpOP = SNTP_OP::Error;    // Activate error handling
            break;
        }

        case SNTP_CONN::Idle:
        {
            break;
        }
        }
        break;
    }

    case SNTP_OP::Error:
    {
        routeLogByValue(LOG_TYPE::ERROR, errMsg);
        sntpOP = SNTP_OP::Idle;
        break;
    }

    case SNTP_OP::Idle:
    {
        break;
    }
    }
    // Implicitly return to the parent object until the next run() call.
}

//
// This is where we execute event handler actions.  Removing all this activity from the event handlers eliminates task contention
// over variables and the event task can get back quickly to other more important event processing activities.
//
void SNTP::runEvents()
{
    // This particular object has only one possible event, so our structure here is simplistic.
    if (lockGetUint8(&sntpEvents) & _sntpTimeUpdated) // We just received a time sync notification.  Mark the time valid and print out the time.
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Printing Time...");

        timeValid = true; // Mark SNTP Time valid.  This will declare our System Time value and stop any waiting processes.

        lockAndUint8(&sntpEvents, ~_sntpTimeUpdated); // Clear the flag

        time_t currentTime;
        time(&currentTime); // Get the current system time.

        struct tm currentTime_info;
        localtime_r(&currentTime, &currentTime_info); // Convert it to local time representation.

        char strftimeBuf[64];
        strftime(strftimeBuf, sizeof(currentTime_info), "%c", &currentTime_info);

        // if (show & _showEvents)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Notification of a time synchronization event.  " + std::string(strftimeBuf));
    }
}