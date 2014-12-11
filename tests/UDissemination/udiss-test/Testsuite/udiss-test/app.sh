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
    i=1; while [ $i -lt 7 ]; do
    pub_cmd[$i]='( [ "$(haggletest  -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app'"$i"' pub  file /tmp/data.'"$(($i+$i+$i))"'0k priority='"$(($i*$i))"' Content="A"  2> /tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app'"$i"' pub file /tmp/data.'"$(($i+$i+$i))"'0k importance='"$(($i*$i))"' Content="A"   \" failed, code: $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      i=$((i+1))
    done

   sleep 60
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*
elif [ "${NODE}" == "n2" ]; then
   sleep 60
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*
else
   echo "node $NODE does not exist" >> /tmp/no_nodes
   exit 0
fi

cat ${TFILE} >> $2
exit 0




