#include "wifi/wifi_.hpp"
#include "system_.hpp" // Class structure and variables

/* Debugging */
void Wifi::printTaskInfoByColumns() // This function is called when the System wants to compile an entire table of task information.
{
    char *name = pcTaskGetName(taskHandleWIFIRun); // Note: The value of NULL can be used as a parameter if the statement is running on the task of your inquiry.
    uint32_t priority = uxTaskPriorityGet(taskHandleWIFIRun);
    uint32_t highWaterMark = uxTaskGetStackHighWaterMark(taskHandleWIFIRun);
    printf("  %-10s   %02ld           %ld\n", name, priority, highWaterMark);
}

void Wifi::logTaskInfo()
{
    char *name = pcTaskGetName(NULL); // Note: The value of NULL can be used as a parameter if the statement is running on the task of your inquiry.
    uint32_t priority = uxTaskPriorityGet(NULL);
    uint32_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
    routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): name: " + std::string(name) + " priority: " + std::to_string(priority) + " highWaterMark: " + std::to_string(highWaterMark));
}
