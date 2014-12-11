What is the "UDP" branch?
=============================

Vanilla Haggle only supports UDP between applications and the daemon. This 
branch expands upon vanilla Haggle to include additional UDP functionality,
as well as dynamic content-based protocol selection, through use of 
ProtocolClassifiers. We also parameterize (via config.xml) many of the hard 
coded protocol parameters.

We support 1) UDP unicast of arbitrary data objects (essentially it functions
the same as TCP, but without reliability guarantees); and 2) UDP broadcast
of node descriptions (not arbitrary data objects).

UDP Limitations
===============

UDP unicast and broadcast are implemented as lightweight protocols
directly utilizing UDP. Due to the inherent limitation of UDP
fragmentation, they are not suitable if the packet loss rate is very
high or for large payloads (in the number of MTUs). Due due lack of
link-layer reliability and RTS/CTS in 802.11g, packet losses rapidly
increase with the number of nodes. Hence, if UDP is used, e.g. to
disseminate node descriptions, they must fit into a small number of
MTUs. Furthermore, the size of clusters in the network should be
limited, so that node descriptions are received with acceptable delays
(the node description refresh is another paramter that may be varied).

There is another limitation that only applies to UDP broadcast with
control protocol enabled (see below). The number of overheared
communications needs to be limited in the current implementation, and
hence it is less effective than the basic UDP broadcast in larger
clusters.

Implementation
==============

Haggle uses a protocol on top of the underlying transport protocol to reduce
network traffic and provide reliable delivery (see 'haggle/doc/protocol.txt').
This higher level protocol is implemented in Protocol.cpp, the base class
that all the other protocols derive from. We've added several classes that
derive from Protocol.cpp to support both UDP unicast and UDP broadcast.

These new UDP protocols interface with the ProtocolManager through a "proxy"
which is responsible for receiving the UDP packets from the network and 
demutiplexing them to the proper Protocol.cpp instance. This is necessary
because Protocol.cpp keeps state on a per-tansport session level, and UDP
is inherently session-less.  

The ProtocolManager is responsible for the lifecycle of Protocol instances. 
In particular, when Haggle decides to send a data object to a 1-hop neighbor, 
it triggers a DATAOBJECT_SEND event, which the ProtocolManager intercepts and 
passes the respective data object to a new Protocol instance (or uses a cached 
instance) for transport. 

We have added the ProtocolClassifier class which is responsible for deciding 
which transport protocol to use, depending on the content. This design pattern
uses the same "tagging" technique that the direct branch uses, and hence
has virtually the same classifiers. 

In order to demultiplex unicast UDP packets between a sender and a receiver,
we use 2 UDP ports to essentially uniquely identify the UDP unicast session.
Specifically, the UDP Unicast sender will send on port A and receive on port
B, while the UDP Unicast receiver will send on port B and receive on port A.
In this way the session between the sender and receiver is uniquely identified,
given both their IP addresses.

A special note on UDPBroadcast of node descriptions: although node 
descriptions are data objects, Protocol.cpp treats them differently than
other data objects. Instead of the sender waiting for an "ACCEPT" message
from the receiver before sending the body of the message, the sender
sends the entire node description and then immediately expects an "ACK". 
Given this simplification, we previously only supported UDPBroadcast of 
node descriptions.

With "udp-bcast", we now support the meta-control protocol on-top of UDP.
Arbitrary data objects can be sent using UDP with or without the control 
protocol, using UDP unicast or UDP broadcast. One potential limitation with
UDP broadcast is the use of a data structure that stores O(k^2) state 
where k is the number of neighbors. The file descriptor monitoring will
prevent this data structure from growing too large, but at the loss
of a potentially overhearing more data objects (limiting the effectiveness 
of broadcast).

Given the point-point design of Protocol.cpp, and the lack of a specific
destination in a UDP broadcast packet, we've designed UDPBroadcast to 
internally keep track of a "peer" receiver, although more nodes than the
receiver may receive the node description. With this scheme, the "peer"
acts as a "reason" or "witness" for the broadcast. In other words, if there
is not "reason" then the node description will not be broadcast at all. 
Note that without control none of the receivers send any "ACK" or "ACCEPT": 
the sender will iterate across its list of neighbors and opportunistically 
set their internal bloomfilter to indicate that they have received the node 
description. Note that this is inexact, and should only be used in conjunction
with node refresh to ensure that these internal bloomfilters remain up-to-date.
If the control protocol is enabled, then this exchange will occur
with the "reason" node, with other nodes potentially overhearing.  

