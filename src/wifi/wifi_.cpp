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

Wifi::Wifi()
{
    // Process of creating this object:
    // 1) Get the system run task handle
    // 2) Create the SNTP object.
    // 3) Set the show flags.
    // 4) Set log levels
    // 5) Create all the semaphores
    // 6) Restore all the object variables from nvs.
    // 7) Lock the object with its entry semaphore.
    // 8) Start this object's run task.
    // 9) Done.

    if (xSemaphoreTake(semSysEntry, portMAX_DELAY)) // Get everything from the system that we need.
    {
        if (sys == nullptr)
            sys = System::getInstance();
        taskHandleSystemRun = sys->getRunTaskHandle();
        xSemaphoreGive(semSysEntry);
    }

    if (sntp == nullptr)
        sntp = new SNTP(); // Becoming RAII compliant may not be terribly interesting until the Esp32 is running asymmetric multiprocessing.

    setShowFlags();            // Enable logging statements for any area of concern.
    setLogLevels();            // Manually sets log levels for other tasks down the call stack.
    createSemaphores();        // Creates any locking semaphores owned by this object.
    restoreVariablesFromNVS(); // Brings back all our persistant data.

    xSemaphoreTake(semWifiEntry, portMAX_DELAY); // Take our semaphore to lock entry during initialization.

    wifiInitStep = WIFI_INIT::Start; // Allow the object to initialize.
    wifiOP = WIFI_OP::Init;
    xTaskCreate(runMarshaller, "wifi_run", 1024 * runStackSizeK, this, TASK_PRIORITY_MID, &taskHandleWIFIRun);
}

Wifi::~Wifi()
{
    // Process of destroying this object:
    // 1) Lock the object with its entry semaphore.
    // 2) Send a task notification to Shutdown.
    // 3) Watch the runTaskHandle (and any other possible task handles) and wait for them to clean up and then become nullptr.
    // 4) Clean up other resources created by calling task.
    // 5) UnLock the its entry semaphore.
    // 6) Destroy all semaphore at the same time (for good organization). Again, these are created by calling task in constructor.
    // 7) Done.

    // The calling task can still send taskNotifications to the wifi task!
    while (!xTaskNotify(taskHandleWIFIRun, static_cast<uint32_t>(WIFI_NOTIFY::CMD_SHUT_DOWN), eSetValueWithoutOverwrite))
        vTaskDelay(pdMS_TO_TICKS(50)); // Wait for the notification to be received.
    taskYIELD();                       // One last yeild to make sure Idle task can run.

    while (taskHandleWIFIRun != nullptr)
        vTaskDelay(pdMS_TO_TICKS(50)); // Wait for the wifi task handle to become null.
    taskYIELD();                       // One last yeild to make sure Idle task can run.

    /* Destroy sntp */
    if (sntp != nullptr)
        delete sntp; // Has no active tasks so it is simple to destroy

    xSemaphoreGive(semWifiEntry);
    destroySemaphores();

    // When the destructor returns to the caller, the entire process is finished.
}

void Wifi::setShowFlags()
{
    // show variable is system wide defined and this exposes for viewing any general processes.
    show = 0;
    show |= _showInit; // Sets this bit
    // show |= _showNVS;
    // show |= _showRun;
    // show |= _showEvents;
    // show |= _showJSONProcessing;
    // show |= _showDebugging;
    // show |= _showProcess;
    // show |= _showPayload;

    // showWifi exposes wifi sub-processes.
    showWifi = 0;
    // showWifi |= _showWifiDirectiveSteps;
    showWifi |= _showWifiConnSteps;
    showWifi |= _showWifiDiscSteps;
    showWifi |= _showWifiShdnSteps;
}

void Wifi::setLogLevels()
{

    if ((show + showWifi) > 0)                // Normally, we are interested in the variables inside our object.
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

void Wifi::destroySemaphores()
{
    // Carefully match this list of actions against the one in createSemaphores()
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
}

TaskHandle_t Wifi::getRunTaskHandle(void)
{
    return taskHandleWIFIRun;
}

QueueHandle_t Wifi::getCmdRequestQueue()
{
    return queueCmdRequests;
}
