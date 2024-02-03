#include "sntp/sntp_.hpp"
#include "system_.hpp" // Class structure and variables
//
// I bring most logging formation here (inside each object) because in a more advanced project, I route logging
// information back to the cloud.  We could also just as easily log to a file storage location like an SD card.
//
// At is also at this location (in my more advanced project) that I store Error information to Flash.  This makes is possible
// to transmit error logging to the cloud after a reboot.
//
/* External Semaphores */
extern SemaphoreHandle_t semSNTPRouteLock;

/* Logging */
// Logging by reference potentially allows a better algorithm for accessing large data throught a pointer.
void SNTP::routeLogByRef(LOG_TYPE _type, std::string *_msg)
{
    if (xSemaphoreTake(semSNTPRouteLock, portMAX_DELAY)) // We use this lock to prevent sys_evt and wifi_run tasks from having conflicts
    {
        LOG_TYPE type = _type;   // Copy our parameters upon entry before they are over-written by another calling task.
        std::string *msg = _msg; // This will point back to the caller's variable.

        switch (type)
        {
        case LOG_TYPE::ERROR:
        {
            ESP_LOGE(TAG, "%s", (*msg).c_str()); // Print out our errors here so we see it in the console.
            break;
        }

        case LOG_TYPE::WARN:
        {
            ESP_LOGW(TAG, "%s", (*msg).c_str()); // Print out our warning here so we see it in the console.
            break;
        }

        case LOG_TYPE::INFO:
        {
            ESP_LOGI(TAG, "%s", (*msg).c_str()); // Print out our information here so we see it in the console.
            break;
        }
        }

        xSemaphoreGive(semSNTPRouteLock);
    }
}

void SNTP::routeLogByValue(LOG_TYPE _type, std::string _msg)
{
    if (xSemaphoreTake(semSNTPRouteLock, portMAX_DELAY)) // We use this lock to prevent sys_evt and wifi_run tasks from having conflicts
    {
        LOG_TYPE type = _type; // Copy our parameters upon entry before they are over-written by another calling task.
        std::string msg = _msg;

        switch (type)
        {
        case LOG_TYPE::ERROR:
        {
            ESP_LOGE(TAG, "%s", (msg).c_str()); // Print out our errors here so we see it in the console.
            break;
        }

        case LOG_TYPE::WARN:
        {
            ESP_LOGW(TAG, "%s", (msg).c_str()); // Print out our warning here so we see it in the console.
            break;
        }

        case LOG_TYPE::INFO:
        {
            ESP_LOGI(TAG, "%s", (msg).c_str()); // Print out our information here so we see it in the console.
            break;
        }
        }

        xSemaphoreGive(semSNTPRouteLock);
    }
}
