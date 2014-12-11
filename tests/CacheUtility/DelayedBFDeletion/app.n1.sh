#!/bin/bash
#this allows manually started scripts to sync up.   Run n1 first
export PATH="$PATH:."
haggletest -a -c -w 1 ap1 sub sync=up

dd if=/dev/zero of=71KB bs=1024 count=71 >& /dev/null

(sleep 1 && haggletest -b 1  ap2 pub key=value )&
(sleep 1 && haggletest -b 1  ap3 pub file 71KB key1=bogus1 ContentType=DelByRelTTL key=value purge_after_seconds=2 ) &
sleep 39
echo ""

