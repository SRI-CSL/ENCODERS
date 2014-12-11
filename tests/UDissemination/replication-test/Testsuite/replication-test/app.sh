#!/bin/bash
TFILE="$(mktemp)"
NOW="$(date '+%s')"
NODE="$1"
APPS_OUTPUT="$2"
FAILLOG="$3"
rm -f ${TFILE}

sleep 10

if [ "${NODE}" == "n0" ]; then
    echo "bogus node"
elif [ "${NODE}" == "n1" ]; then
    sleep 20
    i=0; while [ $i -lt 4 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /tmp/data-a1.10k GroupA=1 ContentOrigin='"$NODE"'  2> /tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /tmp/data-a1.10k GroupA=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 300
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*
elif [ "${NODE}" == "n2" ]; then
    sleep 20
    i=0; while [ $i -lt 2 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /tmp/data-a1.10k GroupA=1 ContentOrigin='"$NODE"'  2> /tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /tmp/data-a1.10k GroupA=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 300
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*
elif [ "${NODE}" == "n3" ]; then
    sleep 23
    i=0; while [ $i -lt 1 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /tmp/data-a1.10k GroupA=1 ContentOrigin='"$NODE"'  2> /tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /tmp/data-a1.10k GroupA=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 297
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*
    
elif [ "${NODE}" == "n4" ]; then
    sleep 26
    i=0; while [ $i -lt 2 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /tmp/data-a1.10k GroupE=1 ContentOrigin='"$NODE"'  2> /tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /tmp/data-a1.10k GroupE=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 294
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*
elif [ "${NODE}" == "n5" ]; then
    sleep 29
    i=0; while [ $i -lt 1 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /tmp/data-a1.10k GroupE=1 ContentOrigin='"$NODE"'  2> /tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /tmp/data-a1.10k GroupE=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 291
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*


elif [ "${NODE}" == "n6" ]; then
    sleep 35

    i=0; while [ $i -lt 1 ]; do
    pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /tmp/data-a1.10k GroupF=1 ContentOrigin='"$NODE"'  2> /tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /tmp/data-a1.10k GroupF=1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 285
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*

else
   echo "node $NODE does not exist" >> /tmp/no_nodes
   exit 0

fi

cat ${TFILE} >> $2
exit 0




