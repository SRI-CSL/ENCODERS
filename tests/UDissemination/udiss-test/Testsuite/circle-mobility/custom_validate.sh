#!/bin/bash

OUTPUT_FILE=$1
OUTPUT_DIR=$( dirname $1 )

returnvalue=0

cd $OUTPUT_DIR

grep "<Node " n1.haggle.log | grep "name=\"n2\"" | sort | uniq > n1.results
grep "<Node " n2.haggle.log | grep "name=\"n1\"" | sort | uniq > n2.results

n1count=`cat n1.results | wc -l `
n2count=`cat n2.results | wc -l `

#should be, at least, 2 changes
if [ "$n1count"  -lt "2" ]; then
   echo "Not enough state changes in n1, need 2+, found $n1count" 
  returnvalue=1
fi

if [ "$n2count"  -lt "2" ]; then
   echo "Not enough state changes in n2, need 2+, found $n1count"
  returnvalue=1
fi

#check if parameters are within range,
#must be at least one zero value, and only one value can be outside the +-30% range
#one or more values must be within expected range
#avg_bytes_per_sec is approx 700KB/s, and connect time is approx 19000ms
#change these values as necessary, dependant on hardware
exp_avgbps="700000"
exp_avgconnect="19000"

minbps=`echo "$exp_avgbps * 0.7" | bc -l | sed 's/[.].*//'`
maxbps=`echo "$exp_avgbps * 1.3" | bc -l | sed 's/[.].*//'`
mincon=`echo "$exp_avgconnect * 0.7" | bc -l | sed 's/[.].*//'`
maxcon=`echo "$exp_avgconnect * 1.3" | bc -l | sed 's/[.].*//'`

echo "Expected results should range from $minbps to $maxbps Bps" 
echo "and expected connect time of $mincon to $maxcon ms"

shopt -s nullglob
IFS=$'\n'
for f in n1.results n2.results
do 
  bpszerocount=0;
  connzerocount=0;
  bpsnotinrange=0;
  conninrange=0;
  bpsnotinrange=0;
  connnotinrange=0;
  file=`cat $f`
  for i in $file; do
	avgbps=$(echo $i | awk -F\" '{ print $2}' )
	avgconnect=$(echo $i | awk -F\" '{ print $4}' )
        echo "evaluation result of $avgbps b/s and $avgconnect time for $f"
  	if [ "$avgbps" -eq "0" ]; then 
           bpszerocount=$((bpszerocount+1))
	elif [ "$avgbps" -gt "$minbps" ]  && [ "$avgbps" -lt "$maxbps" ]; then
	   	bpsinrange=$((bpsinrange+1))
        else 
            	bpsnotinrange=$((bpsnotinrange+1))
	fi

  	if [ "$avgconnect" -eq "0" ]; then 
           connzerocount=$((bpszerocount+1))
	elif [ "$avgconnect" -gt "$mincon" ]  && [ "$avgconnect" -lt "$maxcon" ]; then
	   	conninrange=$((conninrange+1))
        else 
            	connnotinrange=$((connnotinrange+1))
	fi

  done
  if [ "$bpszerocount" -eq "0" ]; then
     	echo "$f: bits/sec count is not initially zero!"
  returnvalue=1
  fi
  if [ "$bpsnotinrange" -gt "1" ]; then
	echo "$f: More than 1 ($bpsnotinrange) bits/sec reading out of expected range!"
  returnvalue=1
  fi 
  if [ "$conninrange" -eq "0" ]; then
     	echo "$f: bits/sec count is never in range!"
  returnvalue=1
  fi

  if [ "$connzerocount" -eq "0" ]; then
     	echo "$f: connect time count is not initially zero!"
  returnvalue=1
  fi
  if [ "$connnotinrange" -gt "1" ]; then
	echo "$f: More than 1 ($connnotinrange) connect time reading out of expected range!"
  returnvalue=1
  fi 
  if [ "$conninrange" -eq "0" ]; then
     	echo "$f: connect time count is never in range!"
  returnvalue=1
  fi

done

#verify DO's were replicated
count=`grep 104448 n1.haggle.db.hash_sizes.log | wc -l`
count2=`grep 102400 n2.haggle.db.hash_sizes.log | wc -l`

if [ "$count" -lt 5 ]; then
   echo "Only $count DO's were replicated from n2"
  returnvalue=1
fi

if [ "$count2" -lt 5 ]; then
   echo "Only $count2 DO's were replicated from n1"
  returnvalue=1
fi

echo "Node n1 replicated $count2 DO's, and node n2 replicated $count DO's."
exit $returnvalue
 
