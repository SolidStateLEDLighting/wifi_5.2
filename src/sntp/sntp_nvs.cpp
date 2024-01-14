#include "sntp/sntp_.hpp"
#include "nvs/nvs_.hpp"
#include "system_.hpp" // Class structure and variables

#include "esp_check.h"

/* External Variables */
extern SemaphoreHandle_t semSysEntry;
extern SemaphoreHandle_t semNVSEntry;

/* NVS */
bool SNTP::restoreVariablesFromNVS()
{
    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "()");

    if (xSemaphoreTake(semNVSEntry, portMAX_DELAY) == pdTRUE)
    {
        if (nvs == nullptr)
            nvs = NVS::getInstance(); // First, get the nvs object handle if didn't already.

        if (nvs->openNVSStorage("sntp", true) == false) // We always open the storage in Read/Write mode because some small changes may happen during the restoral process.
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to nvs->openNVStorage()");
            xSemaphoreGive(semNVSEntry);
            return false;
        }
    }

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): sntp namespace start");

    bool successFlag = true;

    if (successFlag) // Restore serverIndex
    {
        if (nvs->getU8IntegerFromNVS("serverIndex", &serverIndex))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): serverIndex is " + std::to_string(serverIndex));
        }
        else
        {
            successFlag = false;
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to restore serverIndex");
        }
    }

    if (successFlag) // Restore sntpTimeZone
    {
        // Grab a snapshot of the default value.  Typically this is accurate during startup as restoreVariablesFromNVS() is called almost never during normal operation.
        std::string holder = timeZone;

        if (nvs->getStringFromNVS("timeZone", &timeZone))
        {
            if (timeZone.length() < 1) // Do not allow empty strings for thie variable in nvs.
            {
                timeZone = holder;                           // If the stored variable is of zero length,
                nvs->saveStringToNVS("timeZone", &timeZone); // save the default value to nvs.
            }

            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): timeZone is " + timeZone); // Confirm our restored value
        }
        else
        {
            successFlag = false;
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to restore timeZone");
        }
    }

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): sntp namespace end");

    if (successFlag)
    {
        if (show & _showNVS)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Success");
        nvs->closeNVStorage(true); // Commit changes if any are staged.
    }
    else
    {
        nvs->closeNVStorage(false); // No changes
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Failed");
    }

    xSemaphoreGive(semNVSEntry);
    return successFlag; // Flag holds the correct return value
}

bool SNTP::saveVariablesToNVS()
{
    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Start");

    //
    // The best idea is to save only changed values.  Right now, we try to save everything.
    //
    if (xSemaphoreTake(semNVSEntry, portMAX_DELAY) == pdTRUE)
    {
        if (nvs == nullptr)
            nvs = NVS::getInstance(); // First, get the nvs object handle if didn't already.

        if (nvs->openNVSStorage("sntp", true) == false) // Read/Write
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to nvs->openNVStorage()");
            xSemaphoreGive(semNVSEntry);
            return false;
        }
    }

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): sntp namespace start");

    bool successFlag = true;

    if (successFlag) // Save serverIndex
    {
        if (nvs->saveU8IntegerToNVS("serverIndex", serverIndex))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): serverIndex = " + std::to_string(serverIndex));
        }
        else
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Unable to save serverIndex");
            successFlag = false;
        }
    }

    if (successFlag) // Save timeZone
    {
        if (nvs->saveStringToNVS("timeZone", &timeZone))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): timeZone = " + timeZone);
        }
        else
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Unable to save timeZone");
            successFlag = false;
        }
    }

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): sntp namespace end");

    if (successFlag)
    {
        if (show & _showNVS)
            routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Success");
        nvs->closeNVStorage(true); // Commit changes
    }
    else
    {
        routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Failed");
        nvs->closeNVStorage(false); // No changes
    }

    xSemaphoreGive(semNVSEntry);
    return successFlag; // Flag holds the correct return value
}
