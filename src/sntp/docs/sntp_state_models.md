# SNTP State Transition Diagrams  
There is one state variable:
* sntpOP - Operation state.
![Run State Model](./drawings/sntp_state_model_run.svg)
___  
## Connection Operation
This runs only once at the start of the SNTP object.  It is responsible for getting the sntp client started inside the IDF.
![Connect State Model](./drawings/sntp_state_model_connect.svg)
___  
