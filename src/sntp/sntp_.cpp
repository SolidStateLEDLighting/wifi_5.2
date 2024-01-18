#include "sntp/sntp_.hpp"
#include "system_.hpp" // Class structure and variables

#include "esp_check.h"

SNTP *ptrSNTPInternal = nullptr; // Hold 'this' pointer for the SNTP event handlers that can't callback with the 'this' pointer.

/* Local Semaphores */
SemaphoreHandle_t semSNTPEntry = NULL;
SemaphoreHandle_t semSNTPRouteLock = NULL;

SNTP::SNTP()
{
    ptrSNTPInternal = this; // We plan on removing this someday when all ESP event handlers can be passed a 'this' pointer during registration.

    setShowFlags();            // Enable logging statements for any area of concern.
    setLogLevels();            // Manually sets log levels for other tasks down the call stack.
    createSemaphores();        // Create any locking semaphores owned by this object.
    restoreVariablesFromNVS(); // Bring back all our persistant data.
}

SNTP::~SNTP()
{
    if (semSNTPRouteLock != nullptr)
    {
        vSemaphoreDelete(semSNTPRouteLock);
        semSNTPRouteLock = nullptr;
    }

    if (semSNTPEntry != nullptr)
    {
        vSemaphoreDelete(semSNTPEntry);
        semSNTPEntry = nullptr;
    }
}

void SNTP::setShowFlags()
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

    showSNTP = 0;
    // showSNTP |= _showSNTPConnSteps;
}

void SNTP::setLogLevels()
{
    if ((show + showSNTP) > 0)                // Normally, we are interested in the variables inside our object.
        esp_log_level_set(TAG, ESP_LOG_INFO); // If we have any flags set, we need to be sure to turn on the logging so we can see them.
    else
        esp_log_level_set(TAG, ESP_LOG_ERROR); // Likewise, we turn off logging if we are not looking for anything.

    esp_log_level_set("sntp", ESP_LOG_ERROR);
}

void SNTP::createSemaphores()
{
    semSNTPEntry = xSemaphoreCreateBinary(); // External access semaphore
    if (semSNTPEntry != NULL)
    {
        xSemaphoreGive(semSNTPEntry);                // Initialize the Semaphore
        xSemaphoreTake(semSNTPEntry, portMAX_DELAY); // Take the semaphore to lock entry for initialization.
    }

    semSNTPRouteLock = xSemaphoreCreateBinary();
    if (semSNTPRouteLock != NULL)
        xSemaphoreGive(semSNTPRouteLock);
}
