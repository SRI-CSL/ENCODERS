The master branch contains general improvements of Vanilla Haggle.  It
also contains a slightly modified version of Haggle Prophet that
incorporates some features from the Prophet Internet draft, namely a
delta parameter and an improved computation of connection
predictabilities. It also includes a feature to periodically
disseminate routing updates and sample the current neighbors for
better performance in (mostly) static networks. We refer to this
version of Prophet with sampling as Prophet-S. It should be noted that
Haggle Prophet only loosely follows the DTN Prophet protocol (not only
because it is content-based).


Haggle Daemon Options
=====================

An option --config (or equivalently -g) has been added to specify the
path of the xml configuration file.  The default path is ~/.Haggle/config.xml.

Configuration Options
=====================

Neighbor Discovery
------------------

The connectivity manager broadcasts periodic beacons and maintains the
current neighborhood. The default parameters are as follows:

	<ConnectivityManager>
	  <Ethernet beacon_period_ms="5000" beacon_jitter_ms="1000" beacon_epsilon_ms="1000" beacon_loss_max="3" />
	</ConnectivityManager>

The parameters beacon_period_ms and beacon_jitter_ms define the
beaconing frequency and optional jitter added for each
transmission. beacon_loss_max is the number of tolerated losses and
beacon_epsilon_ms is an additional error allowed for delayed beacons.

In a high-loss environment (which could be caused by contention or
fast-paced mobility) it is advisable to increase the beaconing
frequency and the number of tolerated losses, using e.g.

	<ConnectivityManager>
	  <Ethernet beacon_period_ms="2500" beacon_jitter_ms="500" beacon_epsilon_ms="500" beacon_loss_max="6" />
	</ConnectivityManager>


In-Memory Data Base
-------------------

Haggle's sqlite database is a performance bottleneck and should be
disabled if not needed.  To this end, a boolean parameter
use_in_memory_database has been added to the SQLDataStore. Even if
disabled, a data base file is read and startup and written at
shutdown, but not used during normal operation. The speedup is
significant, and hence this setting is strongly recommended. The
sqlite journaling mode can be specified by another parameter
journal_mode (which can be off, memory, persist, or truncate). See
sqlite documentation for details.

Below is a typical excerpt from config.xml which enables the in-memory
data base without journaling:

	<DataManager set_createtime_on_bloomfilter_update="true">
		<Aging period="3600" max_age="86400"/>
		<Bloomfilter default_error_rate="0.01" default_capacity="2000"/>
		<DataStore>
                        <SQLDataStore use_in_memory_database="true" journal_mode="off" />
		</DataStore>
	</DataManager>

Default Interest
----------------

Each node can be equipped with default interest, e.g. to enable
proactive propagation of certain data objects (including node
descriptions). The default interest are added to the device node
description, but not to the application node description running on
this device. Hence, even with default interests, applications have to
perform an explicit subscriptions to fetch the data from the local
cache.

Typical config.xml excepts are:

        <ApplicationManager>
              <Attr name="RegistrarMetadata">description</Attr>
        </ApplicationManager>

        <ApplicationManager>
              <Attr name="CommonInterest" weight="100">true</Attr>
        </ApplicationManager>

The first example may be useful to proactively disseminate rich
metadata objects (used by the Drexel registrar manager). The second
example just establishes common interest so that device node
descriptions have overlapping attributes and are proactively
exchanged. Multiple attributes can be similarly specified.

Forwarding Manager and Prophet 
------------------------------

The variant of Prophet implemented in Vanilla Haggle only disseminates
routing information when the network changes, as typical for
pocket-switched networks. The better deal with typical mobile ad hoc
networks and less dynamic topologies, we added a parameter
periodic_routing_update_interval to the forwarding manager to specify
the interval for periodic routing updates in seconds (0 means
disabled). Another boolean parameter called sampling enables periodic
updates of the Prophet predictabilities by sampling the current
neighbor status (rather than updating them only on changes). Finally,
a delta parameter has been added as recommended in the Internet draft.

A typical except that activates Prophet-S looks as follows:

	<ForwardingManager query_on_new_dataobject="true" periodic_dataobject_query_interval="0" 
                           recursive_routing_updates="false" periodic_routing_update_interval="10">
	  <Forwarder max_generated_delegates="1" max_generated_targets="1" protocol="Prophet">
	    <Prophet strategy="GRTR" P_encounter="0.75" alpha="0.5" beta="0.25" gamma="0.999" 
                     delta="0.01" aging_time_unit="1" sampling="true" />
	  </Forwarder>
	</ForwardingManager>

