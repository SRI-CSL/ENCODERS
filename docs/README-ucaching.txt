What is Utility-based Content-Based Caching?
============================================

This branch extends Haggle to support more intelligent local content
caching, using the content caching pipeline described in the design doc. 
The main idea is to periodically compute a utility (a real between 0 and 1,
where 1 has highest utility) and evict data objects from the cache that do 
not meet a sufficient utility threshold, or evict data objects with the least
utility in the case where the capacity is exceeded. This mechanism allows 
Haggle to more intelligently manage disk space resources. The basic pipeline
is as follows:
 _________________      __________________      _______________ 
| compute utility | -> | threshold filter | -> | plan generator|
 -----------------      ------------------      ---------------

Stage Descriptions:

1. Compute Utility: computes a utility for each data object in the cache using 
a specified utility function. Each data object is assigned a double in the 
range [0,1].

2. Threshold Filter: all data objects with computed utility less than this
threshold are immediately evicted, regardless of the cache capacity. 

3. Plan Generator: in the case where the water-mark cache capacity is exceeded,
we frame the problem of selecting which data objects to keep in the cache
as a 0-1 knapsack problem with the utility as the benefit, and the size
of the data object as the cost. This stage uses a (typically heuristic-based)
knapsack optimizer to calculate the set of data objects to evict.

Implementation
==============

Please see the design document for an explanation of more implementation 
details. The design extends the content caching design, and we implemented
utility-based caching as a cache strategy in the existing design. Also see
the changes file for a description of each new file.

Currently, we support the described caching pipeline with the utility 
functions: popularity (LRU-K or LRFU), and neighborhood (milestone 1).
We have implemented a fixed weight global optimizer, whose settings are
configurable via config.xml. We implemented the heuristic-based knapsack
optimizer, which greedily fills the knapsack using highest marginal utility 
(utility/cost) first (until the watermark capacity is surpassed). 

The pipeline can be executed either periodically, or whenever a data object
is inserted (event based, which occurs upon receiving a data object from a 
remote node), or both. The watermark capacity functions as a soft constraint,
it may be exceeded, but it will not be exceeded after an execution of the 
pipeline. The maximum capacity specification is a hard constraint, data objects
will be dropped without execution of the pipeline if this constraint is 
surpassed. A typical use case is for a data object to be inserted which causes
a watermark constraint violation but not a hard constraint violation, then
upon pipeline execution data objects will be evicted so that the watermark
constraint is no longer violated. 

To avoid frequently computing redundant utility values, a data object's 
utility has a specified age X (specified using `compute_period_ms` in 
config.xml) which will prevent a recomputation of the utility if the utility 
was computed within the past X milliseconds.

The default greedy knapsack optimizer will break ties using the creation time.
Note that an empty aggregate utility function and the greedy optimizer will
treat the cache as a FIFO queue based on data object creation time (this
is similar to the default Haggle behavior, but with explicit cache resource 
constraints).

Configuration Options
=====================

The easiest way to familiarize oneself with the utility based caching pipeline
is to launch haggle in interactive mode with a configuration file that 
enables utility based caching and to press the "z" button to display the
current caching statistics. Note that by default utility based caching only
manages data objects that were received remotely, therefore a simple network
of at least two nodes is necessary to see utility-based caching in effect. 

Below is sample output of one 2-node session: 

        ===== Utility Based Caching Stats (1356664583.504801) =====
        ------------ parameters ---------------
        Knapsack optimizer: CacheKnapsackOptimizerGreedy
        Global optimizer: CacheGlobalOptimizerFixedWeights, threshold: 0.00
            factors: [<CacheUtilityNeighborhood, -1.00><CacheUtilityPopularity, 1.00>]
        Utility function: util(d) = max(0,(1.00*pop(d))-(1.00*nbrhood(d)))
        Utility compute period (ms): 200
        Poll purging: disabled
        Event-based purging: enabled
        Publish stats data object: enabled (CacheStrategyUtility=stats)
        ------------ statistics ---------------
        Total Capacity (kb/kb): 505/1024 = 49.32%
        Watermark Capacity (kb/kb): 505/512 = 98.63%
        Data objects in cache: 7
        Total data objects inserted: 10 (737280 bytes)
        Total data objects evicted: 3 (220160 bytes)
        ----- Top 10 Utility Data Objects -----
          ...
        --- Bottom 10 Utility Data Objects ----
          ...
        ----- Top 10 Largest Data Objects -----
          ...
        ------ Top 10 Oldest Data Objects -----
          ...
        =======================================

