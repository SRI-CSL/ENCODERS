= Utility-based Replication
Sam Wood <sam@suns-tech.com>
James Mathewson <james@suns-tech.com>
v1.0, 2014-03

This document describes the Utility-based Replication system that is used
to replicate content. 

NOTE: This document was written in 'asciidoc' format and can generate html
or latex output. Please install `source-highlight` for syntax highlighting.

== Overview

The purpose of the utility replication branch is to provide a system that
supports intelligent replication policies and is flexible. Similar to how
the utility caching system frames the caching problem as a 0-1 knapsack
optimization problem, this system frames the replication problem as a 0-1
knapsack optimization problem.

== Implementation

Utility based replication does the following steps:

1.  Any neighbor node changes or updates, any new data object inserted, or specified poll period, the 
utility dissemination pipeline is called.

2.  The pipeline differs from the caching pipeline, as it gives a utility not based only upon the data object, but upon
a data object <--> potential destination node pair.

3.  A threshold (min_utility_threshold) is set, and any data object above this value is considered for replication.

4.  A knapsack will take all considered data objects for replication to a node, and the estimated connection time to that node,
and consider how many data objects, estimated, can be sent, up to maximum allowed tokens (max_repl_tokens).    The knapsack considers which data objects have the highest value (utility value/size), that can fit in the knapsack (bound by remaining expected contact time), up to the maximum number of free tokens.

The estimated connection time and bandwidth are obtained by the following steps: 
a.  A node measures neighbors' connect/disconnect time, up to 200 data points, and average them.   
b.  A node measures how much data is transmitted, and how long it takes, for an average bytes/sec.    We cannot measure how
long it takes to receive a file, but using block send, we can measure how long to transmit a file.
c.  A node reports two parameters (average connection time in milliseconds, and average bytes/sec) in its node description.  
d.  If a node never disconnects from a neighbor node, the connection is considered static, thus, all data objects can be, potentially, sent.   
The reported connection time is 0ms (since the nodes are neighbors, this signifies, currently, they are always in contact).
e. Every node stores information from its neighbor nodes' node description about 'average connection time in milliseconds' and 'average throughput'.  

5.  Initially, every node has a limited token count.    Upon every success or
failure, if a node is still a neighbor, a token is put back on the stack, and
the utility pipeline is called for the same destination node.   For example, if 5
tokens are allocated, and we successfully send 1 data object, the returned token
allows 1 more data object to be queued with the prior 4 data objects (and
prioritized).    The tokens sets an upper bound of how many data objects
can be queued for replication at a time.  Optionally, instead of a token count
per node, the token count can be measured against total number of data objects,
to be sent. 

6.  Since a batch of data objects are considered at once, based upon maximum token count and knapsack, the data objects are queued with other data objects in the priority queue.   Replication is considered low priority, thus, it should not interfere with higher priority attribute matches.

Replication Events:
ReplicationManager handles Events slightly different.    Replication Manager has its own events, and translates system events into replication events.    Thus, events, such as notifyDeleteDataObject, notifyInsertedDataObject, notifyNodeChange, etc, are translated into replication events.    Thus, there is a 2 tier event system for replication.


=== Relevant Files

The above files support Doxygen documentation, to generate the documentation,
install Doxygen >= 1.8.5 and execute:

----
cd ~/cbmen-encoders/haggle/
./configure --enable-docs
make docs
----

The documentation can be found:

----
~/cbmen-encoders/haggle/doc/api/hagglekernel/html/ReplicationManager*
----

== Configuration

For a detailed description of each configuration parameter, please see

----
~/cbmen-encoders/src/hagglekernel/ReplicationManagerUtility.h
----

An example Utility-based Replication configuration is:

[source,xml]
----
<ReplicationManager name="ReplicationManagerUtility" >
	<ReplicationManagerUtility 
	enabled="false" 
	max_repl_tokens="1" 
	token_per_node="false" 
	replication_cooldown_ms="200" 
	run_self_test="false" 
	forward_poll_period_ms="10000" 
	compute_period_ms="11000" 
	forward_on_node_change="true" 
	forward_on_do_insert="false" 
	knapsack_optimizer="ReplicationKnapsackOptimizerGreedy" 
	global_optimizer="ReplicationGlobalOptimizerFixedWeights" 
	utility_function="ReplicationUtilityAggregateSum" >
		<ReplicationGlobalOptimizerFixedWeights min_utility_threshold="0.31" >
                	<Factor name="ReplicationUtilityAggregateSum" weight="1.0" />
                    	<Factor name="ReplicationUtilityLocal" weight="0.11" />
         		<Factor name="ReplicationUtilityWait" weight="0.1" />
                   	<Factor name="ReplicationUtilityAttribute" weight="0.59" />
		    	<Factor name="ReplicationUtilityNeighborhoodOtherSocial" weight="0.3" />
		</ReplicationGlobalOptimizerFixedWeights>
		<ReplicationKnapsackOptimizerGreedy discrete_size="350200"  />
		<ReplicationUtilityAggregateSum name="ReplicationUtilityAggregateSum">
			<Factor name="ReplicationUtilityLocal">
    				<ReplicationUtilityLocal protect_local="true" />
			</Factor>
        		<Factor name="ReplicationUtilityWait" >
            			<ReplicationUtilityWait name="ReplicationUtilityWait"/>
        		</Factor>
                	<Factor name="ReplicationUtilityAttribute">
 				<ReplicationUtilityAttribute attribute_name="Important" />
			</Factor>
			<Factor name="ReplicationUtilityNeighborhoodOtherSocial">
    				<ReplicationUtilityNeighborhoodOtherSocial
     				only_physical_neighbors="false"
     				exp_num_neighbor_group="3"
     				exclude_my_group="true"
     				max_group_count="1" />
			</Factor>
		</ReplicationUtilityAggregateSum>
	</ReplicationManagerUtility>
