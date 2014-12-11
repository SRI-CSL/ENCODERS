#!/bin/bash
TFILE="sender_temp_file"
rm -f ${TFILE}

issue_pkt () {
  (./haggletest -b 1 pub file 73KB key="value" first="file") 
  (./haggletest -b 7 pub file 72KB key="value") 
  (./haggletest -b 1 pub file 71KB key="value") 
  return 0
}

#create files
dd if=/dev/zero of=71KB bs=1024 count=71
dd if=/dev/zero of=72KB bs=1024 count=72
dd if=/dev/zero of=73KB bs=1024 count=73

sleep 2 
#sync up nodes
./haggletest -b 1 pub start
./haggletest -w 1 sub response

for i in {1..2}
do
  issue_pkt
  sleep 10
done



