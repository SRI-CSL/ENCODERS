#!/bin/bash

echo ""
echo "Purging based on relative and absolute expiration times"
echo "-------------------------------------------------------"
echo ""

if [ -f haggletest ]; then 
   program="./haggletest "
elif [ -f ../haggletest/haggletest ]; then 
   program="../haggletest/haggletest"
else  #trust PATH to find it
   program="haggletest"
fi

#define pass/fail functions
#pass, $1 is logfile, $2 is expected number of returned hits
 pass_test() {
        res=`grep received: $1`
        indx=`expr index "$res" :`
        retval=${res:$indx}
     retval=${res:$indx}
     if [ "$retval" != "$2" ]; then
         echo "Pass failed to returned objects for $2 != $retval"
         exit 2
     fi
}

#fail test always return 0 hits
#$1 is logfilename
 fail_test() {
     res=`grep received: $1`
     indx=`expr index "$res" :`
     retval=${res:$indx}
     if [ "$retval" != "0" ]; then
         echo "Fail returned objects "
         exit 2
     fi
}

#option definitons
pubAbs1Pub="pub ContentType=DelByAbsTTL DataType=Abs name=Abs1 purge_by_timestamp="
pubRel1Pub="pub ContentType=DelByRelTTL DataType=Rel name=Rel1 purge_after_seconds="
pubAbs2Pub="pub ContentType=DelByAbsTTL DataType=Abs name=Abs2 purge_by_timestamp="
pubRel2Pub="pub ContentType=DelByRelTTL DataType=Rel name=Rel2 purge_after_seconds="
subAbs1Pub="abs1 sub DataType=Abs"
subAbs2Pub="abs2 sub name=Abs2"
subRel1Pub="rel1 sub DataType=Rel"
subRel2Pub="rel2 sub name=Rel2"

subRel2None="rel3 sub name=Rel2"
subAbs2None="abs3 sub name=Abs2"

pubOptions1="-c -s 0 -e logfile1"
pubOptions2="-c -s 0 -e logfile2"
pubOptions3="-c -s 0 -e logfile3"
pubOptions4="-c -s 0 -e logfile4"

#make sure we are not reading older files
rm -f logfile1 logfile2 logfile3 logfile4 loginput1 loginput2 loginput3 loginput4

subOptions1="-s 7 -e loginput1"
subOptions2="-s 7 -e loginput2"
subOptions3="-s 7 -e loginput3"
subOptions4="-s 7 -e loginput4"

#time definitions, 1=first expire, 2=last to expire
time1=15
time2=120

#get now for AbsTTL testing

NOW=$(date '+%s')

futureTime1=$((NOW+time1))
futureTime2=$((NOW+time2))

#now start testing
nop=`$program -c nop`

echo "To enable the feature ContentType=DelByRelTTL (or ContentType=DelByAbsTTL) should be set." 
echo "Publish DOs with purge_after_seconds= (or purge_by_timestamp=)."
echo "" 
cmd1="$program $pubOptions1 $pubRel1Pub""$time1"
cmd2="$program $pubOptions2 $pubRel2Pub""$time2"
cmd3="$program $pubOptions3 $pubAbs1Pub""$futureTime1"
cmd4="$program $pubOptions4 $pubAbs2Pub""$futureTime2"

echo $cmd1
echo $cmd2
echo $cmd3
echo $cmd4

one=`$cmd1`
three=`$cmd3`
two=`$cmd2`
four=`$cmd4`

echo ""
echo "Wait long enough for the half of DOs to be expired."
echo ""
sleep $time1
sleep 20 

cmd1="$program $subOptions1 $subRel1Pub"
cmd2="$program $subOptions2 $subAbs1Pub"
cmd3="$program $subOptions3 $subRel2Pub"
cmd4="$program $subOptions4 $subAbs2Pub"

echo ""
echo "Now, the following subs should return 2 match due to expiration."
echo ""

one=`$cmd1`
two=`$cmd2`
three=`$cmd3`
four=`$cmd4`

echo "$cmd1"
pass_test loginput1 1 
echo "$cmd2"
pass_test loginput2 1 
echo "$cmd3"
pass_test loginput3 1
echo "$cmd4"
pass_test loginput4 1

echo ""
echo "Wait long enough for remaining DOs to be expired."
sleep $time2  
sleep 2 
echo ""
echo "Now, the following subs should return no match."
echo ""

cmd1="$program $subOptions1 $subRel2None"
cmd2="$program $subOptions2 $subAbs2None"
echo "$cmd1"
echo "$cmd2"

one=`$cmd1`
two=`$cmd2`

fail_test loginput1
fail_test loginput2

echo ""
echo "-------------------------------------------------------"
echo "PASS!"
echo ""

exit 0

