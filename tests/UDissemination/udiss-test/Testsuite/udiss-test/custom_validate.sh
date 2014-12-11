#!/bin/bash


OUTPUT_FILE=$1
OUTPUT_DIR=$( dirname $1 )

#counts, to make sure we have each category done
pushd $OUTPUT_DIR
cd $OUTPUT_DIR

count=`cat n1.haggle.db.hash_sizes.log | wc -l`
if [ "$count" != "6" ]; then
    echo "FAIL! n1 Expected 6, found $count DO's"
    exit 1
fi
count=`cat n2.haggle.db.hash_sizes.log | wc -l`
if [ "$count" != "5" ]; then
    echo "FAIL! n2 Expected 5, found $count DO's"
    exit 1
fi
#sort largest to smallest, make sure they come in order
cat n2.haggle.db.hash_sizes.log | awk '{print $2 " " $1 }' > n2.rev.order
sort -nr n2.rev.order > n2.big.small

prior_time=0.0;

do_list=(`cat n2.big.small | awk '{print $2}'`)

#now check times
for i in "${do_list[@]}"
do
   time=`grep "Protocol::receiveDataObject" n2.haggle.log | grep "$i" | head -1 | awk -F: '{print $1}'`
   compare=`echo $time'<'$prior_time | bc -l`
   if [[ $compare -gt 0 ]]; then
      echo "$i arrived too late, want $prior_time < $time"
      exit 1
   fi
   prior_time=$time;
done

exit 0
