#include "wifi/wifi_.hpp"
#include "system_.hpp"

extern SemaphoreHandle_t semLockBool;
extern SemaphoreHandle_t semLockUint32;

bool Wifi::allOperationsFinished() // NOTE: This excludes the Run and Directives operation
{
    if (wifiShdnStep != WIFI_SHUTDOWN::Finished)
        return false;

    if (wifiInitStep != WIFI_INIT::Finished)
        return false;

    if (wifiConnStep != WIFI_CONN::Finished)
        return false;

    if (wifiDiscStep != WIFI_DISC::Finished)
        return false;

    return true;
}

/* Variable Locking */
void Wifi::lockOrUint32(uint32_t *variable, uint32_t value)
{
    if (xSemaphoreTake(semLockUint32, portMAX_DELAY))
    {
        *variable |= value; // Dereference and set the value
        xSemaphoreGive(semLockUint32);
    }
}

void Wifi::lockAndUint32(uint32_t *variable, uint32_t value)
{
    if (xSemaphoreTake(semLockUint32, portMAX_DELAY))
    {
        *variable &= value; // Dereference and set the value
        xSemaphoreGive(semLockUint32);
    }
}

uint32_t Wifi::lockGetUint32(uint32_t *variable)
{
    uint32_t value = 0;
    if (xSemaphoreTake(semLockUint32, portMAX_DELAY))
    {
        value = *variable; // Dereference and return the value
        xSemaphoreGive(semLockUint32);
    }
    return value;
}