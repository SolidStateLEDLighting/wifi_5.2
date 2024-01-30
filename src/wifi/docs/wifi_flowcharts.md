# Wifi Flowcharts
Here are the flowcharts that represent active task and the event handler.  

## wifi_run.cpp flowchart
This is the most important area of processing for the Wifi object.
![Wifi Run Flowchart](./drawings/wifi_flowchart_run.svg)

Here is our Event handing in the run task:

## wifi_events.cpp flowchart
Events arrive at the Wifi Object via the default event loop.  Our strategy is always to quickly dispose of events by copying any provided data, and then marshelling all operational action over to the Run task.  This makes all event handling native to the Wifi object's run thread and removes any possible conflict between the default event loop task and the object's run task.

First, this is raw event handling:
![Event Handling Flowchart](./drawings/wifi_flowchart_events.svg)