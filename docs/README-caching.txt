What is Content-Based Caching?
=============================

This branch contains our cache management framework which allows us to
define flexible strategies for content replacement and purging, such
as order-based replacement and expiration-time-based purging, which
are currently implemented.  The goal of cache management is to better
utilize network resources by prioritizing relevant and discarding
obsolete information, leading to more efficient data distribution and
better performance.  More advanced caching techniques further
extending this framework are under development.

Implementation
==============

Please see the design document for an explanation of more
implementation details.  The overall design closely follows the
forwarding design.

Configuration Options
=====================

The caching strategy is configured by the <CacheStrategy> tag in the
configuration file config.xml.  An excerpt is shown below:

(i) in <CachePurgerParallel> tag, two purgers are configured in
parallel based on absolute and relative expiration times,

(ii) in <CacheReplacementPriority> tag, the lexicographical ordering
is configured using priority field in <CacheReplacement> tag.

NOTE: We only discuss config.xml parameters that are relevant to caching
in this document. For an explanation of the other parameters, please see
the corresponding README for the master branch.

<Haggle persistent="no">
	<Attr name="ManagerConfiguration">*</Attr>
	<DataManager set_createtime_on_bloomfilter_update="true" periodic_bloomfilter_update_interval="60">
		<Aging period="3600" max_age="86400"/>
		<Bloomfilter default_error_rate="0.01" default_capacity="2000"/>
		<DataStore>
                        <SQLDataStore use_in_memory_database="true" journal_mode="off" 
				      in_memory_node_descriptions="true" exclude_zero_weight_attributes="true" />
		</DataStore>
		<CacheStrategy name="CacheStrategyReplacementPurger">
			<CacheStrategyReplacementPurger purger="CachePurgerParallel" replacement="CacheReplacementPriority">
				<CachePurgerParallel> 
					<CachePurger name="CachePurgerAbsTTL">
						<CachePurgerAbsTTL purge_type="purge_by_timestamp" 
						tag_field="ContentType" tag_field_value="DelByAbsTTL" 
						keep_in_bloomfilter="false" min_db_time_seconds="4.5" />
					</CachePurger>
					<CachePurger name="CachePurgerRelTTL"> 
						<CachePurgerRelTTL purge_type="purge_after_seconds" 
						tag_field="ContentType" tag_field_value="DelByRelTTL" 
						keep_in_bloomfilter="false" min_db_time_seconds="3.5" />
					</CachePurger>
				</CachePurgerParallel>
				<CacheReplacementPriority>
					<CacheReplacement name="CacheReplacementTotalOrder" priority="2">
						<CacheReplacementTotalOrder metric_field="MissionTimestamp" id_field="ContentOrigin" 
						tag_field="ContentType" tag_field_value="TotalOrder" />
					</CacheReplacement>
					<CacheReplacement name="CacheReplacementTotalOrder" priority="1">
						<CacheReplacementTotalOrder metric_field="ContentCreateTime" id_field="ContentOrigin" 
						tag_field="ContentType" tag_field_value="TotalOrder" />
					</CacheReplacement>
				</CacheReplacementPriority>
			</CacheStrategyReplacementPurger>
		</CacheStrategy>
	</DataManager>
</Haggle>


CacheStrategy:   

To address some of the limitations of unmodified Haggle, we have added
a generic module to the data manager, the cache strategy module, which
currently has cache replacement and cache purger sub-components. The
Cache replacement component is responsible for handling data that has
been marked with specific tags by the user, through use of attributes.

CacheStrategyReplacementPurger:

The above example cache strategy contains a single cache purger and a
single cache replacement.  Note that the cache purger in this example
is composed of multiple cache purgers, through the parallel
purger. Similarly, the cache replacement is composed of multiple cache
replacements, through the priority replacement.

CachePurgerParallel:   

The cache purger component operates in parallel (the purgers do not
conflict with each other).  Thus, if you have multiple purger policies
the data object can be acted upon by each of the policy.  This is
handy if you wish to have multiple methods of handling the same data
object.  Parallel policies purge when at least one of parallel
policies dictates to purge.  Purging can be triggered for multiple
reasons such as creation time, expiration time, or locality of content
(e.g., travel distance from a creator in traditional hop
values). Since expiration time and locality are independent measures,
purging conditions can be checked in parallel for the same data
object.  The sample configuration specifies purging mechanism with
parallel policies based on absolute and relative expiration times.

CachePurger:

This attribute is responsible for specifying the purger module to load.

Currently, we define two types of purgers:

CachePurgerAbsTTL:

Purging occurs based on an absolute expiration time that is specified
as a predefined time (e.g., Jan 30, 2013 at 5:15pm).  The value
specified is the unix time value, i.e. the epoch time since January
1st, 1970 (in seconds).
    
CachePurgerRelTTL:

