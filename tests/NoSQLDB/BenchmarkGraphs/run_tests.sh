#!/bin/bash

NODE_ATTR=2
DOBJ_ATTR=5
ATTR_POOL=31000
DOBJS=30000
NODES=300

START_TIME=$SECONDS


sudo su -c "sync; echo 3 > /proc/sys/vm/drop_caches" 

rm -f *ps

## run NO-SQL tests

rm -f *mem.log
rm -f memory.ps
rm -f /tmp/time_stats.log 
#rm -f /tmp/mem.log
rm -f ~/.Haggle/*
cp ./config.xml ~/.Haggle/config.xml

touch /tmp/run_test

echo "Running NO-SQL test"

(sleep 1 && ./track_memory.sh $(cat ~/.Haggle/haggle.pid) &> /dev/null ) & 

/usr/bin/time -o /tmp/time_stats.log -v haggle -dd -m -b ${DOBJ_ATTR} ${NODE_ATTR} ${ATTR_POOL} ${DOBJS} ${NODES} >> /dev/null

rm /tmp/run_test

NOSQL_NAME="nosql-${DOBJ_ATTR}-${NODE_ATTR}-${ATTR_POOL}-${DOBJS}-${NODES}"

if [ -d ${NOSQL_NAME} ]; then
    mv ${NOSQL_NAME} ${NOSQL_NAME}_$(date "+%s")
fi

mkdir ${NOSQL_NAME}
cp ~/.Haggle/*log ${NOSQL_NAME}/
#mv /tmp/mem.log ${NOSQL_NAME}/
cp ./scripts/*sh ${NOSQL_NAME}/
mv /tmp/time_stats.log ${NOSQL_NAME}/

cd ${NOSQL_NAME}
./gen_query_delays.sh
cp query_delays.dat ../nosql-query_delays.dat
./gen_node_insert_delays.sh
cp node_insert_delays.dat ../nosql-node_insert_delays.dat
./gen_node_delete_delays.sh
cp node_delete_delays.dat ../nosql-node_delete_delays.dat
./gen_dobj_insert_delays.sh
cp dobj_insert_delays.dat ../nosql-dobj_insert_delays.dat
./gen_dobj_delete_delays.sh
cp dobj_delete_delays.dat ../nosql-dobj_delete_delays.dat
cd ../
sleep 1
mv mem.log* nosql.mem.log

## run SQL tests

sudo su -c "sync; echo 3 > /proc/sys/vm/drop_caches" 

rm -f /tmp/time_stats.log 
rm -f /tmp/mem.log*
rm -f ~/.Haggle/*
cp ./config.xml ~/.Haggle/config.xml

touch /tmp/run_test

(sleep 1 && ./track_memory.sh $(cat ~/.Haggle/haggle.pid) &> /dev/null)&

echo "Running SQL Test"

/usr/bin/time -o /tmp/time_stats.log -v haggle -dd -b ${DOBJ_ATTR} ${NODE_ATTR} ${ATTR_POOL} ${DOBJS} ${NODES}  >> /dev/null
rm /tmp/run_test

SQL_NAME="sql-${DOBJ_ATTR}-${NODE_ATTR}-${ATTR_POOL}-${DOBJS}-${NODES}"

if [ -d ${SQL_NAME} ]; then
    mv ${SQL_NAME} ${SQL_NAME}_$(date "+%s")
fi

mkdir ${SQL_NAME}
cp ~/.Haggle/*log ${SQL_NAME}/
#mv /tmp/mem.log ${SQL_NAME}/
cp ./scripts/*sh ${SQL_NAME}/
mv /tmp/time_stats.log ${SQL_NAME}/

cd ${SQL_NAME}
./gen_query_delays.sh
cp query_delays.dat ../sql-query_delays.dat
./gen_node_insert_delays.sh
cp node_insert_delays.dat ../sql-node_insert_delays.dat
./gen_node_delete_delays.sh
cp node_delete_delays.dat ../sql-node_delete_delays.dat
./gen_dobj_insert_delays.sh
cp dobj_insert_delays.dat ../sql-dobj_insert_delays.dat
./gen_dobj_delete_delays.sh
cp dobj_delete_delays.dat ../sql-dobj_delete_delays.dat
cd ../
sleep 1
mv mem.log* sql.mem.log

echo "Plotting"
./show_memory.sh sql.mem.log nosql.mem.log

#gnuplot sql-memory.plot
#gnuplot nosql-memory.plot
gnuplot query_delays.plot 
gnuplot node_insert_delays.plot 
gnuplot node_delete_delays.plot 
gnuplot dobj_insert_delays.plot 
gnuplot dobj_delete_delays.plot 

RUN_NAME="benchmark-${NODE_ATTR}-${DOBJ_ATTR}-${ATTR_POOL}-${DOBJS}-${NODES}-$(date '+%s')"

mkdir ${RUN_NAME}
mv *ps ${RUN_NAME}/
mv ${SQL_NAME} ${RUN_NAME}/
mv ${NOSQL_NAME} ${RUN_NAME}/

rm -f *dat *log 

DURATION=$(($SECONDS - $START_TIME))

echo "Benchmark done (duration ${DURATION}) seconds"

cd ${RUN_NAME}
# validate results
diff <(grep "data objects in query" ${NOSQL_NAME}/haggle.log | awk '{print $2;}') <(grep "data objects in query" ${SQL_NAME}/haggle.log | awk '{print $2;}')
if [ "$?" != "0" ]; then
    echo "Data objects in lookup query had mismatched results"
fi
diff <(grep "data objects delete from response" ${NOSQL_NAME}/haggle.log | awk '{print $2;}') <(grep "data objects delete from response" ${SQL_NAME}/haggle.log | awk '{print $2;}')
if [ "$?" != "0" ]; then
    echo "Data objects in delete query had mismatched results"
fi

cd ../
