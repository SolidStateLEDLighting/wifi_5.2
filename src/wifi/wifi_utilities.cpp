#include "wifi/wifi_.hpp"
#include "system_.hpp"

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