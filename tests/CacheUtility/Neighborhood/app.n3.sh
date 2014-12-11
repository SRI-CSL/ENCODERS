#!/bin/bash
export PATH="$PATH:."
TFILE="output_file.n3"
rm ${TFILE}

haggletest -a -c -b 1 app1 pub start=now

(sleep 1.00 && haggletest -a -f test_output.n3.0 -c  -s 10  app0 sub key1=value1 ) &
sleep 39.00
echo "test_output.n3.0" > ${TFILE}
cat test_output.n3.0 >> ${TFILE}
rm -f test_output.n3.0
busybox ifconfig usb0 >> ${TFILE}
