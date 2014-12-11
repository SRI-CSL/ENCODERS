#!/bin/bash
TFILE="/tmp/$1_temp_file"
rm -f ${TFILE}
sleep 120
if [ $1 == "n0" ]; then
    echo "bogus node"
elif [ $1 == "n1" ]; then
    (haggletest app0 -f /tmp/test_output.n1 -b 14 -p 10 pub file /tmp/2MB n1) &
    haggletest app1 -f /tmp/test_output.n1.0 -s 45 -c sub n17 n4 n2
    haggletest app2 -f /tmp/test_output.n1.1 -s 45 -c sub n8 n13 n7
    haggletest app3 -f /tmp/test_output.n1.2 -s 45 -c sub n18 n16 n11
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
    haggletest app1 -f /tmp/test_output.n2.0 -s 45 -c sub n17 n5 n6
    haggletest app2 -f /tmp/test_output.n2.1 -s 45 -c sub n9 n8 n1
    haggletest app3 -f /tmp/test_output.n2.2 -s 45 -c sub n14 n20 n18
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
    haggletest app1 -f /tmp/test_output.n3.0 -s 45 -c sub n12 n9 n2
    haggletest app2 -f /tmp/test_output.n3.1 -s 45 -c sub n4 n19 n8
    haggletest app3 -f /tmp/test_output.n3.2 -s 45 -c sub n11 n20 n16
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
    haggletest app1 -f /tmp/test_output.n4.0 -s 45 -c sub n8 n15 n12
    haggletest app2 -f /tmp/test_output.n4.1 -s 45 -c sub n5 n3 n20
    haggletest app3 -f /tmp/test_output.n4.2 -s 45 -c sub n11 n12 n15
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
    haggletest app1 -f /tmp/test_output.n5.0 -s 45 -c sub n2 n11 n9
    haggletest app2 -f /tmp/test_output.n5.1 -s 45 -c sub n12 n10 n1
    haggletest app3 -f /tmp/test_output.n5.2 -s 45 -c sub n11 n15 n3
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
    haggletest app1 -f /tmp/test_output.n6.0 -s 45 -c sub n20 n17 n9
    haggletest app2 -f /tmp/test_output.n6.1 -s 45 -c sub n5 n7 n8
    haggletest app3 -f /tmp/test_output.n6.2 -s 45 -c sub n2 n18 n13
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
    haggletest app1 -f /tmp/test_output.n7.0 -s 45 -c sub n19 n17 n3
    haggletest app2 -f /tmp/test_output.n7.1 -s 45 -c sub n5 n16 n4
    haggletest app3 -f /tmp/test_output.n7.2 -s 45 -c sub n12 n19 n18
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
    haggletest app1 -f /tmp/test_output.n8.0 -s 45 -c sub n9 n17 n13
    haggletest app2 -f /tmp/test_output.n8.1 -s 45 -c sub n7 n1 n20
    haggletest app3 -f /tmp/test_output.n8.2 -s 45 -c sub n11 n5 n16
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
    haggletest app1 -f /tmp/test_output.n9.0 -s 45 -c sub n11 n15 n13
    haggletest app2 -f /tmp/test_output.n9.1 -s 45 -c sub n10 n7 n3
    haggletest app3 -f /tmp/test_output.n9.2 -s 45 -c sub n15 n8 n14
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
    haggletest app1 -f /tmp/test_output.n10.0 -s 45 -c sub n15 n6 n5
    haggletest app2 -f /tmp/test_output.n10.1 -s 45 -c sub n20 n13 n8
    haggletest app3 -f /tmp/test_output.n10.2 -s 45 -c sub n19 n1 n7
    echo "/tmp/test_output.n10" >> ${TFILE}
    cat /tmp/test_output.n10 >> ${TFILE}
    echo "/tmp/test_output.n10.0" >> ${TFILE}
    cat /tmp/test_output.n10.0 >> ${TFILE}
    echo "/tmp/test_output.n10.1" >> ${TFILE}
    cat /tmp/test_output.n10.1 >> ${TFILE}
    echo "/tmp/test_output.n10.2" >> ${TFILE}
    cat /tmp/test_output.n10.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n11" ]; then
    (haggletest app0 -f /tmp/test_output.n11 -b 14 -p 10 pub file /tmp/2MB n11) &
    haggletest app1 -f /tmp/test_output.n11.0 -s 45 -c sub n14 n19 n4
    haggletest app2 -f /tmp/test_output.n11.1 -s 45 -c sub n10 n13 n2
    haggletest app3 -f /tmp/test_output.n11.2 -s 45 -c sub n8 n19 n7
    echo "/tmp/test_output.n11" >> ${TFILE}
    cat /tmp/test_output.n11 >> ${TFILE}
    echo "/tmp/test_output.n11.0" >> ${TFILE}
    cat /tmp/test_output.n11.0 >> ${TFILE}
    echo "/tmp/test_output.n11.1" >> ${TFILE}
    cat /tmp/test_output.n11.1 >> ${TFILE}
    echo "/tmp/test_output.n11.2" >> ${TFILE}
    cat /tmp/test_output.n11.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n12" ]; then
    (haggletest app0 -f /tmp/test_output.n12 -b 14 -p 10 pub file /tmp/2MB n12) &
    haggletest app1 -f /tmp/test_output.n12.0 -s 45 -c sub n15 n3 n17
    haggletest app2 -f /tmp/test_output.n12.1 -s 45 -c sub n7 n20 n6
    haggletest app3 -f /tmp/test_output.n12.2 -s 45 -c sub n11 n10 n3
    echo "/tmp/test_output.n12" >> ${TFILE}
    cat /tmp/test_output.n12 >> ${TFILE}
    echo "/tmp/test_output.n12.0" >> ${TFILE}
    cat /tmp/test_output.n12.0 >> ${TFILE}
    echo "/tmp/test_output.n12.1" >> ${TFILE}
    cat /tmp/test_output.n12.1 >> ${TFILE}
    echo "/tmp/test_output.n12.2" >> ${TFILE}
    cat /tmp/test_output.n12.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n13" ]; then
    (haggletest app0 -f /tmp/test_output.n13 -b 14 -p 10 pub file /tmp/2MB n13) &
    haggletest app1 -f /tmp/test_output.n13.0 -s 45 -c sub n12 n9 n14
    haggletest app2 -f /tmp/test_output.n13.1 -s 45 -c sub n17 n1 n18
    haggletest app3 -f /tmp/test_output.n13.2 -s 45 -c sub n9 n4 n2
    echo "/tmp/test_output.n13" >> ${TFILE}
    cat /tmp/test_output.n13 >> ${TFILE}
    echo "/tmp/test_output.n13.0" >> ${TFILE}
    cat /tmp/test_output.n13.0 >> ${TFILE}
    echo "/tmp/test_output.n13.1" >> ${TFILE}
    cat /tmp/test_output.n13.1 >> ${TFILE}
    echo "/tmp/test_output.n13.2" >> ${TFILE}
    cat /tmp/test_output.n13.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n14" ]; then
    (haggletest app0 -f /tmp/test_output.n14 -b 14 -p 10 pub file /tmp/2MB n14) &
    haggletest app1 -f /tmp/test_output.n14.0 -s 45 -c sub n19 n12 n7
    haggletest app2 -f /tmp/test_output.n14.1 -s 45 -c sub n4 n1 n11
    haggletest app3 -f /tmp/test_output.n14.2 -s 45 -c sub n10 n20 n8
    echo "/tmp/test_output.n14" >> ${TFILE}
    cat /tmp/test_output.n14 >> ${TFILE}
    echo "/tmp/test_output.n14.0" >> ${TFILE}
    cat /tmp/test_output.n14.0 >> ${TFILE}
    echo "/tmp/test_output.n14.1" >> ${TFILE}
    cat /tmp/test_output.n14.1 >> ${TFILE}
    echo "/tmp/test_output.n14.2" >> ${TFILE}
    cat /tmp/test_output.n14.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n15" ]; then
    (haggletest app0 -f /tmp/test_output.n15 -b 14 -p 10 pub file /tmp/2MB n15) &
    haggletest app1 -f /tmp/test_output.n15.0 -s 45 -c sub n8 n19 n2
    haggletest app2 -f /tmp/test_output.n15.1 -s 45 -c sub n16 n17 n14
    haggletest app3 -f /tmp/test_output.n15.2 -s 45 -c sub n1 n9 n19
    echo "/tmp/test_output.n15" >> ${TFILE}
    cat /tmp/test_output.n15 >> ${TFILE}
    echo "/tmp/test_output.n15.0" >> ${TFILE}
    cat /tmp/test_output.n15.0 >> ${TFILE}
    echo "/tmp/test_output.n15.1" >> ${TFILE}
    cat /tmp/test_output.n15.1 >> ${TFILE}
    echo "/tmp/test_output.n15.2" >> ${TFILE}
    cat /tmp/test_output.n15.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n16" ]; then
    (haggletest app0 -f /tmp/test_output.n16 -b 14 -p 10 pub file /tmp/2MB n16) &
    haggletest app1 -f /tmp/test_output.n16.0 -s 45 -c sub n3 n7 n9
    haggletest app2 -f /tmp/test_output.n16.1 -s 45 -c sub n13 n5 n4
    haggletest app3 -f /tmp/test_output.n16.2 -s 45 -c sub n12 n17 n8
    echo "/tmp/test_output.n16" >> ${TFILE}
    cat /tmp/test_output.n16 >> ${TFILE}
    echo "/tmp/test_output.n16.0" >> ${TFILE}
    cat /tmp/test_output.n16.0 >> ${TFILE}
    echo "/tmp/test_output.n16.1" >> ${TFILE}
    cat /tmp/test_output.n16.1 >> ${TFILE}
    echo "/tmp/test_output.n16.2" >> ${TFILE}
    cat /tmp/test_output.n16.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n17" ]; then
    (haggletest app0 -f /tmp/test_output.n17 -b 14 -p 10 pub file /tmp/2MB n17) &
    haggletest app1 -f /tmp/test_output.n17.0 -s 45 -c sub n19 n9 n2
    haggletest app2 -f /tmp/test_output.n17.1 -s 45 -c sub n4 n16 n20
    haggletest app3 -f /tmp/test_output.n17.2 -s 45 -c sub n7 n12 n19
    echo "/tmp/test_output.n17" >> ${TFILE}
    cat /tmp/test_output.n17 >> ${TFILE}
    echo "/tmp/test_output.n17.0" >> ${TFILE}
    cat /tmp/test_output.n17.0 >> ${TFILE}
    echo "/tmp/test_output.n17.1" >> ${TFILE}
    cat /tmp/test_output.n17.1 >> ${TFILE}
    echo "/tmp/test_output.n17.2" >> ${TFILE}
    cat /tmp/test_output.n17.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n18" ]; then
    (haggletest app0 -f /tmp/test_output.n18 -b 14 -p 10 pub file /tmp/2MB n18) &
    haggletest app1 -f /tmp/test_output.n18.0 -s 45 -c sub n9 n19 n3
    haggletest app2 -f /tmp/test_output.n18.1 -s 45 -c sub n6 n1 n12
    haggletest app3 -f /tmp/test_output.n18.2 -s 45 -c sub n13 n15 n7
    echo "/tmp/test_output.n18" >> ${TFILE}
    cat /tmp/test_output.n18 >> ${TFILE}
    echo "/tmp/test_output.n18.0" >> ${TFILE}
    cat /tmp/test_output.n18.0 >> ${TFILE}
    echo "/tmp/test_output.n18.1" >> ${TFILE}
    cat /tmp/test_output.n18.1 >> ${TFILE}
    echo "/tmp/test_output.n18.2" >> ${TFILE}
    cat /tmp/test_output.n18.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n19" ]; then
    (haggletest app0 -f /tmp/test_output.n19 -b 14 -p 10 pub file /tmp/2MB n19) &
    haggletest app1 -f /tmp/test_output.n19.0 -s 45 -c sub n16 n5 n1
    haggletest app2 -f /tmp/test_output.n19.1 -s 45 -c sub n14 n3 n11
    haggletest app3 -f /tmp/test_output.n19.2 -s 45 -c sub n13 n16 n18
    echo "/tmp/test_output.n19" >> ${TFILE}
    cat /tmp/test_output.n19 >> ${TFILE}
    echo "/tmp/test_output.n19.0" >> ${TFILE}
    cat /tmp/test_output.n19.0 >> ${TFILE}
    echo "/tmp/test_output.n19.1" >> ${TFILE}
    cat /tmp/test_output.n19.1 >> ${TFILE}
    echo "/tmp/test_output.n19.2" >> ${TFILE}
    cat /tmp/test_output.n19.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n20" ]; then
    (haggletest app0 -f /tmp/test_output.n20 -b 14 -p 10 pub file /tmp/2MB n20) &
    haggletest app1 -f /tmp/test_output.n20.0 -s 45 -c sub n2 n6 n1
    haggletest app2 -f /tmp/test_output.n20.1 -s 45 -c sub n12 n7 n3
    haggletest app3 -f /tmp/test_output.n20.2 -s 45 -c sub n9 n8 n10
    echo "/tmp/test_output.n20" >> ${TFILE}
    cat /tmp/test_output.n20 >> ${TFILE}
    echo "/tmp/test_output.n20.0" >> ${TFILE}
    cat /tmp/test_output.n20.0 >> ${TFILE}
    echo "/tmp/test_output.n20.1" >> ${TFILE}
    cat /tmp/test_output.n20.1 >> ${TFILE}
    echo "/tmp/test_output.n20.2" >> ${TFILE}
    cat /tmp/test_output.n20.2 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
else
    exit 0
fi
cat ${TFILE} >> $2
