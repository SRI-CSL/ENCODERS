= NoSQL Database
Sam Wood <sam@suns-tech.com>
v1.0, 2013-10

This document describes the NoSQL Data Store extension for more efficient
data querying than Haggle's vanilla SQLDataStore.

NOTE: This document is written in 'asciidoc' format and can generate html
or latex output. Please install `source-highlight` for syntax highlighting. 

== Overview

This feature contains the in-memory database that does not use SQL and is 
optimized for queries with a few interests (< 5) and data objects with a 
moderate number of attributes (< 10). For example, the Drexel code issues 
queries that match a specific data object using a unique cuid. This 
implementation is designed to support a large number of data objects (~20,000) 
where the number of data objects that match a particular interest is small 
(<10).

== Matching Overview

There are 3 main objects for matching in Haggle: 

  . data objects
  . node descriptions
  . filters  
 
Node descriptions have interests which are (key, val, weight) triples,
along with a matching threshold and a maximum number of matches.
They have an underlying data object that encapsulates them. 

Data objects have (key, val) tuples.

Data objects and node descriptions are propagated throughout the network.

Filters are similar to node descriptions, but are used internally for
managers to listen to specific events (i.e., data object insertions with
a certain attribute). 

Node description to data objects matching: :: Find all the data objects that a node
is interested in. Usually, the matched data objects do NOT include
node description data objects, since these are propagated using a different
mechanism that does not use matching. 

Data object to node descriptions matching: :: Find all the nodes that are interested
in a particular data object. Usually, the data object is NOT a node
description data object, since these are propagated using a different
mechanism that does not use matching.

Data object to filter matching: :: Find all the filters that are interested in a 
data object. These MAY include node description data objects since
internal managers will be interested in these events.

Filter to data objects matching: :: Find all the data objects that a particular 
filter is interested in.


== Implementation

A majority of the changes are in `MemoryCache.cpp` and `MemoryDataStore.cpp`.
`MemoryDataStore.cpp` uses `MemoryCache.cpp` (class composition), where the
memory cache maintains the database state.
The new datastore implements the same `DataStore` interface as `SQLDataStore`.
We examined all of the queries that the `DataStore` performs and added multiple
hash tables that act as indicies for fast lookups. 
Data objects can be persisted to and loaded from disk by using the existing
`SQLDataStore`. 

.Unit Tests
[NOTE]
====
`MemoryCache.cpp` defines a unit test that validates itself
and tests the basic cache insertion, deletion, and query functionality. 
====

=== Relevant Files

The implementation can be found:
----
~/cbmen-encoders/src/hagglekernel/MemoryCache.{cpp,h}
~/cbmen-encoders/src/hagglekernel/MemoryDataStore.{cpp,h}
~/cbmen-encoders/src/hagglekernel/BenchmarkManager.{cpp,h}
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
~/cbmen-encoders/haggle/doc/api/hagglekernel/html/MemoryDataStore*
~/cbmen-encoders/haggle/doc/api/hagglekernel/html/BenchmarkManager*
----

== Evaluation

We modified the benchmark manager to test deletions, in addition to the
data object insertion and queries that were already supported. We use
the benchmark manager to stress the system and gain a direct comparison 
of `MemoryDataStore` to `SQLDataStore` in a controlled setting.

The benchmark has the following stages:
  . create and insert nodes
  . create and insert data objects
  . issue data object queries for each node (record statistics to log file)
  . issue the same queries and delete the returned data objects and node 

.Benchmark Manager
[IMPORTANT]
====
To enable benchmarking, you must compile with the following option:

----
cd ~/cbmen-encoders/haggle
./configure --enable-benchmark
----
====

To run the benchmark manager and generate results, please see:

----
~/cbmen-encoders/tests/NoSQLDB/BenchmarkGraphs/README.txt
----

=== Initial results

Below are results for nodes with 2 interests, data objects with 5 attributes,
attributes and interests chosen uniformly from a pool of 31,000,
30,000 data objects inserted, and 300 queries inserted. 

