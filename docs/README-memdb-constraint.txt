= README for memdb-constraint
James Mathewson <james@suns-tech.com>

== What is memory constraint?
Since our target is mobile devices, and the database is usually run in-memory for speed increase, we have put a heavy burden
on the memory of the encoders solution.    In addition to the typical data objects, we keep attributes of relevance from data objects
for faster response in utility functions.    All of this tends to be memory heavy.
Since we can not control other aspects of memory beyond the database, data objects, and their associated overhead in utility functions,
we have created a memory constraint, which works in the following manner:
. Create a watermark for 'x' data objects, maximum, in the database.
. If we exceed 'x' data objects, we evict data objects until we are at 'x' data objects
. Memory purging is based upon utility functionality, as it is similar to content caching/purging rules.    The biggest difference
being, we do not use the knapsack (size of the content) to bias our decisions.    Each data object is approximately the same size,
and is selected for purging, solely based upon its weight.  


== Configuration Options

=== CacheStrategyUtility
CacheStrategy contains the memory constraint code.

Here is a sample of the configuration code, only for memory constraint.

[source,xml]
----
<CacheStrategyUtility manage_db_purging="true" db_size_threshold="100000" self_benchmark_test="true" >
----

The following options are currently supported:

`manage_db_purging = <bool>` ::
   This option, if true, will allow internal DB memory management.  

`db_size_threshold = <long long>` ::
 This option sets the watermark for internal DB DO entries.   When this value is exceeded, the database
 will purge entries until the size is within this watermark. 

`self_benchmark_test = <bool>` ::
 This option, if true, sets up the self benchmark test.  Normal operation can not occur when this is set to
true.  


== Self Test
If the self test is used:
. Take an initial snapshot of memory.   This state is called Start.

. Add 'x' amount of data objects to the system, until the limit is reached (db_size_threshold).   This state is called Up1.

. Lower the threshold by 'x' amount and let the natural routines purge the excess data objects, until we reach zero.  This is state Down1.
 
. Then reset the threshold adding 'x' more data objects at a time.    This is state Up2.

. Lower the threshold again, by 'x' data objects.    This is state Down2.

Sample output entry, enabled by DEBUG output:
----
34.316:[11931-8]{CacheStrategyUtility::selfTest}: 
Threshold(Up1): 5000/200 -- Used bytes: 1955408, Free bytes: 88496, SQL: 585968
----

This sample shows our current state (Up1) current threshold value (5,000), how many data objects are currently in the system (200),
how many bytes are used by malloc() (1955408), how many bytes are free (88496), and how many are used by SQL (585968; this value becomes 0 if the alternative memory benchmark is used).
Since the current implementation of mallopt() does not allow system memory to be returned, graphs should be based upon used bytes.

All the values of a specific state can be returned by grep command for the desired state  example:
----
grep Up1 /home/james/.Haggle/haggle.log | awk -F: '{ print $3}' | awk -F, '{print $1}'
----
This will return a (time) sequential list of how many used bytes from malloc().  It should be noted this returns ALL malloc(), not just those for data objects, so we wont be starting at 0 used memory.

== Benchmark comparison
Comparing the memory used on a system using the following:

. db_size_threshold=5000    5k DataObjects (no attached files) are benchmarked.

. The graph shows (green and red) the results of suns-tech improved memory only database.    The green is a 'compact' option, which will recopy the control data structures to minimize memory use, at the loss of cpu time to recopy.   The red is the database without compaction.

.  Blue is the SQL in-memory footprint.


[[benchmark-results]]
.Benchmark comparison results
image:../studies/cases/memdb-constraint/show-mem.png[Benchmark comparison results, 640, 400] in the evaluation repository. 

From the result:
----
Database used	| Up1 Max |	Up2 Max	|	Down1 Min |	Down2 Min |
SQL-in mem	26867328	25022640	5674224		7854800
mem-db		20781616	15137344	2112320		4499088
mem-db compact	15339136	15318544	2005472		4508432
----

 

