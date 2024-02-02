#include "wifi/wifi_.hpp"
#include "system_.hpp"

#include "esp_check.h"

extern SemaphoreHandle_t semWifiEntry;

void Wifi::runMarshaller(void *arg)
{
    ((Wifi *)arg)->run();
    ((Wifi *)arg)->taskHandleWIFIRun = nullptr; // This doesn't happen automatically but we look at this variable for validity, so set it manually.
    vTaskDelete(((Wifi *)arg)->taskHandleWIFIRun);
}

void Wifi::run(void)
{
    esp_err_t ret = ESP_OK;
    std::string serverName = "";

    uint8_t waitingOnHostConnSec = 0;  // Single second counters
    uint8_t waitingOnIPAddressSec = 0; //
    uint8_t waitingOnValidTimeSec = 0; //

    bool cmdRunDirectives = false;

    uint8_t runDelayForSNTP = 4;

    WIFI_NOTIFY wifiTaskNotifyValue = static_cast<WIFI_NOTIFY>(0);

    while (true)
    {
        switch (wifiOP)
        {
        case WIFI_OP::Run: // We would like to achieve about a 4Hz entry cadence in the Run state.
        {
            /* Event Processing */
            runEvents(lockGetUint32(&wifiEvents));

            /*  Service all Task Notifications */
            // Task Notifications should be used for notifications or commands which need no input and return no data.
            wifiTaskNotifyValue = static_cast<WIFI_NOTIFY>(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(150)));

            if (wifiTaskNotifyValue > static_cast<WIFI_NOTIFY>(0)) // Do I have a notify value?
            {
                switch (wifiTaskNotifyValue)
                {
                case WIFI_NOTIFY::CMD_CLEAR_PRI_HOST: // Some of these notifications set Directive bits - a follow up CMD_RUN_DIRECTIVES task notification starts the action.
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received WIFI_NOTIFY::CMD_CLEAR_PRI_HOST");
                    wifiDirectives |= _wifiClearPriHostInfo;
                    break;
                }

                case WIFI_NOTIFY::CMD_DISC_HOST:
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received WIFI_NOTIFY::CMD_DISC_HOST");
                    wifiDirectives |= _wifiDisconnectHost;
                    break;
                }

                case WIFI_NOTIFY::CMD_CONN_PRI_HOST:
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received WIFI_NOTIFY::CMD_CONN_PRI_HOST");
                    wifiDirectives |= _wifiConnectPriHost;
                    break;
                }

                case WIFI_NOTIFY::CMD_RUN_DIRECTIVES:
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received WIFI_NOTIFY::CMD_RUN_DIRECTIVES");
                    cmdRunDirectives = true;
                    break;
                }

                case WIFI_NOTIFY::CMD_SET_AUTOCONNECT:
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received WIFI_NOTIFY::CMD_SET_AUTOCONNECT");

                    autoConnect = true;
                    saveToNVSDelayCount = 8;
                    break;
                }

                case WIFI_NOTIFY::CMD_CLEAR_AUTOCONNECT:
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received WIFI_NOTIFY::CMD_CLEAR_AUTOCONNECT");

                    autoConnect = false;
                    saveToNVSDelayCount = 8;
                    break;
                }

                case WIFI_NOTIFY::CMD_SHUT_DOWN:
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received WIFI_NOTIFY::CMD_SHUT_DOWN");
                    shutDown = true;
                    break;
                }
                }
            }

            /*  Service all Queued Incoming Commands */
            // Queue based commands should be used for commands which may provide input and perhaps return data.
            if (xQueuePeek(queueCmdRequests, (void *)&ptrWifiCmdRequest, pdMS_TO_TICKS(100))) // We can wait here a while without a negative impact to any other Run operation.
            {
                if (ptrWifiCmdRequest != nullptr)
                {
                    if (show & _showPayload)
                    {
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): data is " + std::string((char *)(ptrWifiCmdRequest->data)));
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): len  is " + std::to_string(ptrWifiCmdRequest->dataLength));
                    }
                }

                switch (ptrWifiCmdRequest->requestedCmd)
                {
                case WIFI_COMMAND::SET_SSID_PRI:
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received the command WIFI_COMMAND::SET_SSID_PRI");

                    ssidPri = std::string((char *)ptrWifiCmdRequest->data, (size_t)ptrWifiCmdRequest->dataLength);
                    hostStatus |= _hostPriValid; // If we don't declare the SSID/Password as valid, the algorithm won't allow the connection.
                    saveVariablesToNVS();        // Can't afford a delay here.
                    break;
                }

                case WIFI_COMMAND::SET_PASSWD_PRI:
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received the command WIFI_COMMAND::SET_PASSWD_PRI");

                    ssidPwdPri = std::string((char *)ptrWifiCmdRequest->data, (size_t)ptrWifiCmdRequest->dataLength);
                    hostStatus |= _hostPriValid; // If we don't declare the SSID/Password as valid, the algorithm won't allow the connection.
                    saveVariablesToNVS();        // Can't afford a delay here.
                    break;
                }

                case WIFI_COMMAND::SET_SHOW_FLAGS:
                {
                    if (show & _showRun)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Received the command WIFI_COMMAND::SET_SHOW_FLAGS");

                    show = ptrWifiCmdRequest->data[0];
                    showWifi = ptrWifiCmdRequest->data[1];

                    setLogLevels();
                    break;
                }
                }
                xQueueReceive(queueCmdRequests, (void *)&ptrWifiCmdRequest, pdMS_TO_TICKS(0)); // Remove the item from the queue
            }

            //
            // State Changes
            //

            if (shutDown) // Don't reset the flag here...
            {
                wifiShdnStep = WIFI_SHUTDOWN::Start;
                wifiOP = WIFI_OP::Shutdown;
                break;
            }

            if (cmdRunDirectives) // Do we have any directives?
            {
                if (allOperationsFinished()) // Don't allow directives to run until all other operations are in a Finished state
                {
                    cmdRunDirectives = false;                    // Clear the flag
                    wifiDirectivesStep = WIFI_DIRECTIVES::Start; //
                    wifiOP = WIFI_OP::Directives;                //
                    break;                                       // Exit out and allow Directives to run
                }
            }

            /* Connecting to Host on delay*/
            if (waitingOnHostConnSec > 0) // 1 second entry here to connect to Host.  If timeout, we disconnect/reconnect
            {
                if (--waitingOnHostConnSec < 1)
                {
                    waitingOnHostConnSec = 5; // We reset the count here early because any break statement can exit our local scope.

                    if (++noHostSecsToRestart > noHostSecsToRestartMax) // Timeout waiting for connection to a host
                    {
                        waitingOnHostConnSec = 0; // stop this second counter
                        noHostSecsToRestart = 0;  //
                        wifiHostTimeOut = true;   // We have a timeout violation

                        wifiDiscStep = WIFI_DISC::Start; // Must always disconnect before connecting again.
                        wifiOP = WIFI_OP::Disconnect;
                        break;
                    }

                    if (noHostSecsToRestart > 10) // Start to show warnings of a restart after 10 seconds waiting
                    {
                        if (show & _showRun)
                            routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): noHostSecsToRestart Seconds " + std::to_string(noHostSecsToRestart) + "/" + std::to_string(noHostSecsToRestartMax) + " before restart.");
                    }

                    wifiOP = WIFI_OP::Connect; // Return to this operation
                    break;
                }
            }

            /* Waiting on IP Addres */
            if (waitingOnIPAddressSec > 0) // 1 second entry here to see if we have an IP Address.  If timeout, we disconnect/reconnect
            {
                if (--waitingOnIPAddressSec < 1)
                {
                    waitingOnIPAddressSec = 5;

                    if (++noIPAddressSecToRestart > noIPAddressSecToRestartMax)
                    {
                        waitingOnIPAddressSec = 0;   // stop this second counter
                        noIPAddressSecToRestart = 0; //
                        wifiIPAddressTimeOut = true; // We have a timeout violation

                        wifiDiscStep = WIFI_DISC::Start; // Must always disconnect before connecting again.
                        wifiOP = WIFI_OP::Disconnect;
                        break;
                    }

                    if (noIPAddressSecToRestart > 10) // Start to show warnings of a restart after 10 seconds waiting
                    {
                        if (show & _showRun)
                            routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): noIPAddressSecToRestart Seconds " + std::to_string(noIPAddressSecToRestart) + "/" + std::to_string(noIPAddressSecToRestartMax) + " before restart.");
                    }

                    wifiOP = WIFI_OP::Connect; // Return to this operation
                    break;
                }
            }

            /* Obtaining Epoch Time */
            if (waitingOnValidTimeSec) // 1 second entry here to cofirm receipt of Epoch Time.  If timeout, we disconnect/reconnect
            {
                if (--waitingOnValidTimeSec < 1)
                {
                    waitingOnValidTimeSec = 5;

                    if (++noValidTimeSecToRestart > noValidTimeSecToRestartMax)
                    {
                        waitingOnValidTimeSec = 0;     // stop this second counter
                        noValidTimeSecToRestart = 0;   //
                        wifiNoValidTimeTimeOut = true; // We have a timeout violation

                        wifiDiscStep = WIFI_DISC::Start; // Must always disconnect before connecting again.
                        wifiOP = WIFI_OP::Disconnect;
                        break;
                    }

                    if (noValidTimeSecToRestart > 20) // Start to show warnings of a restart after 20 seconds waiting
                    {
                        if (show & _showRun)
                            routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): noValidTimeSecToRestart Seconds " + std::to_string(noValidTimeSecToRestart) + "/" + std::to_string(noValidTimeSecToRestartMax) + " before restart.");
                    }

                    wifiOP = WIFI_OP::Connect; // Return to this operation
                    break;
                }
            }

            /* saving data to NVS on delay */
            if (saveToNVSDelayCount > 0) // Counts of 4 equal about 1 second.
            {
                if (--saveToNVSDelayCount < 1)
                    saveVariablesToNVS();
            }

            /* Service sntp if it is running. */
            if (runDelayForSNTP > 0)
            {
                if (--runDelayForSNTP < 1)
                {
                    if (sntp != nullptr)
                        sntp->run();
                    runDelayForSNTP = 4; // We are calling the sntp run function around 1Hz
                }
            }

            break;
        }

            // Positionally, it is important for Shutdown to be serviced right after it is called for.  We don't want other possible operations
            // becoming active unexepectedly.  This is somewhat important.
        case WIFI_OP::Shutdown:
        {
            // A shutdown is a complete undoing of all items that were established or created with our run thread.
            // If we have Directives, cancel them.  If we are connected then disconnect.  If we created resources, then dispose of them here.
            // NVS actions are also canceled.  Shutdown is a rude way to exit immediately.
            switch (wifiShdnStep)
            {
            case WIFI_SHUTDOWN::Start:
            {
                if (showWifi & _showWifiShdnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_SHUTDOWN::Start");

                wifiShdnStep = WIFI_SHUTDOWN::Cancel_Directives;
                [[fallthrough]];
            }

            case WIFI_SHUTDOWN::Cancel_Directives:
            {
                if (showWifi & _showWifiShdnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_SHUTDOWN::Cancel_Directives - Step " + std::to_string((int)WIFI_SHUTDOWN::Cancel_Directives));

                wifiDirectivesStep = WIFI_DIRECTIVES::Finished; // Immediately cancel out all Directives...
                wifiDirectives = 0;
                cmdRunDirectives = false;

                wifiShdnStep = WIFI_SHUTDOWN::Disconnect_Wifi;
                break;
            }

            case WIFI_SHUTDOWN::Disconnect_Wifi:
            {
                if (showWifi & _showWifiShdnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_SHUTDOWN::Disconnect_Wifi - Step " + std::to_string((int)WIFI_SHUTDOWN::Disconnect_Wifi));

                wifiDiscStep = WIFI_DISC::Start;
                wifiOP = WIFI_OP::Disconnect;

                wifiShdnStep = WIFI_SHUTDOWN::Wait_For_Disconnection; // Resume upon return at this step.
                break;
            }

            case WIFI_SHUTDOWN::Wait_For_Disconnection:
            {
                if (showWifi & _showWifiShdnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_SHUTDOWN::Wait_For_Disconnection - Step " + std::to_string((int)WIFI_SHUTDOWN::Wait_For_Disconnection));

                if (wifiConnState == WIFI_CONN_STATE::WIFI_DISCONNECTED)
                    wifiShdnStep = WIFI_SHUTDOWN::Final_Items;
                break;
            }

            case WIFI_SHUTDOWN::Final_Items: // Clean up any resources that were created by the run task.
            {
                if (showWifi & _showWifiShdnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_SHUTDOWN::Final_Items - Step " + std::to_string((int)WIFI_SHUTDOWN::Final_Items));

                // Note: Allow the destructor to play its part.  Don't touch any resources which are destoryed inside the destructor.

                wifiShdnStep = WIFI_SHUTDOWN::Finished;
                break;
            }

            case WIFI_SHUTDOWN::Finished:
            {
                if (showWifi & _showWifiShdnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_SHUTDOWN::Finished");
                // This exits the run function. (notice how the compiler doesn't complain about a missing break statement)
                // In the runMarshaller, the task is deleted and the task handler set to nullptr.
                return;
            }
            }
            break;
        }

        case WIFI_OP::Init:
        {
            switch (wifiInitStep)
            {
            case WIFI_INIT::Start:
            {
                if (show & _showInit)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_INIT::Start");

                wifiConnState = WIFI_CONN_STATE::WIFI_DISCONNECTED; // Always a cold start initialization.

                wifiInitStep = WIFI_INIT::Checks;
                [[fallthrough]];
            }

            case WIFI_INIT::Checks:
            {
                if (show & _showInit)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_INIT::Checks - Step " + std::to_string((int)WIFI_INIT::Checks));

                // NOTE: Checking for all the required variables here helps us reduce complexity in the remainder of the code.

                if (sntp == nullptr)
                {
                    errMsg = std::string(__func__) + "(): Who forgot to instantiate nvs?";
                    wifiOP = WIFI_OP::Error; // Effectively, this is an assert without an abort.
                    break;
                }

                if (taskHandleSystemRun == nullptr)
                {
                    errMsg = std::string(__func__) + "(): taskHandleSystemRun is nullptr, but is needed for notifications...";
                    wifiOP = WIFI_OP::Error; // Effectively, this is an assert without an abort.
                    break;
                }

                wifiInitStep = WIFI_INIT::Init_Queues_Commands;
                [[fallthrough]];
            }

            case WIFI_INIT::Init_Queues_Commands:
            {
                if (show & _showInit)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_INIT::Init_Queues_Commands - Step " + std::to_string((int)WIFI_INIT::Init_Queues_Commands));

                queueCmdRequests = xQueueCreate(1, sizeof(WIFI_CmdRequest *)); // Initialize our Incoming Command Request Queue
                ptrWifiCmdRequest = new WIFI_CmdRequest();

                wifiInitStep = WIFI_INIT::Set_Variables_From_Config;
                break;
            }

            case WIFI_INIT::Set_Variables_From_Config:
            {
                if (show & _showInit)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_INIT::Set_Variables_From_Config - Step " + std::to_string((int)WIFI_INIT::Set_Variables_From_Config));
                //
                // Any value held in Config will over-write any value stored in the system.  Always clear Config values before sending product
                // to the field.
                //
                std::string configValue = CONFIG_ESP_WIFI_STA_SSID_PRI;

                // if the stored nvs value doesn't match the staConfig value and the staConfig value is not "empty", then change to staConfig value.
                if ((ssidPri.compare(configValue) != 0) && (configValue.compare("empty") != 0))
                {
                    ssidPri = CONFIG_ESP_WIFI_STA_SSID_PRI;
                    routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): Using ssidPri    from Configuration with a value of " + ssidPri);
                    saveVariablesToNVS();
                }
                else
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Using ssidPri    from nvs with a value of " + ssidPri);

                configValue = CONFIG_ESP_WIFI_STA_PASSWORD_PRI;

                if ((ssidPwdPri.compare(configValue) != 0) && (configValue.compare("empty") != 0))
                {
                    ssidPwdPri = CONFIG_ESP_WIFI_STA_PASSWORD_PRI;
                    routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): Using ssidPwdPri from Configuration with a value of " + ssidPwdPri);
                    saveVariablesToNVS();
                }
                else
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Using ssidPwdPri from nvs with a value of " + ssidPwdPri);

                wifiInitStep = WIFI_INIT::Auto_Connect;
                [[fallthrough]];
            }

            case WIFI_INIT::Auto_Connect:
            {
                if (show & _showInit)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_INIT::Auto_Connect - Step " + std::to_string((int)WIFI_INIT::Auto_Connect));

                if (autoConnect) // If autoConnect is set, then set a directive bit to connect to the active host.
                {
                    wifiDirectives |= _wifiConnectPriHost;
                    saveVariablesToNVS();
                }

                wifiInitStep = WIFI_INIT::Finished;
                break;
            }

            case WIFI_INIT::Finished:
            {
                if (show & _showInit)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_INIT::Finished");

                xSemaphoreGive(semWifiEntry); // Allow entry from any other calling tasks
                wifiOP = WIFI_OP::Run;
                break;
            }

            case WIFI_INIT::Error:
            {
                if (show & _showInit)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_INIT::Error");

                wifiOP = WIFI_OP::Error;
                wifiInitStep = WIFI_INIT::Finished;
                break;
            }
            }
            break;
        }

        case WIFI_OP::Directives:
        {
            switch (wifiDirectivesStep)
            {
            case WIFI_DIRECTIVES::Start:
            {
                if (showWifi & _showWifiDirectiveSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DIRECTIVES::Start");
                //
                // Directives are stored and executed in a logical order.
                // All directives are cleared once executed.
                //
                // We can manaually test all our directives here on startup.
                // wifiDirectives = 0;
                // wifiDirectives |= _wifiClearPriHostInfo;
                // wifiDirectives |= _wifiDisconnectHost;
                // wifiDirectives |= _wifiConnectPriHost;
                //
                if (showWifi & _showWifiDirectiveSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): wifiDirectives = " + std::to_string(wifiDirectives));

                if (wifiDirectives > 0)
                    wifiDirectivesStep = WIFI_DIRECTIVES::Clear_Data;
                else
                    wifiDirectivesStep = WIFI_DIRECTIVES::Finished;

                continue; // We really don't need an RTOS yield here.
            }

            case WIFI_DIRECTIVES::Clear_Data:
            {
                if (showWifi & _showWifiDirectiveSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DIRECTIVES::Clear_Data");
                //
                // Clear any data as required.  This is a one type action that is cleared from the Directives.
                //
                if (wifiDirectives & _wifiClearPriHostInfo)
                {
                    wifiDirectives &= ~_wifiClearPriHostInfo; // Clear this flag
                    ssidPri = "empty";
                    ssidPwdPri = "empty";
                    hostStatus &= ~_hostPriValid; // Can't be valid or active when cleared.
                    hostStatus &= ~_hostPriActive;
                    saveVariablesToNVS(); // Can't afford a delay here.
                }

                wifiDirectivesStep = WIFI_DIRECTIVES::Disconnect_Host;
                break;
            }

            case WIFI_DIRECTIVES::Disconnect_Host:
            {
                if (showWifi & _showWifiDirectiveSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DIRECTIVES::Disconnect_Host");
                //
                // Disconnect any host that is active. This a one action that is cleared from the Directives.
                // We might call to Disconnect a Host than is in service, but this is rare and typically be used when
                // the device is in a Wifi mesh and does not need to be connected to a Host.
                //
                if (wifiDirectives & _wifiDisconnectHost)
                {
                    wifiDirectives &= ~_wifiDisconnectHost; // Cancel this flag.

                    if (autoConnect) // If true, set to false because we are explicitly disconnecting and we don't want the system to reconnect.
                    {
                        autoConnect = false;
                        saveToNVSDelayCount = 8;
                    }

                    wifiDiscStep = WIFI_DISC::Start; // Start the disconnection process
                    wifiOP = WIFI_OP::Disconnect;
                    break; // Vector off to Disconnect
                }
                wifiDirectivesStep = WIFI_DIRECTIVES::Connect_Host; // Advanced to the next possible directive
                continue;
            }

            case WIFI_DIRECTIVES::Connect_Host:
            {
                if (showWifi & _showWifiDirectiveSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DIRECTIVES::Connect_Host");

                if (wifiDirectives & _wifiConnectPriHost)
                {
                    if (!autoConnect) // If false -- set autoConnect to true
                    {
                        autoConnect = true; // Always set autoConnect UNLESS you manually disconnect...
                        saveToNVSDelayCount = 8;
                    }

                    wifiDirectives &= ~_wifiConnectPriHost; // Cancel this flag.

                    if (wifiConnState != WIFI_CONN_STATE::WIFI_CONNECTED_STA) // Only consider connecting if not connected...
                    {
                        if (showWifi & _showWifiDirectiveSteps)
                            routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): connectPriHost");

                        if (showWifi & _showWifiDirectiveSteps)
                            routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): primary Host Valid");

                        hostStatus |= _hostPriActive; // Indicate that Primary is active
                        saveVariablesToNVS();         // Can't afford a delay here.

                        wifiConnStep = WIFI_CONN::Start;
                        wifiOP = WIFI_OP::Connect;
                        break;
                    }
                }

                wifiDirectivesStep = WIFI_DIRECTIVES::Finished;
                continue;
            }

            case WIFI_DIRECTIVES::Finished:
            {
                if (showWifi & _showWifiDirectiveSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DIRECTIVES::Finished");

                if (wifiDirectives > 0) // If another directive has come in, restart our directives process
                    wifiDirectivesStep = WIFI_DIRECTIVES::Start;

                wifiOP = WIFI_OP::Run;
                break;
            }
            }
            break;
        }

        case WIFI_OP::Connect:
        {
            switch (wifiConnStep)
            {
            case WIFI_CONN::Start:
            {
                if (showWifi & _showWifiConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Start");

                wifiConnStep = WIFI_CONN::Create_Netif_Objects;
                [[fallthrough]];
            }

            case WIFI_CONN::Create_Netif_Objects:
            {
                if (showWifi & _showWifiConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Create_Netif_Objects - Step " + std::to_string((int)WIFI_CONN::Create_Netif_Objects));

                if (defaultSTANetif == nullptr)
                {
                    defaultSTANetif = esp_netif_create_default_wifi_sta();
                    ESP_GOTO_ON_FALSE(defaultSTANetif, ESP_FAIL, wifi_Create_Netif_STA_Object_err, TAG, "esp_netif_create_default_wifi_sta() returned nullptr.");
                }
                else
                    routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): WIFI_CONN::Create_Netif_Objects: defaultSTANetif NOT NULL at the start of the Connection, what happened?");

                wifiConnStep = WIFI_CONN::Wifi_Init;
                break;

            wifi_Create_Netif_STA_Object_err:
                errMsg = std::string(__func__) + "(): WIFI_CONN::Create_Netif_Objects: error " + esp_err_to_name(ret);
                wifiConnStep = WIFI_CONN::Error;
                break;
            }

            case WIFI_CONN::Wifi_Init:
            {
                if (showWifi & _showWifiConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Wifi_Init - Step " + std::to_string((int)WIFI_CONN::Wifi_Init));

                wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
                ESP_GOTO_ON_ERROR(esp_wifi_init(&cfg), wifi_Wifi_Init_err, TAG, "esp_wifi_init(&cfg) failed");
                wifiConnStep = WIFI_CONN::Register_Handlers;
                break;

            wifi_Wifi_Init_err:
                errMsg = std::string(__func__) + "(): WIFI_CONN::Wifi_Init: error: " + esp_err_to_name(ret);
                wifiOP = WIFI_OP::Error;
                break;
            }

            case WIFI_CONN::Register_Handlers:
            {
                if (showWifi & _showWifiConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Register_Handlers - Step " + std::to_string((int)WIFI_CONN::Register_Handlers));

                if (instanceHandlerWifiEventAnyId == nullptr)
                    ESP_GOTO_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandlerWifiMarshaller, this, &instanceHandlerWifiEventAnyId), Wifi_Register_Event_Handlers_err, TAG, "register WIFI_EVENT::ESP_EVENT_ANY_ID failed");

                if (instanceHandlerIpEventGotIp == nullptr)
                    ESP_GOTO_ON_ERROR(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandlerWifiMarshaller, this, &instanceHandlerIpEventGotIp), Wifi_Register_Event_Handlers_err, TAG, "register IP_EVENT::IP_EVENT_STA_GOT_IP failed");

                wifiConnStep = WIFI_CONN::Set_Wifi_Mode;
                break;

            Wifi_Register_Event_Handlers_err:
                errMsg = std::string(__func__) + "(): WIFI_CONN::Register_Handlers : error : " + esp_err_to_name(ret);
                wifiConnStep = WIFI_CONN::Error; // If we have any failures here, then exit out of our process.
                break;
            }

            case WIFI_CONN::Set_Wifi_Mode:
            {
                if (showWifi & _showWifiConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Set_Wifi_Mode - Step " + std::to_string((int)WIFI_CONN::Set_Wifi_Mode));

                ESP_GOTO_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), wifi_Set_Wifi_Mode_err, TAG, "esp_wifi_set_mode(WIFI_MODE_APSTA) failed");
                // ESP_GOTO_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_APSTA), wifi_Set_Wifi_Mode_err, TAG, "esp_wifi_set_mode(WIFI_MODE_APSTA) failed");
                wifiConnStep = WIFI_CONN::Set_Sta_Config;
                break;

            wifi_Set_Wifi_Mode_err:
                errMsg = std::string(__func__) + "(): WIFI_CONN::Set_Wifi_Mode : error : " + esp_err_to_name(ret);
                wifiConnStep = WIFI_CONN::Error; // If we have any failures here, then exit out of our process.
                break;
            }

            case WIFI_CONN::Set_Sta_Config:
            {
                if (showWifi & _showWifiConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Set_Sta_Config - Step " + std::to_string((int)WIFI_CONN::Set_Sta_Config));

                // Must do a read, modify, write with Config data.
                ESP_GOTO_ON_ERROR(esp_wifi_get_config(WIFI_IF_STA, &staConfig), wifi_Set_Sta_Config_err, TAG, "esp_wifi_get_config(WIFI_IF_STA, &staConfig) failed");

                // uint8_t ssid[32];                                            /**< SSID of target AP. */
                // uint8_t password[64];                                        /**< Password of target AP. */
                // wifi_scan_method_t scan_method;                              /**< do all channel scan or fast scan */
                // bool bssid_set;                                              /**< whether set MAC address of target AP or not. Generally, station_config.bssid_set needs to be 0; and it needs to be 1 only when users need to check the MAC address of the AP.*/
                // uint8_t bssid[6];                                            /**< MAC address of target AP*/
                // uint8_t channel;                                             /**< channel of target AP. Set to 1~13 to scan starting from the specified channel before connecting to AP. If the channel of AP is unknown, set it to 0.*/
                // uint16_t listen_interval;                                    /**< Listen interval for ESP32 station to receive beacon when WIFI_PS_MAX_MODEM is set. Units: AP beacon intervals. Defaults to 3 if set to 0. */
                // wifi_sort_method_t sort_method;                              /**< sort the connect AP in the list by rssi or security mode */
                // wifi_scan_threshold_t threshold;                             /**< When sort_method is set, only APs which have an auth mode that is more secure than the selected auth mode and a signal stronger than the minimum RSSI will be used. */
                // wifi_pmf_config_t pmf_cfg;                                   /**< Configuration for Protected Management Frame. Will be advertised in RSN Capabilities in RSN IE. */
                // uint32_t rm_enabled : 1;                                     /**< Whether Radio Measurements are enabled for the connection */
                // uint32_t btm_enabled : 1;                                    /**< Whether BSS Transition Management is enabled for the connection */
                // uint32_t mbo_enabled : 1;                                    /**< Whether MBO is enabled for the connection */
                // uint32_t ft_enabled : 1;                                     /**< Whether FT is enabled for the connection */
                // uint32_t owe_enabled : 1;                                    /**< Whether OWE is enabled for the connection */
                // uint32_t transition_disable : 1;                             /**< Whether to enable transition disable feature */
                // uint32_t reserved : 26;                                      /**< Reserved for future feature set */
                // wifi_sae_pwe_method_t sae_pwe_h2e;                           /**< Configuration for SAE PWE derivation method */
                // wifi_sae_pk_mode_t sae_pk_mode;                              /**< Configuration for SAE-PK (Public Key) Authentication method */
                // uint8_t failure_retry_cnt;                                   /**< Number of connection retries station will do before moving to next AP. scan_method should be set as WIFI_ALL_CHANNEL_SCAN to use this staConfig.
                //                                                                   Note: Enabling this may cause connection time to increase incase best AP doesn't behave properly. */
                // uint32_t he_dcm_set : 1;                                     /**< Whether DCM max.constellation for transmission and reception is set. */
                // uint32_t he_dcm_max_constellation_tx : 2;                    /**< Indicate the max.constellation for DCM in TB PPDU the STA supported. 0: not supported. 1: BPSK, 2: QPSK, 3: 16-QAM. The default value is 3. */
                // uint32_t he_dcm_max_constellation_rx : 2;                    /**< Indicate the max.constellation for DCM in both Data field and HE-SIG-B field the STA supported. 0: not supported. 1: BPSK, 2: QPSK, 3: 16-QAM. The default value is 3. */
                // uint32_t he_mcs9_enabled : 1;                                /**< Whether to support HE-MCS 0 to 9. The default value is 0. */
                // uint32_t he_su_beamformee_disabled : 1;                      /**< Whether to disable support for operation as an SU beamformee. */
                // uint32_t he_trig_su_bmforming_feedback_disabled : 1;         /**< Whether to disable support the transmission of SU feedback in an HE TB sounding sequence. */
                // uint32_t he_trig_mu_bmforming_partial_feedback_disabled : 1; /**< Whether to disable support the transmission of partial-bandwidth MU feedback in an HE TB sounding sequence. */
                // uint32_t he_trig_cqi_feedback_disabled : 1;                  /**< Whether to disable support the transmission of CQI feedback in an HE TB sounding sequence. */
                // uint32_t he_reserved : 22;                                   /**< Reserved for future feature set */
                // uint8_t sae_h2e_identifier[SAE_H2E_IDENTIFIER_LEN];          /**< Password identifier for H2E. this needs to be null terminated string */

                memset(&staConfig.sta.ssid, 0, 32);     // Old ssid/pwd can be here prior to arrival,
                memset(&staConfig.sta.password, 0, 64); // so we always want to clear the entire array.

                if (hostStatus & _hostPriActive)
                {
                    memcpy(&staConfig.sta.ssid, (void *)ssidPri.c_str(), ssidPri.size());
                    memcpy(&staConfig.sta.password, (void *)ssidPwdPri.c_str(), ssidPwdPri.size());
                }

                // Authorization Mode - For quick reference, this is a list of my hosts and their authorzation modes
                // Hippo       4 WIFI_AUTH_WPA_WPA2_PSK
                // Wildcat     3 WIFI_AUTH_WPA2_PSK
                // Wildcat IOT 3 WIFI_AUTH_WPA2_PSK

                staConfig.sta.bssid_set = 0;
                // staConfig.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK; // Can flip from one to another for testing
                staConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
                staConfig.sta.pmf_cfg.capable = true;
                staConfig.sta.pmf_cfg.required = false;

                if (showWifi & _showWifiConnSteps)
                {
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): sta.ssid                    " + std::string((char *)staConfig.sta.ssid));
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): sta.password                " + std::string((char *)staConfig.sta.password));
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): reserved                    " + std::to_string(staConfig.sta.reserved));
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): he_dcm_max_constellation_tx " + std::to_string(staConfig.sta.he_dcm_max_constellation_tx));
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): he_reserved                 " + std::to_string(staConfig.sta.he_reserved));
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): he_mcs9_enabled             " + std::to_string(staConfig.sta.he_mcs9_enabled));
                }

                ESP_GOTO_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &staConfig), wifi_Set_Sta_Config_err, TAG, "esp_wifi_set_config(WIFI_IF_STA, &staConfig) failed");
                wifiConnStep = WIFI_CONN::Wifi_Start;
                break;

            wifi_Set_Sta_Config_err:
                errMsg = std::string(__func__) + "(): WIFI_CONN::Set_Sta_Config: error: " + esp_err_to_name(ret);
                wifiConnStep = WIFI_CONN::Error;
                break;
            }

            case WIFI_CONN::Wifi_Start:
            {
                if (showWifi & _showWifiConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Wifi_Start - Step " + std::to_string((int)WIFI_CONN::Wifi_Start));

                ESP_GOTO_ON_ERROR(esp_wifi_start(), wifi_Wifi_Start_err, TAG, "WIFI_CONN::Wifi_Start esp_wifi_start() failed");

                noHostSecsToRestart = 0; // Zeroing out the time out counter.

                if (showWifi & _showWifiConnSteps) // Announce our intent to Wait To Connect before we start the wait.  This reduces unneeded messaging in that wait state.
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Wifi_Waiting_To_Connect - Step " + std::to_string((int)WIFI_CONN::Wifi_Waiting_To_Connect));

                waitingOnHostConnSec = 5; // Start the second timer
                wifiConnStep = WIFI_CONN::Wifi_Waiting_To_Connect;
                break;

            wifi_Wifi_Start_err:
                errMsg = std::string(__func__) + "(): WIFI_CONN::Wifi_Start : error : " + esp_err_to_name(ret);
                wifiConnStep = WIFI_CONN::Error; // If we have any failures here, then exit out of our process.
                break;
            }

            case WIFI_CONN::Wifi_Waiting_To_Connect:
            {
                if (wifiConnState == WIFI_CONN_STATE::WIFI_CONNECTED_STA)
                {
                    waitingOnHostConnSec = 0; // Stops the second counter
                    noHostSecsToRestart = 0;  // Zeroing out the time-out counter.
                    wifiHostTimeOut = false;  // Done looking for full STA Connection which includes receiving an IP address.

                    if (showWifi & _showWifiConnSteps) // Announce our intent to Wait For IP Addres before we start the wait.  This reduces unneeded messaging in that wait state.
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Wifi_Waiting_For_IP_Address - Step " + std::to_string((int)WIFI_CONN::Wifi_Waiting_For_IP_Address));

                    wifiConnStep = WIFI_CONN::Wifi_Waiting_For_IP_Address;
                    waitingOnIPAddressSec = 5; // Move to the next second timer
                }
                else
                {
                    wifiOP = WIFI_OP::Run; // If we don't have our connection yet, then allow the Run state to calculate the Timeout for this waiting step.
                }
                break;
            }

            case WIFI_CONN::Wifi_Waiting_For_IP_Address:
            {
                if (haveIPAddress)
                {
                    waitingOnIPAddressSec = 0;    // Stops the second counter
                    noIPAddressSecToRestart = 0;  // Resets the time-out.
                    wifiIPAddressTimeOut = false; //

                    wifiConnStep = WIFI_CONN::Wifi_SNTP_Connect;
                    waitingOnValidTimeSec = 5;
                    break;
                }
                else
                {
                    wifiOP = WIFI_OP::Run; // If we don't have our IP Address yet, then allow the Run state to calculate the Timeout for this waiting step.
                }
                break;
            }

            case WIFI_CONN::Wifi_SNTP_Connect:
            {
                if (showWifi & _showWifiConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Wifi_SNTP_Connect - Step " + std::to_string((int)WIFI_CONN::Wifi_SNTP_Connect));

                wifiConnStep = WIFI_CONN::Wifi_Waiting_SNTP_Valid_Time;

                if (showWifi & _showWifiConnSteps) // Techincally we are in the next state and we give an advanced notice here.
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Wifi_Waiting_SNTP_Valid_Time - Step " + std::to_string((int)WIFI_CONN::Wifi_Waiting_SNTP_Valid_Time));

                sntp->sntpOP = SNTP_OP::Connect;   // Ask SNTP to connect
                sntp->connStep = SNTP_CONN::Start; //
                sntp->run();                       // Enter the SNTP run loop
                break;
            }

            case WIFI_CONN::Wifi_Waiting_SNTP_Valid_Time:
            {
                if (sntp->timeValid)
                {
                    waitingOnValidTimeSec = 0;          // Stops the second counter
                    wifiNoValidTimeTimeOut = false;     //
                    wifiConnStep = WIFI_CONN::Finished; //
                }
                else
                {
                    sntp->run(); // Not valid yet, make a another pass through SNTP.
                    wifiOP = WIFI_OP::Run;
                }
                break;
            }

            case WIFI_CONN::Finished:
            {
                if (showWifi & _showWifiConnSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_CONN::Finished");

                while (!xTaskNotify(taskHandleSystemRun, static_cast<uint32_t>(SYS_NOTIFY::NFY_WIFI_CONNECTED), eSetValueWithoutOverwrite))
                    vTaskDelay(pdMS_TO_TICKS(50));

                wifiOP = WIFI_OP::Directives;
                break;
            }

            case WIFI_CONN::Error:
            {
                wifiConnStep = WIFI_CONN::Finished; // Park this state
                wifiOP = WIFI_OP::Error;            // Vector to error handling
                break;
            }
            }
            break;
        }

        case WIFI_OP::Disconnect:
        {
            switch (wifiDiscStep)
            {
            case WIFI_DISC::Start:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Start");

                wifiDiscStep = WIFI_DISC::Cancel_Connect;
                [[fallthrough]];
            }

            case WIFI_DISC::Cancel_Connect:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Cancel_Connect - Step " + std::to_string((int)WIFI_DISC::Cancel_Connect));

                wifiDiscStep = WIFI_DISC::Deinitialize_SNTP; // Next step by default.

                if (wifiConnStep != WIFI_CONN::Finished)
                {
                    //
                    // If we call for a Disconnect in the middle of a Connection process, we need to reverse the process
                    // starting at just the right step.
                    //
                    if (wifiConnStep >= WIFI_CONN::Wifi_Waiting_SNTP_Valid_Time)
                        wifiDiscStep = WIFI_DISC::Deinitialize_SNTP;
                    else if (wifiConnStep == WIFI_CONN::Wifi_SNTP_Connect)
                        wifiDiscStep = WIFI_DISC::Wifi_Disconnect;
                    else if (wifiConnStep >= WIFI_CONN::Set_Wifi_Mode)
                        wifiDiscStep = WIFI_DISC::Wifi_Disconnect;
                    else if (wifiConnStep >= WIFI_CONN::Register_Handlers)
                        wifiDiscStep = WIFI_DISC::Unregister_Handlers;
                    else if (wifiConnStep >= WIFI_CONN::Wifi_Init)
                        wifiDiscStep = WIFI_DISC::Wifi_Deinit;
                    else if (wifiConnStep >= WIFI_CONN::Create_Netif_Objects)
                        wifiDiscStep = WIFI_DISC::Destroy_Netif_Objects;

                    waitingOnHostConnSec = 0; // Reset the connection timers and flags
                    noHostSecsToRestart = 0;
                    wifiHostTimeOut = false;

                    waitingOnIPAddressSec = 0;
                    noIPAddressSecToRestart = 0;
                    wifiIPAddressTimeOut = false;

                    waitingOnValidTimeSec = 0;
                    noValidTimeSecToRestart = 0;
                    wifiNoValidTimeTimeOut = false;

                    wifiConnStep = WIFI_CONN::Finished; // Make sure the the connection process is canceled.
                }

                continue; // Continue without an RTOS break so we jump to the correct step.
            }

            case WIFI_DISC::Deinitialize_SNTP:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Deinitialize_SNTP - Step " + std::to_string((int)WIFI_DISC::Deinitialize_SNTP));

                esp_sntp_stop();
                esp_netif_sntp_deinit(); // Deinitialize esp_netif SNTP module inside the IDF.

                wifiDiscStep = WIFI_DISC::Wifi_Disconnect;
                break;
            }

            case WIFI_DISC::Wifi_Disconnect:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Wifi_Disconnect - Step " + std::to_string((int)WIFI_DISC::Wifi_Disconnect));

                sntp->connStep = SNTP_CONN::Idle; // Reset the states
                sntp->sntpOP = SNTP_OP::Idle;

                ret = esp_wifi_disconnect();

                if (ret == ESP_OK)
                {
                    ret = esp_wifi_stop();

                    if (ret == ESP_OK)
                        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): esp_wifi_stop() suceeded");
                    else if (ret == ESP_ERR_WIFI_NOT_INIT)
                        routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): WiFi is not initialized by esp_wifi_init - esp_wifi_stop()");
                    else // Unknown error
                    {
                        errMsg = std::string(__func__) + "(): WIFI_DISC::Wifi_Disconnect: esp_wifi_stop() error : " + esp_err_to_name(ret);
                        wifiDiscStep = WIFI_DISC::Error;
                        continue;
                    }
                }
                else if (ret == ESP_ERR_WIFI_NOT_INIT)
                    routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): WiFi is not initialized by esp_wifi_init - esp_wifi_disconnect()");
                else if (ret == ESP_ERR_WIFI_NOT_STARTED)
                    routeLogByValue(LOG_TYPE::WARN, std::string(__func__) + "(): WiFi is not started by esp_wifi_start - esp_wifi_disconnect()");
                else // Unknown error
                {
                    errMsg = std::string(__func__) + "(): WIFI_DISC::Wifi_Disconnect: esp_wifi_disconnect() error : " + esp_err_to_name(ret);
                    wifiDiscStep = WIFI_DISC::Error;
                    continue;
                }

                wifiConnState = WIFI_CONN_STATE::WIFI_DISCONNECTED; // Secondary assignment here just in case the Disconnect event doesn't fire as expected.
                wifiDiscStep = WIFI_DISC::Reset_Flags;
                break;
            }

            case WIFI_DISC::Reset_Flags:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Reset_Flags - Step " + std::to_string((int)WIFI_DISC::Reset_Flags));

                sntp->timeValid = false;                       // SNTP Time is not valid
                haveIPAddress = false;                         // We don't have an IP Address
                wifiDiscStep = WIFI_DISC::Unregister_Handlers; //
                break;
            }

            case WIFI_DISC::Unregister_Handlers:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Unregister_Handlers - Step " + std::to_string((int)WIFI_DISC::Unregister_Handlers));

                if (instanceHandlerWifiEventAnyId != nullptr)
                {
                    ESP_GOTO_ON_ERROR(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instanceHandlerWifiEventAnyId), wifi_Unregister_Handlers_err, TAG, "WIFI_DISC::Unregister_Handlers failed");
                    instanceHandlerWifiEventAnyId = nullptr;
                }

                if (instanceHandlerIpEventGotIp != nullptr)
                {
                    ESP_GOTO_ON_ERROR(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instanceHandlerIpEventGotIp), wifi_Unregister_Handlers_err, TAG, "WIFI_DISC::Unregister_Handlers failed");
                    instanceHandlerIpEventGotIp = nullptr;
                }

                wifiDiscStep = WIFI_DISC::Wifi_Deinit;
                break;

            wifi_Unregister_Handlers_err:
                errMsg = std::string(__func__) + "(): WIFI_DISC::esp_event_handler_instance_unregister() error : " + esp_err_to_name(ret);
                wifiDiscStep = WIFI_DISC::Error; // If we have any failures here, then exit out of our process.
                break;
            }

            case WIFI_DISC::Wifi_Deinit:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Wifi_Deinit - Step " + std::to_string((int)WIFI_DISC::Wifi_Deinit));

                ESP_GOTO_ON_ERROR(esp_wifi_deinit(), wifi_Wifi_Deinit_err, TAG, "esp_wifi_deinit() failed");
                wifiDiscStep = WIFI_DISC::Destroy_Netif_Objects;
                break;

            wifi_Wifi_Deinit_err:
                errMsg = std::string(__func__) + "(): WIFI_DISC::Wifi_Deinit error : " + esp_err_to_name(ret);
                wifiDiscStep = WIFI_DISC::Error; // If we have any failures here, then exit out of our process.
                break;
            }

            case WIFI_DISC::Destroy_Netif_Objects:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Destroy_Netif_Objects - Step " + std::to_string((int)WIFI_DISC::Destroy_Netif_Objects));

                if (defaultSTANetif != nullptr)
                {
                    esp_netif_destroy_default_wifi(defaultSTANetif);
                    defaultSTANetif = nullptr;
                }

                wifiDiscStep = WIFI_DISC::Finished;
                break;
            }

            case WIFI_DISC::Finished:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Finished");

                // This is perhaps a secondary call back to the System to annouce a disconnection.
                while (!xTaskNotify(taskHandleSystemRun, static_cast<uint32_t>(SYS_NOTIFY::NFY_WIFI_DISCONNECTED), eSetValueWithoutOverwrite))
                    vTaskDelay(pdMS_TO_TICKS(50));

                if (wifiHostTimeOut || wifiIPAddressTimeOut || wifiNoValidTimeTimeOut) // Are we servicing a Host or SNTP timeout.  We need to restart.
                {
                    // Favor running from the directives if we have any because in the future, we will impliment the option of rotating over to a secondary
                    // network.
                    if (wifiDirectivesStep != WIFI_DIRECTIVES::Finished)
                        wifiOP = WIFI_OP::Directives;
                    else // Otherwise assume this time out is a service interruption.
                    {
                        wifiConnStep = WIFI_CONN::Start; //
                        wifiOP = WIFI_OP::Connect;       // We always try to start again after a disconnection that is not commanded.
                    }
                }
                else
                {
                    if (wifiShdnStep != WIFI_SHUTDOWN::Finished) // Give return priority to a call for Shut Down
                    {
                        wifiOP = WIFI_OP::Shutdown;
                        if (showWifi & _showWifiDiscSteps)
                            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Returning to Shutdown...");
                    }
                    else if (wifiDirectivesStep != WIFI_DIRECTIVES::Finished)
                        wifiOP = WIFI_OP::Directives; // Make plans to return to Directives it if is not in a Finished state.
                }
                break;
            }

            case WIFI_DISC::Error:
            {
                if (showWifi & _showWifiDiscSteps)
                    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): WIFI_DISC::Error");
                wifiOP = WIFI_OP::Error;
                break;
            }
            }
            break;
        }

        case WIFI_OP::Error:
        {
            // By default we will set the state disconnected
            while (!xTaskNotify(taskHandleSystemRun, static_cast<uint32_t>(SYS_NOTIFY::NFY_WIFI_DISCONNECTED), eSetValueWithoutOverwrite))
                vTaskDelay(pdMS_TO_TICKS(50));

            routeLogByValue(LOG_TYPE::ERROR, errMsg);
            wifiOP = WIFI_OP::Idle;
            continue;
        }

        case WIFI_OP::Idle:
        {
            vTaskDelay(pdMS_TO_TICKS(5000));
            break;
        }
        }
        taskYIELD();
    }
}

