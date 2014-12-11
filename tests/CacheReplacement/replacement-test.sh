#!/bin/bash

#define pass/fail functions
#pass my return 1+ hits
pass_test() {
        res=`grep received: logfilep`
        indx=`expr index "$res" :`
        retval=${res:$indx}
     retval=${res:$indx}
     if [ "$retval" != "$1" ]; then
         echo "Pass failed to returned objects for $1 != $retval"
         exit 2
     fi
}

#fail test always return 0 hits
fail_test() {
     res=`grep receive logfilef`
     indx=`expr index "$res" :`
     retval=${res:$indx}
     if [ "$retval" != "0" ]; then
         echo "Fail returned objects "
         exit 2
     fi
}

if [ -f haggletest ]; then 
   program="./haggletest "
elif [ -f ../haggletest/haggletest ]; then 
   program="../haggletest/haggletest"
else  #trust PATH to find it
   program="haggletest"
fi
 
#clear out logs
rm -f logfilef logfilep

echo ""
echo "Replacement based on lexicographical ordering"
echo "---------------------------------------------"
echo ""
echo "To enable the feature ContentType=TotalOrder should be set." 
echo "Lexicographical ordering is based on MissionTimestamp and ContentCreateTime from the same ContentOrigin."  
echo "The test sequences (1..8) are defined in the script (t[1]..t[8])." 
echo ""

name="suns-tech"
pubLGPub="pub ContentType=TotalOrder ContentOrigin=$name"
firstOpt=" MissionTimestamp="
secondOpt=" ContentCreateTime="

#subs
subLGPass=" sub ContentType=TotalOrder ContentOrigin=$name"
subLGFail=" sub "

numPubDOs=1
pubOptions="-c -s 0 -b $numPubDOs "
subOptionsp="-s 16 -e logfilep "
subOptionsf="-s 16 -e logfilef "

#make sure we are not reading older files
rm -f logfile1 logfile2 logfile3 logfile4 loginput1 loginput2 loginput3 loginput4

#TestSeq MissionTimestamp:ContentCreateTime:ExpectSubResults
#e.g., test sequence 7 publishes a DO with MissionTimestamp=600 ContentCreateTime=15, and subscription will return two matches 
t[1]='500:10:Y' 	#initial object 
t[2]='490:5:N'  	#discarded, 490 < 500
t[3]='400:16:N' 	#discarded, 400 < 500
t[4]='500:15:Y' 	#replace; 500=500, 15>10
t[5]='500:5:N'  	#discarded, 500=500, 5<15
t[6]='600:15:Y' 	#replace; 600>500
t[7]='600:15:YY' 	#insert, have 2 600=600,15=15
t[8]='700:5:Y'  	#replace, 700>600

#now start testing
oldIFS=$IFS

matchstring=""
failstring=""
lastnum=""

#initial object to clear out system
nop=`$program -c -s 1 -z nop`

for testid in "${!t[@]}"
do
echo "--$testid--"
   nameprogp="rxp""$testid"
   nameprogf="rxf""$testid"
   ans="${t[testid]}"
   IFS=":"
   arr=($ans)
   IFS=$oldIFS

   #create pub 
   pub="$program $pubOptions $pubLGPub $firstOpt""${arr[0]}"" $secondOpt""${arr[1]}"
   result=${arr[2]} 
   echo "$pub"
   op=`$pub`
   #now figure out which object should be in db
   #count # of Y's
   numYes=`echo $result | grep -o 'Y' | wc -w`
   if [ "$numYes" -gt "0" ]; then
	failstring=$matchstring
        matchstring="$subLGPass $firstOpt""${arr[0]}"" $secondOpt""${arr[1]}"
        lastnum=$numYes
        failstring=""
   else
	failstring="$subLGFail $secondOpt""${arr[1]}"
   fi
 
   nop=`$program -c $nameprog nop `
   nop=`$program -c nop `
   pass=`$program $subOptionsp $nameprogp $matchstring`
   echo "$program $subOptionsp $nameprogp $matchstring"
   pass_test $lastnum
   if [[ -z $failstring ]]; then 
     nop=""
   else
     fail=`$program $subOptionsf $nameprogf $failstring`
     echo "$program $subOptionsf $nameprogf $failstring"
     fail_test
   fi
   nop=`$program -c nop`
   nop=`$program -c $nameprogp nop`
   nop=`$program -c $nameprogf nop`
done

echo ""
echo "In case only a single attribute is set, the ordering is not applied."
echo ""

oneAttrib="$program pub ContentType=TotalOrder ContentOrigin=$name $firstOpt""800 "
secondAttrib="$program pub ContentType=TotalOrder ContentOrigin=$name $secondOpt""30"
echo "$oneAttrib"
command=`$oneAttrib`
echo "$secondAttrib"
command=`$secondAttrib`

oneAttrib="$program pub ContentType=TotalOrder ContentOrigin=$name $firstOpt""810 "
secondAttrib="$program pub ContentType=TotalOrder ContentOrigin=$name $secondOpt""40"
echo "$oneAttrib"
command=`$oneAttrib`
echo "$secondAttrib"
command=`$secondAttrib`

oneAttrib="$program pub ContentType=TotalOrder ContentOrigin=$name $firstOpt""800 "
secondAttrib="$program pub ContentType=TotalOrder ContentOrigin=$name $secondOpt""30"
echo "$oneAttrib"
command=`$oneAttrib`
echo "$secondAttrib"
command=`$secondAttrib`

echo ""
echo "Therefore, the following subscriptions should return 3,2,1,2,1 matches, respectively."
echo ""
nop=`$program -c nop`
nop=`$program -c $nameprog nop`
echo "$program $subOptionsp match $matchstring"
pass=`$program $subOptionsp match $matchstring`
pass_test 3
 
nop=`$program -c nop`
nop=`$program -c phase2 nop`
pass="$program $subOptionsp phase2a sub $firstOpt""*"
echo $pass
command=`$pass`
pass_test 2

nop=`$program -c phase2a nop`
pass="$program $subOptionsp phase2b sub $firstOpt""810"
echo $pass
command=`$pass`
pass_test 1

nop=`$program -c phase2b nop`
pass="$program $subOptionsp phase2c sub $secondOpt""*"
echo $pass
command=`$pass`
pass_test 2

nop=`$program -c phase2c nop`
pass="$program $subOptionsp phase2d sub $secondOpt""40"
echo $pass
command=`$pass`
pass_test 1

echo ""
echo "The ordering is only applied to DOs from same ContentOrigin."
echo "Therefore, the follwoing steps should return 1 match." 
echo "" 
name2="SRI"
sri="$program pub ContentType=TotalOrder ContentOrigin=$name2 $secondOpt""15"
echo $sri
command=`$sri`

nop=`$program -c nop`
pass="$program $subOptionsp phase3b sub ContentOrigin=$name2 "
echo $pass
command=`$pass`
pass_test 1

echo ""
echo "---------------------------------------------"
echo "PASS!"
echo ""

exit 0
