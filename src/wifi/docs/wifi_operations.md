# Wifi Operations
We define a list of wifi operations in wifi_enum.hpp.  There are currently 8.  These operations lead to all other sub-operations within the Wifi object.

* Run Operation
* Shutdown Operation
* Init Operation
* Directive Operation
* Connect Operation
* Disconnect Operation
* Error Operation
* Idle Operation  
![Wifi Operations](./drawings/wifi_operations_block.svg)

### Run Operation
In all objects, a Run operation (contained inside a Run task) is centeral to it's normal operation.  Most of a task's time is spent here.  The Run operation watches for any RTOS communications, it looks for pending actions, and sometimes state changes.  The object's Run task will always return to the centeralized Run operation when waiting is require before the next required action.  In most cases, we have set the loop cycles to 4Hz, but this could be adjusted if processing in any particular object requires a lower latency response time.

### Shutdown Operation
The Shutdown Operation is a fast method to disconnect wifi and destory the containted SNTP object.  You would call Shutdown when you intend to destory Wifi or do an organized reboot of the entire device.  For the more gentile approach to closing down a connection, use the Directive Operation option.  In Directives, they system wait patienctly for a Connection to be stable before it commences with a Disconnection process.

### Init Operation
Most objects have an initialization startup requirement.  Initialization typically occurs right after the creation of the object and during this time the object is usually locked so the outside world can't interfere with it's initialization.   At this time, we don't look for RTOS communication.

### Directives Operation
The directives is a byte of data that helps the Wifi complete one or more operations in a sequence.  Once the directives byte is set by a series of notifications, then another task notificiation command is sent to start the directives.  The directive will always run in order based on the ordered structure of the directives.

### Connect Operation
Asks Wifi to connection to a host.  

### Disconnect Operation
Asks Wifi to disconnection from a host.

### Error Operation
All Error operations everywhere try to handle any recoverable errors at the level for which they exist.  Usually, errors are forwarded on until they reach their highest level and then an error message is routed to a message handler.

### Idle Operation
Idle operation is more of a developmental tool where you can intercept unexepected results.
___  