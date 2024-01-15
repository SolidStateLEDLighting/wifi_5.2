#include "wifi/wifi_.hpp"
#include "nvs/nvs_.hpp"
#include "system_.hpp" // Class structure and variables

#include "esp_check.h"

/* External Variables */
extern SemaphoreHandle_t semSysEntry;
extern SemaphoreHandle_t semNVSEntry;

/* NVS */
bool Wifi::restoreVariablesFromNVS()
{
    uint8_t temp = 0;

    if (nvs == nullptr)
        nvs = NVS::getInstance(); // First, get the nvs object handle if didn't already.

    if (xSemaphoreTake(semNVSEntry, portMAX_DELAY) == pdTRUE)
    {
        if (nvs == nullptr)
            nvs = NVS::getInstance(); // First, get the nvs object handle if didn't already.

        if (nvs->openNVSStorage("wifi", true) == false) // We always open the storage in Read/Write mode because some small changes may happen during the restoral process.
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to nvs->openNVStorage()");
            xSemaphoreGive(semNVSEntry);
            return false;
        }
    }

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): wifi namespace start");

    bool successFlag = true;

    if (successFlag) // Restore runStackSizeK
    {
        temp = runStackSizeK; // This will save the default size if value doesn't exist yet in nvs.

        if (nvs->getU8IntegerFromNVS("runStackSizeK", &temp))
        {
            if (temp > runStackSizeK) // Ok to use any value greater than the default size.
            {
                runStackSizeK = temp;
                saveToNVSDelayCount = 8; // Save it
            }

            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): runStackSizeK       is " + std::to_string(runStackSizeK));
        }
        else
        {
            successFlag = false;
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to restore runStackSizeK");
        }
    }

    if (successFlag) // Restore autoConnect
    {
        if (nvs->getBooleanFromNVS("autoConnect", &autoConnect))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): autoConnect         is " + std::to_string(autoConnect));
        }
        else
        {
            successFlag = false;
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to restore autoConnect");
        }
    }

    if (successFlag) // Restore hostStatus
    {
        if (nvs->getU8IntegerFromNVS("hostStatus", &hostStatus))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): hostStatus          is " + std::to_string(hostStatus));
        }
        else
        {
            successFlag = false;
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to restore hostStatus");
        }
    }

    if (successFlag) // Restore ssidPri
    {
        if (nvs->getStringFromNVS("ssidPri", &ssidPri))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): ssidPri             is " + ssidPri);
        }
        else
        {
            successFlag = false;
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to restore ssidPri");
        }
    }

    if (successFlag) // Restore ssidPwdPri
    {
        if (nvs->getStringFromNVS("ssidPwdPri", &ssidPwdPri))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): ssidPwdPri          is " + ssidPwdPri);
        }
        else
        {
            successFlag = false;
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to restore ssidPwdPri");
        }
    }

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): wifi namespace end");

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

bool Wifi::saveVariablesToNVS()
{
    //
    // The best idea is to save only changed values.  Right now, we try to save everything.
    // The NVS object we call will avoid over-writing variables which already hold the correct value.
    // Later, we may try to add and track 'dirty' bits to avoid trying to save a value that hasn't changed.
    //
    if (nvs == nullptr)
        nvs = NVS::getInstance(); // First, get the nvs object handle if didn't already.

    if (xSemaphoreTake(semNVSEntry, portMAX_DELAY) == pdTRUE)
    {
        if (nvs->openNVSStorage("wifi", true) == false) // Read/Write
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to nvs->openNVStorage()");
            xSemaphoreGive(semNVSEntry);
            return false;
        }
    }

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): wifi namespace start");

    bool successFlag = true;

    if (successFlag) // Save runStackSizeK
    {
        if (nvs->saveU8IntegerToNVS("runStackSizeK", runStackSizeK))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): runStackSizeK       = " + std::to_string(runStackSizeK));
        }
        else
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Unable to save runStackSizeK");
            successFlag = false;
        }
    }

    if (successFlag) // Save autoConnect
    {
        if (nvs->saveBooleanToNVS("autoConnect", autoConnect))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): autoConnect         = " + std::to_string(autoConnect));
        }
        else
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Unable to save autoConnect");
            successFlag = false;
        }
    }

    if (successFlag) // Save hostStatus
    {
        if (nvs->saveU8IntegerToNVS("hostStatus", hostStatus))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): hostStatus          = " + std::to_string(hostStatus));
        }
        else
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Unable to save hostStatus");
            successFlag = false;
        }
    }

    if (successFlag) // Save ssidPri
    {
        if (nvs->saveStringToNVS("ssidPri", &ssidPri))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): ssidPri             = " + ssidPri);
        }
        else
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Unable to save ssidPri");
            successFlag = false;
        }
    }

    if (successFlag) // Save ssidPwdPri
    {
        if (nvs->saveStringToNVS("ssidPwdPri", &ssidPwdPri))
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): ssidPwdPri          = " + ssidPwdPri);
        }
        else
        {
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Unable to save ssidPwdPri");
            successFlag = false;
        }
    }

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): wifi namespace end");

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
