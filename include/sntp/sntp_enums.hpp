#pragma once

#include <stdint.h> // Standard libraries

/* Class Operations */
enum class SNTP_OP : uint8_t
{
    Connect,
    Error,
    Idle,
    Idle_Silent,
};

enum class SNTP_CONN : uint8_t
{
    Start,
    Set_Time_Zone,
    Configure,
    Init,
    Waiting_For_Response,
    Error,
    Idle,
};

enum class SNTP_DISC : uint8_t
{
    Start,
    Finished,
};