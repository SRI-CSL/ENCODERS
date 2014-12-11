This directory contains automated tests to exercise the send priority feature.

== Unit Tests

To run the tests, execute:

./run_tests.sh

Test descriptions:

000-unittests-N1D70s1-200x200-unittests :

Runs the basic unit tests on a single node. The unit tests instantiate 
topological sorters with different partial utility functions (e.g., attribute
and FIFO), insert multiple elements, and pop them off to confirm that they
are returned in the proper order.

001-canary-sendpriority-N2D70s1-400x400-noforward :

A simple 2 node experiment with 1 publisher and 1 subscriber and a
FIFO queue partial order. Confirms that the data objects are sent and received
according to FIFO order.

002-sendpriority-attr-N2D130s1-400x400-noforward :

A simple 2 node experiment with 1 publisher and 1 subscriber and a priority
attribute partial order. Confirms that the data objects are sent and received
according to the attribute priority.

003-sendpriority-complex-N2D130s1-400x400-noforward

A simple 2 node experiment with 1 publisher and 1 subscriber and a partial
order that is the combination of attribute, FIFO, and priority attiribue.

== Evaluation

To run the evaluation, execute:

./run_eval.sh

Make sure that gnuplot is installed in order to generate the graphs.
Test output will be placed in subdirectories of this directory, e.g.:

send_priority_eval1_results_1387431926/*ps
send_priority_eval2_results_1387843353/*ps

To convert the postscript files to png:

convert -rotate 90 delivered.ps delivered.png

Evaluation descriptions:

eval1: A simple 2 node experiment with a single publisher and single subscriber
In this 100 second experiment, the bandwidth is 11Mbps and the publisher
publishes 3 different classes of traffic:

p <-> s

 . High priority (1000KB every 1.2 seconds)
 . Medium priority (1001KB every 1 seconds)
 . Low priority (1002KB every 0.8 seconds)

The partial order specification respects this priority.

eval2: The same as eval1/, but with 8 subscribers and 1 publisher in the following
topology, where every node can communicate w/ every other node, except the corners
cannot communicate with each other.

n1 n2 n3
n4 n5 n6
n7 n8 n9

Where n5 is the publisher.
