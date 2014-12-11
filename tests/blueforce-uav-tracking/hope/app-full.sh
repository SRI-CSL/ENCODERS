#!/bin/bash
TFILE="/tmp/$1_temp_file"
rm -f ${TFILE}
sleep 120
if [ $1 == "n0" ]; then
    echo "bogus node"
elif [ $1 == "n1" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n1 -b 30 -p 10 pub file /tmp/1355252845866 n1) &
    (haggletest app0 -f /tmp/test_output.1355252845870.n1 -b 30 -p 10 pub file /tmp/1355252845866 n1) &
    (haggletest app0 -f /tmp/test_output.1355252845870.n1 -b 30 -p 10 pub file /tmp/1355252845866 n1) &
    haggletest app1 -f /tmp/test_output.1355252845870.n1.0 -s 45 -c sub n12
    haggletest app2 -f /tmp/test_output.1355252845870.n1.1 -s 45 -c sub n7
    haggletest app3 -f /tmp/test_output.1355252845870.n1.2 -s 45 -c sub n11
    haggletest app4 -f /tmp/test_output.1355252845870.n1.3 -s 45 -c sub n5
    haggletest app5 -f /tmp/test_output.1355252845870.n1.4 -s 45 -c sub n23
    haggletest app6 -f /tmp/test_output.1355252845870.n1.5 -s 45 -c sub n21
    haggletest app7 -f /tmp/test_output.1355252845870.n1.6 -s 45 -c sub n18
    haggletest app8 -f /tmp/test_output.1355252845870.n1.7 -s 45 -c sub n5
    haggletest app9 -f /tmp/test_output.1355252845870.n1.8 -s 45 -c sub n4
    haggletest app10 -f /tmp/test_output.1355252845870.n1.9 -s 45 -c sub n14
    haggletest app11 -f /tmp/test_output.1355252845870.n1.10 -s 45 -c sub n9
    haggletest app12 -f /tmp/test_output.1355252845870.n1.11 -s 45 -c sub n5
    haggletest app13 -f /tmp/test_output.1355252845870.n1.12 -s 45 -c sub n23
    haggletest app14 -f /tmp/test_output.1355252845870.n1.13 -s 45 -c sub n5
    haggletest app15 -f /tmp/test_output.1355252845870.n1.14 -s 45 -c sub n12
    echo "/tmp/test_output.1355252845870.n1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n1.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n1.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n2" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n2 -b 30 -p 10 pub file /tmp/1355252845866 n2) &
    haggletest app1 -f /tmp/test_output.1355252845870.n2.0 -s 45 -c sub n5
    haggletest app2 -f /tmp/test_output.1355252845870.n2.1 -s 45 -c sub n10
    haggletest app3 -f /tmp/test_output.1355252845870.n2.2 -s 45 -c sub n9
    haggletest app4 -f /tmp/test_output.1355252845870.n2.3 -s 45 -c sub n12
    haggletest app5 -f /tmp/test_output.1355252845870.n2.4 -s 45 -c sub n24
    haggletest app6 -f /tmp/test_output.1355252845870.n2.5 -s 45 -c sub n19
    haggletest app7 -f /tmp/test_output.1355252845870.n2.6 -s 45 -c sub n28
    haggletest app8 -f /tmp/test_output.1355252845870.n2.7 -s 45 -c sub n17
    haggletest app9 -f /tmp/test_output.1355252845870.n2.8 -s 45 -c sub n8
    haggletest app10 -f /tmp/test_output.1355252845870.n2.9 -s 45 -c sub n22
    haggletest app11 -f /tmp/test_output.1355252845870.n2.10 -s 45 -c sub n9
    haggletest app12 -f /tmp/test_output.1355252845870.n2.11 -s 45 -c sub n12
    haggletest app13 -f /tmp/test_output.1355252845870.n2.12 -s 45 -c sub n6
    haggletest app14 -f /tmp/test_output.1355252845870.n2.13 -s 45 -c sub n12
    haggletest app15 -f /tmp/test_output.1355252845870.n2.14 -s 45 -c sub n14
    echo "/tmp/test_output.1355252845870.n2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n2.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n2.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n3" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n3 -b 30 -p 10 pub file /tmp/1355252845866 n3) &
    haggletest app1 -f /tmp/test_output.1355252845870.n3.0 -s 45 -c sub n27
    haggletest app2 -f /tmp/test_output.1355252845870.n3.1 -s 45 -c sub n16
    haggletest app3 -f /tmp/test_output.1355252845870.n3.2 -s 45 -c sub n4
    haggletest app4 -f /tmp/test_output.1355252845870.n3.3 -s 45 -c sub n16
    haggletest app5 -f /tmp/test_output.1355252845870.n3.4 -s 45 -c sub n21
    haggletest app6 -f /tmp/test_output.1355252845870.n3.5 -s 45 -c sub n27
    haggletest app7 -f /tmp/test_output.1355252845870.n3.6 -s 45 -c sub n17
    haggletest app8 -f /tmp/test_output.1355252845870.n3.7 -s 45 -c sub n23
    haggletest app9 -f /tmp/test_output.1355252845870.n3.8 -s 45 -c sub n11
    haggletest app10 -f /tmp/test_output.1355252845870.n3.9 -s 45 -c sub n8
    haggletest app11 -f /tmp/test_output.1355252845870.n3.10 -s 45 -c sub n28
    haggletest app12 -f /tmp/test_output.1355252845870.n3.11 -s 45 -c sub n9
    haggletest app13 -f /tmp/test_output.1355252845870.n3.12 -s 45 -c sub n6
    haggletest app14 -f /tmp/test_output.1355252845870.n3.13 -s 45 -c sub n12
    haggletest app15 -f /tmp/test_output.1355252845870.n3.14 -s 45 -c sub n21
    echo "/tmp/test_output.1355252845870.n3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n3.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n3.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n4" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n4 -b 30 -p 10 pub file /tmp/1355252845866 n4) &
    haggletest app1 -f /tmp/test_output.1355252845870.n4.0 -s 45 -c sub n8
    haggletest app2 -f /tmp/test_output.1355252845870.n4.1 -s 45 -c sub n14
    haggletest app3 -f /tmp/test_output.1355252845870.n4.2 -s 45 -c sub n21
    haggletest app4 -f /tmp/test_output.1355252845870.n4.3 -s 45 -c sub n10
    haggletest app5 -f /tmp/test_output.1355252845870.n4.4 -s 45 -c sub n9
    haggletest app6 -f /tmp/test_output.1355252845870.n4.5 -s 45 -c sub n8
    haggletest app7 -f /tmp/test_output.1355252845870.n4.6 -s 45 -c sub n30
    haggletest app8 -f /tmp/test_output.1355252845870.n4.7 -s 45 -c sub n17
    haggletest app9 -f /tmp/test_output.1355252845870.n4.8 -s 45 -c sub n5
    haggletest app10 -f /tmp/test_output.1355252845870.n4.9 -s 45 -c sub n11
    haggletest app11 -f /tmp/test_output.1355252845870.n4.10 -s 45 -c sub n23
    haggletest app12 -f /tmp/test_output.1355252845870.n4.11 -s 45 -c sub n22
    haggletest app13 -f /tmp/test_output.1355252845870.n4.12 -s 45 -c sub n24
    haggletest app14 -f /tmp/test_output.1355252845870.n4.13 -s 45 -c sub n22
    haggletest app15 -f /tmp/test_output.1355252845870.n4.14 -s 45 -c sub n5
    echo "/tmp/test_output.1355252845870.n4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n4.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n4.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n5" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n5 -b 30 -p 10 pub file /tmp/1355252845866 n5) &
    haggletest app1 -f /tmp/test_output.1355252845870.n5.0 -s 45 -c sub n18
    haggletest app2 -f /tmp/test_output.1355252845870.n5.1 -s 45 -c sub n28
    haggletest app3 -f /tmp/test_output.1355252845870.n5.2 -s 45 -c sub n13
    haggletest app4 -f /tmp/test_output.1355252845870.n5.3 -s 45 -c sub n28
    haggletest app5 -f /tmp/test_output.1355252845870.n5.4 -s 45 -c sub n2
    haggletest app6 -f /tmp/test_output.1355252845870.n5.5 -s 45 -c sub n29
    haggletest app7 -f /tmp/test_output.1355252845870.n5.6 -s 45 -c sub n2
    haggletest app8 -f /tmp/test_output.1355252845870.n5.7 -s 45 -c sub n12
    haggletest app9 -f /tmp/test_output.1355252845870.n5.8 -s 45 -c sub n22
    haggletest app10 -f /tmp/test_output.1355252845870.n5.9 -s 45 -c sub n8
    haggletest app11 -f /tmp/test_output.1355252845870.n5.10 -s 45 -c sub n30
    haggletest app12 -f /tmp/test_output.1355252845870.n5.11 -s 45 -c sub n25
    haggletest app13 -f /tmp/test_output.1355252845870.n5.12 -s 45 -c sub n12
    haggletest app14 -f /tmp/test_output.1355252845870.n5.13 -s 45 -c sub n22
    haggletest app15 -f /tmp/test_output.1355252845870.n5.14 -s 45 -c sub n16
    echo "/tmp/test_output.1355252845870.n5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n5.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n5.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n6" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n6 -b 30 -p 10 pub file /tmp/1355252845866 n6) &
    haggletest app1 -f /tmp/test_output.1355252845870.n6.0 -s 45 -c sub n12
    haggletest app2 -f /tmp/test_output.1355252845870.n6.1 -s 45 -c sub n29
    haggletest app3 -f /tmp/test_output.1355252845870.n6.2 -s 45 -c sub n28
    haggletest app4 -f /tmp/test_output.1355252845870.n6.3 -s 45 -c sub n10
    haggletest app5 -f /tmp/test_output.1355252845870.n6.4 -s 45 -c sub n3
    haggletest app6 -f /tmp/test_output.1355252845870.n6.5 -s 45 -c sub n30
    haggletest app7 -f /tmp/test_output.1355252845870.n6.6 -s 45 -c sub n21
    haggletest app8 -f /tmp/test_output.1355252845870.n6.7 -s 45 -c sub n2
    haggletest app9 -f /tmp/test_output.1355252845870.n6.8 -s 45 -c sub n9
    haggletest app10 -f /tmp/test_output.1355252845870.n6.9 -s 45 -c sub n17
    haggletest app11 -f /tmp/test_output.1355252845870.n6.10 -s 45 -c sub n3
    haggletest app12 -f /tmp/test_output.1355252845870.n6.11 -s 45 -c sub n11
    haggletest app13 -f /tmp/test_output.1355252845870.n6.12 -s 45 -c sub n9
    haggletest app14 -f /tmp/test_output.1355252845870.n6.13 -s 45 -c sub n2
    haggletest app15 -f /tmp/test_output.1355252845870.n6.14 -s 45 -c sub n23
    echo "/tmp/test_output.1355252845870.n6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n6.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n6.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n7" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n7 -b 30 -p 10 pub file /tmp/1355252845866 n7) &
    haggletest app1 -f /tmp/test_output.1355252845870.n7.0 -s 45 -c sub n25
    haggletest app2 -f /tmp/test_output.1355252845870.n7.1 -s 45 -c sub n18
    haggletest app3 -f /tmp/test_output.1355252845870.n7.2 -s 45 -c sub n2
    haggletest app4 -f /tmp/test_output.1355252845870.n7.3 -s 45 -c sub n25
    haggletest app5 -f /tmp/test_output.1355252845870.n7.4 -s 45 -c sub n12
    haggletest app6 -f /tmp/test_output.1355252845870.n7.5 -s 45 -c sub n18
    haggletest app7 -f /tmp/test_output.1355252845870.n7.6 -s 45 -c sub n9
    haggletest app8 -f /tmp/test_output.1355252845870.n7.7 -s 45 -c sub n23
    haggletest app9 -f /tmp/test_output.1355252845870.n7.8 -s 45 -c sub n1
    haggletest app10 -f /tmp/test_output.1355252845870.n7.9 -s 45 -c sub n12
    haggletest app11 -f /tmp/test_output.1355252845870.n7.10 -s 45 -c sub n11
    haggletest app12 -f /tmp/test_output.1355252845870.n7.11 -s 45 -c sub n6
    haggletest app13 -f /tmp/test_output.1355252845870.n7.12 -s 45 -c sub n4
    haggletest app14 -f /tmp/test_output.1355252845870.n7.13 -s 45 -c sub n18
    haggletest app15 -f /tmp/test_output.1355252845870.n7.14 -s 45 -c sub n6
    echo "/tmp/test_output.1355252845870.n7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n7.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n7.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n8" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n8 -b 30 -p 10 pub file /tmp/1355252845866 n8) &
    haggletest app1 -f /tmp/test_output.1355252845870.n8.0 -s 45 -c sub n7
    haggletest app2 -f /tmp/test_output.1355252845870.n8.1 -s 45 -c sub n3
    haggletest app3 -f /tmp/test_output.1355252845870.n8.2 -s 45 -c sub n16
    haggletest app4 -f /tmp/test_output.1355252845870.n8.3 -s 45 -c sub n2
    haggletest app5 -f /tmp/test_output.1355252845870.n8.4 -s 45 -c sub n17
    haggletest app6 -f /tmp/test_output.1355252845870.n8.5 -s 45 -c sub n21
    haggletest app7 -f /tmp/test_output.1355252845870.n8.6 -s 45 -c sub n25
    haggletest app8 -f /tmp/test_output.1355252845870.n8.7 -s 45 -c sub n24
    haggletest app9 -f /tmp/test_output.1355252845870.n8.8 -s 45 -c sub n12
    haggletest app10 -f /tmp/test_output.1355252845870.n8.9 -s 45 -c sub n15
    haggletest app11 -f /tmp/test_output.1355252845870.n8.10 -s 45 -c sub n7
    haggletest app12 -f /tmp/test_output.1355252845870.n8.11 -s 45 -c sub n13
    haggletest app13 -f /tmp/test_output.1355252845870.n8.12 -s 45 -c sub n30
    haggletest app14 -f /tmp/test_output.1355252845870.n8.13 -s 45 -c sub n17
    haggletest app15 -f /tmp/test_output.1355252845870.n8.14 -s 45 -c sub n6
    echo "/tmp/test_output.1355252845870.n8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n8.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n8.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n9" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n9 -b 30 -p 10 pub file /tmp/1355252845866 n9) &
    haggletest app1 -f /tmp/test_output.1355252845870.n9.0 -s 45 -c sub n13
    haggletest app2 -f /tmp/test_output.1355252845870.n9.1 -s 45 -c sub n16
    haggletest app3 -f /tmp/test_output.1355252845870.n9.2 -s 45 -c sub n12
    haggletest app4 -f /tmp/test_output.1355252845870.n9.3 -s 45 -c sub n27
    haggletest app5 -f /tmp/test_output.1355252845870.n9.4 -s 45 -c sub n30
    haggletest app6 -f /tmp/test_output.1355252845870.n9.5 -s 45 -c sub n21
    haggletest app7 -f /tmp/test_output.1355252845870.n9.6 -s 45 -c sub n14
    haggletest app8 -f /tmp/test_output.1355252845870.n9.7 -s 45 -c sub n4
    haggletest app9 -f /tmp/test_output.1355252845870.n9.8 -s 45 -c sub n1
    haggletest app10 -f /tmp/test_output.1355252845870.n9.9 -s 45 -c sub n15
    haggletest app11 -f /tmp/test_output.1355252845870.n9.10 -s 45 -c sub n7
    haggletest app12 -f /tmp/test_output.1355252845870.n9.11 -s 45 -c sub n26
    haggletest app13 -f /tmp/test_output.1355252845870.n9.12 -s 45 -c sub n29
    haggletest app14 -f /tmp/test_output.1355252845870.n9.13 -s 45 -c sub n28
    haggletest app15 -f /tmp/test_output.1355252845870.n9.14 -s 45 -c sub n12
    echo "/tmp/test_output.1355252845870.n9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n9.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n9.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n10" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n10 -b 30 -p 10 pub file /tmp/1355252845866 n10) &
    haggletest app1 -f /tmp/test_output.1355252845870.n10.0 -s 45 -c sub n5
    haggletest app2 -f /tmp/test_output.1355252845870.n10.1 -s 45 -c sub n6
    haggletest app3 -f /tmp/test_output.1355252845870.n10.2 -s 45 -c sub n22
    haggletest app4 -f /tmp/test_output.1355252845870.n10.3 -s 45 -c sub n6
    haggletest app5 -f /tmp/test_output.1355252845870.n10.4 -s 45 -c sub n17
    haggletest app6 -f /tmp/test_output.1355252845870.n10.5 -s 45 -c sub n6
    haggletest app7 -f /tmp/test_output.1355252845870.n10.6 -s 45 -c sub n14
    haggletest app8 -f /tmp/test_output.1355252845870.n10.7 -s 45 -c sub n23
    haggletest app9 -f /tmp/test_output.1355252845870.n10.8 -s 45 -c sub n5
    haggletest app10 -f /tmp/test_output.1355252845870.n10.9 -s 45 -c sub n8
    haggletest app11 -f /tmp/test_output.1355252845870.n10.10 -s 45 -c sub n30
    haggletest app12 -f /tmp/test_output.1355252845870.n10.11 -s 45 -c sub n26
    haggletest app13 -f /tmp/test_output.1355252845870.n10.12 -s 45 -c sub n30
    haggletest app14 -f /tmp/test_output.1355252845870.n10.13 -s 45 -c sub n21
    haggletest app15 -f /tmp/test_output.1355252845870.n10.14 -s 45 -c sub n26
    echo "/tmp/test_output.1355252845870.n10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n10.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n10.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n11" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n11 -b 30 -p 10 pub file /tmp/1355252845866 n11) &
    haggletest app1 -f /tmp/test_output.1355252845870.n11.0 -s 45 -c sub n22
    haggletest app2 -f /tmp/test_output.1355252845870.n11.1 -s 45 -c sub n15
    haggletest app3 -f /tmp/test_output.1355252845870.n11.2 -s 45 -c sub n19
    haggletest app4 -f /tmp/test_output.1355252845870.n11.3 -s 45 -c sub n26
    haggletest app5 -f /tmp/test_output.1355252845870.n11.4 -s 45 -c sub n12
    haggletest app6 -f /tmp/test_output.1355252845870.n11.5 -s 45 -c sub n22
    haggletest app7 -f /tmp/test_output.1355252845870.n11.6 -s 45 -c sub n16
    haggletest app8 -f /tmp/test_output.1355252845870.n11.7 -s 45 -c sub n1
    haggletest app9 -f /tmp/test_output.1355252845870.n11.8 -s 45 -c sub n7
    haggletest app10 -f /tmp/test_output.1355252845870.n11.9 -s 45 -c sub n13
    haggletest app11 -f /tmp/test_output.1355252845870.n11.10 -s 45 -c sub n14
    haggletest app12 -f /tmp/test_output.1355252845870.n11.11 -s 45 -c sub n21
    haggletest app13 -f /tmp/test_output.1355252845870.n11.12 -s 45 -c sub n18
    haggletest app14 -f /tmp/test_output.1355252845870.n11.13 -s 45 -c sub n1
    haggletest app15 -f /tmp/test_output.1355252845870.n11.14 -s 45 -c sub n4
    echo "/tmp/test_output.1355252845870.n11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n11.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n11.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n12" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n12 -b 30 -p 10 pub file /tmp/1355252845866 n12) &
    haggletest app1 -f /tmp/test_output.1355252845870.n12.0 -s 45 -c sub n27
    haggletest app2 -f /tmp/test_output.1355252845870.n12.1 -s 45 -c sub n20
    haggletest app3 -f /tmp/test_output.1355252845870.n12.2 -s 45 -c sub n30
    haggletest app4 -f /tmp/test_output.1355252845870.n12.3 -s 45 -c sub n5
    haggletest app5 -f /tmp/test_output.1355252845870.n12.4 -s 45 -c sub n10
    haggletest app6 -f /tmp/test_output.1355252845870.n12.5 -s 45 -c sub n16
    haggletest app7 -f /tmp/test_output.1355252845870.n12.6 -s 45 -c sub n3
    haggletest app8 -f /tmp/test_output.1355252845870.n12.7 -s 45 -c sub n9
    haggletest app9 -f /tmp/test_output.1355252845870.n12.8 -s 45 -c sub n13
    haggletest app10 -f /tmp/test_output.1355252845870.n12.9 -s 45 -c sub n20
    haggletest app11 -f /tmp/test_output.1355252845870.n12.10 -s 45 -c sub n25
    haggletest app12 -f /tmp/test_output.1355252845870.n12.11 -s 45 -c sub n9
    haggletest app13 -f /tmp/test_output.1355252845870.n12.12 -s 45 -c sub n10
    haggletest app14 -f /tmp/test_output.1355252845870.n12.13 -s 45 -c sub n13
    haggletest app15 -f /tmp/test_output.1355252845870.n12.14 -s 45 -c sub n23
    echo "/tmp/test_output.1355252845870.n12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n12.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n12.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n13" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n13 -b 30 -p 10 pub file /tmp/1355252845866 n13) &
    haggletest app1 -f /tmp/test_output.1355252845870.n13.0 -s 45 -c sub n18
    haggletest app2 -f /tmp/test_output.1355252845870.n13.1 -s 45 -c sub n8
    haggletest app3 -f /tmp/test_output.1355252845870.n13.2 -s 45 -c sub n19
    haggletest app4 -f /tmp/test_output.1355252845870.n13.3 -s 45 -c sub n23
    haggletest app5 -f /tmp/test_output.1355252845870.n13.4 -s 45 -c sub n17
    haggletest app6 -f /tmp/test_output.1355252845870.n13.5 -s 45 -c sub n23
    haggletest app7 -f /tmp/test_output.1355252845870.n13.6 -s 45 -c sub n25
    haggletest app8 -f /tmp/test_output.1355252845870.n13.7 -s 45 -c sub n26
    haggletest app9 -f /tmp/test_output.1355252845870.n13.8 -s 45 -c sub n10
    haggletest app10 -f /tmp/test_output.1355252845870.n13.9 -s 45 -c sub n16
    haggletest app11 -f /tmp/test_output.1355252845870.n13.10 -s 45 -c sub n20
    haggletest app12 -f /tmp/test_output.1355252845870.n13.11 -s 45 -c sub n25
    haggletest app13 -f /tmp/test_output.1355252845870.n13.12 -s 45 -c sub n20
    haggletest app14 -f /tmp/test_output.1355252845870.n13.13 -s 45 -c sub n7
    haggletest app15 -f /tmp/test_output.1355252845870.n13.14 -s 45 -c sub n5
    echo "/tmp/test_output.1355252845870.n13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n13.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n13.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n14" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n14 -b 30 -p 10 pub file /tmp/1355252845866 n14) &
    haggletest app1 -f /tmp/test_output.1355252845870.n14.0 -s 45 -c sub n22
    haggletest app2 -f /tmp/test_output.1355252845870.n14.1 -s 45 -c sub n9
    haggletest app3 -f /tmp/test_output.1355252845870.n14.2 -s 45 -c sub n28
    haggletest app4 -f /tmp/test_output.1355252845870.n14.3 -s 45 -c sub n3
    haggletest app5 -f /tmp/test_output.1355252845870.n14.4 -s 45 -c sub n10
    haggletest app6 -f /tmp/test_output.1355252845870.n14.5 -s 45 -c sub n11
    haggletest app7 -f /tmp/test_output.1355252845870.n14.6 -s 45 -c sub n26
    haggletest app8 -f /tmp/test_output.1355252845870.n14.7 -s 45 -c sub n2
    haggletest app9 -f /tmp/test_output.1355252845870.n14.8 -s 45 -c sub n28
    haggletest app10 -f /tmp/test_output.1355252845870.n14.9 -s 45 -c sub n19
    haggletest app11 -f /tmp/test_output.1355252845870.n14.10 -s 45 -c sub n18
    haggletest app12 -f /tmp/test_output.1355252845870.n14.11 -s 45 -c sub n21
    haggletest app13 -f /tmp/test_output.1355252845870.n14.12 -s 45 -c sub n1
    haggletest app14 -f /tmp/test_output.1355252845870.n14.13 -s 45 -c sub n21
    haggletest app15 -f /tmp/test_output.1355252845870.n14.14 -s 45 -c sub n1
    echo "/tmp/test_output.1355252845870.n14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n14.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n14.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n15" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n15 -b 30 -p 10 pub file /tmp/1355252845866 n15) &
    haggletest app1 -f /tmp/test_output.1355252845870.n15.0 -s 45 -c sub n14
    haggletest app2 -f /tmp/test_output.1355252845870.n15.1 -s 45 -c sub n3
    haggletest app3 -f /tmp/test_output.1355252845870.n15.2 -s 45 -c sub n1
    haggletest app4 -f /tmp/test_output.1355252845870.n15.3 -s 45 -c sub n9
    haggletest app5 -f /tmp/test_output.1355252845870.n15.4 -s 45 -c sub n13
    haggletest app6 -f /tmp/test_output.1355252845870.n15.5 -s 45 -c sub n26
    haggletest app7 -f /tmp/test_output.1355252845870.n15.6 -s 45 -c sub n14
    haggletest app8 -f /tmp/test_output.1355252845870.n15.7 -s 45 -c sub n24
    haggletest app9 -f /tmp/test_output.1355252845870.n15.8 -s 45 -c sub n8
    haggletest app10 -f /tmp/test_output.1355252845870.n15.9 -s 45 -c sub n2
    haggletest app11 -f /tmp/test_output.1355252845870.n15.10 -s 45 -c sub n1
    haggletest app12 -f /tmp/test_output.1355252845870.n15.11 -s 45 -c sub n25
    haggletest app13 -f /tmp/test_output.1355252845870.n15.12 -s 45 -c sub n8
    haggletest app14 -f /tmp/test_output.1355252845870.n15.13 -s 45 -c sub n14
    haggletest app15 -f /tmp/test_output.1355252845870.n15.14 -s 45 -c sub n3
    echo "/tmp/test_output.1355252845870.n15" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n15.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n15.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n16" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n16 -b 30 -p 10 pub file /tmp/1355252845866 n16) &
    haggletest app1 -f /tmp/test_output.1355252845870.n16.0 -s 45 -c sub n6
    haggletest app2 -f /tmp/test_output.1355252845870.n16.1 -s 45 -c sub n28
    haggletest app3 -f /tmp/test_output.1355252845870.n16.2 -s 45 -c sub n1
    haggletest app4 -f /tmp/test_output.1355252845870.n16.3 -s 45 -c sub n4
    haggletest app5 -f /tmp/test_output.1355252845870.n16.4 -s 45 -c sub n25
    haggletest app6 -f /tmp/test_output.1355252845870.n16.5 -s 45 -c sub n11
    haggletest app7 -f /tmp/test_output.1355252845870.n16.6 -s 45 -c sub n14
    haggletest app8 -f /tmp/test_output.1355252845870.n16.7 -s 45 -c sub n15
    haggletest app9 -f /tmp/test_output.1355252845870.n16.8 -s 45 -c sub n30
    haggletest app10 -f /tmp/test_output.1355252845870.n16.9 -s 45 -c sub n18
    haggletest app11 -f /tmp/test_output.1355252845870.n16.10 -s 45 -c sub n19
    haggletest app12 -f /tmp/test_output.1355252845870.n16.11 -s 45 -c sub n25
    haggletest app13 -f /tmp/test_output.1355252845870.n16.12 -s 45 -c sub n19
    haggletest app14 -f /tmp/test_output.1355252845870.n16.13 -s 45 -c sub n3
    haggletest app15 -f /tmp/test_output.1355252845870.n16.14 -s 45 -c sub n19
    echo "/tmp/test_output.1355252845870.n16" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n16.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n16.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n17" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n17 -b 30 -p 10 pub file /tmp/1355252845866 n17) &
    haggletest app1 -f /tmp/test_output.1355252845870.n17.0 -s 45 -c sub n21
    haggletest app2 -f /tmp/test_output.1355252845870.n17.1 -s 45 -c sub n4
    haggletest app3 -f /tmp/test_output.1355252845870.n17.2 -s 45 -c sub n6
    haggletest app4 -f /tmp/test_output.1355252845870.n17.3 -s 45 -c sub n12
    haggletest app5 -f /tmp/test_output.1355252845870.n17.4 -s 45 -c sub n19
    haggletest app6 -f /tmp/test_output.1355252845870.n17.5 -s 45 -c sub n25
    haggletest app7 -f /tmp/test_output.1355252845870.n17.6 -s 45 -c sub n23
    haggletest app8 -f /tmp/test_output.1355252845870.n17.7 -s 45 -c sub n2
    haggletest app9 -f /tmp/test_output.1355252845870.n17.8 -s 45 -c sub n28
    haggletest app10 -f /tmp/test_output.1355252845870.n17.9 -s 45 -c sub n12
    haggletest app11 -f /tmp/test_output.1355252845870.n17.10 -s 45 -c sub n13
    haggletest app12 -f /tmp/test_output.1355252845870.n17.11 -s 45 -c sub n29
    haggletest app13 -f /tmp/test_output.1355252845870.n17.12 -s 45 -c sub n14
    haggletest app14 -f /tmp/test_output.1355252845870.n17.13 -s 45 -c sub n2
    haggletest app15 -f /tmp/test_output.1355252845870.n17.14 -s 45 -c sub n28
    echo "/tmp/test_output.1355252845870.n17" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n17.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n17.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n18" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n18 -b 30 -p 10 pub file /tmp/1355252845866 n18) &
    haggletest app1 -f /tmp/test_output.1355252845870.n18.0 -s 45 -c sub n28
    haggletest app2 -f /tmp/test_output.1355252845870.n18.1 -s 45 -c sub n14
    haggletest app3 -f /tmp/test_output.1355252845870.n18.2 -s 45 -c sub n30
    haggletest app4 -f /tmp/test_output.1355252845870.n18.3 -s 45 -c sub n14
    haggletest app5 -f /tmp/test_output.1355252845870.n18.4 -s 45 -c sub n28
    haggletest app6 -f /tmp/test_output.1355252845870.n18.5 -s 45 -c sub n25
    haggletest app7 -f /tmp/test_output.1355252845870.n18.6 -s 45 -c sub n19
    haggletest app8 -f /tmp/test_output.1355252845870.n18.7 -s 45 -c sub n3
    haggletest app9 -f /tmp/test_output.1355252845870.n18.8 -s 45 -c sub n24
    haggletest app10 -f /tmp/test_output.1355252845870.n18.9 -s 45 -c sub n5
    haggletest app11 -f /tmp/test_output.1355252845870.n18.10 -s 45 -c sub n3
    haggletest app12 -f /tmp/test_output.1355252845870.n18.11 -s 45 -c sub n15
    haggletest app13 -f /tmp/test_output.1355252845870.n18.12 -s 45 -c sub n20
    haggletest app14 -f /tmp/test_output.1355252845870.n18.13 -s 45 -c sub n21
    haggletest app15 -f /tmp/test_output.1355252845870.n18.14 -s 45 -c sub n30
    echo "/tmp/test_output.1355252845870.n18" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n18.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n18.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n19" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n19 -b 30 -p 10 pub file /tmp/1355252845866 n19) &
    haggletest app1 -f /tmp/test_output.1355252845870.n19.0 -s 45 -c sub n23
    haggletest app2 -f /tmp/test_output.1355252845870.n19.1 -s 45 -c sub n11
    haggletest app3 -f /tmp/test_output.1355252845870.n19.2 -s 45 -c sub n17
    haggletest app4 -f /tmp/test_output.1355252845870.n19.3 -s 45 -c sub n7
    haggletest app5 -f /tmp/test_output.1355252845870.n19.4 -s 45 -c sub n13
    haggletest app6 -f /tmp/test_output.1355252845870.n19.5 -s 45 -c sub n23
    haggletest app7 -f /tmp/test_output.1355252845870.n19.6 -s 45 -c sub n10
    haggletest app8 -f /tmp/test_output.1355252845870.n19.7 -s 45 -c sub n22
    haggletest app9 -f /tmp/test_output.1355252845870.n19.8 -s 45 -c sub n4
    haggletest app10 -f /tmp/test_output.1355252845870.n19.9 -s 45 -c sub n10
    haggletest app11 -f /tmp/test_output.1355252845870.n19.10 -s 45 -c sub n29
    haggletest app12 -f /tmp/test_output.1355252845870.n19.11 -s 45 -c sub n21
    haggletest app13 -f /tmp/test_output.1355252845870.n19.12 -s 45 -c sub n1
    haggletest app14 -f /tmp/test_output.1355252845870.n19.13 -s 45 -c sub n20
    haggletest app15 -f /tmp/test_output.1355252845870.n19.14 -s 45 -c sub n6
    echo "/tmp/test_output.1355252845870.n19" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n19.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n19.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n20" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n20 -b 30 -p 10 pub file /tmp/1355252845866 n20) &
    haggletest app1 -f /tmp/test_output.1355252845870.n20.0 -s 45 -c sub n18
    haggletest app2 -f /tmp/test_output.1355252845870.n20.1 -s 45 -c sub n10
    haggletest app3 -f /tmp/test_output.1355252845870.n20.2 -s 45 -c sub n2
    haggletest app4 -f /tmp/test_output.1355252845870.n20.3 -s 45 -c sub n11
    haggletest app5 -f /tmp/test_output.1355252845870.n20.4 -s 45 -c sub n19
    haggletest app6 -f /tmp/test_output.1355252845870.n20.5 -s 45 -c sub n10
    haggletest app7 -f /tmp/test_output.1355252845870.n20.6 -s 45 -c sub n19
    haggletest app8 -f /tmp/test_output.1355252845870.n20.7 -s 45 -c sub n8
    haggletest app9 -f /tmp/test_output.1355252845870.n20.8 -s 45 -c sub n24
    haggletest app10 -f /tmp/test_output.1355252845870.n20.9 -s 45 -c sub n11
    haggletest app11 -f /tmp/test_output.1355252845870.n20.10 -s 45 -c sub n13
    haggletest app12 -f /tmp/test_output.1355252845870.n20.11 -s 45 -c sub n24
    haggletest app13 -f /tmp/test_output.1355252845870.n20.12 -s 45 -c sub n16
    haggletest app14 -f /tmp/test_output.1355252845870.n20.13 -s 45 -c sub n25
    haggletest app15 -f /tmp/test_output.1355252845870.n20.14 -s 45 -c sub n4
    echo "/tmp/test_output.1355252845870.n20" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n20.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n20.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n21" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n21 -b 30 -p 10 pub file /tmp/1355252845866 n21) &
    haggletest app1 -f /tmp/test_output.1355252845870.n21.0 -s 45 -c sub n1
    haggletest app2 -f /tmp/test_output.1355252845870.n21.1 -s 45 -c sub n8
    haggletest app3 -f /tmp/test_output.1355252845870.n21.2 -s 45 -c sub n9
    haggletest app4 -f /tmp/test_output.1355252845870.n21.3 -s 45 -c sub n25
    haggletest app5 -f /tmp/test_output.1355252845870.n21.4 -s 45 -c sub n6
    haggletest app6 -f /tmp/test_output.1355252845870.n21.5 -s 45 -c sub n27
    haggletest app7 -f /tmp/test_output.1355252845870.n21.6 -s 45 -c sub n26
    haggletest app8 -f /tmp/test_output.1355252845870.n21.7 -s 45 -c sub n12
    haggletest app9 -f /tmp/test_output.1355252845870.n21.8 -s 45 -c sub n29
    haggletest app10 -f /tmp/test_output.1355252845870.n21.9 -s 45 -c sub n26
    haggletest app11 -f /tmp/test_output.1355252845870.n21.10 -s 45 -c sub n27
    haggletest app12 -f /tmp/test_output.1355252845870.n21.11 -s 45 -c sub n22
    haggletest app13 -f /tmp/test_output.1355252845870.n21.12 -s 45 -c sub n15
    haggletest app14 -f /tmp/test_output.1355252845870.n21.13 -s 45 -c sub n30
    haggletest app15 -f /tmp/test_output.1355252845870.n21.14 -s 45 -c sub n17
    echo "/tmp/test_output.1355252845870.n21" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n21.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n21.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n22" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n22 -b 30 -p 10 pub file /tmp/1355252845866 n22) &
    haggletest app1 -f /tmp/test_output.1355252845870.n22.0 -s 45 -c sub n7
    haggletest app2 -f /tmp/test_output.1355252845870.n22.1 -s 45 -c sub n12
    haggletest app3 -f /tmp/test_output.1355252845870.n22.2 -s 45 -c sub n21
    haggletest app4 -f /tmp/test_output.1355252845870.n22.3 -s 45 -c sub n23
    haggletest app5 -f /tmp/test_output.1355252845870.n22.4 -s 45 -c sub n27
    haggletest app6 -f /tmp/test_output.1355252845870.n22.5 -s 45 -c sub n9
    haggletest app7 -f /tmp/test_output.1355252845870.n22.6 -s 45 -c sub n19
    haggletest app8 -f /tmp/test_output.1355252845870.n22.7 -s 45 -c sub n14
    haggletest app9 -f /tmp/test_output.1355252845870.n22.8 -s 45 -c sub n24
    haggletest app10 -f /tmp/test_output.1355252845870.n22.9 -s 45 -c sub n20
    haggletest app11 -f /tmp/test_output.1355252845870.n22.10 -s 45 -c sub n13
    haggletest app12 -f /tmp/test_output.1355252845870.n22.11 -s 45 -c sub n8
    haggletest app13 -f /tmp/test_output.1355252845870.n22.12 -s 45 -c sub n2
    haggletest app14 -f /tmp/test_output.1355252845870.n22.13 -s 45 -c sub n9
    haggletest app15 -f /tmp/test_output.1355252845870.n22.14 -s 45 -c sub n11
    echo "/tmp/test_output.1355252845870.n22" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n22.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n22.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n23" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n23 -b 30 -p 10 pub file /tmp/1355252845866 n23) &
    haggletest app1 -f /tmp/test_output.1355252845870.n23.0 -s 45 -c sub n21
    haggletest app2 -f /tmp/test_output.1355252845870.n23.1 -s 45 -c sub n17
    haggletest app3 -f /tmp/test_output.1355252845870.n23.2 -s 45 -c sub n9
    haggletest app4 -f /tmp/test_output.1355252845870.n23.3 -s 45 -c sub n15
    haggletest app5 -f /tmp/test_output.1355252845870.n23.4 -s 45 -c sub n26
    haggletest app6 -f /tmp/test_output.1355252845870.n23.5 -s 45 -c sub n18
    haggletest app7 -f /tmp/test_output.1355252845870.n23.6 -s 45 -c sub n30
    haggletest app8 -f /tmp/test_output.1355252845870.n23.7 -s 45 -c sub n4
    haggletest app9 -f /tmp/test_output.1355252845870.n23.8 -s 45 -c sub n21
    haggletest app10 -f /tmp/test_output.1355252845870.n23.9 -s 45 -c sub n7
    haggletest app11 -f /tmp/test_output.1355252845870.n23.10 -s 45 -c sub n27
    haggletest app12 -f /tmp/test_output.1355252845870.n23.11 -s 45 -c sub n28
    haggletest app13 -f /tmp/test_output.1355252845870.n23.12 -s 45 -c sub n7
    haggletest app14 -f /tmp/test_output.1355252845870.n23.13 -s 45 -c sub n5
    haggletest app15 -f /tmp/test_output.1355252845870.n23.14 -s 45 -c sub n13
    echo "/tmp/test_output.1355252845870.n23" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n23.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n23.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n24" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n24 -b 30 -p 10 pub file /tmp/1355252845866 n24) &
    haggletest app1 -f /tmp/test_output.1355252845870.n24.0 -s 45 -c sub n16
    haggletest app2 -f /tmp/test_output.1355252845870.n24.1 -s 45 -c sub n23
    haggletest app3 -f /tmp/test_output.1355252845870.n24.2 -s 45 -c sub n4
    haggletest app4 -f /tmp/test_output.1355252845870.n24.3 -s 45 -c sub n21
    haggletest app5 -f /tmp/test_output.1355252845870.n24.4 -s 45 -c sub n30
    haggletest app6 -f /tmp/test_output.1355252845870.n24.5 -s 45 -c sub n6
    haggletest app7 -f /tmp/test_output.1355252845870.n24.6 -s 45 -c sub n23
    haggletest app8 -f /tmp/test_output.1355252845870.n24.7 -s 45 -c sub n19
    haggletest app9 -f /tmp/test_output.1355252845870.n24.8 -s 45 -c sub n11
    haggletest app10 -f /tmp/test_output.1355252845870.n24.9 -s 45 -c sub n29
    haggletest app11 -f /tmp/test_output.1355252845870.n24.10 -s 45 -c sub n2
    haggletest app12 -f /tmp/test_output.1355252845870.n24.11 -s 45 -c sub n21
    haggletest app13 -f /tmp/test_output.1355252845870.n24.12 -s 45 -c sub n30
    haggletest app14 -f /tmp/test_output.1355252845870.n24.13 -s 45 -c sub n15
    haggletest app15 -f /tmp/test_output.1355252845870.n24.14 -s 45 -c sub n5
    echo "/tmp/test_output.1355252845870.n24" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n24.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n24.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n25" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n25 -b 30 -p 10 pub file /tmp/1355252845866 n25) &
    haggletest app1 -f /tmp/test_output.1355252845870.n25.0 -s 45 -c sub n1
    haggletest app2 -f /tmp/test_output.1355252845870.n25.1 -s 45 -c sub n21
    haggletest app3 -f /tmp/test_output.1355252845870.n25.2 -s 45 -c sub n15
    haggletest app4 -f /tmp/test_output.1355252845870.n25.3 -s 45 -c sub n6
    haggletest app5 -f /tmp/test_output.1355252845870.n25.4 -s 45 -c sub n29
    haggletest app6 -f /tmp/test_output.1355252845870.n25.5 -s 45 -c sub n27
    haggletest app7 -f /tmp/test_output.1355252845870.n25.6 -s 45 -c sub n5
    haggletest app8 -f /tmp/test_output.1355252845870.n25.7 -s 45 -c sub n8
    haggletest app9 -f /tmp/test_output.1355252845870.n25.8 -s 45 -c sub n9
    haggletest app10 -f /tmp/test_output.1355252845870.n25.9 -s 45 -c sub n19
    haggletest app11 -f /tmp/test_output.1355252845870.n25.10 -s 45 -c sub n23
    haggletest app12 -f /tmp/test_output.1355252845870.n25.11 -s 45 -c sub n14
    haggletest app13 -f /tmp/test_output.1355252845870.n25.12 -s 45 -c sub n29
    haggletest app14 -f /tmp/test_output.1355252845870.n25.13 -s 45 -c sub n18
    haggletest app15 -f /tmp/test_output.1355252845870.n25.14 -s 45 -c sub n3
    echo "/tmp/test_output.1355252845870.n25" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n25.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n25.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n26" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n26 -b 30 -p 10 pub file /tmp/1355252845866 n26) &
    haggletest app1 -f /tmp/test_output.1355252845870.n26.0 -s 45 -c sub n19
    haggletest app2 -f /tmp/test_output.1355252845870.n26.1 -s 45 -c sub n29
    haggletest app3 -f /tmp/test_output.1355252845870.n26.2 -s 45 -c sub n16
    haggletest app4 -f /tmp/test_output.1355252845870.n26.3 -s 45 -c sub n2
    haggletest app5 -f /tmp/test_output.1355252845870.n26.4 -s 45 -c sub n24
    haggletest app6 -f /tmp/test_output.1355252845870.n26.5 -s 45 -c sub n11
    haggletest app7 -f /tmp/test_output.1355252845870.n26.6 -s 45 -c sub n3
    haggletest app8 -f /tmp/test_output.1355252845870.n26.7 -s 45 -c sub n12
    haggletest app9 -f /tmp/test_output.1355252845870.n26.8 -s 45 -c sub n17
    haggletest app10 -f /tmp/test_output.1355252845870.n26.9 -s 45 -c sub n22
    haggletest app11 -f /tmp/test_output.1355252845870.n26.10 -s 45 -c sub n11
    haggletest app12 -f /tmp/test_output.1355252845870.n26.11 -s 45 -c sub n1
    haggletest app13 -f /tmp/test_output.1355252845870.n26.12 -s 45 -c sub n10
    haggletest app14 -f /tmp/test_output.1355252845870.n26.13 -s 45 -c sub n4
    haggletest app15 -f /tmp/test_output.1355252845870.n26.14 -s 45 -c sub n16
    echo "/tmp/test_output.1355252845870.n26" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n26.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n26.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n27" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n27 -b 30 -p 10 pub file /tmp/1355252845866 n27) &
    haggletest app1 -f /tmp/test_output.1355252845870.n27.0 -s 45 -c sub n9
    haggletest app2 -f /tmp/test_output.1355252845870.n27.1 -s 45 -c sub n25
    haggletest app3 -f /tmp/test_output.1355252845870.n27.2 -s 45 -c sub n1
    haggletest app4 -f /tmp/test_output.1355252845870.n27.3 -s 45 -c sub n10
    haggletest app5 -f /tmp/test_output.1355252845870.n27.4 -s 45 -c sub n14
    haggletest app6 -f /tmp/test_output.1355252845870.n27.5 -s 45 -c sub n8
    haggletest app7 -f /tmp/test_output.1355252845870.n27.6 -s 45 -c sub n28
    haggletest app8 -f /tmp/test_output.1355252845870.n27.7 -s 45 -c sub n16
    haggletest app9 -f /tmp/test_output.1355252845870.n27.8 -s 45 -c sub n15
    haggletest app10 -f /tmp/test_output.1355252845870.n27.9 -s 45 -c sub n11
    haggletest app11 -f /tmp/test_output.1355252845870.n27.10 -s 45 -c sub n30
    haggletest app12 -f /tmp/test_output.1355252845870.n27.11 -s 45 -c sub n11
    haggletest app13 -f /tmp/test_output.1355252845870.n27.12 -s 45 -c sub n3
    haggletest app14 -f /tmp/test_output.1355252845870.n27.13 -s 45 -c sub n2
    haggletest app15 -f /tmp/test_output.1355252845870.n27.14 -s 45 -c sub n6
    echo "/tmp/test_output.1355252845870.n27" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n27.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n27.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n28" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n28 -b 30 -p 10 pub file /tmp/1355252845866 n28) &
    haggletest app1 -f /tmp/test_output.1355252845870.n28.0 -s 45 -c sub n11
    haggletest app2 -f /tmp/test_output.1355252845870.n28.1 -s 45 -c sub n8
    haggletest app3 -f /tmp/test_output.1355252845870.n28.2 -s 45 -c sub n15
    haggletest app4 -f /tmp/test_output.1355252845870.n28.3 -s 45 -c sub n27
    haggletest app5 -f /tmp/test_output.1355252845870.n28.4 -s 45 -c sub n17
    haggletest app6 -f /tmp/test_output.1355252845870.n28.5 -s 45 -c sub n19
    haggletest app7 -f /tmp/test_output.1355252845870.n28.6 -s 45 -c sub n12
    haggletest app8 -f /tmp/test_output.1355252845870.n28.7 -s 45 -c sub n6
    haggletest app9 -f /tmp/test_output.1355252845870.n28.8 -s 45 -c sub n17
    haggletest app10 -f /tmp/test_output.1355252845870.n28.9 -s 45 -c sub n5
    haggletest app11 -f /tmp/test_output.1355252845870.n28.10 -s 45 -c sub n3
    haggletest app12 -f /tmp/test_output.1355252845870.n28.11 -s 45 -c sub n17
    haggletest app13 -f /tmp/test_output.1355252845870.n28.12 -s 45 -c sub n21
    haggletest app14 -f /tmp/test_output.1355252845870.n28.13 -s 45 -c sub n1
    haggletest app15 -f /tmp/test_output.1355252845870.n28.14 -s 45 -c sub n7
    echo "/tmp/test_output.1355252845870.n28" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n28.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n28.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n29" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n29 -b 30 -p 10 pub file /tmp/1355252845866 n29) &
    haggletest app1 -f /tmp/test_output.1355252845870.n29.0 -s 45 -c sub n27
    haggletest app2 -f /tmp/test_output.1355252845870.n29.1 -s 45 -c sub n22
    haggletest app3 -f /tmp/test_output.1355252845870.n29.2 -s 45 -c sub n10
    haggletest app4 -f /tmp/test_output.1355252845870.n29.3 -s 45 -c sub n28
    haggletest app5 -f /tmp/test_output.1355252845870.n29.4 -s 45 -c sub n22
    haggletest app6 -f /tmp/test_output.1355252845870.n29.5 -s 45 -c sub n30
    haggletest app7 -f /tmp/test_output.1355252845870.n29.6 -s 45 -c sub n1
    haggletest app8 -f /tmp/test_output.1355252845870.n29.7 -s 45 -c sub n13
    haggletest app9 -f /tmp/test_output.1355252845870.n29.8 -s 45 -c sub n20
    haggletest app10 -f /tmp/test_output.1355252845870.n29.9 -s 45 -c sub n21
    haggletest app11 -f /tmp/test_output.1355252845870.n29.10 -s 45 -c sub n3
    haggletest app12 -f /tmp/test_output.1355252845870.n29.11 -s 45 -c sub n10
    haggletest app13 -f /tmp/test_output.1355252845870.n29.12 -s 45 -c sub n20
    haggletest app14 -f /tmp/test_output.1355252845870.n29.13 -s 45 -c sub n17
    haggletest app15 -f /tmp/test_output.1355252845870.n29.14 -s 45 -c sub n4
    echo "/tmp/test_output.1355252845870.n29" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n29.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n29.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n30" ]; then
    (haggletest app0 -f /tmp/test_output.1355252845870.n30 -b 30 -p 10 pub file /tmp/1355252845866 n30) &
    haggletest app1 -f /tmp/test_output.1355252845870.n30.0 -s 45 -c sub n20
    haggletest app2 -f /tmp/test_output.1355252845870.n30.1 -s 45 -c sub n19
    haggletest app3 -f /tmp/test_output.1355252845870.n30.2 -s 45 -c sub n6
    haggletest app4 -f /tmp/test_output.1355252845870.n30.3 -s 45 -c sub n10
    haggletest app5 -f /tmp/test_output.1355252845870.n30.4 -s 45 -c sub n1
    haggletest app6 -f /tmp/test_output.1355252845870.n30.5 -s 45 -c sub n19
    haggletest app7 -f /tmp/test_output.1355252845870.n30.6 -s 45 -c sub n20
    haggletest app8 -f /tmp/test_output.1355252845870.n30.7 -s 45 -c sub n14
    haggletest app9 -f /tmp/test_output.1355252845870.n30.8 -s 45 -c sub n17
    haggletest app10 -f /tmp/test_output.1355252845870.n30.9 -s 45 -c sub n18
    haggletest app11 -f /tmp/test_output.1355252845870.n30.10 -s 45 -c sub n10
    haggletest app12 -f /tmp/test_output.1355252845870.n30.11 -s 45 -c sub n1
    haggletest app13 -f /tmp/test_output.1355252845870.n30.12 -s 45 -c sub n20
    haggletest app14 -f /tmp/test_output.1355252845870.n30.13 -s 45 -c sub n27
    haggletest app15 -f /tmp/test_output.1355252845870.n30.14 -s 45 -c sub n13
    echo "/tmp/test_output.1355252845870.n30" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.0" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.0 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.1" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.1 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.2" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.2 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.3" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.3 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.4" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.4 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.5" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.5 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.6" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.6 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.7" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.7 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.8" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.8 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.9" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.9 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.10" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.10 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.11" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.11 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.12" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.12 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.13" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.13 >> ${TFILE}
    echo "/tmp/test_output.1355252845870.n30.14" >> ${TFILE}
    cat /tmp/test_output.1355252845870.n30.14 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
else
    exit 0
fi
cat ${TFILE} >> $2
