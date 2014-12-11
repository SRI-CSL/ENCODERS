#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)
#   Tim McCarthy (TTM)

# Runs on Linux

. $ANDROID_TESTRUNNER/adbtool.sh

logpath=$1
NUM_DEVICES=$2
ANDROID_TEST_DIR=$3

mkdir -p ${logpath}

echo -n "pulling logs from $NUM_DEVICES nodes..."

for i in $(seq 1 $NUM_DEVICES);
do
  NODE_NAME="n$i"
  USB="usb$((i-1))"
  NODE_DIR=$logpath/android-files/$NODE_NAME

  runADB $USB "shell \"ls -l /sdcard/haggle/ > $ANDROID_TEST_DIR/received_files.log\"" &> /dev/null
  #runADB $USB "shell tar czf $ANDROID_TEST_DIR/received_files.tgz /sdcard/haggle" &> /dev/null

  FILES="fail.log logcat maps outputfile received_files.log received_files.tgz resources.log size.log observer.log"

  for file in $FILES; do
    runADB $USB "pull $ANDROID_TEST_DIR/${file} $logpath/android-files/$NODE_NAME/" &> /dev/null
  done


  runADB $USB "pull /data/data/org.haggle.kernel/files/haggle.db $NODE_DIR/haggle.db" &> /dev/null
  runADB $USB "pull /data/data/org.haggle.kernel/files/haggle.log $NODE_DIR/haggle.log" &> /dev/null
  runADB $USB "pull /data/data/org.haggle.kernel/files/config.xml $NODE_DIR/config.xml" &> /dev/null

  runADB $USB "pull /data/tombstones $NODE_DIR/tombstones" &> /dev/null
  touch "$NODE_DIR/fail.log"
done

runADB usb0 "pull /data/app/org.haggle.kernel-1.apk $logpath/" &> /dev/null
# runADB usb0 "pull /system/lib $logpath/system/lib" &> /dev/null
# runADB usb0 "pull /data/data/org.haggle.kernel/lib $logpath/system/lib" &> /dev/null

dbfiles=$(ls $logpath/android-files/n*/*.db 2> /dev/null)
logfiles=$(ls $logpath/android-files/n*/*.{log,xml} 2> /dev/null)

for lfile in ${logfiles}; do
    node=$(echo ${lfile} | sed 's/.*\(n[0-9]\+\).*/\1/g')
    logname="${node}.$(basename ${lfile})"
    cp ${lfile} "${logpath}/${logname}"
done

cat $logpath/android-files/n*/outputfile >> "${logpath}/apps_output"
cat $logpath/android-files/n*/fail.log >> $logpath/fail.log

# copy doid list
mkdir -p "${logpath}/db/"
for lfile in ${dbfiles}; do
    node=$(echo ${lfile} | sed 's/.*\(n[0-9]\+\).*/\1/g')
    logname="${node}.$(basename ${lfile})"
    python $ANDROID_TESTRUNNER/ls_db.py ${lfile} > "${logpath}/${logname}.hash_sizes.log"
    cp ${lfile} "${logpath}/db/${logname}"
done

pushd ${logpath} &> /dev/null
tar -czf "${logpath}/db.tar.gz" db
popd &> /dev/null
rm -rf "${logpath}/db/"
