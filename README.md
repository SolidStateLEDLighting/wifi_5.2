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
___  
## SNTP Object
We create a wrapper object to abstract the IDF's Simple Network Time Protocol funtions. This is a companion to the Wifi and it does not run its own task.  The Wifi object calls the SNTP object's run function periodically to service all its needs.  The SNTP functions on its own.
___  
