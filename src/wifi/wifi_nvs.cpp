#include "wifi/wifi_.hpp"
#include "nvs/nvs_.hpp"
#include "system_.hpp" // Class structure and variables

#include "esp_check.h"

/* External Variables */
extern SemaphoreHandle_t semSysEntry;
extern SemaphoreHandle_t semNVSEntry;

/* NVS */
void Wifi::restoreVariablesFromNVS()
{
    esp_err_t ret = ESP_OK;
    bool successFlag = true;
    uint8_t temp = 0;

    if (nvs == nullptr)
        nvs = NVS::getInstance(); // First, get the nvs object handle if didn't already.

    if (xSemaphoreTake(semNVSEntry, portMAX_DELAY))
        ESP_GOTO_ON_ERROR(nvs->openNVSStorage("wifi"), wifi_restoreVariablesFromNVS_err, TAG, "nvs->openNVSStorage('wifi') failed");

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): wifi namespace start");

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
        ret = nvs->getBooleanFromNVS("autoConnect", &autoConnect);

        if (ret == ESP_OK)
        {
            if (show & _showNVS)
                routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): autoConnect         is " + std::to_string(autoConnect));
        }
        else
        {
            successFlag = false;
            routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error, Unable to restore autoConnect. Error = " + esp_err_to_name(ret));
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
    }
    else
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): Failed");

    nvs->closeNVStorage();
    xSemaphoreGive(semNVSEntry);
    return;

wifi_restoreVariablesFromNVS_err:
    routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error " + esp_err_to_name(ret));
    xSemaphoreGive(semNVSEntry);
}

void Wifi::saveVariablesToNVS()
{
    //
    // The best idea is to save only changed values.  Right now, we try to save everything.
    // The NVS object we call will avoid over-writing variables which already hold the identical values.
    // Later, we will add 'dirty' bits to avoid trying to save a value that hasn't changed.
    //
    esp_err_t ret = ESP_OK;
    bool successFlag = true;

    if (nvs == nullptr)
        nvs = NVS::getInstance(); // First, get the nvs object handle if didn't already.

    if (xSemaphoreTake(semNVSEntry, portMAX_DELAY))
        ESP_GOTO_ON_ERROR(nvs->openNVSStorage("wifi"), wifi_saveVariablesToNVS_err, TAG, "nvs->openNVSStorage('wifi') failed");

    if (show & _showNVS)
        routeLogByValue(LOG_TYPE::INFO, std::string(__func__) + "(): wifi namespace start");

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
        if (nvs->saveBooleanToNVS("autoConnect", autoConnect) == ESP_OK)
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
    }
    else
        routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Failed");

    nvs->closeNVStorage();
    xSemaphoreGive(semNVSEntry);
    return;

wifi_saveVariablesToNVS_err:
    routeLogByValue(LOG_TYPE::ERROR, std::string(__func__) + "(): Error " + esp_err_to_name(ret));
    xSemaphoreGive(semNVSEntry);
}
