#include "sntp/sntp_.hpp"
#include "system_.hpp" // Class structure and variables

#include "esp_check.h"

extern SNTP *ptrSNTPInternal;

/* Event Callback Functions - SNTP */
void SNTP::eventHandlerSNTPMarshaller(struct timeval *tv)
{
    ptrSNTPInternal->eventHandlerSNTP(tv);
}

//
// NOTE:  We do not allow long processes to execute inside this handler.  Our goal is to release the calling Task quickly because it has
// other more pressing work to handle.  All events are off-loaded to another run process.
//
void SNTP::eventHandlerSNTP(struct timeval *tv)
{
    // The TCP/IP stack's task (tiT) will arrive here to drive this handler.  Be sure to implement locking for any shared variables.

    if (show & _showEvents)
        ESP_LOGW(TAG, "eventHandlerSNTP()");
    lockOrUint8(&sntpEvents, _sntpTimeUpdated); // Register our event in the sntpEvents flag
}