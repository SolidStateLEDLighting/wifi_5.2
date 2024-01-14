#include "wifi/wifi_.hpp"
#include "system_.hpp" // Class structure and variables

/* Debugging */
void Wifi::printTaskInfo()
{
    char *name = pcTaskGetName(taskHandleWIFIRun); // Note: The value of NULL can be used as a parameter if the statement is running on the task of your inquiry.
    uint32_t priority = uxTaskPriorityGet(taskHandleWIFIRun);
    uint32_t highWaterMark = uxTaskGetStackHighWaterMark(taskHandleWIFIRun);
    printf("  %-10s   %02ld priority   %ld highWaterMark\n", name, priority, highWaterMark);
}

void Wifi::printOpState() // During development and testing it is sometimes helpful to print out the operation and sub-operational states.
{
    if (wifiOP == WIFI_OP::Run)
    {
        ESP_LOGW(TAG, "Op is Run");
        ESP_LOGW(TAG, "Init %d", (int)wifiInitStep);
        ESP_LOGW(TAG, "Directives %d", (int)wifiDirectivesStep);
        ESP_LOGW(TAG, "Connect %d", (int)wifiConnStep);
        ESP_LOGW(TAG, "Disconnect %d", (int)wifiDiscStep);
        ESP_LOGW(TAG, "Shutdown %d", (int)wifiShdnStep);
    }
    else if (wifiOP == WIFI_OP::Init)
        ESP_LOGW(TAG, "Op/State is Init/%d", (int)wifiInitStep);
    else if (wifiOP == WIFI_OP::Directives)
        ESP_LOGW(TAG, "Op/State is Directives/%d", (int)wifiDirectivesStep);
    else if (wifiOP == WIFI_OP::Connect)
        ESP_LOGW(TAG, "Op/State is Connect/%d", (int)wifiConnStep);
    else if (wifiOP == WIFI_OP::Disconnect)
        ESP_LOGW(TAG, "Op/State is Disconnect/%d", (int)wifiDiscStep);
    else if (wifiOP == WIFI_OP::Shutdown)
        ESP_LOGW(TAG, "Op/State is Shutdown/%d", (int)wifiShdnStep);
    else if (wifiOP == WIFI_OP::Shutdown)
        ESP_LOGW(TAG, "Op is Error");
    else if (wifiOP == WIFI_OP::Idle)
        ESP_LOGW(TAG, "Op is Idle");
    else if (wifiOP == WIFI_OP::Idle_Silent)
        ESP_LOGW(TAG, "Op is Idle_Silent");
}
