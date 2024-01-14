#include "wifi/wifi_.hpp"
#include "system_.hpp"

extern SemaphoreHandle_t semLockBool;
extern SemaphoreHandle_t semWifiUint32Lock;

/* Utility Functions */
void Wifi::lockOrUint32(uint32_t *variable, uint32_t value)
{
    if (xSemaphoreTake(semWifiUint32Lock, portMAX_DELAY))
    {
        *variable |= value; // Dereference and set the value
        xSemaphoreGive(semWifiUint32Lock);
    }
}

void Wifi::lockAndUint32(uint32_t *variable, uint32_t value)
{
    if (xSemaphoreTake(semWifiUint32Lock, portMAX_DELAY))
    {
        *variable &= value; // Dereference and set the value
        xSemaphoreGive(semWifiUint32Lock);
    }
}

uint32_t Wifi::lockGetUint32(uint32_t *variable)
{
    uint32_t value = 0;
    if (xSemaphoreTake(semWifiUint32Lock, portMAX_DELAY))
    {
        value = *variable; // Dereference and return the value
        xSemaphoreGive(semWifiUint32Lock);
    }
    return value;
}