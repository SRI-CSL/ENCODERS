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
    i=0; while [ $i -lt 10 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub  file /tmp/data-a1.10k important="0.6" plt=plt ContentType=incident1 ContentOrigin='"$NODE"'  2> /tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app88'"$i"' pub file /tmp/data-a1.10k plt=plt important="0.6" ContentType=incident1 ContentOrigin='"$NODE"'   \" failed, code: $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 2
      i=$((i+1))
    done

   sleep 90
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   /sbin/ifconfig eth1 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*

elif [ "${NODE}" == "n2" ]; then
    #specific pub, subs
    i=0; while [ $i -lt 1 ]; do
      pub_cmd[$i]='( [ "$(haggletest -j ContentType -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 -k app9'"$i"' pub  file /tmp/data-b1.11k important="0.6" plt=plt ContentType=incident2 ContentOrigin='"$NODE"'  2> /tmp/test_output.'"$NODE"'.'"$i"'.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.'"$i"'.errcode; cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /tmp/test_output.'"$NODE"'.'"$i"'.houtput -p 1 app9'"$i"' pub file /tmp/data-b1.10k important="0.6" plt=plt ContentType=incident2 ContentOrigin='"$NODE"'   \" failed, code: $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errcode) $(cat /tmp/test_output.'"$NODE"'.'"$i"'.errmsg)!" >> ${FAILLOG}) ) &'
    eval ${pub_cmd[$i]}
      i=$((i+1))
    done
echo ${pub_cmd[0]}
   sleep 120
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   /sbin/ifconfig eth1 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*

else
   #echo "node $NODE does not exist" >> /tmp/no_nodes
   #exit 0
sub_cmds[0]='( [ "$(haggletest -j ContentOrigin -d -a -f /tmp/test_output.'"$NODE"'.0.houtput -c -s 150  app0 sub ContentType=incident1 2> /tmp/test_output.'"$NODE"'.0.errmsg > /dev/null; echo $? > /tmp/test_output.'"$NODE"'.0.errcode; cat /tmp/test_output.'"$NODE"'.0.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /tmp/test_output.'"$NODE"'.0.houtput -c  -s 70  app0 sub ContentType=incident1\" failed, code: $(cat /tmp/test_output.'"$NODE"'.0.errcode) $(cat /tmp/test_output.'"$NODE"'.0.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

   sleep 120
   tail -n +1 /tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   /sbin/ifconfig eth0 >> ${TFILE}
   rm -f /tmp/test_output.${NODE}.*
fi

cat ${TFILE} >> $2
exit 0




