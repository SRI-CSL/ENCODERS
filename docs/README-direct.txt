What is DIRECT?
=============================

DIRECT is forwarding module used to forward a specific data object to a 
specific destination. 

The Haggle implementation of DIRECT is _loosely_ based off the DIRECT paper 
[1], which  floods interests and unicasts data (on the reverse path). 
Due to the soft state nature of DIRECT, the interests must be reflooded 
periodically to discover new paths and remove old paths.  

This branch also contains a primitive version of proactive replication,
where certain data objects can be flooded throughout the network (or
connected component) based on their content.

[1] Solis, Garcia-Luna-Aceves, "Robust content dissemination in disrupted
environments' Proceedings of the third ACM workshop on challenged networks,
pages 3-10, 2008.

Implementation Discussion
==============

Please see the design documents within this directory for an explanation of
more implementation details. 

Not included in these documents is a discussion on purging and the in memory
node descriptions. Purging is used to avoid forwarding on stale paths, and 
the "in memory node descriptions" is an optimization to avoid many database
accesses due to an increased propagation of node descriptions (due to 
node description refresh).

The current implementation of proactive replication is a subset of the 
features discussed in the proposal. In particular, we support unbounded
flooding of arbitrary data objects, using the same ForwardingClassifiers to
designate eligible data objects for flooding. 

Both node descriptions and arbitrary data objects can be flooded in 2 
different ways: 1) contemporaneous flooding, and 2) and contemporaneous 
flooding with "push on contact". In 1), a received data object that is
eligible for flooding will immediately be sent to the receiver's 1-hop
neighbors. In contrast, 2) will also perform 1), but in addition it will 
effectively store these data objects in a cache, so if a new neighbor (say N) 
becomes connected to the receiver, say R, at a later point in time, R will
forward all its data objects marked for flooding to N. Note that 2) supports 
flooding a data object over multiple connected components via message ferrying,
whereas 1) does necessarily do so (unless the ferry has an explicit interest
in the data object, for example). 

The non-counting bloomfilter for 'this' node functions slightly differently
in this branch than the other branches. In particular, we periodically 
replace the non-counting bloomfilter with the abstraction of the counting
bloomfilter. In other words, the two bloomfilters are not always consistent,
with the non-counting bloomfilter acting as opportunistic cache which may
contain invalid state that is not propagated to the counting bloom filter,
and is replaced by the valid abstraction of the counting bloomfilter,
periodically.

Configuration Options
=====================

DIRECT is configured through the <ForwardingManager> and <NodeManager> tags.
The forwarding manager tag specifies which delegate forwarding modules to use
for what content, while the node manager tag specifies the node description
propagation parameters. 

Below is an example excerpt from config.xml which enables flooding of 
node descriptions, small data objects, and tagged data objects, whereas it 
uses DIRECT for specifically tagged data objects and for all other unmatched
data objects:

NOTE: we only discuss config.xml parameters that are relevant to DIRECT
in this document. For an explanation of the other parameters, please see
the corresponding README for the master branch.

    <ForwardingManager max_nodes_to_find_for_new_dataobjects="30"
            query_on_new_dataobject="true" periodic_dataobject_query_interval="0" 
            enable_target_generation="false" push_node_descriptions_on_contact="true">
        <ForwardingClassifier name="ForwardingClassifierPriority">
            <ForwardingClassifierPriority>
                <ForwardingClassifier name="ForwardingClassifierAttribute" priority="5">
                    <ForwardingClassifierAttribute attribute_name="ContentType" attribute_value="Direct" class_name="hw" />
                </ForwardingClassifier>
                <ForwardingClassifier name="ForwardingClassifierNodeDescription" priority="4">
                    <ForwardingClassifierNodeDescription class_name="lw" />
                </ForwardingClassifier>
                <ForwardingClassifier name="ForwardingClassifierAttribute" priority="3">
                    <ForwardingClassifierAttribute attribute_name="ContentType" attribute_value="Flood" class_name="lw" />
                </ForwardingClassifier>
                <ForwardingClassifier name="ForwardingClassifierSizeRange" priority="2">
                    <ForwardingClassifierSizeRange min_bytes="0" max_bytes="1024" class_name="lw" />
                </ForwardingClassifier>
                <ForwardingClassifier name="ForwardingClassifierAllMatch" priority="1">
                    <ForwardingClassifierAllMatch class_name="hw" />
                </ForwardingClassifier>
            </ForwardingClassifierPriority>
        </ForwardingClassifier>
        <Forwarder protocol="Flood" contentTag="lw">
            <Flood push_on_contact="true" />
        </Forwarder>
        <Forwarder protocol="AlphaDirect" contentTag="hw" />
    </ForwardingManager>

