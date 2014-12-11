Simple Mobility Example

This scenario was presented in the Septemeber 2012 powerpoint presentation, to
demonstrate how different content dissemination techniques handle simple 
topology changes.

Duration: approximately 90 seconds.
----

This scenario demonstrates a simple mobility scenario to highlight how 
DIRECT and Prophet can behave very differently. Indeed, this is a pathological
case for Prophet.

There are 5 nodes (n1..n5) and 1 wireless LAN (wlan1). n5 is the only mobile
node.

The initial topology is as follows:


t1: n5
t2:                                        n5
    |                                      |
 wlan1                                   wlan1
    |                                      |   
    n1  -- wlan1 -- n2 -- n3  --- wlan1 -- n4

where n1 publishes content, and n4 subscribes. 

n5 slowly travels back and forth between n1 and n4, with a period of about 60
seconds.

Please follow the steps below to run the test:

1) load the .imn file in this directory, in core

2) start the simulaton, but do not start the mobility script. Make sure 
~/.Haggle directory is virtualzed and the network is configured as in the 
topology diagram.

3) copy the config.xml file in this directory to each host's 
~/.Haggle/config.xml

4) start haggle on each host

5) using haggletest, run 2 apps to continuously publish some content on n1: 

haggletest app1 pub -l RoutingType=Prophet test1 &
haggletest app2 pub -l RoutingType=Direct test2  &

6) using haggletest, subscribe to the content on n4:

haggletest sub test1 test2

7) press the "play" button to start the mobility script

8) after about 30 seconds, n5 will be connected to n4.

9) after 60 seconds (30 seconds after the previous step) n5 will be connected 
to n1 a again. Observe the difference in average latency between the two 
protocols.

Explanation:

Prophet uses an encounter history based approach to build a probability 
matrix of contacting another node. Since n5 visits n4 frequently, Prophet
tries to send data objects destined for n4 on n5. Due to the frequent
topology changes, Prophet has to continuously rebuild the probability matrix,
introducing further inefficiencies.
