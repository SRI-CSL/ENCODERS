#!/bin/bash


echo "Starting Replacement-TimeExpire Testing"

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

rm -f log1

pubOptions=" -s 0 "
subOptions=" -s 10 -e log1 "
pubAttr="pub ContentType=TotalOrder ContentOrigin=suns-tech RoutingType=Flood DataType=amalgamation  ContentCreateTime="
pub2Attr=" ContentType=DelByRelTTL purge_after_seconds="

subAttr=" sub DataType=amalgamation"


cmd="$program $pubOptions  $pubAttr""10 " 
echo "$cmd"
execmd=`$cmd`

cmd="$program $subOptions step2 $subAttr"
echo "$cmd"
execmd=`$cmd`
pass_test log1 1
#<Should return a single value, ContentCreateTime=10>
echo

#3.   Send an object that wont replace, with a large lifespan.   It should not be inserted.
cmd="$program  $pubOptions $pubAttr""5 $pub2Attr""500"
echo "$cmd"
exe=`$cmd`

#<Should return a single value, ContentCreateTime=10; ContentCreateTime=5 should not be inserted>
cmd="$program $subOptions step3 $subAttr"
echo "$cmd"
exe=`$cmd`
pass_test log1 1

#verify it is the correct object
cmd="$program $subOptions step3b sub ContentCreateTime=10"
echo "$cmd"
exe=`$cmd`
pass_test log1 1
echo

#4.   Send an object that will replace, with a moderate lifespan.   It should replace, and be deleted after a time.
cmd="$program  $pubOptions  $pubAttr""50 $pub2Attr""45"
echo "$cmd"
exe=`$cmd`

cmd="$program $subOptions step4 $subAttr"
echo "$cmd"
exe=`$cmd`
pass_test log1 1
cmd="$program $subOptions step4b sub ContentCreateTime=50"
echo "$cmd"
exe=`$cmd`
pass_test log1 1
echo
#<Should return a single value, ContentCreateTime=50;>



#<after 45 seconds>
echo "wait over 45 seconds for content to expire"
sleep 55
cmd="$program $subOptions step4c $subAttr"
echo "$cmd"
exe=`$cmd`
fail_test log1
#<should return nothing, it expired>

#all passed
echo "PASS! Replacement-TimeExpire test successful!"

exit 0