Note that the use of Prophet-S as above is not recommended, but
provided for backward compatibility. Ideally, DIRECT (or Prophet-S)
should be used with lightweight flooding and in-memory representation
of node descriptions as documented in README-direct.

Additional parameters of the forwarding manager are as follows:

dataobject_retries: The default is 1 to mimic behaviour of original
Haggle.  This applies to data objects that are sent or forwarded to
peers (not to applications) that are not node descriptions (see next
parameter). Note that these retries are in addition to the retries of
the protocol (if any). Note also that node refresh (see direct branch)
provides another level that can trigger retransmissions (but this does
not apply to proactive dissemination).  

EXCEPTION: The dataobject_retries parameter does not apply to data
objects that use the optimized fastpath (see
dataobject_retries_shortcircuit in the direct branch). Proactively
disseminated data objects fall into this category.

node_description_retries: The initial transmission (but not the
forwarding) of a node description has its own retry count (as part of
the node manager, see below). This parameter defines the maximum
number of retries for forwarding node description (.e.g beyond the
first hop). Typically with node refresh, since node descriptions are
short-lived (e.g. 30sec) retries are not needed.

max_nodes_to_find_for_new_dataobjects: The default is 30. This is an
upper bound for the number of possible (remote) targets that can be
generated in response to an incoming data object. If use with
first-class applications (see semantics branch) it needs to be large
enough to include all target applications. Also consider that there
might be hidden applications, e.g. in the interest manager (see imodel
branch) that should be included.

enable_target_generation: The default is true as in original Haggle,
but we recommend to set this to false. Target generation is a potentially
expensive operation that for each incoming data object and each current
neighbor generates all targets that can be reached through this neighbor.
Without this feature all targets are generated only once for the data object
and then the best delegate neighbor is selected based on the result.

Haggle Node Descriptions
------------------------

Node descriptions are not of importance to applications and hence
should not be counted when imposing a bound on the number of data
objects (see semantics branch) returned by a query/subscription. To
this end, it is possible to set count_node_descriptions="false" as in
the excerpt below:

	<DataManager set_createtime_on_bloomfilter_update="true">
		<Aging period="3600" max_age="86400"/>
		<Bloomfilter default_error_rate="0.01" default_capacity="2000"/>
                <DataStore>
                        <SQLDataStore use_in_memory_database="true" journal_mode="off" count_node_descriptions="false"/>
		</DataStore>
 	</DataManager>

In Vanilla Haggle, each node description has a

        NodeDescription=<nodeid> 

attribute (with weight 1), which is used by Haggle internally to
detect node descriptions.  By abstracting from the specific <nodeid>,
using the setting node_description_attribute="type", the node
description attribute can also be used to ensure that node
descriptions of peers overlap (like common interest), which enables
them to propagate through the network with the Vanilla Haggle routing
mechanism. Here is a sample for such a configuration:

	<NodeManager>
		<Node matching_threshold="0" max_dataobjects_in_match="10" node_description_attribute="type"/>
		<NodeDescriptionRetry retries="3" retry_wait="10.0"/>
	</NodeManager>

In this context it is also possible to use node_description_weight="0" to
prevent node descriptions to interfere with the application semantics
(see also semantics branch).

The NodeDescription attribute is used when node descriptions are
stored in the data base like other objects (as in Vanilla Haggle) but
it is not needed when they are kept in memory as possible in our new
routing framework (see in_memory_node_descriptions feature in direct
branch). To this end, we recommend to eliminate this attribute in
order to save bandwidth using node_description_attribute="none" as
below:

	<NodeManager>
		<Node matching_threshold="0" max_dataobjects_in_match="10" node_description_attribute="none"/>
		<NodeDescriptionRetry retries="3" retry_wait="10.0"/>
	</NodeManager>

Debugging
---------

If the debug trace is enabled it is by default written to a file,
e.g. haggle.log. In addition is can be written to stdout by using the
following configuration:

	<DebugManager>
		<DebugTrace enable="true" enable_stdout="true"/>
	</DebugManager>

Four different options are available for the trace type: ERROR,
STAT, DEBUG (default), DEBUG1, and DEBUG2 in order of increasing verbosity, as specified
by type in the following excerpt:

	<DebugManager>
		<DebugTrace enable="true" type="DEBUG1"/>
	</DebugManager>

With the flush option, the log file is flushed after each entry:

	<DebugManager>
		<DebugTrace enable="true" type="DEBUG1" flush="true"/>
	</DebugManager>

This is not generally recommended due to the slowdown that unbuffered
logging can cause, but useful for debugging purposes.