Purging occurs based on a relative expiration time that is specified
as a minimum time to live (in seconds) from the reception (at the node
executing the strategy), which allows data objects reasonably
sufficient time to propagate yet aims to control the amount of cached
content, e.g., to reduce access times for incoming data objects.

The parameters are as follows:

	purge_type - specifies the type of purging mechanism 
	tag_field/ - specifies which tag that this specific purger module is responsible for (ContentType)
	tag_field_value - identifies which tag value that this specific purger module is responsible for (DelByAbsTTL, DelByRelTTL)
	keep_in_bloomfilter - specifies whether the data object should be retained in the bloomfilter after being purged 
	min_db_time_seconds - specifies the minimum time to keep it in the database before being purged

CacheReplacementPriority:

We have added a priority replacement component which can be composed
of multiple total order replacement components.  This functionality
allows a user to customize the config file to specify complex
replacement rules, such as lexicographical ordering.

When a data object is received, it uses priority to decide which
algorithm may act up on the data object.  If multiple cache
replacement orderings are applicable to a data object, the priority
policies can be used to define a lexicographical
ordering. Replacement ordering with the highest priority first
examines the first element of the attributes.  Subsequently,
replacement ordering with the n-th highest priority examines the n-th
element of the attributes.

Let us take an example with two attributes, in (X, Y) format, where
the first attribute X is handled by replacement order O1, and Y is
handled by replacement order O2 in case X cannot resolve the ordering
between two data objects: Suppose a data object with attributes of (A,
C) is already cached in the Haggle.  Now, a new object is inserted to
the system with attributes (C, D). The following lexicographical rules
are applied:

First, we compare A to C by applying O1.  

  a.  If A > C, drop the object with (C, D)

  b.  Else if A < C, delete the object with (A, B), insert the object with (C, D)  

  c.  Otherwise, we compare B to D by applying O2. 
     i.   If B > D, we drop the object with (C, D)
     ii.  Else if B < D, we delete the object with (A, B), insert the object with (C, D)
     iii. Otherwise, we insert the object with (C, D).  

  At this point, we have 2 objects with (A, B) and with (C, D).

In case only one replacement ordering can be applied, i.e., a data
object with a single corresponding attribute, only the corresponding
ordering is applied. Suppose a data object with attributes of (A) is
already cached, and a new object is inserted to the system with an
attribute (C).

  a.  If A > C, drop the object with (C)

  b.  Else if A < C, delete the object with (A), insert the object with (C).  

  c.  Otherwise, insert the object with (C), which leads to 2 objects in cache.

Note that the rules for multiple cache replacement orderings are
applied per exact matches.  Suppose Haggle already caches a data
object with (A1, B), and a new object is inserted with (A2).  In this
case, you will have both data objects in the cache (regardless of the
comparison between A1 and A2) since the ordering cannot be
applied. With the same reason, the orderings O1, O2, O3 that apply to
data objects with attributes (X, Y, Z) respectively will not be
applied to data objects with different set of attributes.  A data
object that is applicable to O1 and O3 will only be compared to other
data objects that have the attributes (X, Z) without having (Y).

CacheReplacement:

This attribute is responsible for specifying the replacement module to load.  
Currently we only support "CacheReplacementTotalOrder."

CacheReplacementTotalOrder:

This attribute loads total order replacement, and specifies what content
qualifies for total order replacement (using attributes of the content).
Only the most recent version of a data object that qualifies for total order
replacement is stored in the cache, all other content is discarded.

The parameters are as follows:

	metric_field - the name of the attribute whose value is interpreted as an
                       integer such that greater integers are fresher and smaller integers are
                       staler. Stale content is discarded. ("ContentCreateTime" for this excerpt)

	id_field - the name of the attribute whose value is used to determine a 
                   data object. Only the freshest content is maintained for a data object
                   with a specific id.  For example, if two data objects have different ids then
                   they do not influence each other's caching policy. If two data objects
                   have the same id (where the id is interpreted by the attribute with the name
                   matching the specified id field, "ContentOriginator" for this excerpt) 
                   and is subject to total order replacement, then only the most recent version 
                   (freshest) is stored.

	tag_field - only content that has the attribute specified here is subject to be replaced. 
                    For this excerpt, only data has the attribute with
                    name "ContentType" will use total order replacement. 

	tag_field_value - identifies which tag value that this specific replacement module is responsible for. 
                          ("TotalOrder" for this excerpt) 

Testing
=======

Please look at the following directories in the cbmen-encoders-eval
repository for tests:

    tests/CacheReplacement

In addition, test apps are present in haggle/serc/repltest

Sample test cases:

README-unscripted explains basic use-cases.
README-time-expire-test contains instructions to exercise scripts for parallel purging test. 
README-replacement-test contains instructions to exercise scripts for lexicographical replacement test.
README-replacement-time-expire-test contains instructions to exercise scripts for combined test.
