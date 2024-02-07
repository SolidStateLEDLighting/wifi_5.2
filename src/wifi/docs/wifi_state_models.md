# Wifi State Transition Diagrams 
There is one state variable with eight opertional states:
* wifiOP - Operation state variable.
![Run State Model](./drawings/wifi_state_model.svg)
___  
This is the wifi connection state process.  It is a bit complex in that it has 3 timer states that watch for WIFI_EVENT_STA_CONNECTED, IP_EVENT_STA_GOT_IP, and Epoch time.   If any of those timer states fail, the system restarts wifi.  This model isn't perfect, but it's fairly close to accurate.
![Connect State Model](./drawings/wifi_state_model_connect.svg)
___  
The disconnection state process will be placed here...
___   
The directives state process will be placed here...
___  
The shutdown state process will be placed here...
___  