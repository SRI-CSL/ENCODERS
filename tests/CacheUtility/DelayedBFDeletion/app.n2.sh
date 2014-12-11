#!/bin/bash
TFILE="test_output.n2"
export PATH="$PATH:."
#This allows manually started scripts to sync up.   Run n2 first.
haggletest -a -c ap0 pub sync=up

(sleep 1.00 && haggletest -a -s 3  -c -f test_output.n2.0 ap1 sub key=value) & 
(sleep 7 && haggletest -a -s 4  -c -f test_output.n2.1 ap2 sub key1=bogus1) &
(sleep 8 &&  haggletest -a -w 1  -c -f test_output.n2.2 ap3 sub CacheStrategyUtility=stats > faillog.n2 ) &

sleep 39
echo "Node 2 finished"
echo "test_output.n2.0" > ${TFILE} 
cat test_output.n2.0 >> ${TFILE}
rm -f test_output.n2.0
echo "test_output.n2.1" >> ${TFILE} 
cat test_output.n2.1 >> ${TFILE}
rm -f test_output.n2.1
echo "test_output.n2.2" >> ${TFILE} 
cat test_output.n2.2 >> ${TFILE}
rm -f test_output.n2.2
busybox ifconfig usb0 >> ${TFILE}

