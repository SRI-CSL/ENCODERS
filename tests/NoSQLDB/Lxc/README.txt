This directory contains automated tests to exercise the in-memory NOSQL
database. 

UNIT TESTS:

To run the tests, execute:

./run_tests.sh

Test descriptions:

./Testsuite/001-unittests-N1D70s1-200x200-unittests/ : 

Runs the unit tests for MemoryCache.{cpp,h} on a single node. Inserts
various fake nodes and data objects and confirms the proper matching occurs. 

./Testsuite/002-canary-N2D70s1-400x400-noforward/ :

A simple 2 node experiment where one node publishes a 1 10KB file to the 
neighbor. This test uses in memory node descriptions. 

./Testsuite/003-canary-mem-basic-N2D70s1-400x400-noforward/ :

This test is identical to 002*, but does not use in memory node descriptions. 

./Testsuite/004-canary-N2D80s1-400x400-noforward/ :

This test exercises the botique caching utility functions (total order and
time to delete). It is a 2 node experiment where subscriber subscribes
at the end of the test, and the publisher publishes 4 data objects.
The first data object has a TTL to delete after 1 second, the second
data object is published with a TO ordering of 1, the third data object
has an order of 3, and the fourth data object has an order of 2. We confirm
that the subscriber only receves data object #3. 

EVALUATION TEST:

This test is a 4x4 grid with 1000 data objects published between random
source destination pairs. These pairs have a unique data object name
which the subscriber only 

To run the end-to-end evaluation test, execute:

./run_grid_test.sh

This will generate a directory of the form:

nosql_grid_results_*/

Which contains the latency results for the end-to-end evaluation.
