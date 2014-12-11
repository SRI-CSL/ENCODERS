#!/bin/bash
export PATH="$PATH:."
dd if=/dev/zero of=71KB bs=1024 count=71 >& /dev/null
dd if=/dev/zero of=72KB bs=1024 count=72 >& /dev/null

TFILE="output_file.n1"
rm ${TFILE}

haggletest -c -a -w 1 app2 sub start=now
(sleep 1.00 && haggletest -f test_output.n1.0  -b 1  app0 pub file 71KB key=value) &
(sleep 1.00 && haggletest -f test_output.n1.1  -b 1  app1 pub file 72KB key1=value1 key=value) &
sleep 39.00
echo "test_output.n1.0" > ${TFILE}
cat test_output.n1.0 >> ${TFILE}
rm -f test_output.n1.0
echo "test_output.n1.1" >> ${TFILE}
cat test_output.n1.1 >> ${TFILE}
rm -f test_output.n1.1
busybox ifconfig usb0 >> ${TFILE}

