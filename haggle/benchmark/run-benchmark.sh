#!/bin/bash

trap exit SIGHUP SIGINT SIGTERM

BENCH_DIR=`dirname $0`
HAGGLE_DIR=$BENCH_DIR/../bin

#echo $BENCH_DIR
#echo $HAGGLE_DIR

# Benchmark paramaters in the same order as given on the haggle
# command line
DataObjects_Attr="10"
Nodes_Attr="100"
Attr_Num="1000"
DataObjects_Num="10 50 100 500 1000 5000 10000"
Test_Num=100

TRACES=

i=0
for aNo in $Nodes_Attr; do
for aDO in $DataObjects_Attr; do
	for nDO in $DataObjects_Num; do
	    for nAttr in $Attr_Num; do
			echo "Running $i nDO=$nDO numAttr=$nAttr"
			$HAGGLE_DIR/haggle -b $aDO $aNo $nAttr $nDO $Test_Num
			i=$[i+1]
    	done
	done
done
done

# Run analysis
#TRACES=`ls -1 --sort=time benchmark-*`
TRACES=`ls benchmark-*`
BENCH_LOG=benchmark.dat

$BENCH_DIR/haggle-benchmark-analysis.pl $TRACES > $BENCH_LOG

$BENCH_DIR/haggle-benchmark-plot.sh $BENCH_LOG
