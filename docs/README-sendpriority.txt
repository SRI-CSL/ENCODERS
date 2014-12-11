= Send Priority
Sam Wood <sam@suns-tech.com>
v1.0, 2013-10

This document describes the Send Priority Manager extension for prioritized
data dissemination. 

NOTE: This document was written in 'asciidoc' format and can generate html
or latex output. Please install `source-highlight` for syntax highlighting.

== Overview

The purpose of the send priority manager is to maintain an ordering on which
data objects are sent. This ordering is formulated as a partial order,
specified in the configuration file. 
A partial order is a binary relation that is 
1) reflexive (a lte  a), 
2) anti-symmetric (if a lte b and b lte a, then a = b;
3) transitive (if lte b and b lte c, then a lte c .
Partial orders can be composed of other partial orders, and different types 
of partial orders can be combined to define flexible prioritization policies. 
Through these policies, the network operator can ensure that high priority 
content is delivered before low priority content.

== Implementation 

This mechanism is important when links have limited bandwidth and are 
connected for small durations--the most important data should be sent
first. This manager intercepts data objects before they are placed on the
protocol queue, and it maintains its own queues which are drained into the
protocol queue. 
When sending a data object, a manager will traditionally raise the 
DATAOBJECT_SEND event. Instead, this manager (e.g., ForwardingManager) 
sends the EVENT_TYPE_SEND_PRIORITY which is received by the PriorityManager. 
The received data object is placed on a queue that constructs a topological
order of set of queued data objects according to the partial order 
specification. 
The manager maintains a number of send stations which are data objects that
are presently being sent by a protocol. If all of the send stations are in 
use, then the data object is queued--otherwise, it is sent immediately. 
If a data object is sent successfully, or fails to send, then the Protocol
will raise the SEND_SUCCESS/SEND_FAILURE event, as well as the new event
SEND_PRIORITY_SUCCESS/SEND_PRIORITY_FAILURE to indicate to the manager
that the data object can be removed from the queue, and a new data object
can be added. 
The send stations are maintained for each node destination.

To construct a topological order given the partial order, we implemented
a heap whose elements higher in the heap must be sent before elements
lower in the heap. This is effectively a Hesse Diagram or Graph.
Upon element insertion (at the head of the queue), it is pushed down 
(placed at the greatest depth) as far as possible, while respecting the 
partial order specification.

This algorithm has O(n) insertion, and O(1) deletion.  
The partial orders are implemented as a class which derives from PartialOrder 
and implements a compare function, which returns an integer representing
either <, >, = or | (incomparable). 

=== Relevant Files


The implementation can be found:
----
~/cbmen-encoders/src/hagglekernel/SendPriorityManager.{cpp,h}
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
~/cbmen-encoders/haggle/doc/api/hagglekernel/html/SendPriorityManager*
----

== Configuration

For a detailed description of each configuration parameter, please see

----
~/cbmen-encoders/src/hagglekernel/SendPriorityManager.h
----

An example Send Priority configuration is:

[source,xml]
----
<SendPriorityManager 
  enable="true" 
  run_self_tests="false" 
  partial_order_class="PartialOrderCombinerOrdered">
    <PartialOrderCombinerOrdered 
      high_class_name="PartialOrderAttribute" 
      high_class_param_name="PartialOrderAttribute1" 
      low_class_name="PartialOrderCombinerOrdered" 
      low_class_param_name="PartialOrderCombinerOrdered1"> 
        <PartialOrderAttribute1 
          attribute_name="bolo" />
        <PartialOrderCombinerOrdered1 
          high_class_name="PartialOrderPriorityAttribute" 
          high_class_param_name="PartialOrderPriorityAttribute1"
          low_class_name="PartialOrderFIFO" 
          low_class_param_name="PartialOrderFIFO1" >
            <PartialOrderPriorityAttribute1 
              priority_attribute_name="priority" />
            <PartialOrderFIFO1 />
        </PartialOrderCombinerOrdered1>
    </PartialOrderCombinerOrdered>
</SendPriorityManager>
----

Here, a total order is defined by combining multiple partial orders. The order
is as follows:

 . PartialOrderAttribute with attribute 'bolo'
 . PartialOrderPriorityAttribute with attribute 'priority'
 . PartialOrderFIFO

For example, consider the following 5 data objects that are queued in the
following order to be sent to a neighbor:

 * D1: { "bolo" : "", "priority" : 1 }
 * D2: { "bolo" : "", "priority" : 2 }
 * D3: { "bolo" : "" }
 * D4: { "priority" : 1 }
 * D5: { "priority" : 2 }

Based on this partial order, they will be ordered to send as follows:

 . D2
 . D1
 . D3
 . D5
 . D4

Note that D3 is prioritized before D5 and D4 due to the PartialOrderFIFO 
attribute.

The following `SendPriorityManager` configuration options are supported:

`enable`::
    Set to `true` to enable the `SendPriorityManager`, `false` otherwise.

`debug``::
    Set to `true` to enable verbose debug messages, `false` otherwise.

`parallel_factor`::
    The number of concurrent data objects to be queued for a particular receiver. 

`run_self_tests`::
    Set to `true` to run unit tests on manager initialization, `false` otherwise.

`partial_order_class`::
    The class name of the partial order class to load. The configuration options
for the class are loaded by looking for a tag with the same name within the
`SendPriorityManager` tag.

The following `PartialOrder` classes are supported and their options are supported:

  . *PartialOrderAttribute* : data objects with the specified attribute have
priority (>) over data objects without the attribute. Data objects either
both with the attribute, or without the attribute are incomparable (|). 

`attribute_name`::
    Set to the attribute name which gives priority to data objects with that
attribute over data objects without. 

  . *PartialOrderPriorityAttribute* : uses the integer value of attributes with a 
specific name to order said data objects--higher values have greater priority
than lower values. If one (or both) of the data objects do not have the
specified attribute, then they are incomparable (|). One could configure
this partial order to respect the node's role: for example, a UAV
may have a configuration setting that gives high priority to squad leader
content to improve squad-leader communication, whereas within a squad
blue force tracking might have higher priority for squad members.  

`priority_attribute_name`::
    Set to the attribute name whose value is interpreted as an integer for
prioritization.

  . *PartialOrderFIFO* : totally orders the data objects by the queue insertion 
date, where data objects that were inserted earlier have priority over data
objects that are inserted later.

  . *PartialOrderCombinerOrdered* : Combines two `PartialOrder`'s to
construct a new `PartialOrder`, where one partial order is consulted before
the second partial order when ordering two data objects. When two data objects
are compared, if the first partial order (high priority partial order) has a result 
other than |, then that result is returned. Otherwise, the result of the 
second partial order (low priority partial order) is returned.

`high_class_name`:: The high priority partial order class name.

`high_class_param_name`:: The name of the sub-tag which contains the parameters
to initialize the high priority partial order.

`low_class_name`:: The low priority partial order class name.

`low_class_param_name`:: The name of the sub-tag which contains the parameters
to initialize the low priority partial order.

== Evaluation 

=== Eval1

To evaluate the performance and the overhead of the implementation, we first
constructed a two node experiment with a single publisher and single subscriber
(eval1). In this 100 second experiment, the bandwidth is 11Mbps and the publisher 
publishes 3 different classes of traffic: 

 . High priority (1000KB every 1.2 seconds)
 . Medium priority (1001KB every 1 seconds)
 . Low priority (1002KB every 0.8 seconds)

The partial order specification respects this priority.  We report the data objects 
delivered, and the delay distribution broken down by each class, as an average 
over 3 runs. 

[[img-eval1-topology]]
.Eval1 Topology
image::SendPriorityResults/eval1-topology.png[Eval1 Topology, 300, 200]

[[img-eval1-delivered]]
.Eval1 Data Objects Delivered
image::SendPriorityResults/eval1-delivered.png[Data Objects Delivered, 300, 200]

[[img-eval1-low-delay]]
.Eval1 Low Priority Delays
image::SendPriorityResults/eval1-delays_low.png[Delay Distribution for Low Priority Data Objects, 300, 200]

[[img-eval1-medium-delay]]
.Eval1 Medium Priority Delays
image::SendPriorityResults/eval1-delays_medium.png[Delay Distribution for Medium Priority Data Objects, 300, 200]

[[img-eval1-high-delay]]
.Eval1 High Priority Delays
image::SendPriorityResults/eval1-delays_high.png[Delay Distribution for High Priority Data Objects, 300, 200]

In this experiment, we see that for roughly the same data objects delivered
enabling the `SendPriorityManager` reduces the delivery delay for 
high priority data objects and delivers more than 2x high priority data objects, 
reduces the delivery delay for medium priority data objects and delivers roughly the
same amount, and does not deliver any low priority data objects. This simple
scenario demonstrates that the `SendPriorityManager` can utilize data object 
attributes as priority information to lead to more efficient bandwidth utilization. 

=== Eval2

We repeated this experiment but with 8 subscribers and 1 publisher (center node,
node 5), as seen in the below figure. The traffic patterns were the same in this
scenario as in the previous scenario. The key difference between this scenario and the
previous one is that here subscribers act as relays and transfer some data 
objects to other subscribers.

[[img-eval2-topology]]
.Eval2 Topology
image::SendPriorityResults/eval2-topology.png[Eval2 Topology, 300, 200]

The results for this experiment are seen below, and follow the same trends
as in the 2 node experiment:

[[img-eval2-delivered]]
.Eval2 Data Objects Delivered
image::SendPriorityResults/eval2-delivered.png[Data Objects Delivered, 300, 200]

[[img-eval2-low-delay]]
.Eval2 Low Priority Delays
image::SendPriorityResults/eval2-delays_low.png[Delay Distribution for Low Priority Data Objects, 300, 200]

[[img-eval2-medium-delay]]
.Eval2 Medium Priority Delays
image::SendPriorityResults/eval2-delays_medium.png[Delay Distribution for Medium Priority Data Objects, 300, 200]

[[img-eval2-high-delay]]
.Eval2 High Priority Delays
image::SendPriorityResults/eval2-delays_high.png[Delay Distribution for High Priority Data Objects, 300, 200]

For this scenario, we see that enabling the `SendPriorityManager` increases
the number of high priority and medium priority data objects that are
delivered, reduces their delays, and delivers the same number over data objects
overall in comparison to disabling the `SendPriorityManager`.


== Demo

An example configuration file is available here:

----
~/cbmen-encoders/haggle/resources/config-sendpriority.xml
----

Tests can be found at tests/SendPriority in the evaluation repository. 
