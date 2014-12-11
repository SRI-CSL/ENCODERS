#!/bin/bash

echo ""
echo "Purging When Haggle Startup based on relative and absolute expiration times"
echo "---------------------------------------------------------------------------"
echo ""
sleep 1

#how many objects for each timeout (x objects)
numDOPerExpire=1
expPubDOs=$[numDOPerExpire*4]

expSubDOs1=$[numDOPerExpire*2]
expSubDOs2=0
if [ -f haggletest ]; then 
   program="./haggletest "
elif [ -f ../haggletest/haggletest ]; then 
   program="../haggletest/haggletest"
else  #trust PATH to find it
   program="haggletest"
fi

#option definitons
subAbsPub="part2 sub ContentType=DelByAbsTTL "
subRelPub="part2 sub ContentType=DelByRelTTL "

subOptions1="-s 10 -e loginput1"
subOptions2="-s 10 -e loginput2"
subOptions3="-s 10 -e loginput3"
subOptions4="-s 10 -e loginput4"

#time definitions, 1=first expire, 2=last to expire
time1=15
time2=110

#wait for haggle to bootup

#wait x time for 1/2 to expire
cmd1="$program $subOptions1 $subRelPub"
cmd2="$program $subOptions2 $subAbsPub"

#commands for the 2nd half of testing
#want to keep logs separate
cmd3="$program $subOptions3 $subRelPub"
cmd4="$program $subOptions4 $subAbsPub"

echo "The following subs would return 1 match each. NOP clears out the existing subs." 
echo ""
echo $program "-c nop"
echo $cmd1
echo $program "-c nop"
echo $cmd2  
echo $program "-c nop"

nop=`$program -c nop`
one=`$cmd1`
nop=`$program -c nop`
two=`$cmd2`
nop=`$program -c nop`

res1=`grep received: loginput1`
res2=`grep received: loginput2`
#sum up sent objects, does it match?
indx=`expr index "$res1" :`
sum=${res1:$indx}

indx=`expr index "$res2" :`
add=${res2:$indx}
sum=$[ $sum+$add ]

if [ "$expSubDOs1" != "$sum" ]; then
   echo ""
   echo "---------------------------------------------------------------------------"
   echo "FAIL! expected $expSubDOs1, matched $sum."
   echo ""
   exit 2
fi

echo ""
echo "Wait long enough for remaining DOs to be expired."
echo ""
sleep $[ $time2-$time1 ]
sleep 1

echo "The following subs should return no match. NOP clears out the existing subs." 
echo ""
echo $program "-c nop"
echo $cmd3
echo $program "-c nop"
echo $cmd4

nop=`$program -c nop`
one=`$cmd3`
nop=`$program -c nop`
two=`$cmd4`

res1=`grep receive loginput3`
res2=`grep receive loginput4`
#sum up sent objects, does it match?
indx=`expr index "$res1" :`
sum=${res1:$indx}

indx=`expr index "$res2" :`
add=${res2:$indx}
sum=$[ $sum+$add ]

if [ "$expSubDOs2" != "$sum" ]; then
   echo ""
   echo "---------------------------------------------------------------------------"
   echo "FAIL! expected $expSubDOs2, matched $sum."
   echo ""
   exit 3
fi

echo ""
echo "---------------------------------------------------------------------------"
echo "PASS!"
echo ""

exit 0