Note that this design closely parallels the forwarder design.

The ARP Issues
==============

ARP is a protocol that is used to map IP addresses to MAC address, and 
typically does so transparently for most applications. For example, when
establishing a TCP connection to new neighbor, the TCP socket library
will buffer the packets to be transmitted, issue an ARP request, wait
for the ARP reply, then transmit the buffered packets. This works for 
TCP since TCP is a reliable protocol with buffers, however when switching
to an unreliable protocol like UDP, problems can arise. Specifically, if the
IP address of the peer does not have an entry in the ARP cache, then the 
UDP library will simply drop the UDP packet and then issue an ARP request. 
This is OK because UDP does not have any reliability guarantees--UDP packets
can be dropped at any point in time, and application developers must write
additional code to ensure reliability. However, as a result, a naive
implementation of UDP unicast will simply drop the first packet to a new
neighbor, leading to poor performance. Similarly, Haggle expects that every
data object it receives has an Interface with a valid MAC address, thus
if Haggle receives a data object via UDPBroadcast from a node which it does
not have an ARP cache entry for, then it will drop this Data Object.

Due to the hop-by-hop architecture of Haggle, and the node description 
exchange, the Haggle application actually has the IP address and the mac 
address of the peer, even before the entry is in the ARP cache. Thus, we
have added functionality (via the "arphelper" C program) that allows Haggle 
to insert the ARP cache entries directly, instead of dropping these packets.
arphelper is simply a C wrapper to "arp -s", which should have setuid root
permissions (chmod +s, i.e. '-rwsr-xr-x 1 root root').
We have added the ability to snoop IP address/MAC pairs from overhearing
node descriptions. 

The default location for arphelper is: "/etc/arphelper", one may need to 
execute "sudo ln -s /usr/local/bin/arphelper /etc/arphelper" if the default 
is used.

In the case where "arphelper" is unavailable, we've added a "ping"
mechanism which will ping the IP address if it is not in the cache.
ping typically does not require root (whereas "arp" does), and will issue 
an ARP request if the IP address is not in the cache.

Configuration Options
=====================

UDP is configured within the <ProtocolManager> tags.
A classifier (which may be composed of multiple classifiers) is responsible
for assigning a "tag" to a data object, and different protocols can register
for a single "tag." 

Below is an example excerpt from config.xml which enables broadcast of 
node descriptions, UDP unicast for small data objects, and TCP for all other
data objects. Also, it uses the "arphelper" program for manual arp insertion.

NOTE: we only discuss config.xml parameters that are relevant to the UDP
branch in this document. For an explanation of the other parameters, please see
the corresponding README for the master branch.
	<ProtocolManager>
	<ProtocolClassifier name="ProtocolClassifierPriority">
		<ProtocolClassifierPriority>
			<ProtocolClassifier name="ProtocolClassifierNodeDescription" priority="3">
				<ProtocolClassifierNodeDescription outputTag="nd" />	
			</ProtocolClassifier>
			<ProtocolClassifier name="ProtocolClassifierSizeRange" priority="2">
				<ProtocolClassifierSizeRange minBytes="0" maxBytes="4416" outputTag="lw" />
			</ProtocolClassifier>
			<ProtocolClassifier name="ProtocolClassifierAllMatch" priority="1">
				<ProtocolClassifierAllMatch outputTag="hw" />	
			</ProtocolClassifier>
		</ProtocolClassifierPriority>
	</ProtocolClassifier>
	<Protocol name="ProtocolUDPBroadcast" inputTag="nd">
		<ProtocolUDPBroadcast waitTimeBeforeDoneMillis="60000" use_arp_manual_insertion="true" arp_manual_insertion_path="/etc/arphelper" />
	</Protocol>
	<Protocol name="ProtocolUDPUnicast" inputTag="lw">
		<ProtocolUDPUnicast waitTimeBeforeDoneMillis="60000" connectionWaitMillis="500" maxSendTimeouts="10" />
	</Protocol>
	<Protocol name="ProtocolTCP" inputTag="hw">
		<ProtocolTCP backlog="30" />
	</Protocol>
	</ProtocolManager>
    <ConnectivityManager use_arp_manual_insertion="true" arp_manual_insertion_path="/etc/arphelper" />