</ReplicationManager>
----

The following `ReplicationManagerUtility` configuration options are supported:

`enable`::
    Set to `true` to enable the `ReplicationManagerUtility`, `false` otherwise.

`run_self_test`::
    Set to true to enable internal verification.   This only tests individual components, not
the full functionality.    The results are output into the log.

'replication_cooldown_ms'::
     This option sets how long to wait, before the knapsack will be used to
replicate again (in milliseconds).

'max_repl_tokens'::
      This sets how many tokens, maximum.    This affects either how many data
objects, total, or how many data objects, per node, to be considered for the
knapsack.

'token_per_node'::
     This sets either the tokens to be counted per node (true), or, as data
objects total (false).

`compute_period_ms`::
     The minimum amount of time before allowing the udissemination pipeline to recalcuate new values.   

`forward_on_node_change`::
     Boolean value to call the udissemination pipeline when a node change event occurs.


`forward_on_do_insert`::
     Boolean value to call the udissemination pipeline when a new data object is inserted.

`forward_poll_period_ms`::
     This sets the timer (in milliseconds), of how often to execute the replication utility code.   It should be noted, that the replication code will always be called upon a new data object (data object inserted) or a change in neighbor nodes (a new node seen, a node is no longer in 1-hop neighborhood, or a node change (such as updated contact times)).

`knapsack_optimizer`::
     Specify which knapsack optimizer, which will decide, based upon file size, utility value, and estimated amount of time to replicate, which data objects to prioritize.    Currently, there is only one knapsack, `ReplicationKnapsackOptimizerGreedy`, is supported.

`global_optimizer`::
    This option sets the global optimizer.   Currently, only `ReplicationGlobalOptimizerFixedWeights` is supported.    

`utility_function`::
    Specifies the title of xml code which will be used in utility replication, and which ReplicationUtilityAggregate[Minimum, Sum, Maximum] will be used.    

The following 'ReplicationGlobalOptimizerFixedWeights` configurations options are supported, which functions similar to CacheGlobalOptimizerFixedWeights. 
The difference is, CacheGlobalOptimizerFixedWeights purges data objects if its utility is below a certain threshold, while ReplicationGlobalOptimizerFixedWeights considers to replicate data objects if its utility is above a certain threshold.    All data objects above a threshold are passed to the knapsack, to optimize which data objects are sent first.    

`min_utility_threshold`::
    The minimum utility value considered for replication.

Within `ReplicationGlobalOptimizerFixedWeights`:
<Factor name=[utility] weight="x"/>    This gives each 'utility' function a weight of 'x'.   Valid utilities are:
    a. ReplicationUtilityAggregateMin
    b. ReplicationUtilityAggregateMax
    c. ReplicationUtilityAggregateSum
    d. ReplicationUtilityNOP
    e. ReplicationUtilityRandom
    f. ReplicationUtilityAttribute
    g. ReplicationUtilityWait 
    h. ReplicationLocal
    i. ReplicationNeighborhoodOtherSocial
    
`ReplicationUtilityWait` gives a value of '0', until a specified amount of time has passed, in which case, it returns a '1'.   
The following options are supported for `ReplicationUtilityWait`:

`wait_s`::
   Specify, in seconds, how long this function should return zero.    After this time interval has expired, return a value of '1'.

Refer READEME-ucaching.txt for other utility functions. 

The following ReplicationKnapsackOptimizerGreedy configuration options are supported:

`discrete_size`::
    Specify a block size to be used for consideration of the knapsack.   It is calculated with the formula: discreteSize = CEIL(filesize/discrete_size).     The, if data object A has a size of 50k bytes, and data object B has a size of 20k bytes, and both have a utility value of '1' (where value is utility/discreteSize), then data object A will have a value of 1/50k and data object B will have a value of 1/20k.    If the discrete_size="50000", both data object A and B will have the same value value (1/1).   If the discrete_size="25000", then data object A will have a value of 1/2 (50k/25k = 2), and data object B will have a value of 1/1 = 1 (25k/25k = 1).


Under this dissemination implementation, the utility requires a node, as well as a data object.   This means, the data object does not have a single utility value, but has a utility value TO THAT NODE.
Thus, data object A may have a value of 0 to node n1, and a value of 1 to node n2.   For caching, there is no node but self.    For replication, the value is based upon the data object, potential node destination tuple.    The following utilities are supported.

    a. ReplicationUtilityAggregateMin
    b. ReplicationUtilityAggregateMax
    c. ReplicationUtilityAggregateSum
    d. ReplicationUtilityNOP
    e. ReplicationUtilityRandom
    f. ReplicationUtilityAttribute
    g. ReplicationUtilityWait 
    h. ReplicationUtilityLocal 
    i. ReplicationUtilityNeighborhoodOtherSocial 

