The mathematical semantics of Haggle query resolution is defined
in the following paper (see appendix) ...

A Search-based Network Architecture for Mobile Devices
Erik Nordstroem, Per Gunningberg and Christian Rohner
Uppsala University, Sweden.

In a nutshell, the relevance of a data object relative to a weighted
set of interests (e.g. a node description representing the query) is
computed by the weighted sum of overlapping attributes. Weights always
referring to weights of interest attributes, not attributes of data
objects (which are not weighted). The result of a query is the ranked
set of data objects up to a given matching threshold and up to a
certain maximum rank (limiting the number of results in the
query). Weights and thresholds in Haggle are represented as integers
in the range 0...100.

This is a fairly general and powerful concept that goes beyond
traditional disjunctive and conjunctive queries. Unfortunately, Haggle
does not implement this semantics consistently. For instance, the
attributes of node descriptions are aggregated from multiple
applications an additional internal attribute is added so that from
the viewpoint of the application the semantics looks quite is
different.  Another complication is that Haggle uses two different
variations of matching, one for applications (called filtering) and
one for routing (called matching). Our experience is that in Vanilla
Haggle only a disjunctive interpretation of interest (that is
consistent with aggregation by union) is of practical use.

In the semantics branch we have modified Haggle to consistently use
the above-mentioned mathematical semantics at the application-level,
which is what matters for end-users. In this version application nodes
become first-class citizen (as opposed to be approximately represented
by their proxy, the device node). Aggregation is disabled in favor of
accuracy.

This feature is called first-class applications, and can be enabled
by turning on the boolean first_class_applications config option of the
application manager, as in the following excerpt:

	<ApplicationManager first_class_applications="true">
        </ApplicationManager>

Turning the feature off (the default setting) should yield the Vanilla
Haggle semantics with aggregation, but it is not recommended, because
our routing algorithms can be expected to exploit the first-class
application semantics (especially the separation of application
interest and device Bloomfilters that comes with it) in the future.

It should be noted that the first-class application feature
nicely works together with our extended Haggle API functions

   haggle_ipc_set_matching_threshold and
   haggle_ipc_set_max_data_objects_in_match,

which allows applications to set the above-mentioned parameters
(threshold and maximal rank) for each application independently (which
was not possible in Vanilla Haggle). With these function it is
possible to support not only fully general threshold queries, but also
disjunctions of conjunctions (i.e. precise logical queries).

The default settings for the above mentioned parameters are
0 and 10, respectively, and can be configured in the config file, e.g. using

	<NodeManager>
		<Node matching_threshold="0" max_dataobjects_in_match="100"/>
	</NodeManager>

Vanilla Haggle supports the use of wildcards for matching in the local
data base, but not for remote searches. With the first-class
applications features there is a single semantics for both kinds of
matching, and wildcards are currently not supported.

Testing
=======

Please look at the following directories in the cbmen-encoders-eval
repository for tests:

    tests/SemanticsTest