As an alternative to proactive flooding of content as configured above using the Flood forwarder,
we also support reactive flooding, which is configured as follows.

        <Forwarder protocol="Flood" contentTag="flood">
            <Flood push_on_contact="true" enable_delegate_generation="true" reactive_flooding="true"/>
        </Forwarder>

It can be used as a replacement of or in addition to proactive flooding
(in which case a different content tag needs to be defined).


ForwardingManager :
----
push_node_descriptions_on_contact : true or false, if enabled, this feature 
allows flooded node descriptions to be propagated to new neighbors that may
become connected after the initial flood.

dataobject_retries_shortcircuit: Similar to data_object_retries (see
master branch), but applied to data objects using the fast path, such
as those proactively disseminated by the flood forwarder.

max_forwarding_delay_base, max_forwarding_delay_linear: Specifies the
maximum forwarding delay in millisec. This number is used to add a
randomized delay to data objects (excluding node descriptions) that
are sent to multiple nodes at the same time. It is especially
important to desynchronize transmissions in clusters and hence to
reduce the number of redundant transmissions.  The delay will be 0 for
objects sent to only one peer and randomly chosen from 0
... max_forwarding_delay_base + max_forwarding_delay * (number of
target neighbors - 1) otherwise.  Here, max_forwarding_delay_base is a
minimum randomized delay that will always be added (independent of
the number of target neighbors). The default is 20ms, to randomize the
order in the choice of send events and hence in the choice of the
(primary) sender protocol in case of broadcast (where subsequent send
event are suppressed by Bloom filters). The default for
max_forwarding_delay_linear is 0. max_forwarding_delay is synonymous
to max_forwarding_delay_linear. Note that additional delays can
be specified at the protocol level (see udp-bcast branch).

max_node_desc_forwarding_delay_base and
max_node_desc_forwarding_delay_linear (synonymous to
max_node_desc_forwarding_delay) are similar parameters for node
descriptions. Defaults are 0 and 20ms, respectively.

accept_neighbor_node_descriptions_from_third_party: Using the default
setting false node descriptions for neighbors should come from the
neighbor directly and are ignored otherwise.

neighbor_forwarding_shortcut: By default this option is true
to directly forward data objects to neighbors that are interested,
in contrast to let the routing algorithm make the choice
(treating it as multi-hop delegation). For best efficiency
the recommended setting is true, but there may be reasons to
set this option to false in connection with certain security
(digital signature) features and the previous option
accept_neighbor_node_descriptions_from_third_party.

load_reduction_min_queue_size, load_reduction_max_queue_size: These
parameters are used to reduce load (currently probabilistically
skipping queries) with the goal to keep kernel event queue size below
the max bound (but there is no guarantee). Default is unlimited queue
size (i.e. no load reduction).

ForwardingClassifier :
----
This attribute is responsible for specifying a classification module, so that
different classes of content can be routed differently. 

The parameters are as follows:

name - This attribute specifies which classification module to use. 

ForwardingClassifierBasic :
----
NOTE: *DEPRECATED: use ForwardingClassifierNodeDescription instead*
This classification module classifies content into two categories, 
light-weight and heavy-weight. Currently, node descriptions are classified
as light-weight content, and data objects are classified as heavy weight
content. 

The parameters are as follows:

lightWeightClassName - The "tag" that should be assigned to light-weight
content. Forwarder modules are specified on a per-tag basis.

heavyWeightClassName - The "tag" that should be assigned to heavy-weight
content. Forwarder modules are specified on a per-tag basis.

ForwardingClassifierNodeDescription:
----
This classification module only tags data objects that contain a
node description. 