ProtocolClassifierNodeDescription parameters:
--
Tags data objects which represent node descriptions.

outputTag : The name of the "tag" that node description data objects will be
assigned.

ProtocolClassifierSizeRange:
--

Tags data objects with a certain size.

minBytes : The minimum number of bytes of the data object size that will be
tagged.

maxBytes : The maximum number of bytes of the data object size that will be
tagged. 

outputTag : The name of the "tag" that data objects with size S, where
S in [minBytes, maxBytes], are assigned.

ProtocolClassifierAllMatch:
--

Tags all data objects with a specific tag. 

outputTag : The name of the "tag".

ProtocolClassifierBasic (DEPRECATED):
--
Tags node descriptions with one tag, and all other data objects with
another tag. Replaced by ProtocolClassifierNodeDescription.

lightWeightClassName : The tag name for node descriptions.

heavyWeightClassName : The tag name for all other data objects.

ProtocolClassifierPriority
--
Impose a total order on the classifiers, so that conflicts across classifiers
are resolved. 

Uses the form:

		<ProtocolClassifierPriority>
			<ProtocolClassifier name="X" priority="Y">
                ..
			</ProtocolClassifier>

Where "X" is the name of a classifier, and "Y" is its priority (greater number
has higher priority), and ".." contains the configuration for "X".

The priority classifier iterates across the classifiers by priority, and 
assigns the "best" tag for a data object, where "best" is a valid tag
given by the classifier with highest priority.

Protocol parameters (and any derived class from Protocol):
--
Generic base class for all protocols.

'waitTimeBeforeDoneMillis' : The maximum number of milliseconds that a
protocol sender or receiver will wait for either data to send or data
to receive. A high value is useful to avoid Protocol instance churn
and the expense of possibly maintaining stale protocols, while a low
value by prematurely remove fresh protocols but keep lower state. The
default is 60000ms.

'passiveWaitTimeBeforeDoneMillis' : similar to previous parameter, but
for passive UDP receivers, which should be quickly terminated if not
needed. The default is 10000ms. Only relevant for UDP broadcast with
control (see below).

'connectionAttempts' : The maximum number of times to try connecting to
a peer (used mainly by TCP). Default is 4.

'maxBlockingTries' : The maximum number of times to try receiving on the
receive socket, and having it block. Default is 5.

'blockingSleepMillis' : The number of seconds to wait when the receive
socket is blocked, waiting to receive data. Default is 400ms.

'connectionWaitMillis' : The maximum number of milliseconds to wait when 
establishing a connection (used mainly by TCP). Default is 20000ms.

'connectionPauseMillis' : The number of milliseconds to pause before trying
to connect again, upon a failure. Default is 5000ms.

'connectionPauseJitterMillis' : The number of milliseconds to add to the 
connectionPauseMillis, drawing uniformly from [0, connectionPauseJitterMillis).
Default is 20000ms.

'maxProtocolErrors' : The maximum number of protocol errors allowable before
closing and deleting the protocol. Default is 4.

