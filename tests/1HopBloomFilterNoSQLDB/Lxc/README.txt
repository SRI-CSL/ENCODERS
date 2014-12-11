This directory contains automated tests to exercise the 1-hop bloomfilters.

== Unit Tests

To run the tests, execute:

./run_tests.sh

Test descriptions:

Testsuite/000-unittests-N1D70s1-200x200-unittests: basic unit tests which 
encode/decode application node descriptions from an interest data object.

Testsuite/001-canary-1hop-N3D70s1-400x400-noforward: a simple 3 node test
that confirms that device node descriptions do not propagate beyond 1 hop,
but application node descriptions do.

Testsuite/002-1hop-consume*: a simple 3 node tests that confirms that
the "consume_interest" parameter correctly removes the interest upon 
forwarding due to an interest match. 

Testsuite/003-1hop-multi-consume*: a 4 node test that confirms that
the "consume_interest" parameter correctly removes all of the matching
interests upon forwarding due to an interest match.

Testsuite/004-1hop-grid*: a 9 node test that confirms end-to-end
functionality of a publisher and subscriber on opposite corners
of the grid.

== Evaluation

To run the evaluation, execute:
./run_evaluation.sh