The parameters are as follows:

class_name - The "tag" that should be assigned to data objects belonging
to a node description.

ForwardingClassifierAttribute:
----
This classification module tags data objects that have a certain attribute 
(name,value) pair.

The parameters are as follows:

attribute_name - The name of the attribute to be tagged.

attribute_value - The value of the attribute to be tagged.

class_name - The "tag" that should be assigned to data objects that have the 
correct attribute name, value. 

ForwardingClassifierSizeRange:
----
This classification module tags data objects that are within a certain size
range.

The parameters are as follows:

min_bytes - The minimum number of bytes of the data object size to be
classified.

max_bytes - The maximum number of bytes of the data object size to be
classified.

class_name - The "tag" that should be assigned to the data object that
has the correct size.

ForwardingClassifierAllMatch:
----
This classification module tags every data object with a specified tag. It
is useful as a catch-all with the priority classifier.

ForwardingClassifierPriority:
----
This classification module is an aggregate of classifiers, organized by their
priority, where a priority value has more priority. 

It has no parameters, but expects child tags of the form:

<ForwardingClassifier name="ForwardingClassifierX.." priority="...">
    <ForwardingClassifierX... />
</ForwardingClassifier>

Where "name" specifies the name of the classifier, and "priority" specifies
the priority. 

-----

Forwarder:
----
This attribute is responsible for specifying and configuring a forwarder
module. A forwarder module is assigned responsibility for a specific tag,
where the tag is assigned to the piece of content by the classifier.

The parameters are as follows:

protocol - Specifies which forwarder module to load. Currently we support
Prophet, AlphaDirect and Flood. 

contentTag - Specifies which "tag" (assigned by the classifier) that this
specific forwarder module is responsible for. 

NOTE: This value can be omitted for one forwarder. In this case, the 
default forwarder is responsible for all content that does not have an
existing forwarder responsible for its propagation.  

Flood:
----
This forwarder floods a data object to each 1-hop neighbor. Upon insertion
of a flooded data object, this module short-circuits the haggle matching 
mechanism, and instead attempts to immediately send the data object to
each 1-hop neighbor (at the time of receiving the data object).

The parameters are as follows:

push_on_contact - "true" or "false": if "true" then all data objects marked for
flooding will be sent to a neighbor upon contact, provided that the neighbor
does not already have the data object. If set to "false" then a flooded
data object will only propagate within the connected component of the publisher,
at the time that the data object was published. 

enable_delegate_generation - "true" or "false": if "true" then flooding can
additionally be triggered by incoming queries from remote nodes. This makes
only sense together with the following parameter. Default is "false".

reactive_flooding - "true" or "false": if "true" content is not immediately
flooded when injected by an application, but rather waits in the local cache
of the publishing node till it is requested, in which case it is flooded,
but only if requested by a remote node that is not an immediate neighbor. 
The default is false (i.e. proactive flooding). If "true", this parameter 
needs to be used together with enable_delegate_generation = "true".

AlphaDirect:
----
This forwarder uses the "DIRECT" algorithm to forward data objects along the
reverse path of the interest propagation. Please see the design documents in 
this directory for more details.

There are no parameters.  

Prophet:
----
This forwarder uses the "Prophet" algorithm to forward data objects according
to a topology connectivity matrix. Please see the documents in the master
branch for more details.

-----

Below is an excerpt from config.xml that enables node refresh:

NOTE: as before, we only discuss direct related parameters in this document.
For a description of the other parameters, see the corresponding README for
the master branch.

  <NodeManager>
    <Node matching_threshold="0" max_dataobjects_in_match="10"/>
    <NodeDescriptionRetry retries="3" retry_wait="10.0"/>
    <NodeDescriptionRefresh refresh_period_ms="30000" refresh_jitter_ms="1000" />
    <NodeDescriptionPurge purge_max_age_ms="90000" purge_poll_period_ms="30000" />
  </NodeManager>

NodeDescriptionRefresh 
----
This attribute is responsible for specifying how frequently the node 
descriptions (and the corresponding interests) are propagated through the
network. 

Omitting this tag disables this refresh mechanism.

The parameters are as follows:

