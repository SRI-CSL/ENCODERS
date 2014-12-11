#!/bin/bash
TFILE="$(mktemp /data/tmp/temp.XXXX)"
NOW="$(date '+%s')"
NODE="$1"
APPS_OUTPUT="$2"
FAILLOG="$3"
rm -f ${TFILE}



# all nodes sub to plt=plt, but each group (A, B, C) will want to cache their social group primarily 
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.0.houtput -c  -s 350  app0 sub plt=plt 2> /data/tmp/test_output.'"$NODE"'.0.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.0.errcode; cat /data/tmp/test_output.'"$NODE"'.0.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0 sub plt=plt\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.0.errcode) $(cat /data/tmp/test_output.'"$NODE"'.0.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)


if [ "${NODE}" == "n0" ]; then
    echo "bogus node"
elif [ "${NODE}" == "n1" ]; then
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=A 2> /data/tmp/test_output.'"$NODE"'.1.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.1.errcode; cat /data/tmp/test_output.'"$NODE"'.1.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=A\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.1.errcode) $(cat /data/tmp/test_output.'"$NODE"'.1.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

    sleep 40 
    i=0; while [ $i -lt 1 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j squad -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub  file /data/tmp/data-a1.10K  plt=plt squad=A  2> /data/tmp/test_output.'"$NODE"'.2.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.2.errcode; cat /data/tmp/test_output.'"$NODE"'.2.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub file /data/tmp/data-a1.10K squad=A   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.2.errcode) $(cat /data/tmp/test_output.'"$NODE"'.2.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 1
      i=$((i+1))
    done
#    (i=0; while [ $i -lt 10 ]; do sleep 6; eval ${pub_cmd[$i]}; i=$((i+1)); done)

   sleep 310 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*

    #specific pub, subs
#squad3
elif [ "${NODE}" == "n2" ]; then
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=A 2> /data/tmp/test_output.'"$NODE"'.1.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.1.errcode; cat /data/tmp/test_output.'"$NODE"'.1.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=A\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.1.errcode) $(cat /data/tmp/test_output.'"$NODE"'.1.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

    sleep 60 
    i=0; while [ $i -lt 1 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j squad -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub  file /data/tmp/data-a2.10K  plt=plt squad=A  2> /data/tmp/test_output.'"$NODE"'.2.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.2.errcode; cat /data/tmp/test_output.'"$NODE"'.2.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub file /data/tmp/data-a2.10K squad=A   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.2.errcode) $(cat /data/tmp/test_output.'"$NODE"'.2.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 1
      i=$((i+1))
    done
#    (i=0; while [ $i -lt 10 ]; do sleep 6; eval ${pub_cmd[$i]}; i=$((i+1)); done)

   sleep 290 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*


elif [ "${NODE}" == "n3" ]; then
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=A 2> /data/tmp/test_output.'"$NODE"'.1.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.1.errcode; cat /data/tmp/test_output.'"$NODE"'.1.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=A\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.1.errcode) $(cat /data/tmp/test_output.'"$NODE"'.1.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

    sleep 70 
    i=0; while [ $i -lt 1 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j squad -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub  file /data/tmp/data-a3.10K  plt=plt squad=A  2> /data/tmp/test_output.'"$NODE"'.2.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.2.errcode; cat /data/tmp/test_output.'"$NODE"'.2.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub file /data/tmp/data-a3.10K squad=A   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.2.errcode) $(cat /data/tmp/test_output.'"$NODE"'.2.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 1
      i=$((i+1))
    done

   sleep 280 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*

elif [ "${NODE}" == "n4" ]; then
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=B  2> /data/tmp/test_output.'"$NODE"'.1.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.1.errcode; cat /data/tmp/test_output.'"$NODE"'.1.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=B\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.1.errcode) $(cat /data/tmp/test_output.'"$NODE"'.1.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

    sleep 80 
    i=0; while [ $i -lt 1 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j squad -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub  file /data/tmp/data-b1.10K  plt=plt squad=B 2> /data/tmp/test_output.'"$NODE"'.2.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.2.errcode; cat /data/tmp/test_output.'"$NODE"'.2.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub file /data/tmp/data-b1.10K squad=B   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.2.errcode) $(cat /data/tmp/test_output.'"$NODE"'.2.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 1
      i=$((i+1))
    done
#    (i=0; while [ $i -lt 10 ]; do sleep 6; eval ${pub_cmd[$i]}; i=$((i+1)); done)

   sleep 270 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*

    #specific pub, subs
#squad3
elif [ "${NODE}" == "n5" ]; then
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=B 2> /data/tmp/test_output.'"$NODE"'.1.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.1.errcode; cat /data/tmp/test_output.'"$NODE"'.1.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=B\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.1.errcode) $(cat /data/tmp/test_output.'"$NODE"'.1.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

    sleep 65 
    i=0; while [ $i -lt 1 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j squad -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub  file /data/tmp/data-b2.10K  plt=plt squad=B 2> /data/tmp/test_output.'"$NODE"'.2.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.2.errcode; cat /data/tmp/test_output.'"$NODE"'.2.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub file /data/tmp/data-b2.10K squad=B   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.2.errcode) $(cat /data/tmp/test_output.'"$NODE"'.2.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 1
      i=$((i+1))
    done

   sleep 285 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*


elif [ "${NODE}" == "n6" ]; then
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=B 2> /data/tmp/test_output.'"$NODE"'.1.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.1.errcode; cat /data/tmp/test_output.'"$NODE"'.1.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=B\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.1.errcode) $(cat /data/tmp/test_output.'"$NODE"'.1.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

    sleep 75 
    i=0; while [ $i -lt 1 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j squad -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub  file /data/tmp/data-b3.10K  plt=plt squad=B  2> /data/tmp/test_output.'"$NODE"'.2.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.2.errcode; cat /data/tmp/test_output.'"$NODE"'.2.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub file /data/tmp/data-b3.10K squad=B   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.2.errcode) $(cat /data/tmp/test_output.'"$NODE"'.2.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 1
      i=$((i+1))
    done

   sleep 275 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*


elif [ "${NODE}" == "n7" ]; then
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=C 2> /data/tmp/test_output.'"$NODE"'.1.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.1.errcode; cat /data/tmp/test_output.'"$NODE"'.1.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=C\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.1.errcode) $(cat /data/tmp/test_output.'"$NODE"'.1.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

    sleep 45 
    i=0; while [ $i -lt 1 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j squad -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub  file /data/tmp/data-c1.10K  plt=plt squad=C  2> /data/tmp/test_output.'"$NODE"'.2.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.2.errcode; cat /data/tmp/test_output.'"$NODE"'.2.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub file /data/tmp/data-c1.10K squad=C   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.2.errcode) $(cat /data/tmp/test_output.'"$NODE"'.2.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 1
      i=$((i+1))
    done
#    (i=0; while [ $i -lt 10 ]; do sleep 6; eval ${pub_cmd[$i]}; i=$((i+1)); done)

   sleep 305 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*

    #specific pub, subs
#squad3
elif [ "${NODE}" == "n8" ]; then
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=C 2> /data/tmp/test_output.'"$NODE"'.1.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.1.errcode; cat /data/tmp/test_output.'"$NODE"'.1.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=C\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.1.errcode) $(cat /data/tmp/test_output.'"$NODE"'.1.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

    sleep 85 
    i=0; while [ $i -lt 1 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j squad -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub  file /data/tmp/data-c2.10K  plt=plt squad=C  2> /data/tmp/test_output.'"$NODE"'.2.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.2.errcode; cat /data/tmp/test_output.'"$NODE"'.2.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub file /data/tmp/data-c2.10K squad=A   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.2.errcode) $(cat /data/tmp/test_output.'"$NODE"'.2.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 1
      i=$((i+1))
    done
#    (i=0; while [ $i -lt 10 ]; do sleep 6; eval ${pub_cmd[$i]}; i=$((i+1)); done)

   sleep 265 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*


elif [ "${NODE}" == "n9" ]; then
sub_cmds[0]='( [ "$(haggletest -j squad -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=C 2> /data/tmp/test_output.'"$NODE"'.1.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.1.errcode; cat /data/tmp/test_output.'"$NODE"'.1.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -d -a -f /data/tmp/test_output.'"$NODE"'.1.houtput -c  -s 350  app0'"$NODE"' sub squad=C\" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.1.errcode) $(cat /data/tmp/test_output.'"$NODE"'.1.errmsg)!" >> ${FAILLOG}) )&'
    (i=0; while [ $i -lt 1 ]; do eval ${sub_cmds[$i]}; i=$((i+1)); done)

    sleep 90 
    i=0; while [ $i -lt 1 ]; do
     pub_cmd[$i]='( [ "$(haggletest -j squad -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub  file /data/tmp/data-c3.10K  plt=plt squad=C  2> /data/tmp/test_output.'"$NODE"'.2.errmsg > /dev/null; echo $? > /data/tmp/test_output.'"$NODE"'.2.errcode; cat /data/tmp/test_output.'"$NODE"'.2.errcode)" != "0" ] && (echo "'"${NODE}"': $(date) \"haggletest -f /data/tmp/test_output.'"$NODE"'.2.houtput -p 1 -k app88'"$NODE"' pub file /data/tmp/data-c3.10K squad=C   \" failed, code: $(cat /data/tmp/test_output.'"$NODE"'.2.errcode) $(cat /data/tmp/test_output.'"$NODE"'.2.errmsg)!" >> ${FAILLOG}) ) &'
      eval ${pub_cmd[$i]}
      sleep 1
      i=$((i+1))
    done

   sleep 260 
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*
else
   echo "node $NODE does not exist" >> /data/tmp/no_nodes
   #exit 0
   sleep 350
   tail -n +1 /data/tmp/test_output.${NODE}.*{houtput,appout} >> ${TFILE}
   busybox ifconfig usb0 >> ${TFILE}
   rm -f /data/tmp/test_output.${NODE}.*
fi
cat ${TFILE} >> $2
exit 0



test_output.nc1.1.houtput