If the "publish_stats_dataobject" parameter is set to "true", then a node
will publish a data object with the attribute "CacheStrategyUtility=stats" 
every time the pipeline is executed, with these stats as an attribute. 
Internally, we use total order replacement on this data object to only keep 
the most recent stats. 

Below is a sample configuration file, and a description of its parameters.

	<CacheStrategy name="CacheStrategyUtility">
            <CacheStrategyUtility knapsack_optimizer="CacheKnapsackOptimizerGreedy" global_optimizer="CacheGlobalOptimizerFixedWeights" 
                   utility_function="CacheUtilityAggregateSum" max_capacity_kb="1024" watermark_capacity_kb="512" 
                   compute_period_ms="200" purge_poll_period_ms="1000" purge_on_insert="true" publish_stats_dataobject="false" discrete_size="51200"> 
                <CacheKnapsackOptimizerGreedy />
                <CacheGlobalOptimizerFixedWeights min_utility_threshold="0">
                    <Factor name="CacheUtilityPopularity" weight="1" />
                    <Factor name="CacheUtilityNeighborhood" weight="-0.1" />
                    <Factor name="CacheUtilityNewTimeImmunity" weight="1" />
                </CacheGlobalOptimizerFixedWeights>
                <CacheUtilityAggregateSum>
                    <Factor name="CacheUtilityPopularity">
                        <CacheUtilityPopularity>
                            <EvictStrategyManager default="LRFU">
                                <EvictStrategy name="LRFU" className="LRFU" countType="VIRTUAL" pValue="2.0" lambda=".01" />
                                <EvictStrategy name="LRU_K" className="LRU_K" countType="TIME" kValue="3" />
                            </EvictStrategyManager>
                        </CacheUtilityPopularity>
                    </Factor>
                    <Factor name="CacheUtilityNeighborhood" />
                    <Factor name="CacheUtilityNewTimeImmunity"/>
                </CacheUtilityAggregateSum>
            </CacheStrategyUtility>
	</CacheStrategy>


This configuration file uses the greedy knapsack optimizer as specified above,
a fixed weight global optimizer, and an aggregate (summation) utility function.
Data objects will not have the utilities computed more frequently than every
200 ms, the pipeline will be executed every second or when data objects
are inserted, and a statistics data object will be published upon every
execution. Since the minimum threshold is 0, no filtering will occur. 
We give popularity a weight of 1, and neighborhood a weight of -1: these
weights are multiplied by the [0,1] utility values computed by each 
component utility function, and then summed by the aggregate (with a min of 0).

<CacheStrategy>

name : we implemented utility based caching as a cache strategy named 
"CacheStrategyUtility" under the <DataManager> tag.

<CacheStrategyUtility>

knapsack_optimizer : specifies which knapsack optimizer to use. Currently
we only support the heuristic "CacheKnapsackOptimizerGreedy" optimizer.

global_optimizer : specifies the global optimizer which weights utility [-1,1]
functions and thresholds in response to network feedback. Currently we only
support the "CacheGlobalOptimizerFixedWeights" which uses fixed weights specified
in config.xml. 

utility_function : specifies which utility function to use for computing data
object utility. Each utility function can be specified a name, which
is referenced by the global optimizer. If no name is specified, then
the default name is the utility function's class name. 

Currently we support the following utility functions:
----

Utility Functions: all utility functions return a value between [0,1], while
some utility functions return only 0 or 1.
A value of 0 is interpreted as the utility function designating that the
data object has no utility (it should not be stored in the cache). While
a value of 1 indicates that the data object has a lot of utility and should
be kept in the cache.

----

"CacheUtilityAggregateMin" : takes the minimum utility value of its constituent
utility functions.

i.e.:
<Factor name="CacheUtilityAggregateMin">
    <CacheUtilityAggregateMin>
    ...
    </CacheUtilityAggregateMin>
</Factor>

----

"CacheUtilityAggregateMax" : takes the maximum utility value of its constituent
utility functions.

