#pragma once

#include <stdint.h> // Standard libraries
#include <string>

/* Run Notifications and Commands */
// Task Notifications should be used for notifications or commands which need no input and return no data.
enum class WIFI_NOTIFY : uint8_t
{
    NONE = 0,
    CMD_CLEAR_PRI_ROUTER,  // (Directive) Sets bit to clear SSID and PWD data for the Pri router
    CMD_DISC_ROUTER,       // (Directive) Sets bit to disconnect any router that may be currently connected
    CMD_CONN_PRI_ROUTER,   // (Directive) Sets bit to connects to the Pri router
    CMD_RUN_DIRECTIVES,    // Runs all commands set in the Directives byte
    CMD_SHUT_DOWN,         // Shuts down the wifi connection completely (ready for object deletion)
    CMD_SET_AUTOCONNECT,   // Sets flag to autoconnect
    CMD_CLEAR_AUTOCONNECT, // Clears flag to autoconnect
    CMD_PRINT_STATES,
};

// Queue based commands should be used for commands which may provide input and perhaps return data.
enum class WIFI_COMMAND : uint8_t
{
    NONE = 0,
    SET_SSID_PRI,   // Sets SSID Pri
    SET_PASSWD_PRI, // Sets PWD Pri
    SET_SHOW_FLAGS, // Enables and disables logging from a distance
};

/* Message Operations */
enum class WIFI_RESP_CODE : int8_t
{
    OK = 0,
    FAIL = -1,
};

struct WIFI_CmdRequest
{
    WIFI_COMMAND requestedCmd; //
    uint8_t data[64];          // Can hold show flags (length 2) or SSID or a Password of full length.
    uint8_t dataLength;        //
};

/* Class Operations */
enum class WIFI_OP : uint8_t
{
    Run,
    Init,
    Directives,
    Connect,
    // SNTP_Connect,
    Disconnect,
    // Provision,
    Shutdown,
    Error,
    Idle,
    Idle_Silent,
};

enum class WIFI_RUN_DIRECTIVES : uint8_t
{
    Start,
    Clear_Data,
    Disconnect_Router,
    // Provision_Router,
    Connect_Router,
    Finished,
};

enum class WIFI_INIT : uint8_t
{
    Start,
    Checks,
    Init_Queues_Commands,
    Set_Variables_From_Config,
    Auto_Connect,
    Finished,
    Error,
};

enum class WIFI_CONN : uint8_t
{
    Start,
    Create_Netif_Objects,
    // Create_Netif_SAP_Object,
    Wifi_Init,
    Register_Handlers,
    Set_Wifi_Mode,
    Set_Sta_Config,
    // Set_Ap_Config,
    Wifi_Start,
    Wifi_Waiting_To_Connect,
    Wifi_Waiting_For_IP_Address,
    Wifi_SNTP_Connect,
    Wifi_Waiting_SNTP_Valid_Time,
    Finished,
    Error,
};

enum class WIFI_DISC : uint8_t
{
    Start,
    Deinitialize_SNTP,
    Wifi_Disconnect,
    Reset_Flags,
    Unregister_Handlers,
    Destroy_Netif_Objects,
    Finished,
    Error,
};

enum class WIFI_SHUTDOWN : uint8_t
{
    Start,
    Wifi_Wait_Connection,
    Disconnect_Wifi,
    Wait_For_Disconnection,
    // Wait_For_Provisioning,
    Final_Items,
    Finished,
    Idle,
};

enum class WIFI_SNTP_CONN : uint8_t
{
    Start,
    Set_Time_Zone,
    Configure,
    SNTP_Init,
    Waiting_For_Response,
    Error,
    SNTP_Time_Valid,
    Idle,
};

/* Object States */
enum class WIFI_CONN_STATE : uint8_t
{
    NONE = 0,
    WIFI_READY_TO_CONNECT,
    WIFI_CONNECTING_STA, // We created this state to indicate that the Connect command has already been issued.
    WIFI_CONNECTED_STA,
    WIFI_DISCONNECTING_STA, // We created this state to indicate that the Disconnect command has already been issued.
    WIFI_DISCONNECTED,
};
