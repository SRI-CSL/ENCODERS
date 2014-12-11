#!/bin/bash


OUTPUT_FILE=$1
OUTPUT_DIR=$( dirname $1 )

#counts, to make sure we have each category done
zero=0
one=0
two=0
three=0
four=0
five=0

cd $OUTPUT_DIR
dolist=$(grep NHOTHERSOCIAL n6.haggle.log | grep _purgeCache | awk -F: '{print $5}' | awk -F[ '{print $1;}' | sort | uniq )

for i in $dolist; do
    value=$(grep NHOTHERSOCIAL n6.haggle.log | grep _purgeCache | grep $i | tail -n 1 | awk -F'>' '{ print $2}')
    count=$(grep "Received," $OUTPUT_FILE |  grep $i | wc -l)
    n1=$(grep "Received," $OUTPUT_FILE |  grep $i | grep n1 | awk -F, '{ print $9 }' | sort | uniq | wc -l)
    count=$(($count - 1 + $n1))
    
    echo "$i = $count count => [$value] value"
    if [ "$count" -eq "0" ] && [ "$value" != " 0.000000" ]; then
       echo "Incorrect value!  FAIL!"
       exit 1
    else zero=1
    fi

    if [ "$count" -eq "1" ] && [ "$value" != " 0.200000" ]; then
       echo "Incorrect value!  FAIL!"
       exit 1
    else one=1
    fi

    if [ "$count" -eq "2" ] && [ "$value" != " 0.400000" ]; then
       echo "Incorrect value!  FAIL!"
       exit 1
    else two=1
    fi

    if [ "$count" -eq "3" ] && [ "$value" != " 0.600000" ]; then
       echo "Incorrect value!  FAIL!"
       exit 1
    else three=1
    fi

    if [ "$count" -eq "4" ] && [ "$value" != " 0.800000" ]; then
       echo "Incorrect value!  FAIL!"
       exit 1
    else four=1
    fi

    if [ "$count" -eq "5" ] && [ "$value" != " 1.000000" ]; then
       echo "Incorrect value!  FAIL!"
       exit 1
    else five=1
    fi

    if [ "$count" -gt "5" ]; then
       echo "Incorrect value!  FAIL!"
       exit 1
    fi

done

#now make sure all 6 quantities were found
if [ "$one" -eq "1" ] && [ "$two" -eq "1" ] && [ "$three" -eq "1" ] && [ "$four" -eq "1" ] && [ "$five" -eq "1" ] && [ "$zero" -eq "1" ]; then 
   echo "Test PASSED with 100% Social group recognition!"
else 
   echo "Test Failed, wrong social group counts!"
   exit 1
fi
exit 0