i.e.:
<Factor name="CacheUtilityAggregateMax">
    <CacheUtilityAggregateMax>
    ...
    </CacheUtilityAggregateMax>
</Factor>

"CacheUtilityAggregateSum" : simply sums the values of its constituent utility 
functions, and ensures the compute utility is within [0,1]. 

i.e.:
<Factor name="CacheUtilityAggregateSum">
    <CacheUtilityAggregateSum>
    ...
    </CacheUtilityAggregateSum>
</Factor>

----

"CacheUtilityNeighborhood" : computes a utility based on the frequency of the
data object within the 1-hop neighborhood (by inspecting the Bloom filter).
This is usually used to reduce the utility of keeping a particular data object
that frequently appears within a neighborhood, and the global optimizer 
multiplies the utility by a negative constant. 

i.e.:
<Factor name="CacheUtilityNeighborhood" />
    <CacheUtilityNeighborhood discrete_probablistic="true" neighbor_fudge="0" />
</Factor>

The function examines the number of replicas R within the 1-hop neighborhood
of size N to compute a probability of evicting the data object P, where:

P = R / (N + F) 

Where F specified in the 'neighbor_fudge'. 

If 'discrete_probablistic' is true, then the utility will be 1 with
probability P, and 0 otherwise. 
If 'discrete_probablistic' is false, then the utility is P. 

----

"CacheUtilityNeighborhoodSocial" : generalizes "CacheUtilityNeighborhood" by
examining specific nodes within a social group (as defined by a common 
interest), possibly outside the 1-hop neighborhood. 

i.e.:

<Factor name="CacheUtilityNeighborhoodSocial">
      <CacheUtilityNeighborhoodSocial discrete_probablistic="false" neighbor_fudge="4" 
           default_value="0" social_identifier="squad" only_physical_neighbors="false" />
</Factor>

'discrete_probablistic' and 'neighbor_fudge' are equivalent to that in 
"CacheUtilityNeighborhood", and P is computed in the same way. 

'social_identifier' defines the attribute A=(K,V) key K where V identifies the
social group (nodes with a common interest). 

If 'only_physical_neighbors' is true, then only 1-hop neighbors within the 
social group will be inspected for replicas.
If 'only_physical_neighbors' is false, then all received node descriptions 
for the specified social group will be inspected.

'default_value' is the utility for a data object that is returned when the
current node does not belong to any social group.  

----

"CacheUtilityNeighborhoodOtherSocial" : this utility function is
based upon the concept that nodes in the same social group will
have a high likelihood of being in contact with nodes of the same group.
This utility is biased towards the number of unique groups
containing the content in question.

i.e.:
<Factor name="CacheUtilityNeighborhoodOtherSocial">
    <CacheUtilityNeighborhoodOtherSocial
     only_physical_neighbors="true"
     exp_num_neighbor_group="4"
     less_is_more="false"
     exclude_my_group="false"
     max_group_count="1" />
</Factor>

`only_physical_neighbors=<bool>` ::
This option will only look at the social group names of its 1-hop neighbors, if true.
If false, we look at the entire known network nodes social group names.

`exp_num_neighbor_group=<int>` ::
This sets the expectied number of social groups in mission. For example, if exp_num_neighbor_group=4, and
a node only sees 3 distinct groups, it can assume there is a 4th, unseen group. 
This is handy for defined hierarchy social groups (e.g. military), where you know
your own group, and how many other groups are out there.

`less_is_more=<bool>` ::
This is an option, if true, returns '(1-value)'. Otherwise, it returns
'(value)'.  This helps tweaking the utility function to have more copies in more
groups vs high diversity w.r.t. unique data object. 

`exclude_my_group=<bool>`::
This will exclude a node's own group in the utility calculation. 

`max_group_count=<int>`::
This value sets the maximum number in a group to be counted, if they have
the requested data object.

NodeManager configuration option can setup a predefined group label of a
node to use CacheUtilityNeighborhoodOtherSocial utility function. 
All nodes having the same label are considered in the same social group.    
The relationships between nodes and group names is handled in NodeStore.     
All nodes not given an explicit social group name are considered to be
in a single group.   

i.e.
<NodeManager social_group="Platoon1" >

