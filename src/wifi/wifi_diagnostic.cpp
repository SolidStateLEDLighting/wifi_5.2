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
