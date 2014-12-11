= Interest Manager (1-Hop Bloom filters)
Sam Wood <sam@suns-tech.com>
v1.0, 2013-10

This document describes the Interest Manager extensions for more efficient
dissemination than flooding of Bloom filters. 

NOTE: This document is written in 'asciidoc' format and can generate html
or latex output. Please install `source-highlight` for syntax highlighting. 

== Overview

The purpose of multi-hop Bloom filters is to reduce the number of matching data
objects in a result set for a particular query. This reduction can save 
bandwidth when the queries are general and may match lots of content (e.g., 
``garfield''), but do not provide any benefits when the query is very specific
(i.e., a hash of the data object's content): when a node receives the requested
data object it simply unsubscribes. 
The `InterestManager` is responsible for interest dissemination, and the
`NodeManager` remains responsible for 1-hop Bloom filter dissemination. 

== Implementation 

The original haggle aggregated the interests from all the local applications
and disseminated node descriptions that contained this aggregation as well
as a cache summary (Bloom filter). 
The semantics feature separated the application's interests into their own
application node descriptions which are disseminated separately from the
device node descriptions which contain the Bloom filters. 
The direct forwarding module enabled propagation of node descriptions over multiple
hops using flooding.
This feature builds on the semantics feature and the direct forwarding module. Device
node descriptions containing Bloom filters are only propagated 1-hop. 
A new Interest Manager periodically generates a data object that aggregates
these application node descriptions into an Interest Data Object and publishes 
this data object.
The interest data object is then amenable to forwarding and caching using
the existing forwarding and caching pipelines. 
Upon receiving an interest data object, nodes unpack the encoded application
node descriptions and insert them into the data store (as if there was
no aggregation).
If a data object matches a target application node description and is forwarded
towards that target, then the application node description may be removed 
footnote:[Enabled by the so-called `consume_interest` parameter.].

Interest data objects have the following attributes: 

. `Interests=Aggregate`: Hardcoded attribute to identify interest data objects.
. `SeqNo=sequence_no`: Incrementing sequence number to replace old interests.
. `ATTL_MS=duration_ms`: Absolute TTL in milliseconds after which point
the node descriptions are deleted.

Where the `ForwardingManager` and `DataStore` manager use these attributes
to make forwarding and caching decisions. 

=== Relevant Files

The implementation can be found:
----
~/cbmen-encoders/src/hagglekernel/InterestManager.{cpp,h}
----

The above files support Doxygen documentation, to generate the documentation,
install Doxygen >= 1.8.5 and execute:

----
cd ~/cbmen-encoders/
./configure --enable-docs
make docs
----

The documentation can be found:

----
~/cbmen-encoders/haggle/doc/api/hagglekernel/html/InterestManager*
----

== Configuration

For a detailed description of each configuration parameter, please see 
----
~/cbmen-encoders/src/hagglekernel/InterestManager.h
----

An example Interest Manager configuration is:
  
[source,xml]
----
<InterestManager 
  enable="true" 
  interest_refresh_period_ms="30000" 
  interest_refresh_jitter_ms="500" 
  absolute_ttl_ms="90000" />
----

Here, interests will be periodically refreshed after 30 seconds, with half a second of jitter. After
90 seconds the interests will be removed (if the appropriate caching settings are enabled).
 
The following `InterestManager` configuration options are supported:
  
`enable`:: 
    Set to `true` to enable the interest manager
 
`interest_refresh_period_ms`::
    Interest refresh period in milliseconds.
 
`interest_refresh_jitter_ms`::
    Interest jitter in milliseconds.
  
`absolute_ttl_ms`::
    Absolute ttl in milliseconds for an interest data object.

`debug`::
    Set to `true` to enable extra verbose debug messages.

`consume_interest`::
    Set to `true` to enable removing an interest upon matching and forwarding. NOTE: This feature
    only applies to interests that have a maximum match of 1 (`-m 1` option to `haggletest`)!
 
`run_self_tests`::
     Set to `true` to run basic unit tests.

=== Node Manager Settings

The following `NodeManager` settings are recommended:

[source,xml]
----
<NodeManager> 
  <Node 
      matching_threshold="0" 
      max_dataobjects_in_match="10" 
      node_description_attribute="none" 
      node_description_attribute_weight="0" 
      send_app_node_descriptions="false" /> <1>
  <NodeDescriptionRetry 
      retries="0" 
      retry_wait="10.0"/>
</NodeManager>
----
<1> This is a new parameter that is used to disable forwarding of
application node descriptions by the `NodeManager`.

These settings will disable node description purging (the Connectivity time outs
will be used to remove stale 1-hop neighbors), and not propagate application
node descriptions. This functionality is the responsibility of the `InterestManager`.

=== DataManager Settings 

Example caching policies are as follows:

[source,xml]
----
<CacheStrategy name="CacheStrategyUtility">
  <CacheStrategyUtility 
      knapsack_optimizer="CacheKnapsackOptimizerGreedy" 
      global_optimizer="CacheGlobalOptimizerFixedWeights" 
      utility_function="CacheUtilityAggregateMin" 
      max_capacity_kb="10240" 
      watermark_capacity_kb="5120" 
      compute_period_ms="200" 
      purge_poll_period_ms="1000" 
      purge_on_insert="true" 
      publish_stats_dataobject="false" 
      keep_in_bloomfilter="true" 
      handle_zero_size="true"> 
    <CacheKnapsackOptimizerGreedy />
    <CacheGlobalOptimizerFixedWeights 
        min_utility_threshold="0.1">
      <Factor 
          name="CacheUtilityAggregateMin" 
          weight="1" />
      <Factor 
          name="CacheUtilityReplacementTotalOrder" 
          weight="1" />
      <Factor 
          name="CacheUtilityPurgerAbsTTL" 
        weight="1" />
    </CacheGlobalOptimizerFixedWeights>
    <CacheUtilityAggregateMin 
        name="CacheUtilityAggregateMin">
      <Factor 
          name="CacheUtilityReplacementTotalOrder">
      	<CacheUtilityReplacementTotalOrder 
            metric_field="SeqNo" 
            id_field="Origin" 
            tag_field="Interests" 
            tag_field_value="Aggregate" />
      </Factor>
      <Factor 
          name="CacheUtilityPurgerAbsTTL">
      	<CacheUtilityPurgerAbsTTL 
            purge_type="ATTL_MS" 
            tag_field="Interests" 
            tag_field_value="Aggregate" />
      </Factor>
    </CacheUtilityAggregateMin>
  </CacheStrategyUtility>
</CacheStrategy>
----

This will use total order replacement on interest data objects (fresher interest data objects
replace staler interest data objects), and it will use the absolute TTL specified 
in the `<InterestManager>` settings to purge old interest data objects.

.CPU-limit and Total Order Replacement
[IMPORTANT] 
====
We noticed in our evaluation that occasionaly `cpulimit` would interfere  
with the condition variable used for `CacheReplacementTotalOrder`. To
avoid these problems, we recommend disabling `cpulimit` in the automated
tests:

----
    ...
    "cpu_limit" : "none",
    ...
----
====


=== ForwardingManager Settings

Example forwarder policies for interest data objects are as follows:

[source,xml]
----
<ForwardingManager 
    max_nodes_to_find_for_new_dataobjects="30" 
    max_forwarding_delay="2000"
    node_description_retries="0" 
    dataobject_retries="1" 
    dataobject_retries_shortcircuit="2" 
    query_on_new_dataobject="true" 
    periodic_dataobject_query_interval="0" 
    enable_target_generation="false" 
    push_node_descriptions_on_contact="false"
    load_reduction_min_queue_size="500" 
    load_reduction_max_queue_size="1000" 
    enable_multihop_bloomfilters="false">  <1>
  <ForwardingClassifier 
      name="ForwardingClassifierAttribute">
    <ForwardingClassifierAttribute 
        class_name="flood" 
        attribute_name="Interests" 
        attribute_value="Aggregate" />
  </ForwardingClassifier>
  <Forwarder 
      protocol="Flood" 
      contentTag="flood" />
  <Forwarder 
      protocol="AlphaDirect" />
</ForwardingManager>
----
<1> `enable_multihop_bloomfilters` is a new parameter that should *always* be
set to `false` when using the `InterestManager`, as the `ForwardingManager` 
is no longer responsible for propagating node descriptions.

This configuration will propagate interest data objects epidemically.

=== ApplicationManager Settings

.Application De-registration
[IMPORTANT] 
====
We strongly suggest that the following configuration is set:

[source,xml]
----
    <ApplicationManager delete_state_on_deregister="true" />
----

This will enable triggering interest refresh when applications deregister and subsequently register.
====

== Evaluation

To evaluate the removal of multi-hop Bloom filters, we created a 5x5 grid
scenario where node 1 (upper left corner) publishes 250 data objects of
size 0 that node 25 (lower right corner) subscribes to.  
We then report the total bytes received (RX) and transmitted (TX) for when 
multi-hop Bloom filters are enabled (default) and disabled (1hop):

[[img-eval-overhead]]
.Control Overhead 
image::1hopbf-figs/overhead.png[Control Overhead, 300, 200]

We observe that in this scenario the removal of multi-hop Bloom filters results
in a significant reduction in both transmit and receive bandwidth.

== Demo

An example configuration file is available here:

----
~/cbmen-encoders/haggle/resources/config-1hopbf.xml 
----

Tests can be found at tests/1HopBloomFilter in the evaluation repository. 