----

"CacheUtilityLocal" can be used to ensure at least one copy of content
available in the network (on the publisher node).
This is different from <CacheStrategyUtility  manage_only_remote_files="true" />
option, this has the effect of not doing ANY processing on local files, which
prevent replacement or time expiration.     

i.e.
<Factor name="CacheUtilityLocal">
    <CacheUtilityLocal protect_local="true" />
</Factor>

`protect_local=<bool>` ::
This option turns on all local produced content to return a value of '1'.
If this option is false, or the content is non local, it returns a '0'.

----

"CacheUtilitySecureCoding" : is a security mechanism to bound the number of 
network coded blocks per data object. In particular, each node will only
store up to P% of the linearly independent network coded blocks necessary
in order to deconstruct the data object. This mechanism is useful to prevent
the case where squad member's phone is stolen: no member in the squad has
enough blocks to deconstruct the file, but within the squad there are enough
blocks to retrieve the file at any time. 

i.e.:

<Factor name="CacheUtilitySecureCoding">
    <CacheUtilitySecureCoding max_block_ratio="0.5" rel_ttl_since_last_block="20" />
</Factor>

"max_block_ratio" is the maximum ratio of linearly independent blocks to keep
for each data object. 

"rel_ttl_since_last_block" - is the delay to wait before evicting blocks 
belonging to data object D in order to be under the "max_block_ratio". 
Specifically, we wait until no blocks have been received for D within 
"rel_ttl_since_last_block" seconds before we start evicting blocks.

----

"CacheUtilityPopularity" uses LRU-K or LRFU to assign higher utility to data
objects that are frequently "accessed". We define an "access" as i) insertion,
ii) successfully sending the data object to a peer. 

<EvictStrategy> defines the various LRU methods. Currently, we support 2 
methods, LRU-k, and LRFU.
"default" : specifies  the name of the default LRU. 
All LRU modules are updated whenever a DO is received or a filter search 
matches the DO, but unless you specifically ask for a result by name, only 
the default LRU method result is returned.

