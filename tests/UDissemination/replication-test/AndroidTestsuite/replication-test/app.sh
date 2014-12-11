#!/bin/bash
TFILE="$(mktemp /data/tmp/temp.XXXX)"
NOW="$(date '+%s')"
NODE="$1"
APPS_OUTPUT="$2"
FAILLOG="$3"
rm -f ${TFILE}

sleep 10

if [ "${NODE}" == "n0" ]; then
    echo "bogus node"
elif [ "${NODE}" == "n1" ]; then
    sleep 25 
    i=0; while [ $i -lt 4 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /data/tmp/data-a1.10k GroupA=1 ContentOrigin='"$NODE"'  2> /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /data/tmp/data-a1.10k GroupA=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 11 
      i=$((i+1))
    done

   sleep 258 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*
elif [ "${NODE}" == "n2" ]; then
    sleep 45 
    i=0; while [ $i -lt 2 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /data/tmp/data-a1.10k GroupB=1 ContentOrigin='"$NODE"'  2> /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /data/tmp/data-a1.10k GroupB=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 15
      i=$((i+1))
    done

   sleep 251
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*
elif [ "${NODE}" == "n3" ]; then
    sleep 63
    i=0; while [ $i -lt 1 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /data/tmp/data-a1.10k GroupB2=1 ContentOrigin='"$NODE"'  2> /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /data/tmp/data-a1.10k GroupB2=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 257
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*
    
elif [ "${NODE}" == "n4" ]; then
    sleep 86
    i=0; while [ $i -lt 2 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /data/tmp/data-a1.10k GroupC=1 ContentOrigin='"$NODE"'  2> /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /data/tmp/data-a1.10k GroupC=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 13 
      i=$((i+1))
    done

   sleep 212
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*
elif [ "${NODE}" == "n5" ]; then
    sleep 129
    i=0; while [ $i -lt 1 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /data/tmp/data-a1.10k GroupC2=1 ContentOrigin='"$NODE"'  2> /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /data/tmp/data-a1.10k GroupC2=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 191
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*


elif [ "${NODE}" == "n6" ]; then
    sleep 35

    i=0; while [ $i -lt 1 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /data/tmp/data-a1.10k GroupD=1 ContentOrigin='"$NODE"'  2> /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /data/tmp/data-a1.10k GroupD=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /data/tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 285
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*

else
   echo "node $NODE does not exist" >> /data/tmp/no_nodes
   exit 0

fi

cat ${TFILE} >> $2
exit 0




