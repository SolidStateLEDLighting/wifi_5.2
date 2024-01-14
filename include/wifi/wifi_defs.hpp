#pragma once
#include "wifi/wifi_enums.hpp" // Local definitions, structs, and enumerations

/* showWIFI */
#define _showDrtvSteps 0x01 // LSB
#define _showConnSteps 0x02
#define _showDiscSteps 0x04
#define _showProvSteps 0x08 // NOTE: In a larger project I have universal provisioning included
#define _showShdnSteps 0x10
#define _showUnused1Steps 0x20
#define _showUnused2Steps 0x40

/* Provision Directive masks */
#define _wifiClearPriRouter 0x01  // LSB
#define _wifiClearSecdRouter 0x02 // NOTE: In a larger project I a have primary and secondary rounter.
#define _wifiDisconnectRouter 0x04
#define _wifiProvisionPriRouter 0x08
#define _wifiProvisionSecdRouter 0x10
#define _wifiConnectPriRouter 0x20
#define _wifiConnectSecdRouter 0x40
#define _wifiXXXXXXXXRouter 0x80

/* Router Status masks */
#define _rtrPriValid 0x01 // LSB
#define _rtrPriActive 0x02
#define _rtrPriUnused2 0x04
#define _rtrPriUnused3 0x08
#define _rtrSecdValid 0x10 // Secondary router is currently included inside this project
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