For <LRU_K> or <LRFU>, if the name is not defined, it goes by the default 
type (e.g. <LRFU> default name is LRFU. countType is either COUNT (each DO 
that is accessed increments the count. We define an access as either an
insertion or a send of the data object to a neighbor.

i.e.:
<Factor name="CacheUtilityPopularity">
<CacheUtilityPopularity>
    <EvictStrategyManager default="LRFU">
        <EvictStrategy name="LRFU" className="LRFU" countType="VIRTUAL" pValue="2.0" lambda=".0000001" />
    </EvictStrategyManager>
</CacheUtilityPopularity>
</Factor>

----

"CacheUtilityNewTimeImmunity" gives a positive value if the DO was received within a
certain timeframe.  Default is 2.5 seconds, but can be adjusted with xml option.
Thus, all DO's that (by default) are within 2.5 seconds of the current time, 
are given a '1', and all that are not are given a '0'. This feature was added, 
as it was noted on  full systems, large new DOs were dropped before they 
had a chance to disperse, as older DOs were distributed several times, 
giving them higher values.

i.e.: 
<Factor name="CacheUtilityNewTimeImmunity">
    <CacheUtilityNewTimeImmunity TimeWindowInMS="120000" />
</Factor>

----

"CacheUtilityPurgerRelTTL" : computes a {1,0} utility based on whether the 
data object has expired by a relative received date. This utility function is 
a carry over from the CacheReplacement architecture. As in the previous 
architecture, it takes the parameters: "purge_type", "tag_field", 
"tag_field_value" and "min_db_time_seconds". Unlike in the other architecture, 
the data object is only evicted upon pipeline execution (as with every other 
utility function). 

i.e.:
<Factor name="CacheUtilityPurgerRelTTL">
    <CacheUtilityPurgerRelTTL purge_type="purge_after_seconds" tag_field="ContentType" tag_field_value="DelByRelTTL" min_db_time_seconds="1" />
</Factor>

----

"CacheUtilityPurgerAbsTTL" : This utility function is identical to the
RelTTL, but uses absolute time. 

i.e. 
<Factor name="CacheUtilityPurgerAbsTTL">
    <CacheUtilityPurgerAbsTTL purge_type="purge_by_timestamp" tag_field="ContentType" tag_field_value="DelByAbsTTL" min_db_time_seconds="1" />
</Factor>

----

CacheUtilityNewTimeImmunity, CacheUtilityPurgerRelTTL, and
CacheUtilityPurgerAbsTTL have  'linear_declining' configuration option 
to allow utility declines in linear function (instead of a 1,0 utility).   

i.e.
<Factor name="CacheUtilityNewTimeImmunity">
    <CacheUtilityNewTimeImmunity TimeWindowInMS="10000" linear_declining="true" />
</Factor>

----

"CacheUtilityReplacementTotalOrder" : computes a {1,0} utility based on whether
the data object is subsumed by another data object (using a total order
replacement, specified in the configuration). This utility function is a 
carry over from the CacheReplacement architecture. As in the previous
architecture, it takes the parameters: "metric_field", "id_field", "tag_field"
and "tag_field_value".

i.e.:
<Factor name="CacheUtilityReplacementTotalOrder">
    <CacheUtilityReplacementTotalOrder metric_field="ContentCreateTime" id_field="ContentOrigin" tag_field="ContentType" tag_field_value="TotalOrder" />
</Factor>

----

"CacheUtilityReplacementPriority" : computes a {1,0} utility based on an ordered
list of replacement utility functions. Note that the specification is slightly
different than aggregate utility functions (there are no <Factors> within
the CacheUtilityReplacementPriority. This utility function is a carry
over from the CacheReplacement architecture. 

i.e.:
<Factor name="CacheUtilityReplacementPriority">
    <CacheUtilityReplacementPriority>
        <CacheUtilityReplacement name="CacheUtilityReplacementTotalOrder" priority="2">
            <CacheUtilityReplacementTotalOrder metric_field="MissionTimestamp" id_field="ContentOrigin" tag_field="ContentType" tag_field_value="TotalOrder" />
        </CacheUtilityReplacement>
        <CacheUtilityReplacement name="CacheUtilityReplacementTotalOrder" priority="1">
        <CacheUtilityReplacementTotalOrder metric_field="ContentCreateTime" id_field="ContentOrigin" tag_field="ContentType" tag_field_value="TotalOrder" />
        </CacheUtilityReplacement>
    </CacheUtilityReplacementPriority>
</Factor>

----

"CacheUtilityAttribute" : computes a utility based on a specific attribute.  
If Haggle receives a data object with attribute: utility=0.66
then it will assign a utility of 0.66 (it is still subject to the 
global optimizer and other utilities like every other function). 
"attr_max_value" specifies a factor to divide the utility 
to compute the utility over an attribute that isn't
necessarily [0,1]. Default is 1.

i.e.: 
<CacheGlobalOptimizerFixedWeights min_utility_threshold="0.34">
    <Factor name="CacheUtilityAttribute" weight="1" />
</CacheGlobalOptimizerFixedWeights>
<CacheUtilityAttribute attribute_name="utility" />

----

max_capacity_kb : a hard constraint on the cache capacity. Data objects will
be dropped immediately (without triggering the pipeline to make space) if 
their insertion will surpass this constraint. 

watermark_capacity_kb : a soft constraint on the cache capacity. Data objects
may be inserted causing this constraint to be violated. Upon execution of the
pipeline, data objects will be evicted so that the watermark is not exceeded.

compute_period_ms : specifies a bound on the maximum frequency that a data
object's utility is computed. Data objects will not have their utility
computed more frequently than this period.

purge_poll_period_ms : specifies how often to run the pipeline. To disable
poll based pipeline execution, set this value to 0.

purge_on_insert : "true" or "false" : enable/disable pipeline execution based
on data object insertion. NOTE: event-based execution may be slow
in systems with a large number of data objects. 

publish_stats_dataobject : "true" or "false" : enable/disable publishing
a data object containing cache statistics every time the pipeline is executed.
Only the most recent statistics data object is kept in the cache. These
data objects have an attribute "CacheStrategyUtility=stats".

manage_only_remote_files : "true" or "false" : enable/disable management of
locally published files in addition to files received remotely. In most
cases this value should be "true": "false" allows files published
by an application to be deleted, which is possible, because Haggle takes ownership
of those files. A value of "true" only manages files that are 
received remotely and are stored in the ~/.Haggle directory. 

keep_in_bloomfilter : "true" or "false" : if "true", then data objects
evicted will remain in the bloomfilter, otherwise they are removed.

bloomfilter_remove_delay_ms : must have keep_in_bloomfilter="false".
This option will wait the specified amount of time prior to removing
the data object from the bloomfilter.  It is useful to avoid strong
assumptions on time synchronization in case of expiration-based purging.

discrete_size : when the knapsack optimizer computes the marginal
utility, it will use this parameter to discretize the sizes 
of the data objects to a less granular level. For example,
if the discrete_size is set to 10KB then for all intents
and purposes, the knapsack optimizer will treat a
data object of 71KB and 72KB as the same size (
and will then fall back to eviction based on create time
if they have the same marginal utility). 
Default is 1 (acts the same way as before, no greater discretization).

Testing
=======

Please look at the following directories in the cbmen-encoders-eval
repository for tests:

    tests/CacheUtility

Meta-Configuration
==================

Strength of our method lies in combining multiple sources of knowledge to
influence caching decisions, and the ease and flexibility in specifying the
utility function (declarative approach).

Below is a meta-configuration that demonstrates almost all of the utility based
caching features. We then discuss the utility function that it defines. This
is mainly for illustrative purposes, and would not necessarily be an effective
utility function (for example, a weight of 0 simply disables the NOP function,
making it redundant, and a min function within a min is redundant).

<CacheStrategy name="CacheStrategyUtility">
    <CacheStrategyUtility knapsack_optimizer="CacheKnapsackOptimizerGreedy" global_optimizer="CacheGlobalOptimizerFixedWeights" 
                utility_function="CacheUtilityAggregateMin" max_capacity_kb="81920" watermark_capacity_kb="71680" 
                compute_period_ms="500" purge_poll_period_ms="400" purge_on_insert="true" 
                publish_stats_dataobject="true" keep_in_bloomfilter="false" handle_zero_size="true" 
                bloomfilter_remove_delay_ms="16000" manage_only_remote_files="true"> 
        <CacheKnapsackOptimizerGreedy />
        <CacheGlobalOptimizerFixedWeights min_utility_threshold="0.1">
            <Factor name="CacheUtilityAggregateMin1" weight="1" />
            <Factor name="CacheUtilityAggregateMax" weight="1" />
            <Factor name="CacheUtilityNewTimeImmunity" weight="1" />
            <Factor name="CacheUtilityAggregateSum" weight="1" />
            <Factor name="CacheUtilityRandom" weight=".1" />
            <Factor name="CacheUtilityPopularity" weight=".7" />
            <Factor name="CacheUtilityNOP" weight="0" />
            <Factor name="CacheUtilityNeighborhood" weight=".3" />
            <Factor name="CacheUtilityAggregateMin2" weight="1" />
            <Factor name="CacheUtilityPurgerRelTTL" weight="1" />
            <Factor name="CacheUtilityPurgerAbsTTL" weight="1" />
            <Factor name="CacheUtilityReplacementPriority" weight="1" /> 
        </CacheGlobalOptimizerFixedWeights>
        <CacheUtilityAggregateMin name="CacheUtilityAggregateMin1">
            <Factor name="CacheUtilityAggregateMax">
                <CacheUtilityAggregateMax>
                    <Factor name="CacheUtilityAggregateSum">
                        <CacheUtilityAggregateSum>
                            <Factor name="CacheUtilityNeighborhood">
                                <CacheUtilityNeighborhood discrete_probablistic="true" neighbor_fudge="1" />
                            </Factor>
                            <Factor name="CacheUtilityRandom"/>
                            <Factor name="CacheUtilityPopularity">
                                <CacheUtilityPopularity>
                                    <EvictStrategyManager default="LRFU">
                                        <EvictStrategy name="LRFU" className="LRFU" countType="VIRTUAL" pValue="2.0" lambda=".01" />
                                        <EvictStrategy name="LRU_K" className="LRU_K" countType="TIME" kValue="2" />
                                    </EvictStrategyManager>
                                </CacheUtilityPopularity>
                            </Factor>
                            <Factor name="CacheUtilityNOP"/>
                        </CacheUtilityAggregateSum>
                    </Factor>
                    <Factor name="CacheUtilityNewTimeImmunity">
                        <CacheUtilityNewTimeImmunity TimeWindowInMS="12000" />
                    </Factor>
                </CacheUtilityAggregateMax>
            </Factor>
            <Factor name="CacheUtilityAggregateMin">
                <CacheUtilityAggregateMin name="CacheUtilityAggregateMin2">
                    <Factor name="CacheUtilityPurgerRelTTL">
                        <CacheUtilityPurgerRelTTL purge_type="purge_after_seconds" tag_field="ContentType" tag_field_value="DelByRelTTL" min_db_time_seconds="1" />
                    </Factor>
                    <Factor name="CacheUtilityPurgerAbsTTL">
                        <CacheUtilityPurgerAbsTTL purge_type="purge_by_timestamp" tag_field="ContentType" tag_field_value="DelByAbsTTL" min_db_time_seconds="1" />
                    </Factor>
                </CacheUtilityAggregateMin>
            </Factor>
            <Factor name="CacheUtilityReplacementPriority">
                <CacheUtilityReplacementPriority>
                    <CacheUtilityReplacement name="CacheUtilityReplacementTotalOrder" priority="2">
                        <CacheUtilityReplacementTotalOrder metric_field="MissionTimestamp" id_field="ContentOrigin" tag_field="ContentType" tag_field_value="TotalOrder" />
                    </CacheUtilityReplacement>
                    <CacheUtilityReplacement name="CacheUtilityReplacementTotalOrder" priority="1">
                        <CacheUtilityReplacementTotalOrder metric_field="ContentCreationTime" id_field="ContentOrigin" tag_field="ContentType" tag_field_value="TotalOrder" />
                    </CacheUtilityReplacement>
                </CacheUtilityReplacementPriority>
            </Factor>
        </CacheUtilityAggregateMin>
    </CacheStrategyUtility>
</CacheStrategy>

This defines a utility function as follows:

utility(d) = min(
                    max(
                            0.3*nbrhood(d) + 0.10*random(d) + 0.70*pop(d) + 0*NOP, 
                            newtimeimmune(d)
                    ),
                    min(    
                        relttl(d), 
                        absttl(d)
                    ),
                    replacement(d)
                )

Where:
------

nborhood    - corresponds to the defined neighborhood function
random      - corresponds to the defined random function
pop         - corresponds to the defined LRU_K/LRFU function 
            (only LRFU is used, but LRFU_K updates the scratchpad)
NOP         - corresponds to the NOP function (returns 1)
relttl      - corresponds to relative TTL purging
absttl      - corresponds to absolute TTL purging
replacement - corresponds to total order replacement 

What it does:
-------------
Recall that every utility function returns a value between 0 or 1, and 
utility(d) is simply a utility function composed of other utility functions.
0 indicates that the data object has no value and should be evicted from
the cache, while 1 means that the data object has the highest possible value.
The threshold and constants set by the global optimizer allow utility caching
to fine tune the influence of each utility function. Here, any data object d
which has utility(d) < 0.1 will be evicted immediately. Note that a weight
of 0 simply disables the utility function (here the NOP function is disabled).

By using the max/min functions, one can create "overrides" that allow a 
utility function to ignore the utilities of other utility functions. 
In this configuration, if either replacement or the purgers (relative ttl,
rel ttl, or absolute ttl, abs ttl) return a 0, then the entire utility
function will return 0 (since they are wrapped in a min() function). If
none of them return 0 (they only return 0 if they believe the DO should be
evicted, otherwise they return 1), then the max function is used. 

The max function in this example allows the TimeImmunity utility function 
(newtimeimmune) to override the utility from the summation function. This
is useful to allow a data object that is not purged by replacement or time
purging to remain in the cache for at least a specified amount of time (12
seconds in this case). Once the time immunity has expired, then the summation
utility function is executed. 

The summation function here takes the values of neighborhood (nbrhood), 
random, LRFU (pop) and NOP, and sums them to construct a single utility
value. The weights specified by the global optimizer limits the amount of
"influence" one of these functions have on the entire value. For example,
the global optimizer gives NOP a value of 0 to disable it (it simply returns 
1), while popularity has the most influence.