The STAT logging level is useful to collect internal statistics

	<DebugManager>
		<DebugTrace enable="true" type="STAT"/>
	</DebugManager>

Statistics are marked as "Statistics" in the haggle log file, e.g. try 

   grep Statistics haggle.log  

for a summary.

Observation
-----------

If the Observation mechanism is enabled,
Haggle will periodically publish data objects with a specified
attribute(s) that contain various metrics about the system.

To enable, add a configuration snippet to the ApplicationManager
section, as below:

    <ObserverConfiguration>
        <ObserveInterfaces>true</ObserveInterfaces>
        <ObserveCacheStrategy>true</ObserveCacheStrategy>
        <ObserveNodeDescription>true</ObserveNodeDescription>
        <ObserveNodeStore>true</ObserveNodeStore>
        <ObserveCertificates>true</ObserveCertificates>
        <ObserveRoutingTable>true</ObserveRoutingTable>
        <ObserveDataStoreDump>true</ObserveDataStoreDump>
        <ObserveNodeMetrics>true</ObserveNodeMetrics>
        <ObserveProtocols>true</ObserveProtocols>
        <NotificationInterval>15</NotificationInterval>
        <Attributes>
            <Attr name="ObserverDataObject">true</Attr>
            <Attr name="ContentCreationTime" every="2">%%replace_current_time%%</Attr>
            <Attr name="ContentOrigin">%%replace_current_node_name%%</Attr>
            <Attr name="ObserverDataObjectOrigin">%%replace_current_node_name%%</Attr>
        </Attributes>
    </ObserverConfiguration>

The %%replace_%% values will be replaced by the corresponding values
automatically at run time when the data objects are created. If "every"
is greater than one, then the attribute will be added only every
"every"-th data object.

Any of the listed observables can be enabled simply by
creating an entry in the given section with the value
"true". To disable, simply omit the entry or set the
value to "false".

The data objects will be created every
"NotificationInterval" seconds.

It is possible to have these distributed observer
data objects delivered remotely through the use of
the Default Interest mechanism described above.

One can also route these specially by adding specific
attributes to the observer data objects; and then
configuring the forwarding manager to handle
data objects with those attributes specially.

Dynamic Configuration
---------------------

The configuration settings for haggle can now be dynamically
changed at run time through the use of an IPC call.

To dynamically change the configuration,
the calling application should use the IPC call 
haggle_ipc_update_configuration_dynamic which requires a
string of XML to be passed in. 

An example of this can be seen in the haggle observer
application, where there are options to change the 
configuration dynamically based on the network state
(by restricting observer data object dissemination to
just one-hop).

Each manager only supports a subset of options that may be
changed using the dynamic configuration options. Currently,
the following settings can be changed dynamically:

ApplicationManager:
    The observer settings (notification interval, observables).

Important Logging Messages
--------------------------

To diagnose potential problems it is advisable to grep
for the following messages in the haggle log file after
termination ...

The normal sequence is ...

    *** HAGGLE STARTUP ***
    *** PREPARE SHUTDOWN EVENT ***
    *** SHUTDOWN EVENT ***
    *** SHUTDOWN COMPLETED ***

Shutdown can also be triggered by a fatal error ...

    FATAL ERROR - EMERGENCY SHUTDOWN INITIATED

In this case a clean shutdown is attempted but 
not always possible if out of resources.

Master-Branch in the SRI-Team Repo - Key Directories
----------------------------------------------------
setup - contains SAIC/SRI instructions to set up the development/test environment
haggle/README.sri - contains built instructions
haggle/INSTALL - and haggle/README contain the original Haggle build instructions
haggle/build_android.sri.sh - builds everything for android
docs - contains SRI documentation, change logs, and readmes
haggle - contains the enhanced Haggle system with build scripts
haggle/doc - contains original Haggle development documentation
haggle/config - contains sample configuration files
haggle/src/hagglekernel - contains haggle kernel
haggle/src/haggletest - contains a generic test app
haggle/src/haggleobserver - contains a generic observation app
haggle/src - new test apps should be added in their own directories here
tests - directory is intended for test scenarios

NOTES FOR ANDROID:

libxml2-2.6.31 used by haggle has been replaced by
libxml2-2.9 (which was already used by Drexel's engine).

IMPORTANT: Do not change the config.h under haggle/extlibs/libxml2-2.9.
It has been configured in a minimal configuration for thread-safe use
of libxml.

sqlite should be built in thread-unsafe mode for best performance,
because Haggle does not rely on its multi-threading capabilities. 
To this end, please excute conf-sqlite.sh under haggle before
build_android.sh.
