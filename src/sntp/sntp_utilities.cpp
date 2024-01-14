#include "sntp/sntp_.hpp"
#include "system_.hpp"

extern SemaphoreHandle_t semLockBool;
extern SemaphoreHandle_t semLockUint8;

/* Utility Functions */
void SNTP::lockOrUint8(uint8_t *variable, uint8_t value)
{
    if (xSemaphoreTake(semLockUint8, portMAX_DELAY))
    {
        *variable |= value; // Dereference and set the value
        xSemaphoreGive(semLockUint8);
    }
}

void SNTP::lockAndUint8(uint8_t *variable, uint8_t value)
{
    if (xSemaphoreTake(semLockUint8, portMAX_DELAY))
    {
        *variable &= value; // Dereference and set the value
        xSemaphoreGive(semLockUint8);
    }
}

uint8_t SNTP::lockGetUint8(uint8_t *variable)
{
    uint8_t value = 0;
    if (xSemaphoreTake(semLockUint8, portMAX_DELAY))
    {
        value = *variable; // Dereference and return the value
        xSemaphoreGive(semLockUint8);
    }
    return value;
}
