#pragma once

#include <stdint.h> // Standard libraries
#include <string>

/* Run Notifications and Commands */
// Task Notifications should be used for notifications or commands which need no input and return no data.
enum class WIFI_NOTIFY : uint8_t
{
    CMD_CLEAR_PRI_HOST = 1, // (Directive) Sets bit to clear SSID and PWD data for the Pri host
    CMD_DISC_HOST,          // (Directive) Sets bit to disconnect any host that may be currently connected
    CMD_CONN_PRI_HOST,      // (Directive) Sets bit to connects to the Pri host
    CMD_RUN_DIRECTIVES,     // Runs all commands set in the Directives byte
    CMD_SET_AUTOCONNECT,    // Sets flag to autoconnect
    CMD_CLEAR_AUTOCONNECT,  // Clears flag to autoconnect
    CMD_SHUT_DOWN,          // Shuts down the wifi connection completely and calls for deletion.
};

// Queue based commands should be used for commands which may provide input and perhaps return data.
enum class WIFI_COMMAND : uint8_t
{
    SET_SSID_PRI = 1, // Sets SSID Pri
    SET_PASSWD_PRI,   // Sets PWD Pri
    SET_SHOW_FLAGS,   // Enables and disables logging from a distance
};

struct WIFI_CmdRequest
{
    WIFI_COMMAND requestedCmd; //
    uint8_t data[64];          // Can hold show flags (length 2) or SSID or a Password of full length.
    uint8_t dataLength;        //
};

struct WIFI_Event
{
    const char *event_base;
    int32_t event_id;
};

/* Class Operations */
enum class WIFI_OP : uint8_t
{
    Run = 1,
    Shutdown,
    Init,
    Directives,
    Connect,
    Disconnect,
    Error,
    Idle,
};

enum class WIFI_SHUTDOWN : uint8_t
{
    Start,
    Cancel_Directives,
    Disconnect_Wifi,
    Wait_For_Disconnection,
    Finished,
};

enum class WIFI_INIT : uint8_t
{
    Start,
    Checks,
    Set_Variables_From_Config,
    Auto_Connect,
    Finished,
    Error,
};

enum class WIFI_DIRECTIVES : uint8_t
{
    Start,
    Clear_Data,
    Disconnect_Host,
    Connect_Host,
    Finished,
};

enum class WIFI_CONN : uint8_t
{
    Start,
    Create_Netif_Objects,
    Wifi_Init,
    Register_Handlers,
    Set_Wifi_Mode,
    Set_Sta_Config,
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
    Cancel_Connect,
    Deinitialize_SNTP,
    Wifi_Disconnect,
    Reset_Flags,
    Unregister_Handlers,
    Wifi_Deinit,
    Destroy_Netif_Objects,
    Finished,
    Error,
};

/* Connection States */
enum class WIFI_CONN_STATE : uint8_t
{
    NONE = 0,
    WIFI_READY_TO_CONNECT,
    WIFI_CONNECTING_STA, // We created this state to indicate that the Connect command has already been issued.
    WIFI_CONNECTED_STA,
    WIFI_DISCONNECTING_STA, // We created this state to indicate that the Disconnect command has already been issued.
    WIFI_DISCONNECTED,
};
