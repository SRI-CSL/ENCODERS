#!/bin/bash
TFILE="/tmp/$1_temp_file"
rm -f ${TFILE}
sleep 120
if [ $1 == "n0" ]; then
    echo "bogus node"
elif [ $1 == "n1" ]; then
    haggletest app0 -f /tmp/test_output.gps.n1 -l -p 3 pub ContentType=gps ContentOrigin=n1 &
    haggletest app1 -f /tmp/test_output.phymon.n1 -l -p 30 ContentType=PHYSMON ContentOrigin=n1 &
    (haggletest app2 -f /tmp/test_output.opordmap.n1 pub file /tmp/opordmap data=opord:10 type=map)
    (haggletest app2 -f /tmp/test_output.opordtext1.n1 pub file /tmp/opordtxt1 data=opordtype=txt)
    (haggletest app2 -f /tmp/test_output.opordtext2.n1 pub file /tmp/opordtxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n1 pub file /tmp/opordtxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n1 pub file /tmp/opordtxt4 data=opord:10 type=txt)
    haggletest app3 -f output.gps.n1 sub ContentType=gps &
    haggletest app4 -f output.gps.n1 -s 200 sub data=opord:10 &
    sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    sleep 600
    (haggletest app2 -f /tmp/test_output.evidence1.n1 pub file /tmp/evidence1 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence2.n1 pub file /tmp/evidence2 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence3.n1 pub file /tmp/evidence3 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence4.n1 pub file /tmp/evidence4 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence5.n1 pub file /tmp/evidence5 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence6.n1 pub file /tmp/evidence6 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence7.n1 pub file /tmp/evidence7 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence8.n1 pub file /tmp/evidence8 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence9.n1 pub file /tmp/evidence9 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence10.n1 pub file /tmp/evidence10 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence11.n1 pub file /tmp/evidence11 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence12.n1 pub file /tmp/evidence12 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence13.n1 pub file /tmp/evidence13 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence14.n1 pub file /tmp/evidence14 data=opord:10 type=add)
    (haggletest app2 -f /tmp/test_output.evidence15.n1 pub file /tmp/evidence15 data=opord:10 type=add)

    echo "/tmp/test_output.gps.n1" >> ${TFILE}
    cat /tmp/testout_gps.n1 >> ${TFILE}
    echo "/tmp/test_output.physmon.n1" >> ${TFILE}
    cat /tmp/test_output.physmon.n1 >> ${TFILE}
    echo "/tmp/test_output.opord.n1" >> ${TFILE}
    cat /tmp/test_output.opord.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n1" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n1 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n1" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n1  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n2" ]; then
    haggletest app0 -f /tmp/test_output.gps.n2 -l -p 3 pub ContentType=gps ContentOrigin=n2 &
    haggletest app1 -f /tmp/test_output.phymon.n2 -l -p 30 ContentType=PHYSMON ContentOrigin=n2 &
    haggletest app3 -f output.gps.n2 sub ContentType=gps &
    haggletest app4 -f output.gps.n2 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n2" >> ${TFILE}
    cat /tmp/testout_gps.n2 >> ${TFILE}
    echo "/tmp/test_output.physmon.n2" >> ${TFILE}
    cat /tmp/test_output.physmon.n2 >> ${TFILE}
    echo "/tmp/test_output.opord.n2" >> ${TFILE}
    cat /tmp/test_output.opord.n2 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n2" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n2 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n2" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n2  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n3" ]; then
    haggletest app0 -f /tmp/test_output.gps.n3 -l -p 3 pub ContentType=gps ContentOrigin=n3 &
    haggletest app1 -f /tmp/test_output.phymon.n3 -l -p 30 ContentType=PHYSMON ContentOrigin=n3 &
    haggletest app3 -f output.gps.n3 sub ContentType=gps &
    haggletest app4 -f output.gps.n3 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n3" >> ${TFILE}
    cat /tmp/testout_gps.n3 >> ${TFILE}
    echo "/tmp/test_output.physmon.n3" >> ${TFILE}
    cat /tmp/test_output.physmon.n3 >> ${TFILE}
    echo "/tmp/test_output.opord.n3" >> ${TFILE}
    cat /tmp/test_output.opord.n3 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n3" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n3 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n3" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n3  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n4" ]; then
    haggletest app0 -f /tmp/test_output.gps.n4 -l -p 3 pub ContentType=gps ContentOrigin=n4 &
    haggletest app1 -f /tmp/test_output.phymon.n4 -l -p 30 ContentType=PHYSMON ContentOrigin=n4 &
    haggletest app3 -f output.gps.n4 sub ContentType=gps &
    haggletest app4 -f output.gps.n4 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n4" >> ${TFILE}
    cat /tmp/testout_gps.n4 >> ${TFILE}
    echo "/tmp/test_output.physmon.n4" >> ${TFILE}
    cat /tmp/test_output.physmon.n4 >> ${TFILE}
    echo "/tmp/test_output.opord.n4" >> ${TFILE}
    cat /tmp/test_output.opord.n4 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n4" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n4 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n4" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n4  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n5" ]; then
    haggletest app0 -f /tmp/test_output.gps.n5 -l -p 3 pub ContentType=gps ContentOrigin=n5 &
    haggletest app1 -f /tmp/test_output.phymon.n5 -l -p 30 ContentType=PHYSMON ContentOrigin=n5 &
    haggletest app3 -f output.gps.n5 sub ContentType=gps &
    haggletest app4 -f output.gps.n5 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n5" >> ${TFILE}
    cat /tmp/testout_gps.n5 >> ${TFILE}
    echo "/tmp/test_output.physmon.n5" >> ${TFILE}
    cat /tmp/test_output.physmon.n5 >> ${TFILE}
    echo "/tmp/test_output.opord.n5" >> ${TFILE}
    cat /tmp/test_output.opord.n5 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n5" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n5 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n5" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n5  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n6" ]; then
    haggletest app0 -f /tmp/test_output.gps.n6 -l -p 3 pub ContentType=gps ContentOrigin=n6 &
    haggletest app1 -f /tmp/test_output.phymon.n6 -l -p 30 ContentType=PHYSMON ContentOrigin=n6 &
    haggletest app3 -f output.gps.n6 sub ContentType=gps &
    haggletest app4 -f output.gps.n6 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n6" >> ${TFILE}
    cat /tmp/testout_gps.n6 >> ${TFILE}
    echo "/tmp/test_output.physmon.n6" >> ${TFILE}
    cat /tmp/test_output.physmon.n6 >> ${TFILE}
    echo "/tmp/test_output.opord.n6" >> ${TFILE}
    cat /tmp/test_output.opord.n6 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n6" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n6 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n6" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n6  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n7" ]; then
    haggletest app0 -f /tmp/test_output.gps.n7 -l -p 3 pub ContentType=gps ContentOrigin=n7 &
    haggletest app1 -f /tmp/test_output.phymon.n7 -l -p 30 ContentType=PHYSMON ContentOrigin=n7 &
    haggletest app3 -f output.gps.n7 sub ContentType=gps &
    haggletest app4 -f output.gps.n7 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n7" >> ${TFILE}
    cat /tmp/testout_gps.n7 >> ${TFILE}
    echo "/tmp/test_output.physmon.n7" >> ${TFILE}
    cat /tmp/test_output.physmon.n7 >> ${TFILE}
    echo "/tmp/test_output.opord.n7" >> ${TFILE}
    cat /tmp/test_output.opord.n7 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n7" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n7 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n7" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n7  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n8" ]; then
    haggletest app0 -f /tmp/test_output.gps.n8 -l -p 3 pub ContentType=gps ContentOrigin=n8 &
    haggletest app1 -f /tmp/test_output.phymon.n8 -l -p 30 ContentType=PHYSMON ContentOrigin=n8 &
    haggletest app3 -f output.gps.n8 sub ContentType=gps &
    haggletest app4 -f output.gps.n8 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n8" >> ${TFILE}
    cat /tmp/testout_gps.n8 >> ${TFILE}
    echo "/tmp/test_output.physmon.n8" >> ${TFILE}
    cat /tmp/test_output.physmon.n8 >> ${TFILE}
    echo "/tmp/test_output.opord.n8" >> ${TFILE}
    cat /tmp/test_output.opord.n8 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n8" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n8 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n8" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n8  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n9" ]; then
    haggletest app0 -f /tmp/test_output.gps.n9 -l -p 3 pub ContentType=gps ContentOrigin=n9 &
    haggletest app1 -f /tmp/test_output.phymon.n9 -l -p 30 ContentType=PHYSMON ContentOrigin=n9 &
    haggletest app3 -f output.gps.n9 sub ContentType=gps &
    haggletest app4 -f output.gps.n9 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n9" >> ${TFILE}
    cat /tmp/testout_gps.n9 >> ${TFILE}
    echo "/tmp/test_output.physmon.n9" >> ${TFILE}
    cat /tmp/test_output.physmon.n9 >> ${TFILE}
    echo "/tmp/test_output.opord.n9" >> ${TFILE}
    cat /tmp/test_output.opord.n9 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n9" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n9 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n9" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n9  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n10" ]; then
    haggletest app0 -f /tmp/test_output.gps.n10 -l -p 3 pub ContentType=gps ContentOrigin=n10 &
    haggletest app1 -f /tmp/test_output.phymon.n10 -l -p 30 ContentType=PHYSMON ContentOrigin=n10 &
    haggletest app3 -f output.gps.n10 sub ContentType=gps &
    haggletest app4 -f output.gps.n10 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
