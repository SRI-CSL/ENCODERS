This directory contains scripts to benchmark the SQLDataStore and the
MemoryDataStore and generate graphs with the results. 
This benchmark also can serve as a test, since we compare the output
of the SQLDataStore against that of the MemoryDataStore to confirm
that the sizes of the result sets of all the queries are identical. 

The test takes 5 parameters:

NODE_ATTR : 
the number of interests that each node has (drawn from the attribute pool)

DOBJ_ATTR : 
the number of attributes that each data object has (draw from the attribute pool)

ATTR_POOL : 
the number of attributes in the attribute pool

DOBJS : 
the number of data objects inserted into the database

NODES :
the number of nodes inserted into the database

The benchmark manager undergoes several stages to stress the database, and
collects metrics in a log file, for the details please see BenchmarkManager.h

The benchmark manager will first insert $NODES nodes each wtih $NODE_ATTR
interests drawn from $ATTR_POOL. 
Then the benchmark manager will insert $DOBJS data objects, each with
$DOBJ_ATTR attributes into the database. 
After these insertions, each node is queried for matching data objects.
After all of the queries, each node is queried again and this time the
matching data objects are deleted, and the node is deleted. 

These parameters are set at the top of the "./run_tests.sh" file.

The configuration that is used for the tests can be found in "./config.xml"

Note that Haggle must be compiled with the benchmark option enabled:

cd ~/cbmen/haggle 
make clean
./autogen.sh
./configure --enable-benchmark
make 
sudo make install

GNUplot should be installed:

sudo apt-get install gnuplot

To run the tests execute:
./run_tests.sh

