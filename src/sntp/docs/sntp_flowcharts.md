# SNTP Flowcharts  
*This flow chart is being worked on right now*  
Keep in mind that I wrote my algorithm to rotate between several time servers before Espessif added the mechnism for rotating through a list of time servers (so I believe).  Therefore I don't use Espressif's approach to triggering the rotation of time servers.   I have my own method where I rotate with an ordinal number through the time(n).google.com servers.  My plan puts faith in Google, but  Espressif's method allows you to select any number of servers you like (I believe that is how it works).  At some point I will upgrade my technique and adopt Espressif's plan.  
![SNTP Run Flowchart](./drawings/sntp_flowchart_run.svg)  
___ 