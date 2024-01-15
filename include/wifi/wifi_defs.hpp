#pragma once
#include "wifi/wifi_enums.hpp" // Local definitions, structs, and enumerations

/* showWIFI */
#define _showDiretiveSteps 0x01 // LSB
#define _showConnSteps 0x02
#define _showDiscSteps 0x04
#define _showProvSteps 0x08 // NOTE: In a larger project I have universal provisioning working
#define _showShdnSteps 0x10
#define _showUnused1Steps 0x20
#define _showUnused2Steps 0x40

/* wifiDirective */
#define _wifiClearPriHostInfo 0x01 // LSB
#define _wifiClearSecdHostInfo 0x02
#define _wifiDisconnectHost 0x04    //
#define _wifiProvisionPriHost 0x08  // NOTE: In a larger project I have service for a primary and secondary modem/router.
#define _wifiProvisionSecdHost 0x10 //
#define _wifiConnectPriHost 0x20
#define _wifiConnectSecdHost 0x40
#define _wifiXXXXXXXXHost 0x80

/* hostStatus */
#define _rtrPriValid 0x01 // LSB
#define _rtrPriActive 0x02
#define _rtrPriUnused2 0x04
#define _rtrPriUnused3 0x08
#define _rtrSecdValid 0x10 // Secondary host is currently included inside this project
#define _rtrSecdActive 0x20
#define _rtrSecdUnused2 0x40
#define _rtrSecdUnused3 0x80

/* wifiEvents */
#define _wifiEventScanDone 0x00000001
#define _wifiEventSTAStart 0x00000002
#define _wifiEventSTAStop 0x00000004
#define _wifiEventSTAConnected 0x00000008

#define _wifiEventSTADisconnected 0x00000010
#define _wifiEventAPStart 0x00000020
#define _wifiEventAPStop 0x00000040
#define _wifiEventAPConnected 0x00000080

#define _wifiEventAPDisconnected 0x00000100
#define _wifiEventBeaconTimeout 0x00000200
#define _ipEventSTAGotIp 0x00000400