refresh_period_ms - Specifies how frequently to send a new node description,
in milliseconds.

refresh_jitter_ms - A number is picked uniformly at random from the interval
[0, refresh_jitter_ms) and added to the period. The purpose of adding this
value is to prevent synchronized floods of node descriptions. 

NodeDescriptionPurge
----
This attribute is responsible for specifying when to purge stale node 
descriptions from the cache. This prevents DIRECT from forwarding on invalid
paths (paths which have not recently been refreshed).

Omitting this tag disables this purging mechanism.

purge_max_age_ms - Specifies the age for a node description after which it
is eligible for purging (in milliseconds). No node description that is younger 
than this age will be purged by this mechanism.

purge_poll_period_ms - Specifies how frequently node descriptions should be
checked for expiration, in milliseconds. A higher frequency means that 
nodes eligible for purging will be purged sooner, but at the expense of
higher CPU due to more events. Conversely, a lower frequency means that
nodes will be checked for purging less often, but with lower CPU utilization.

----

Due to the increased database utilization from increased propagation of 
node descriptions, we have added an optimization, so called "in memory node 
descriptions", that does not put node descriptions in the database. 

Below is an except from config.xml that demonstrates how to enable this 
optimization.

Omitting this parameter disables this optimization.

  <DataManager set_createtime_on_bloomfilter_update="true" periodic_bloomfilter_update_interval="60">
    <Aging period="3600" max_age="86400"/>
    <Bloomfilter default_error_rate="0.01" default_capacity="2000"/>
    <DataStore>
      <SQLDataStore use_in_memory_database="true" journal_mode="off" in_memory_node_descriptions="true" />
    </DataStore>
  </DataManager>

in_memory_node_descriptions - When set to "true", this optimization will not
place node descriptions in the database. Note that this optimization should
only be used in conjunction with the forwarder flood mechanism for light
weight content, since the database resolution operation is short-circuited.
In other words, this disables matching node descriptions as data objects to
targets. 

periodic_bloomfilter_update_interval - Allows the user to specify, in
seconds, how often to take non-counting abstractions of the counting
bloomfilter, and set them for "this" node. This replaces potentially invalid
state in the stale in the non-counting bloomfilter.

In the the latest version the node store maintains two Bloomfilter
abstractions per peer to maintain a certain degree of continuity when
the abstractions are updated. This feature is turned on by default
(continuousBloomfilters is set to true). In this case, the local
Bloomfilter abstraction is replaced only if a minimum time has passed
(continuousBloomfilterUpdateInterval with a default of 5000ms) 
and is updated by a merge (i.e. set union) otherwise.

Special Monitoring Application Support
======================================

To monitor when content arrives at a node without explicitly
requesting it, we support a special application named MONITOR, which
should register once after startup and subscribe to "*=*". After this
subscription all data objects that are inserted into the data base or
sent to normal local applications are also sent to the monitoring
application. A monitoring application is useful for debugging and
testing purposes. With a monitoring application on each node it is
possible to trace how content flows and is replicated throughout the
network.

Testing
=======

Please look at the following directories in the cbmen-encoders-eval
repository for tests:

    tests/AlphaDirect++

For example,

tests/AlphaDIRECT++/LxcNodesOnCore/*
tests/AlphaDIRECT++/LxcNodesOnCore/README.txt
tests/AlphaDIRECT++/LxcNodesOnCore/test_info.txt
tests/AlphaDIRECT++/UnscriptedScenarios/test_info.txt
tests/AlphaDIRECT++/UnscriptedScenarios/*/README.txt

test_info.txt gives an overview of each test. For example, it describes the
tests that demonstrate proactive replication functionality.

The scenarios from the September 2012 briefing can be found below:

tests/AlphaDIRECT++/LxcNodesOnCore/Testsuite/004-demo-static1/
tests/AlphaDIRECT++/LxcNodesOnCore/Testsuite/004-demo-static2/
tests/AlphaDIRECT++/UnscriptedScenarios/mobile1/
tests/AlphaDIRECT++/UnscriptedScenarios/mobile2/

A message ferry example that highlights proactive replication can be found 
here:

tests/AlphaDIRECT++/UnscriptedScenarios/mobile3/
