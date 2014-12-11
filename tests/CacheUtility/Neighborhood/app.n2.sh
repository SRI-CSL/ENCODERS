#!/bin/bash
export PATH="$PATH:."
TFILE="output_file.n2"
rm ${TFILE}

haggletest -c -a -w 1 app2 sub start=now

(sleep 1.00 && haggletest -a -f test_output.n2.0 -c  -s 10  app0 sub key=value ) &
(sleep 14.00 && haggletest -a -f test_output.n2.1 -c  -w 1  app1 sub CacheStrategyUtility=stats > test_output.n2.1.appout) &
sleep 39.00
echo "test_output.n2.0" > ${TFILE}
cat test_output.n2.0 >> ${TFILE}
rm -f test_output.n2.0
echo "test_output.n2.1" >> ${TFILE}
cat test_output.n2.1 >> ${TFILE}
rm -f test_output.n2.1
echo "test_output.n2.1.appout" >> ${TFILE}
cat test_output.n2.1.appout >> ${TFILE}
rm -f test_output.n2.1.appout
busybox ifconfig usb0 >> ${TFILE}