'maxSendTimeouts' : The maximum number of times to retry sending a data object,
upon send failure. The default is 0 (don't use this with TCP).

'load_reduction_min_queue_size' and 'load_reduction_max_queue_size' :
These per-protocol parameters enable a simple probabilistic load
reduction mechanism which probabilistically discards data objects to
keep queue size below maximum (but without guarantee). The default
queue size is unlimited (i.e. no load reduction).

ProtocolTCP parameters (including Protocol parameters):
--
Protocol for transporting data objects using TCP.

'port' - The TCP port used by the protocol. The default is 9697.

'backlog' - The number of entries that can fit in the listen queue 
(effectively the maximum number of concurrent TCP sessions on this port).

It is possible to configure multiple instances of the same protocol
type, but the port numbers must not conflict. For instance, a second
instance of TCP can be specified as follows:

       <Protocol name="ProtocolTCP" inputTag="tcp2">
                <ProtocolTCP port="9698" />
        </Protocol>

ProtocolUDPBroadcast parameters (including Protocol parameters):
--
Protocol for transporting node description data objects using UDP broadcast.

'broadcastPort' - The UDP port to broadcast and listen for UDP broadcasts on.

'arp_manual_insertion' - true or false, manually insert an ARP entry if it 
missing and the protocol receives a node description with the mac address. 

'arp_manual_insertion_path' - system path, location of the compiled arphelper
program which wraps "arp -s" and has setuid root.

'use_control_protocol' - "true" or "false" to enable/disable the control
protocol.

ProtocolUDPUnicast parameters (including Protocol parameters):
--
Protocol for transporting data objects using UDP unicast.

'control_port_a' and 'control_port_b' - The ports that the sender and
receiver uses to exchange control messages (in connection with the
control protocol).

'no_control_port' - The port that is used to exchange data messages
representing metadata and payload of data objects.

The defaults for these ports are 8791-8793 for UDP broadcast and 8788-8790
for UDP unicast, respectively. For both, TCP and UDP, it is possible 
to configure multiple instances of the same protocol type, but the
port numbers must not conflict, hence they need to explicitly specified for
each additional instance. For instance, for a second instance of UDP
broadcast you can use:

        <Protocol name="ProtocolUDPBroadcast" inputTag="bcast2">
                <ProtocolUDPBroadcast control_port_a="8794" control_port_b="8795" no_control_port="8796" />
        </Protocol>


'useArpHack' - true or false, issue a ping in the event that the arp cache 
entry is missing for the destination, in order to indirectly trigger an
arp request.

'use_control_protocol' - "true" or "false" to enable/disable the control
protocol.


ConnectivityManager parameters:
--
Responsible for peer discovery via the hello protocol.
See master-branch readme for beaconing-related parameters.

'use_arp_manual_insertion' : true or false, enables or disables ARP insertion 
when discovering neighbors. Useful in conjunction with ProtocolUDPUnicast
and ProtocolUDPBroadcast. 

'arp_manual_insertion_path': system path, location of the compiled arphelper 
program which wraps "arp -s" and has setuid root. Used if 
'use_arp_manual_insertion' is set to true.

Limiting protocol instances:

Too many protocol instances can lead to resource bottlenecks and
potential fatal errors (e.g. due to lack of memory or file
descriptors), hence the number of instances should be conservatively
limited. For TCP protocols it is advisable to use a small number of
sender instances per link together with suitable kernel setting to
allow quick termination of protocols in case of disruptions. For
example, the kernel setting

          sysctl -w net.ipv4.tcp_syn_retries=0 

reduces the timeout for unsuccessful TCP connections to ~10 sec instead
of the usual 180s with 5 retries.

'maxInstances' - maximum number of sender protocol instances on a
single node (default 100). Relevant for TCP and UDP protocols.

'maxInstancesPerLink' - maximum number of sender protocol instances
for each link, i.e. pair of nodes (default 3). Relevant for TCP and UDP
protocols. For UDP based protocols there is no significant startup/shutdown
time, hence we recommend to limit the instances to one as exemplified
in the configuration except below.

'maxReceiverInstances' - maximum number of receiver instances for
UDP protocols (default 100). Relevant for UDP protocols.

'maxReceiverInstancesPerLink' - maximum number of sender protocol
instances for each link, i.e. pair of nodes (default 1). Relevant for
UDP protocols.

'maxPassiveReceiverInstances' - maximum number of passive receiver
instances for UDP broadcast protocols with control (default 100).

'maxPassiveReceiverInstancesPerLink' - maximum number of receiver
protocol instances for each link (between neighbors), i.e. pair of
nodes, for UDP broadcast protocols with control (default 50).
It is not recommended to increase this further in the current version.

Currently, no attempt is made to limit the number of neighbors in
dense networks. If the system conducts an EMERGENCY shutdown, a
possible cause is that the number of active protocol instance is too
large (e.g. due to a large number of neighbors).

Limiting Protocol Sending Rates:

UDP protocols without the control protocol do not have explicit flow
control, hence it is advisable to configure a maximum rate for data
object transmission to reduce the likelihood of packet losses due to
buffer overflows. Even in case of TCP and UDP with control, the
sending rate for certain kinds of traffic (e.g. node descriptions) can
be further limited using the following parameters.

'minSendDelayBaseMillis' - the minimum time in ms between consequtive
data objects transmitted by the same protocol instance.

'minSendDelayLinearMillis', 'minSendDelaySquareMillis' - additional
delays can be specified by these coefficients to be linear or
quadratic in the number of neighbors.

To use these parameters it is important to understand that for UDP
broadcast each neighbor gives rise to an independent protocol
instance, hence specifying 10000ms for minSendDelayBaseMillis leads to
an average delay of 1000ms if there are 10 neighbors and the 10
corresponding protocol instances are used concurrently. In case of
broadcast, each transmission may be overhead by many nodes.

The following sample excerpt shows how to configure a neighbor-dependent
rate limit for UDP broadcast.

	<Protocol name="ProtocolUDPBroadcast" inputTag="bcast">
		<ProtocolUDPBroadcast 
                      waitTimeBeforeDoneMillis="60000" use_arp_manual_insertion="true" 
                      maxInstancesPerLink="1"
		      minSendDelayBaseMillis="1000" minSendDelayLinearMillis="100" minSendDelaySquareMillis="10" />
	</Protocol>

'maxRandomSendDelayMillis' - random delay in ms just before sending a
data object (default is 100ms). This reduces the likelihood of
redundant UDP broadcast transmissions (which are suppressed by local
peer Bloom filter abstractions). See also maxForwardingDelay
for a similar delay in the forwarding manager.


Two-Hop Neighborhood Optimization
---------------------------------

UDP broadcast can optimistically suppress transmissions if every
target node has been covered by a previous transmission. Eventually
Bloomfilters will be updated and retransmissions occur as necessary
to provide an additional layer of reliability. To enable this two-hop
neighborhood optimization (which is generally recommended) the node
parameter send_neighborhood should be set to true, as in the following
excerpt:

	<NodeManager>
		<Node matching_threshold="0" max_dataobjects_in_match="10" 
                      node_description_attribute="none" node_description_attribute_weight="0"
		      send_neighborhood="true" />
		<NodeDescriptionRetry retries="0" retry_wait="10.0"/>
		<NodeDescriptionRefresh refresh_period_ms="30000" refresh_jitter_ms="1000" />
		<NodeDescriptionPurge purge_max_age_ms="90000" purge_poll_period_ms="30000" />
	</NodeManager>


Sample Configuration Excerpt with Multiple Protocols
----------------------------------------------------

The following excerpt uses rate-limited UDP broadcast for node descriptions
and TCP (without limits) for all other data objects:

	<NodeManager>
		<Node matching_threshold="0" max_dataobjects_in_match="10" 
                      node_description_attribute="none" node_description_attribute_weight="0"
		      send_neighborhood="true" />
		<NodeDescriptionRetry retries="0" retry_wait="10.0"/>
		<NodeDescriptionRefresh refresh_period_ms="30000" refresh_jitter_ms="1000" />
		<NodeDescriptionPurge purge_max_age_ms="90000" purge_poll_period_ms="30000" />
	</NodeManager>
	<ConnectivityManager use_arp_manual_insertion="true" />
	<ProtocolManager>
	<ProtocolClassifier name="ProtocolClassifierPriority">
		<ProtocolClassifierPriority>
			<ProtocolClassifier name="ProtocolClassifierNodeDescription" priority="3">
				<ProtocolClassifierNodeDescription outputTag="bcast" />	
			</ProtocolClassifier>
			<ProtocolClassifier name="ProtocolClassifierAllMatch" priority="1">
				<ProtocolClassifierAllMatch outputTag="tcp" />	
			</ProtocolClassifier>
		</ProtocolClassifierPriority>
	</ProtocolClassifier>
	<Protocol name="ProtocolUDPBroadcast" inputTag="bcast">
		<ProtocolUDPBroadcast waitTimeBeforeDoneMillis="60000" use_arp_manual_insertion="true" maxInstancesPerLink="1"
				      minSendDelayBaseMillis="1000"
				      minSendDelayLinearMillis="100" minSendDelaySquareMillis="10" />
	</Protocol>
	<Protocol name="ProtocolTCP" inputTag="tcp">
		<ProtocolTCP waitTimeBeforeDoneMillis="60000" connectionWaitMillis="60000" backlog="30"
			     load_reduction_min_queue_size="100" load_reduction_max_queue_size="200" />
	</Protocol>
	</ProtocolManager>

Testing
=======

Please look at the following directories in the cbmen-encoders-eval
repository for tests:

    tests/UDPBroadcast