//
// This is where we execute event handler actions.  Removing all this activity from the event handlers eliminates task contention
// over variables and the event task can get back quickly to other more important event processing activities.
//
void Wifi::runEvents(uint32_t event)
{
    if (!event) // If there are no pending events to process, then return immediately.
        return;

    esp_err_t ret = ESP_OK;

    if (event & _wifiEventScanDone) // Handle only one event during any run loop
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventScanDone");
        lockAndUint32(&wifiEvents, ~_wifiEventScanDone); // Clear the flag
    }
    else if (event & _wifiEventSTAStart)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventSTAStart");
        lockAndUint32(&wifiEvents, ~_wifiEventSTAStart); // Clear the flag

        if (wifiConnState != WIFI_CONN_STATE::WIFI_READY_TO_CONNECT)
        {
            wifiConnState = WIFI_CONN_STATE::WIFI_READY_TO_CONNECT;

            if (show & _showEvents)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): wifiConnState = WIFI_READY_TO_CONNECT");
        }

        sntp->timeValid = false; // Everytime we start the STA Connection process, the SNTP time must be declared invalid.

        ESP_GOTO_ON_ERROR(esp_wifi_connect(), wifi_eventRun_err, TAG, "esp_wifi_connect() failed");

        wifiConnState = WIFI_CONN_STATE::WIFI_CONNECTING_STA;

        while (!xTaskNotify(taskHandleSystemRun, static_cast<uint32_t>(SYS_NOTIFY::NFY_WIFI_CONNECTING), eSetValueWithoutOverwrite))
            vTaskDelay(pdMS_TO_TICKS(50));
    }
    else if (event & _wifiEventSTAStop)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventSTAStop");
        lockAndUint32(&wifiEvents, ~_wifiEventSTAStop); // Clear the flag

        sntp->timeValid = false; // When we stop the sta connection, sntp time must be invalidated.

        if (wifiConnState != WIFI_CONN_STATE::WIFI_DISCONNECTED)
            wifiConnState = WIFI_CONN_STATE::WIFI_DISCONNECTING_STA; // Only run these items one time at a Disconnection.

        while (!xTaskNotify(taskHandleSystemRun, static_cast<uint32_t>(SYS_NOTIFY::NFY_WIFI_DISCONNECTING), eSetValueWithoutOverwrite))
            vTaskDelay(pdMS_TO_TICKS(50));
    }
    else if (event & _wifiEventSTAConnected)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventSTAConnected");
        lockAndUint32(&wifiEvents, ~_wifiEventSTAConnected); // Clear the flag
        //
        // We have connected to the Host.   This doesn't mean that we have an IP or access to the Internet yet,
        // but we can confirm the ssid and password are valid for the targeted host.
        //
        wifiConnState = WIFI_CONN_STATE::WIFI_CONNECTED_STA;
    }
    else if (event & _wifiEventSTADisconnected)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventSTADisconnected");
        lockAndUint32(&wifiEvents, ~_wifiEventSTADisconnected); // Clear the flag

        sntp->timeValid = false; // When we disconnect, sntp time must be invalidated.

        if (wifiConnState != WIFI_CONN_STATE::WIFI_DISCONNECTED) // The first time we disconnect, set the state and send any required notifications.
            wifiConnState = WIFI_CONN_STATE::WIFI_DISCONNECTED;

        while (!xTaskNotify(taskHandleSystemRun, static_cast<uint32_t>(SYS_NOTIFY::NFY_WIFI_DISCONNECTED), eSetValueWithoutOverwrite))
            vTaskDelay(pdMS_TO_TICKS(50));

        if (autoConnect)
        {
            ESP_GOTO_ON_ERROR(esp_wifi_connect(), wifi_eventRun_err, TAG, "esp_wifi_connect() failed"); // Try to reconnect (But don't reset our timeout counts)
            wifiConnState = WIFI_CONN_STATE::WIFI_CONNECTING_STA;

            while (!xTaskNotify(taskHandleSystemRun, static_cast<uint32_t>(SYS_NOTIFY::NFY_WIFI_CONNECTING), eSetValueWithoutOverwrite))
                vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    else if (event & _wifiEventAPStart)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventSAStart");
        lockAndUint32(&wifiEvents, ~_wifiEventAPStart); // Clear the flag
    }
    else if (event & _wifiEventAPStop)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventSAStop");
        lockAndUint32(&wifiEvents, ~_wifiEventAPStop); // Clear the flag
    }
    else if (event & _wifiEventAPConnected)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventAPConnected");
        lockAndUint32(&wifiEvents, ~_wifiEventAPConnected); // Clear the flag
    }
    else if (event & _wifiEventAPDisconnected)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventAPDisconnected");
        lockAndUint32(&wifiEvents, ~_wifiEventAPDisconnected); // Clear the flag
    }
    else if (event & _wifiEventBeaconTimeout)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _wifiEventBeaconTimeout");
        lockAndUint32(&wifiEvents, ~_wifiEventBeaconTimeout); // Clear the flag
    }
    else if (event & _ipEventSTAGotIp)
    {
        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): _ipEventSTAGotIp");
        lockAndUint32(&wifiEvents, ~_ipEventSTAGotIp); // Clear the flag

        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        ESP_GOTO_ON_FALSE(netif, ESP_FAIL, wifi_eventRun_err, TAG, "Could not locate the WIFI_STA_DEF netif instance..."); // Effectively, this is an assert without an abort.

        esp_netif_ip_info_t ip_info;
        ESP_GOTO_ON_ERROR(esp_netif_get_ip_info(netif, &ip_info), wifi_eventRun_err, TAG, "esp_netif_get_ip_info() failed ");

        uint8_t num1 = ((&(ip_info.ip.addr))[0]);
        uint8_t num2 = ((&(ip_info.ip.addr))[1]);
        uint8_t num3 = ((&(ip_info.ip.addr))[2]);
        uint8_t num4 = ((&(ip_info.ip.addr))[3]);

        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): IP_EVENT:IP_EVENT_STA_GOT_IP: My IP address " + std::to_string(num1) + "." + std::to_string(num2) + "." + std::to_string(num3) + "." + std::to_string(num4));

        num1 = ((&(ip_info.gw.addr))[0]);
        num2 = ((&(ip_info.gw.addr))[1]);
        num3 = ((&(ip_info.gw.addr))[2]);
        num4 = ((&(ip_info.gw.addr))[3]);

        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): IP_EVENT:IP_EVENT_STA_GOT_IP: My Gateway address " + std::to_string(num1) + "." + std::to_string(num2) + "." + std::to_string(num3) + "." + std::to_string(num4));

        num1 = ((&(ip_info.netmask.addr))[0]);
        num2 = ((&(ip_info.netmask.addr))[1]);
        num3 = ((&(ip_info.netmask.addr))[2]);
        num4 = ((&(ip_info.netmask.addr))[3]);

        if (show & _showEvents)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): IP_EVENT:IP_EVENT_STA_GOT_IP: My Netmask is " + std::to_string(num1) + "." + std::to_string(num2) + "." + std::to_string(num3) + "." + std::to_string(num4));

        haveIPAddress = true; // This will stop the wifi waiting process.
    }
    else
    {
        routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Did not include a run handler for this unknown Wifi Event");
    }
    return;

wifi_eventRun_err:
    errMsg = std::string(__func__) + "(): error: " + esp_err_to_name(ret);
    wifiOP = WIFI_OP::Error;
}
