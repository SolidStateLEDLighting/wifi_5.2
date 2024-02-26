# Wifi Flowcharts
Here are the flowcharts that represent the run task and the event handler.  

Keep in mind that some logging and debugging statements have been omitted for clarity.  You may also see just a bit of pseudo code when appropriate. 
___  
## wifi_run.cpp flowcharts
This is the most important area of processing for the Wifi object.

**NOTE: This drawing has become so large that you may not be able to view it inside a browser. You may be forced to download it and view it locally.**

**THIS CHART IS BEING UPDATED**  

___  
Here is our Event handing in the run task:

**THIS CHART IS BEING UPDATED**  

___  
## wifi_events.cpp flowchart
Events arrive at the Wifi Object via the default event loop.  Our strategy is always to quickly dispose of events by copying any provided data, and then marshelling all operational action over to the Run task.  This makes all event handling native to the Wifi object's run thread and removes any possible resource access conflict between the default event loop task and the wifi object's run task.

**THIS CHART IS BEING UPDATED**  

___  