Message Ferry Example

This scenario demonstrates how different content dissemination techniques 
handle message ferrying.

Duration: approximately 90 seconds.
----

This scenario demonstrates 1 way message ferrying using "Flood" and message
ferrying using DIRECT (with two passes).

There are 5 nodes (n1..n5) and 1 wireless LAN (wlan1).

The initial topology is as follows:


    n5
    |
 wlan1
    |                                        
    n1  -- wlan1 -- n2            n3  --- wlan1 -- n4

Where n1 and n2 form "island 1", and n3 and n4 form "island 2".

n5 slowly travels back and forth between island 1 and island 2, with a period
of about 60 seconds.

Please follow the steps below to test message ferrying:

1) load the .imn file in this directory, in core

2) start the simulaton, but do not start the mobility script. Make sure 
~/.Haggle directory is virtualzed and the network is configured as in the 
topology diagram.

3) copy the config.xml file in this directory to each host's 
~/.Haggle/config.xml

4) start haggle on each host

5) using haggletest, publish some content to be ferried, on n1:

haggletest pub RoutingType=Flood test
haggletest pub RoutingType=Direct test

6) using haggletest, subscribe to the content on n4:

haggletest sub test

7) press the "play" button to start the mobility script

8) after about 30 seconds, n5 will be connected to island 2, and n4 should
report that it received the first data object, but not the second.

9) after 90 seconds (a minute after the previous step) n5 will be connected to
island 2 a second time, and n4 should report that it received the second 
data object.

Explanation:

The flooded data object uses "push on contact" to delivery the data object.
In the beginning of the simulation, n5 receives only the flooded data object,
and at 30 seconds when it connects to island 2, it will reflood this
data object to n3 and n4. Hence n4 will report the first data object.

Upon meeting n4, n5 will receive n4's node description and learn of n4's
interest in "test." When n5 connects back to island 1, it will send this
node description to n1. When n1 receives n4's node description, it will trigger
DIRECT delegate forwarding so that n1 will send the second data object to
n5 to be delivered to n4. n5 will then connect to island 2 again, and send
the second data object to n4 (either directly to n4, or via n3 depending on
the timing).
