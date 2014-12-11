Dynamic Network Microbenchmark

This scenario was presented in the Septemeber 2012 powerpoint presentation, to
demonstrate how different content dissemination techniques handle changes in
network topology. It is useful as a microbenchmark for content dissemination
performance.

Duration: approximately 30 seconds.
----

This network is used to microbenchmark content dissemination techniques
in the face of network topology changes.

There are 4 nodes (n1..n4) and 1 wireless LAN (wlan1).

The initial topology is as follows:


    n1 -- wlan1 -- n2 -- wlan1 -- n3 -- wlan1 -- n4

Then, n2 an n3 switch places to form the topology:

    n1 -- wlan1 -- n3 -- wlan1 -- n2 -- wlan1 -- n4

This transition takes approximately 30 seconds.

Please follow the steps to microbenchmark the different dissemination
strategies:

1) load the .imn file in this directory, in core

2) start the simulaton, but do not start the mobility script. Make sure 
~/.Haggle directory is vitualzed and the network is configured as in the 
topology diagram.

3) copy the config.xml file in this directory to each host's 
~/.Haggle/config.xml

4) start haggle on each host

5) using haggletest, publish some content:

haggletest pub -b 20 RoutingType=K test

where K = Direct, Flood or Prophet.

6) using haggletest, subscribe to the content on n4:

haggletest sub test

7) press the "play" button to start the mobility script

All of the data objects will be delivered, but with a different average delay
depending on the content dissemination protocol. Note that "config.xml" has
all three protocols activated for each test, thus the network overhead
of Prophet will be included in the "Direct" and "Flood" test. To remove
this overhead, then please remove the Prophet section (and the corresponding
classifiers) from the configuration.
