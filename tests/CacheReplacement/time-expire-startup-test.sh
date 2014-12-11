#!/bin/bash

echo ""
echo "Purging When Haggle Startup based on relative and absolute expiration times"
echo "---------------------------------------------------------------------------"
echo ""

#how many object for each timeout (x objects)
numDOPerExpire=1
expPubPkts=$[numDOPerExpire*4]

expSubPkts1=$[numDOPerExpire*2]
expSubPkts2=0
if [ -f haggletest ]; then 
   program="./haggletest "
elif [ -f ../haggletest/haggletest ]; then 
   program="../haggletest/haggletest"
else  #trust PATH to find it
   program="haggletest"
fi


#option definitons
pubAbsPub="pub ContentType=DelByAbsTTL purge_by_timestamp="
pubRelPub="pub ContentType=DelByRelTTL purge_after_seconds="

pubOptions1="-c -s 0 -b $numDOPerExpire -e logfile1"
pubOptions2="-c -s 0 -b $numDOPerExpire -e logfile2"
pubOptions3="-c -s 0 -b $numDOPerExpire -e logfile3"
pubOptions4="-c -s 0 -b $numDOPerExpire -e logfile4"

#make sure we are not reading older files
rm -f logfile1 logfile2 logfile3 logfile4 loginput1 loginput2 loginput3 loginput4

#time definitions, 1=first expire, 2=last to expire
time1=15
time2=110

#get now for AbsTTL testing

NOW=$(date '+%s')
future15s=$((NOW+time1))
future70s=$((NOW+time2))

#now start testing
echo "To enalbe the feature ContentType=DelByRelTTL (or ContentType=DelByAbsTTL) should be set." 
echo "Publish DOs with purge_after_seconds= (or purge_by_timestamp=)."
echo "" 
#send out test object 
cmd1="$program $pubOptions1 $pubRelPub""$time1"
cmd2="$program $pubOptions2 $pubRelPub""$time2"
cmd3="$program $pubOptions3 $pubAbsPub""$future15s"
cmd4="$program $pubOptions4 $pubAbsPub""$future70s"

echo $cmd1
echo $cmd2
echo $cmd3
echo $cmd4

one=`$cmd1`
two=`$cmd2`
three=`$cmd3`
four=`$cmd4`

#quit server, wait, restart it
echo ""
echo "Stop the Haggle daemon and wait for about 15 seconds to pass the expiration times of some DOs." 
echo "" 
#$program -x -q nop
#sleep $time1
echo "Next, start the Haggle daemon, which will purge expired DOs."
echo ""
echo "Continute with the \"bash time-expire-start-test-b.sh\", which will purge the remaining DOs according to their expiration times."
echo ""
exit 0
