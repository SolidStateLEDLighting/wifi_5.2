# SNTP Operations

We define a list of SNTP operations in sntp_enum.hpp.  There are currently 3.  These operations lead to all other sub-operations within the sntp object.

There is no run operation here because the SNTP object has no run task.  SNTP is a support class to the Wifi and doesn't need very much processing power.  The Wifi object will call the SNTP::run() and it will route to initialization.   Once the SNTP object is initialized, it waits for events to arrive from the IDF that indicate that time information has been received.

* Init Operation
* Error Operation
* Idle Operation
![Run Operation Diagram](./drawings/sntp_operations_block.svg)
___  