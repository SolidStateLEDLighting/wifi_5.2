#include "wifi/wifi_.hpp"
#include "system_.hpp" // Class structure and variables

#include "freertos/task.h" // RTOS libraries

#include "esp_check.h"

/* Local Semaphores */
SemaphoreHandle_t semWifiEntry = NULL;
SemaphoreHandle_t semWifiRouteLock = NULL;
SemaphoreHandle_t semLockBool = NULL;
SemaphoreHandle_t semLockUint8 = NULL;
SemaphoreHandle_t semLockUint32 = NULL;

/* External Semaphores */
extern SemaphoreHandle_t semSysEntry;

//
//
// Menu Config Settings
//
// [*] Keep TCP connections when IP changed via LwIP menuconfig.
//
//

Wifi::Wifi()
{
    if (xSemaphoreTake(semSysEntry, portMAX_DELAY)) // Get everything from the system that we need.
    {
        if (sys == nullptr)
            sys = System::getInstance();
        taskHandleSystemRun = sys->get_runTaskHandle();
        xSemaphoreGive(semSysEntry);
    }

    sntp = new SNTP(); // To keep code simple, only instantiate and destroy inside contructor and destructor

    setShowFlags();            // Enable logging statements for any area of concern.
    setLogLevels();            // Manually sets log levels for other tasks down the call stack.
    createSemaphores();        // Create any locking semaphores owned by this object.
    restoreVariablesFromNVS(); // Bring back all our persistant data.

    wifiInitStep = WIFI_INIT::Start; // Allow the object to initialize.
    wifiOP = WIFI_OP::Init;

    xSemaphoreTake(semWifiEntry, portMAX_DELAY); // Take our semaphore to lock entry during initialization.

    xTaskCreate(runMarshaller, "wifi_run", 1024 * runStackSizeK, this, 7, &taskHandleWIFIRun); // Tasks
}

Wifi::~Wifi()
{
    // Please run the Disconnect process before arriving here.

    /* Destroy sntp */
    if (sntp != nullptr)
        sntp->~SNTP();

    /* Dispose of the Run task and handle */
    if (taskHandleWIFIRun != nullptr)
    {
        vTaskDelete(taskHandleWIFIRun);
        taskHandleWIFIRun = nullptr;
    }

    /* Delete Semaphores */
    if (semWifiEntry != nullptr)
    {
        vSemaphoreDelete(semWifiEntry);
        semWifiEntry = nullptr;
    }

    if (semWifiRouteLock != nullptr)
    {
        vSemaphoreDelete(semWifiRouteLock);
        semWifiRouteLock = nullptr;
    }

    if (semLockBool != nullptr)
    {
        vSemaphoreDelete(semLockBool);
        semLockBool = nullptr;
    }

    if (semLockUint8 != nullptr)
    {
        vSemaphoreDelete(semLockUint8);
        semLockUint8 = nullptr;
    }

    if (semLockUint32 != nullptr)
    {
        vSemaphoreDelete(semLockUint32);
        semLockUint32 = nullptr;
    }

    /* Delete Command Queue items*/
    if (ptrWifiCmdRequest != nullptr)
    {
        delete ptrWifiCmdRequest;
        ptrWifiCmdRequest = nullptr;
    }

    if (queueCmdRequests != nullptr)
    {
        vQueueDelete(queueCmdRequests);
        queueCmdRequests = nullptr;
    }
}

void Wifi::setShowFlags()
{
    show = 0;
    // show |= _showInit; // Sets this bit
    // show |= _showNVS;
    // show |= _showRun;
    // show |= _showEvents;
    // show |= _showJSONProcessing;
    // show |= _showDebugging;
    // show |= _showProcess;
    // show |= _showPayload;

    showWIFI = 0;
    // showWIFI |= _showDirectiveSteps;
    // showWIFI |= _showConnSteps;
    // showWIFI |= _showDiscSteps;
    // showWIFI |= _showShdnSteps;
}

void Wifi::setLogLevels()
{

    if ((show + showWIFI) > 0)                // Normally, we are interested in the variables inside our object.
        esp_log_level_set(TAG, ESP_LOG_INFO); // If we have any flags set, we need to be sure to turn on the logging so we can see them.
    else
        esp_log_level_set(TAG, ESP_LOG_ERROR); // Likewise, we turn off logging if we are not looking for anything.

    //
    // To increase log information - set any or all the levels below at ESP_LOG_INFO, or ESP_LOG_DEBUG or ESP_LOG_VERBOSE
    //

    // We currently define our TAG "_wifi", but there is another TAG definition below ours in the IDF library that is also called "wifi".
    esp_log_level_set("wifi", ESP_LOG_ERROR);

    esp_log_level_set("esp-tls", ESP_LOG_ERROR);   // All these TAGs will reveal other information if the level is lowered.  Comment these out
    esp_log_level_set("wifi_init", ESP_LOG_ERROR); // to view any default log information that is normally generated.
    esp_log_level_set("phy_init", ESP_LOG_ERROR);
    esp_log_level_set("esp_netif_lwip", ESP_LOG_ERROR);
    esp_log_level_set("esp_netif_handlers", ESP_LOG_ERROR);
}

void Wifi::createSemaphores()
{
    semWifiEntry = xSemaphoreCreateBinary(); // External access semaphore
    if (semWifiEntry != NULL)
        xSemaphoreGive(semWifiEntry); // Initialize the Semaphore

    semWifiRouteLock = xSemaphoreCreateBinary();
    if (semWifiRouteLock != NULL)
        xSemaphoreGive(semWifiRouteLock);

    semLockBool = xSemaphoreCreateBinary();
    if (semLockBool != NULL)
        xSemaphoreGive(semLockBool);

    semLockUint8 = xSemaphoreCreateBinary();
    if (semLockUint8 != NULL)
        xSemaphoreGive(semLockUint8);

    semLockUint32 = xSemaphoreCreateBinary();
    if (semLockUint32 != NULL)
        xSemaphoreGive(semLockUint32);
}

TaskHandle_t Wifi::getRunTaskHandle(void)
{
    return taskHandleWIFIRun;
}

QueueHandle_t Wifi::getCmdRequestQueue()
{
    return queueCmdRequests;
}
