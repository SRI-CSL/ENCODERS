#!/bin/bash
TFILE="/tmp/$1_temp_file"
rm -f ${TFILE}
sleep 100 
if [ $1 == "n0" ]; then
    echo "bogus node"
elif [ $1 == "n1" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
    /sbin/ifconfig eth1 >> ${TFILE}
elif [ $1 == "n2" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n2 -l -p 3 pub file /tmp/n2.gps ContentType=gps ContentOrigin=n2) &
    (haggletest app1 -f /tmp/test_output.phymon.n2 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n2) &
    (haggletest app2 -f /tmp/test_output.opordmap.n2 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n3" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n4" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n5" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n6" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n7" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n8" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n9" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n10" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n11" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n12" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n13" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n14" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n15" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n16" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n17" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n18" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n19" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
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
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n22" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n23" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n24" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n25" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n26" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n27" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n28" ]; then
    (haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub file /tmp/n1.gps ContentType=gps ContentOrigin=n1) &
    (haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 pub file /tmp/n1.phymon ContentType=PHYSMON ContentOrigin=n1) &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map) 
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/oportxt1 data=opordtype=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt) 
    haggletest app3 -f /tmp/test_output.gps.n1 sub ContentType=gps &
    haggletest app4 -s 200 -f /tmp/test_output.opord.n1  sub data=opord &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/test_output.gps.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1 >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
else
    exit 0
fi
cat ${TFILE} >> $2
