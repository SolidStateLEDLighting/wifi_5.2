# Wifi 5.2 Overview

**This component includes both Wifi and SNTP functionality**
___  
## Wifi Object

One notable behavior that is important to observe is that the process of connecting to Wifi takes a fair amount of time. It is composed of 3 parts:
* Connect to a Host
* Obtaining an IP Address
* Contacting an SNTP server and requesting epoch time

This process can take 5 to 10 seconds.  If you issue any kind of command to terminate the process before a connection is made, then the complexity of reclaiming resouces becomes more difficult to track.

The first and best way to disconnect is to wait until the connection process is complete first, then disconnect.  This is the low risk process.  It takes more time, but the results are more predictable.

The alternative choice is to invoke a **Shutdown**.   This is a faster process but the paths for unexpected behavior are increased.

For example, if you are putting the unit to Sleep, the normal disconnection is the recommended approach.  But, if you are going to reboot or restart, then a Shutdown is permissable.

### Observational Note:
The Wifi connection/disconnection process has to be done just right for your unit to correctly entire power saving modes.  Through a very long series of tests and experimentation, it is clear that sloppy programming will lead to Esp modules that will run hotter than expected.   If you follow all the details, the results should be so good, that you feel very little extra heat on the Esp32 unit which it is sitting there idle wirelessly connected to a host. 

You may follow these links to Wifi documentation:
1) [Wifi Abstractions](./src/wifi/docs/wifi_abstractions.md)
2) [Wifi Block Diagram](./src/wifi/docs/wifi_blocks.md)
3) [Wifi Flowcharts](./src/wifi/docs/wifi_flowcharts.md)
4) [Wifi Operations](./src/wifi/docs/wifi_operations.md)
5) [Wifi Sequences](./src/wifi/docs/wifi_sequences.md)
6) [Wifi State Models](./src/wifi/docs/wifi_state_models.md)  
___  
## SNTP Object
We create a wrapper object to abstract the IDF's Simple Network Time Protocol functions. This is a companion to the Wifi object and it does not run its own task.  The Wifi object calls the SNTP object's run function periodically to service all its needs.  One of the primary benefits having the Wifi task run the SNTP command loops is that we don't any RTOS locking to share variables between those two objects.  
___  
