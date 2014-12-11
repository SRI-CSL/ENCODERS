#!/bin/bash


OUTPUT_FILE=$1
OUTPUT_DIR=$( dirname $1 )

acount=$(grep "Received," ${OUTPUT_FILE} | grep A | wc -l)
bcount=$(grep "Received," ${OUTPUT_FILE} | grep B | wc -l)
ccount=$(grep "Received," ${OUTPUT_FILE} | grep C | wc -l)

if [ "${acount}" != "45" ]; then
   echo "Only ${acount}/45 squad=A members found!" 
   exit 1
fi

if [ "${bcount}" != "45" ]; then
   echo "Only ${bcount}/45 squad=B members found!" 
   exit 1
fi

if [ "${ccount}" != "45" ]; then
   echo "Only ${ccount}/45 squad=C members found!" 
   exit 1
fi

#Verify each squad has, at least, the correct content in their caches
grep "Published," ${OUTPUT_FILE} | grep A | awk -F, '{print $2}' > /tmp/pubA
grep "Published," ${OUTPUT_FILE} | grep B | awk -F, '{print $2}' > /tmp/pubB
grep "Published," ${OUTPUT_FILE} | grep C | awk -F, '{print $2}' > /tmp/pubC

pushd $OUTPUT_DIR
cd $OUTPUT_DIR

files=`ls n[1-3].*hash_sizes.log`
for i in $( cat /tmp/pubA )
do
     count=$(grep $i $files | wc -l )
     if [ "${count}" != 3 ]; then
          echo "Only $count instances of $i (squad A)"
          popd
          exit 1
     fi
done

files=`ls n[4-6].*hash_sizes.log`
for i in $(cat /tmp/pubB); do
     count=$(grep $i $files | wc -l )
     if [ "${count}" != 3 ]; then
          echo "Only $count instances of $i (squad B)"
          popd
          exit 1
     fi
done

files=`ls n[7-9].*hash_sizes.log`
for i in $(cat /tmp/pubC); do
     count=$(grep $i $files | wc -l )
     if [ "${count}" != 3 ]; then
          echo "Only $count instances of $i (squad C)"
	  popd
          exit 1
     fi
done

popd
echo "Test passed with 100% reception rate, and 100% social caching per node (each group)"
exit 0