sleep 500
    (haggletest app2 -f /tmp/test_output.evidencea.n10 pub file /tmp/evidencea data=opord:10 type=frago)
    (haggletest app2 -f /tmp/test_output.evidenceb.n10 pub file /tmp/evidenceb data=opord:10 type=frago)
    (haggletest app2 -f /tmp/test_output.evidencec.n10 pub file /tmp/evidencec data=opord:10 type=frago)
    echo "/tmp/test_output.gps.n10" >> ${TFILE}
    cat /tmp/testout_gps.n10 >> ${TFILE}
    echo "/tmp/test_output.physmon.n10" >> ${TFILE}
    cat /tmp/test_output.physmon.n10 >> ${TFILE}
    echo "/tmp/test_output.opord.n10" >> ${TFILE}
    cat /tmp/test_output.opord.n10 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n10" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n10 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n10" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n10  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n11" ]; then
    haggletest app0 -f /tmp/test_output.gps.n11 -l -p 3 pub ContentType=gps ContentOrigin=n11 &
    haggletest app1 -f /tmp/test_output.phymon.n11 -l -p 30 ContentType=PHYSMON ContentOrigin=n11 &
    haggletest app3 -f output.gps.n11 sub ContentType=gps &
    haggletest app4 -f output.gps.n11 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n11" >> ${TFILE}
    cat /tmp/testout_gps.n11 >> ${TFILE}
    echo "/tmp/test_output.physmon.n11" >> ${TFILE}
    cat /tmp/test_output.physmon.n11 >> ${TFILE}
    echo "/tmp/test_output.opord.n11" >> ${TFILE}
    cat /tmp/test_output.opord.n11 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n11" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n11 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n11" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n11  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n12" ]; then
    haggletest app0 -f /tmp/test_output.gps.n12 -l -p 3 pub ContentType=gps ContentOrigin=n12 &
    haggletest app1 -f /tmp/test_output.phymon.n12 -l -p 30 ContentType=PHYSMON ContentOrigin=n12 &
    haggletest app3 -f output.gps.n12 sub ContentType=gps &
    haggletest app4 -f output.gps.n12 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n12" >> ${TFILE}
    cat /tmp/testout_gps.n12 >> ${TFILE}
    echo "/tmp/test_output.physmon.n12" >> ${TFILE}
    cat /tmp/test_output.physmon.n12 >> ${TFILE}
    echo "/tmp/test_output.opord.n12" >> ${TFILE}
    cat /tmp/test_output.opord.n12 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n12" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n12 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n12" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n12  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n13" ]; then
    haggletest app0 -f /tmp/test_output.gps.n13 -l -p 3 pub ContentType=gps ContentOrigin=n13 &
    haggletest app1 -f /tmp/test_output.phymon.n13 -l -p 30 ContentType=PHYSMON ContentOrigin=n13 &
    haggletest app3 -f output.gps.n13 sub ContentType=gps &
    haggletest app4 -f output.gps.n13 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n13" >> ${TFILE}
    cat /tmp/testout_gps.n13 >> ${TFILE}
    echo "/tmp/test_output.physmon.n13" >> ${TFILE}
    cat /tmp/test_output.physmon.n13 >> ${TFILE}
    echo "/tmp/test_output.opord.n13" >> ${TFILE}
    cat /tmp/test_output.opord.n13 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n13" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n13 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n13" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n13  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n14" ]; then
    haggletest app0 -f /tmp/test_output.gps.n14 -l -p 3 pub ContentType=gps ContentOrigin=n14 &
    haggletest app1 -f /tmp/test_output.phymon.n14 -l -p 30 ContentType=PHYSMON ContentOrigin=n14 &
    haggletest app3 -f output.gps.n14 sub ContentType=gps &
    haggletest app4 -f output.gps.n14 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n14" >> ${TFILE}
    cat /tmp/testout_gps.n14 >> ${TFILE}
    echo "/tmp/test_output.physmon.n14" >> ${TFILE}
    cat /tmp/test_output.physmon.n14 >> ${TFILE}
    echo "/tmp/test_output.opord.n14" >> ${TFILE}
    cat /tmp/test_output.opord.n14 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n14" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n14 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n14" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n14  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n15" ]; then
    haggletest app0 -f /tmp/test_output.gps.n15 -l -p 3 pub ContentType=gps ContentOrigin=n15 &
    haggletest app1 -f /tmp/test_output.phymon.n15 -l -p 30 ContentType=PHYSMON ContentOrigin=n15 &
    haggletest app3 -f output.gps.n15 sub ContentType=gps &
    haggletest app4 -f output.gps.n15 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n15" >> ${TFILE}
    cat /tmp/testout_gps.n15 >> ${TFILE}
    echo "/tmp/test_output.physmon.n15" >> ${TFILE}
    cat /tmp/test_output.physmon.n15 >> ${TFILE}
    echo "/tmp/test_output.opord.n15" >> ${TFILE}
    cat /tmp/test_output.opord.n15 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n15" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n15 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n15" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n15  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n16" ]; then
    haggletest app0 -f /tmp/test_output.gps.n16 -l -p 3 pub ContentType=gps ContentOrigin=n16 &
    haggletest app1 -f /tmp/test_output.phymon.n16 -l -p 30 ContentType=PHYSMON ContentOrigin=n16 &
    haggletest app3 -f output.gps.n16 sub ContentType=gps &
    haggletest app4 -f output.gps.n16 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n16" >> ${TFILE}
    cat /tmp/testout_gps.n16 >> ${TFILE}
    echo "/tmp/test_output.physmon.n16" >> ${TFILE}
    cat /tmp/test_output.physmon.n16 >> ${TFILE}
    echo "/tmp/test_output.opord.n16" >> ${TFILE}
    cat /tmp/test_output.opord.n16 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n16" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n16 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n16" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n16  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n17" ]; then
    haggletest app0 -f /tmp/test_output.gps.n17 -l -p 3 pub ContentType=gps ContentOrigin=n17 &
    haggletest app1 -f /tmp/test_output.phymon.n17 -l -p 30 ContentType=PHYSMON ContentOrigin=n17 &
    haggletest app3 -f output.gps.n17 sub ContentType=gps &
    haggletest app4 -f output.gps.n17 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n17" >> ${TFILE}
    cat /tmp/testout_gps.n17 >> ${TFILE}
    echo "/tmp/test_output.physmon.n17" >> ${TFILE}
    cat /tmp/test_output.physmon.n17 >> ${TFILE}
    echo "/tmp/test_output.opord.n17" >> ${TFILE}
    cat /tmp/test_output.opord.n17 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n17" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n17 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n17" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n17  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n18" ]; then
    haggletest app0 -f /tmp/test_output.gps.n18 -l -p 3 pub ContentType=gps ContentOrigin=n18 &
    haggletest app1 -f /tmp/test_output.phymon.n18 -l -p 30 ContentType=PHYSMON ContentOrigin=n18 &
    haggletest app3 -f output.gps.n18 sub ContentType=gps &
    haggletest app4 -f output.gps.n18 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n18" >> ${TFILE}
    cat /tmp/testout_gps.n18 >> ${TFILE}
    echo "/tmp/test_output.physmon.n18" >> ${TFILE}
    cat /tmp/test_output.physmon.n18 >> ${TFILE}
    echo "/tmp/test_output.opord.n18" >> ${TFILE}
    cat /tmp/test_output.opord.n18 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n18" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n18 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n18" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n18  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n19" ]; then
    haggletest app0 -f /tmp/test_output.gps.n19 -l -p 3 pub ContentType=gps ContentOrigin=n19 &
    haggletest app1 -f /tmp/test_output.phymon.n19 -l -p 30 ContentType=PHYSMON ContentOrigin=n19 &
    haggletest app3 -f output.gps.n19 sub ContentType=gps &
    haggletest app4 -f output.gps.n19 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n19" >> ${TFILE}
    cat /tmp/testout_gps.n19 >> ${TFILE}
    echo "/tmp/test_output.physmon.n19" >> ${TFILE}
    cat /tmp/test_output.physmon.n19 >> ${TFILE}
    echo "/tmp/test_output.opord.n19" >> ${TFILE}
    cat /tmp/test_output.opord.n19 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n19" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n19 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n19" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n19  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n20" ]; then
    haggletest app0 -f /tmp/test_output.gps.n20 -l -p 3 pub ContentType=gps ContentOrigin=n20 &
    haggletest app1 -f /tmp/test_output.phymon.n20 -l -p 30 ContentType=PHYSMON ContentOrigin=n20 &
    haggletest app3 -f output.gps.n20 sub ContentType=gps &
    haggletest app4 -f output.gps.n20 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n20" >> ${TFILE}
    cat /tmp/testout_gps.n20 >> ${TFILE}
    echo "/tmp/test_output.physmon.n20" >> ${TFILE}
    cat /tmp/test_output.physmon.n20 >> ${TFILE}
    echo "/tmp/test_output.opord.n20" >> ${TFILE}
    cat /tmp/test_output.opord.n20 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n20" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n20 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n20" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n20  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n21" ]; then
    haggletest app0 -f /tmp/test_output.gps.n21 -l -p 3 pub ContentType=gps ContentOrigin=n21 &
    haggletest app1 -f /tmp/test_output.phymon.n21 -l -p 30 ContentType=PHYSMON ContentOrigin=n21 &
    haggletest app3 -f output.gps.n21 sub ContentType=gps &
    haggletest app4 -f output.gps.n21 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n21" >> ${TFILE}
    cat /tmp/testout_gps.n21 >> ${TFILE}
    echo "/tmp/test_output.physmon.n21" >> ${TFILE}
    cat /tmp/test_output.physmon.n21 >> ${TFILE}
    echo "/tmp/test_output.opord.n21" >> ${TFILE}
    cat /tmp/test_output.opord.n21 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n21" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n21 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n21" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n21  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n22" ]; then
    haggletest app0 -f /tmp/test_output.gps.n22 -l -p 3 pub ContentType=gps ContentOrigin=n22 &
    haggletest app1 -f /tmp/test_output.phymon.n22 -l -p 30 ContentType=PHYSMON ContentOrigin=n22 &
    haggletest app3 -f output.gps.n22 sub ContentType=gps &
    haggletest app4 -f output.gps.n22 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n22" >> ${TFILE}
    cat /tmp/testout_gps.n22 >> ${TFILE}
    echo "/tmp/test_output.physmon.n22" >> ${TFILE}
    cat /tmp/test_output.physmon.n22 >> ${TFILE}
    echo "/tmp/test_output.opord.n22" >> ${TFILE}
    cat /tmp/test_output.opord.n22 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n22" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n22 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n22" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n22  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n23" ]; then
    haggletest app0 -f /tmp/test_output.gps.n23 -l -p 3 pub ContentType=gps ContentOrigin=n23 &
    haggletest app1 -f /tmp/test_output.phymon.n23 -l -p 30 ContentType=PHYSMON ContentOrigin=n23 &
    haggletest app3 -f output.gps.n23 sub ContentType=gps &
    haggletest app4 -f output.gps.n23 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n23" >> ${TFILE}
    cat /tmp/testout_gps.n23 >> ${TFILE}
    echo "/tmp/test_output.physmon.n23" >> ${TFILE}
    cat /tmp/test_output.physmon.n23 >> ${TFILE}
    echo "/tmp/test_output.opord.n23" >> ${TFILE}
    cat /tmp/test_output.opord.n23 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n23" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n23 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n23" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n23  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n24" ]; then
    haggletest app0 -f /tmp/test_output.gps.n24 -l -p 3 pub ContentType=gps ContentOrigin=n24 &
    haggletest app1 -f /tmp/test_output.phymon.n24 -l -p 30 ContentType=PHYSMON ContentOrigin=n24 &
    haggletest app3 -f output.gps.n24 sub ContentType=gps &
    haggletest app4 -f output.gps.n24 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n24" >> ${TFILE}
    cat /tmp/testout_gps.n24 >> ${TFILE}
    echo "/tmp/test_output.physmon.n24" >> ${TFILE}
    cat /tmp/test_output.physmon.n24 >> ${TFILE}
    echo "/tmp/test_output.opord.n24" >> ${TFILE}
    cat /tmp/test_output.opord.n24 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n24" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n24 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n24" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n24  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n25" ]; then
    haggletest app0 -f /tmp/test_output.gps.n25 -l -p 3 pub ContentType=gps ContentOrigin=n25 &
    haggletest app1 -f /tmp/test_output.phymon.n25 -l -p 30 ContentType=PHYSMON ContentOrigin=n25 &
    haggletest app3 -f output.gps.n25 sub ContentType=gps &
    haggletest app4 -f output.gps.n25 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n25" >> ${TFILE}
    cat /tmp/testout_gps.n25 >> ${TFILE}
    echo "/tmp/test_output.physmon.n25" >> ${TFILE}
    cat /tmp/test_output.physmon.n25 >> ${TFILE}
    echo "/tmp/test_output.opord.n25" >> ${TFILE}
    cat /tmp/test_output.opord.n25 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n25" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n25 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n25" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n25  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n26" ]; then
    haggletest app0 -f /tmp/test_output.gps.n26 -l -p 3 pub ContentType=gps ContentOrigin=n26 &
    haggletest app1 -f /tmp/test_output.phymon.n26 -l -p 30 ContentType=PHYSMON ContentOrigin=n26 &
    haggletest app3 -f output.gps.n26 sub ContentType=gps &
    haggletest app4 -f output.gps.n26 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n26" >> ${TFILE}
    cat /tmp/testout_gps.n26 >> ${TFILE}
    echo "/tmp/test_output.physmon.n26" >> ${TFILE}
    cat /tmp/test_output.physmon.n26 >> ${TFILE}
    echo "/tmp/test_output.opord.n26" >> ${TFILE}
    cat /tmp/test_output.opord.n26 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n26" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n26 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n26" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n26  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n27" ]; then
    haggletest app0 -f /tmp/test_output.gps.n27 -l -p 3 pub ContentType=gps ContentOrigin=n27 &
    haggletest app1 -f /tmp/test_output.phymon.n27 -l -p 30 ContentType=PHYSMON ContentOrigin=n27 &
    haggletest app3 -f output.gps.n27 sub ContentType=gps &
    haggletest app4 -f output.gps.n27 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
    echo "/tmp/test_output.gps.n27" >> ${TFILE}
    cat /tmp/testout_gps.n27 >> ${TFILE}
    echo "/tmp/test_output.physmon.n27" >> ${TFILE}
    cat /tmp/test_output.physmon.n27 >> ${TFILE}
    echo "/tmp/test_output.opord.n27" >> ${TFILE}
    cat /tmp/test_output.opord.n27 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n27" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n27 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n27" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n27  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
