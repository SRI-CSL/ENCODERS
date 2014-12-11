#!/bin/bash
TFILE="/tmp/$1_temp_file"
rm -f ${TFILE}
sleep 120
if [ $1 == "n0" ]; then
    echo "bogus node"
elif [ $1 == "n1" ]; then
    (haggletest app0 -f /tmp/test_output.n1 -b 14 -p 10 pub file /tmp/2MB n1) &
    haggletest app1 -f /tmp/test_output.n1.0 -s 45 -c sub n4 n9 n7
    haggletest app2 -f /tmp/test_output.n1.1 -s 45 -c sub n6 n2 n10
    haggletest app3 -f /tmp/test_output.n1.2 -s 45 -c sub n7 n5 n3
    echo "/tmp/test_output.n1" >> ${TFILE}
    cat /tmp/test_output.n1 >> ${TFILE}
    echo "/tmp/test_output.n1.0" >> ${TFILE}
    cat /tmp/test_output.n1.0 >> ${TFILE}
    echo "/tmp/test_output.n1.1" >> ${TFILE}
    cat /tmp/test_output.n1.1 >> ${TFILE}
    echo "/tmp/test_output.n1.2" >> ${TFILE}
    cat /tmp/test_output.n1.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n2" ]; then
    (haggletest app0 -f /tmp/test_output.n2 -b 14 -p 10 pub file /tmp/2MB n2) &
    haggletest app1 -f /tmp/test_output.n2.0 -s 45 -c sub n4 n1 n10
    haggletest app2 -f /tmp/test_output.n2.1 -s 45 -c sub n8 n6 n5
    haggletest app3 -f /tmp/test_output.n2.2 -s 45 -c sub n10 n7 n4
    echo "/tmp/test_output.n2" >> ${TFILE}
    cat /tmp/test_output.n2 >> ${TFILE}
    echo "/tmp/test_output.n2.0" >> ${TFILE}
    cat /tmp/test_output.n2.0 >> ${TFILE}
    echo "/tmp/test_output.n2.1" >> ${TFILE}
    cat /tmp/test_output.n2.1 >> ${TFILE}
    echo "/tmp/test_output.n2.2" >> ${TFILE}
    cat /tmp/test_output.n2.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n3" ]; then
    (haggletest app0 -f /tmp/test_output.n3 -b 14 -p 10 pub file /tmp/2MB n3) &
    haggletest app1 -f /tmp/test_output.n3.0 -s 45 -c sub n5 n9 n8
    haggletest app2 -f /tmp/test_output.n3.1 -s 45 -c sub n10 n7 n4
    haggletest app3 -f /tmp/test_output.n3.2 -s 45 -c sub n9 n6 n5
    echo "/tmp/test_output.n3" >> ${TFILE}
    cat /tmp/test_output.n3 >> ${TFILE}
    echo "/tmp/test_output.n3.0" >> ${TFILE}
    cat /tmp/test_output.n3.0 >> ${TFILE}
    echo "/tmp/test_output.n3.1" >> ${TFILE}
    cat /tmp/test_output.n3.1 >> ${TFILE}
    echo "/tmp/test_output.n3.2" >> ${TFILE}
    cat /tmp/test_output.n3.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n4" ]; then
    (haggletest app0 -f /tmp/test_output.n4 -b 14 -p 10 pub file /tmp/2MB n4) &
    haggletest app1 -f /tmp/test_output.n4.0 -s 45 -c sub n6 n10 n1
    haggletest app2 -f /tmp/test_output.n4.1 -s 45 -c sub n5 n3 n9
    haggletest app3 -f /tmp/test_output.n4.2 -s 45 -c sub n8 n6 n7
    echo "/tmp/test_output.n4" >> ${TFILE}
    cat /tmp/test_output.n4 >> ${TFILE}
    echo "/tmp/test_output.n4.0" >> ${TFILE}
    cat /tmp/test_output.n4.0 >> ${TFILE}
    echo "/tmp/test_output.n4.1" >> ${TFILE}
    cat /tmp/test_output.n4.1 >> ${TFILE}
    echo "/tmp/test_output.n4.2" >> ${TFILE}
    cat /tmp/test_output.n4.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n5" ]; then
    (haggletest app0 -f /tmp/test_output.n5 -b 14 -p 10 pub file /tmp/2MB n5) &
    haggletest app1 -f /tmp/test_output.n5.0 -s 45 -c sub n2 n10 n3
    haggletest app2 -f /tmp/test_output.n5.1 -s 45 -c sub n6 n8 n9
    haggletest app3 -f /tmp/test_output.n5.2 -s 45 -c sub n7 n4 n2
    echo "/tmp/test_output.n5" >> ${TFILE}
    cat /tmp/test_output.n5 >> ${TFILE}
    echo "/tmp/test_output.n5.0" >> ${TFILE}
    cat /tmp/test_output.n5.0 >> ${TFILE}
    echo "/tmp/test_output.n5.1" >> ${TFILE}
    cat /tmp/test_output.n5.1 >> ${TFILE}
    echo "/tmp/test_output.n5.2" >> ${TFILE}
    cat /tmp/test_output.n5.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n6" ]; then
    (haggletest app0 -f /tmp/test_output.n6 -b 14 -p 10 pub file /tmp/2MB n6) &
    haggletest app1 -f /tmp/test_output.n6.0 -s 45 -c sub n8 n4 n2
    haggletest app2 -f /tmp/test_output.n6.1 -s 45 -c sub n1 n7 n5
    haggletest app3 -f /tmp/test_output.n6.2 -s 45 -c sub n10 n4 n8
    echo "/tmp/test_output.n6" >> ${TFILE}
    cat /tmp/test_output.n6 >> ${TFILE}
    echo "/tmp/test_output.n6.0" >> ${TFILE}
    cat /tmp/test_output.n6.0 >> ${TFILE}
    echo "/tmp/test_output.n6.1" >> ${TFILE}
    cat /tmp/test_output.n6.1 >> ${TFILE}
    echo "/tmp/test_output.n6.2" >> ${TFILE}
    cat /tmp/test_output.n6.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n7" ]; then
    (haggletest app0 -f /tmp/test_output.n7 -b 14 -p 10 pub file /tmp/2MB n7) &
    haggletest app1 -f /tmp/test_output.n7.0 -s 45 -c sub n9 n1 n3
    haggletest app2 -f /tmp/test_output.n7.1 -s 45 -c sub n8 n4 n10
    haggletest app3 -f /tmp/test_output.n7.2 -s 45 -c sub n6 n5 n2
    echo "/tmp/test_output.n7" >> ${TFILE}
    cat /tmp/test_output.n7 >> ${TFILE}
    echo "/tmp/test_output.n7.0" >> ${TFILE}
    cat /tmp/test_output.n7.0 >> ${TFILE}
    echo "/tmp/test_output.n7.1" >> ${TFILE}
    cat /tmp/test_output.n7.1 >> ${TFILE}
    echo "/tmp/test_output.n7.2" >> ${TFILE}
    cat /tmp/test_output.n7.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n8" ]; then
    (haggletest app0 -f /tmp/test_output.n8 -b 14 -p 10 pub file /tmp/2MB n8) &
    haggletest app1 -f /tmp/test_output.n8.0 -s 45 -c sub n5 n6 n2
    haggletest app2 -f /tmp/test_output.n8.1 -s 45 -c sub n4 n3 n7
    haggletest app3 -f /tmp/test_output.n8.2 -s 45 -c sub n5 n1 n2
    echo "/tmp/test_output.n8" >> ${TFILE}
    cat /tmp/test_output.n8 >> ${TFILE}
    echo "/tmp/test_output.n8.0" >> ${TFILE}
    cat /tmp/test_output.n8.0 >> ${TFILE}
    echo "/tmp/test_output.n8.1" >> ${TFILE}
    cat /tmp/test_output.n8.1 >> ${TFILE}
    echo "/tmp/test_output.n8.2" >> ${TFILE}
    cat /tmp/test_output.n8.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n9" ]; then
    (haggletest app0 -f /tmp/test_output.n9 -b 14 -p 10 pub file /tmp/2MB n9) &
    haggletest app1 -f /tmp/test_output.n9.0 -s 45 -c sub n6 n1 n5
    haggletest app2 -f /tmp/test_output.n9.1 -s 45 -c sub n3 n2 n8
    haggletest app3 -f /tmp/test_output.n9.2 -s 45 -c sub n4 n6 n1
    echo "/tmp/test_output.n9" >> ${TFILE}
    cat /tmp/test_output.n9 >> ${TFILE}
    echo "/tmp/test_output.n9.0" >> ${TFILE}
    cat /tmp/test_output.n9.0 >> ${TFILE}
    echo "/tmp/test_output.n9.1" >> ${TFILE}
    cat /tmp/test_output.n9.1 >> ${TFILE}
    echo "/tmp/test_output.n9.2" >> ${TFILE}
    cat /tmp/test_output.n9.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n10" ]; then
    (haggletest app0 -f /tmp/test_output.n10 -b 14 -p 10 pub file /tmp/2MB n10) &
    haggletest app1 -f /tmp/test_output.n10.0 -s 45 -c sub n2 n5 n1
    haggletest app2 -f /tmp/test_output.n10.1 -s 45 -c sub n8 n3 n7
    haggletest app3 -f /tmp/test_output.n10.2 -s 45 -c sub n5 n4 n6
    echo "/tmp/test_output.n10" >> ${TFILE}
    cat /tmp/test_output.n10 >> ${TFILE}
    echo "/tmp/test_output.n10.0" >> ${TFILE}
    cat /tmp/test_output.n10.0 >> ${TFILE}
    echo "/tmp/test_output.n10.1" >> ${TFILE}
    cat /tmp/test_output.n10.1 >> ${TFILE}
    echo "/tmp/test_output.n10.2" >> ${TFILE}
    cat /tmp/test_output.n10.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
else
    exit 0
fi
cat ${TFILE} >> $2