The following configuration parameters were used:
[source,xml]
----
<MemoryDataStore 
    run_self_tests="false" 
    in_memory_node_descriptions="true" 
    shutdown_save="false" 
    enable_compaction="false" 
    exclude_zero_weight_attributes="true" />
<SQLDataStore 
    use_in_memory_database="true" 
    journal_mode="off" 
    in_memory_node_descriptions="true" 
    exclude_zero_weight_attributes="true" />
----

[[img-do-delete-delay]]
.Data object delete delay
image::memdb-figs/dobj_delete_delays.png[Data object delete delay, 300, 200]

[[img-do-insert-delay]]
.Data object insert delay
image::memdb-figs/dobj_insert_delays.png[Data object insert delay, 300, 200]

[[img-memory]]
.Memory usage
image::memdb-figs/memory.png[Memory usage, 300, 200]

[[img-node-delete-delay]]
.Node delete delay
image::memdb-figs/node_delete_delays.png[Node delete delay, 300, 200]

[[img-node-insert-delay]]
.Node insert delay
image::memdb-figs/node_insert_delays.png[Node insert delay, 300, 200]

[[img-query-delays]]
.Query delay
image::memdb-figs/query_delays.png[Query delay, 300, 200]

We observe that with few exceptions, the `MemoryDataStore` significantly
outperforms `SQLDataStore` in the above scenario. 
We believe that the data object insert delays which exceed `SQLDataStore`
may be delays due to the hash table data structures resizing (this resizing
incurs the cost of a copy of the table). 

=== End-to-end Results

To test the end-end performance of the `MemoryDataStore`, we created a 4x4 
grid scenario with 800 random source destination pairs (unique attribute
/ interest for each data object with only 1 subscriber). Below
is the latency results for this scenario.

----
cd ~/cbmen-encoders/tests/NoSQLDB/Lxc/
./run_grid_test.sh
----

[[img-eval-latency]]
.End-to-end Delivery latency
image::memdb-figs/latency.png[End-to-end delivery latency results, 300, 200]

We observe that for this scenario, the `MemoryDataStore`
outperforms `SQLDataStore` under the latency metric. 

== Configuration Options

A detailed description of the configuration parameters and implementation
can be found in `MemoryDataStore.h`.

.Haggle Start-up 
[IMPORTANT]
====
Haggle *must* be started with the new `-m` parameter in order to
use the No SQL database, and the MemoryDataStore option must be specified
in the config:

[source,xml]
----
<DataManager ...>
  <DataStore>
    <MemoryDataStore 
        count_node_descriptions="false"
        exclude_node_descriptions="true"
        in_memory_node_descriptions="true" 
        exclude_zero_weight_attributes="true"
        max_nodes_to_match="30"
        shutdown_save="false" 
        enable_compaction="false"
        run_self_tests="false" />
  </DataStore>
</DataManager>
----
====

The following `MemoryDataStore` configuration options are supported:

`count_node_descriptions`:: Set to `true` to count node descriptions when
matching against `max_matches` parameter. Default is `false`.

`shutdown_save`:: Set to `true` to persist the database to disk during
shutdown (uses `SQLDataStore`). Default is `true`.

`exclude_node_descriptions`:: Set to `true` to remove node description
data objects from the result set when finding all data objects for a
particular node description. Default is `false`.

`in_memory_node_descriptions`:: Set to `true` to *not* keep node description
data objects in the database (implies `exclude_node_descriptions="true"`). 
Default is `true`.

`exclude_zero_weight_attributes`:: Set to `true` to not use attributes with
weight 0 in matching. Default is `false`.

`max_nodes_to_match`:: The maximum number of nodes to match for a particular
data object. Default is `30`.

`enable_compaction`:: Shrink the database upon sufficient deletions of
nodes and data objects (compaction can be slow!).

`run_self_tests`:: Set to `true` to run the unit tests and report 
success/failure in the log file. Default is `false`.

== Demo

An example configuration file is available here:

----
~/cbmen-encoders/haggle/resources/config-memdb.xml
----

Tests can be found at tests/NoSQLDB in the evaluation repository. 