elif [ $1 == "n28" ]; then
    haggletest app0 -f /tmp/test_output.gps.n28 -l -p 3 pub ContentType=gps ContentOrigin=n28 &
    haggletest app1 -f /tmp/test_output.phymon.n28 -l -p 30 ContentType=PHYSMON ContentOrigin=n28 &
    haggletest app3 -f output.gps.n28 sub ContentType=gps &
    haggletest app4 -f output.gps.n28 -s 200 sub data=opord:10 &
sleep 200
    haggletest app5 -f /tmp/test_output.opord-add.n1 sub data=opord:10 type=add:10 &
    haggletest app6 -f /tmp/test_output.opord-frago.n1 sub data=opord:10 type=frago:10 &
sleep 1000
    (haggletest app2 -f /tmp/test_output.fragopic1.n28 pub file /tmp/opordmap data=opord:10 type=frago)
    (haggletest app2 -f /tmp/test_output.fragopic2.n28 pub file /tmp/opordmap data=opord:10 type=frago)
    (haggletest app2 -f /tmp/test_output.fragopic3.n28 pub file /tmp/opordmap data=opord:10 type=frago)
    (haggletest app2 -f /tmp/test_output.fragopic4.n28 pub file /tmp/opordmap data=opord:10 type=frago)
    (haggletest app2 -f /tmp/test_output.fragopic5.n28 pub file /tmp/opordmap data=opord:10 type=frago)
    (haggletest app2 -f /tmp/test_output.fragopic6.n28 pub file /tmp/opordmap data=opord:10 type=frago)
    (haggletest app2 -f /tmp/test_output.opordtext1.n28 pub file /tmp/fragotxt1 data=opordtype=txt)
    (haggletest app2 -f /tmp/test_output.opordtext2.n28 pub file /tmp/fragotxt2 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext3.n28 pub file /tmp/fragotxt3 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext4.n28 pub file /tmp/fragotxt4 data=opord:10 type=txt) 
    (haggletest app2 -f /tmp/test_output.opordtext5.n28 pub file /tmp/fragotxt5 data=opord:10 type=tx
    (haggletest app2 -f /tmp/test_output.opordtext6.n28 pub file /tmp/fragotxt6 data=opord:10 type=tx
    echo "/tmp/test_output.gps.n28" >> ${TFILE}
    cat /tmp/testout_gps.n28 >> ${TFILE}
    echo "/tmp/test_output.physmon.n28" >> ${TFILE}
    cat /tmp/test_output.physmon.n28 >> ${TFILE}
    echo "/tmp/test_output.opord.n28" >> ${TFILE}
    cat /tmp/test_output.opord.n28 >> ${TFILE}
    echo "/tmp/test_output.opord-add.n28" >> ${TFILE}
    cat  /tmp/test_output.opord-add.n28 >> ${TFILE}
    echo "/tmp/test_output.opord-frago.n28" >> ${TFILE}
    cat /tmp/test_output.opord-frago.n28  >> ${TFILE}
    /sbin/ifconfig eth0 >> ${TFILE}
else
    exit 0
fi
cat ${TFILE} >> $2
