#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

unset dev_ids

# check ANDROID_HOME is set
if [ -z $ANDROID_HOME ]; then
   echo ""
   echo "[ERROR] ANDROID_HOME not set in environment. exiting..."
   echo ""
   exit
fi

# check adb is available
ADB=$ANDROID_HOME/platform-tools/adb
if [ ! -f $ADB ] || [ ! -x $ADB ]; then
   echo ""
   echo "[ERROR] can't find $ADB (or is not executable). exiting..."
   echo""
   exit
fi

DEVICE_LIST="/tmp/device_ids.txt"
if ! [ -f $DEVICE_LIST ]; then
	echo "[ERROR] There is no device list : $DEVICE_LIST"
	exit
fi

join () {
	for pid in $(jobs -l | awk '{print $2}'); do
		wait $pid
	done
}

# $1 : device num	#2 : cmd
adb_dev () {
	devNum
	if [ $1 -ge $? ]; then
		return 0
	fi
	dev_num=$1
	cmd=$2
	$ADB -s ${dev_ids[dev_num]} $cmd
	return 1
}

devNum () {
	return ${#dev_ids[@]}
}

adb_retry () {
  duration=$1
  device=$2
  shift 2
  cmd=$@
  success=0

  while [ $success == 0 ]; do
    timeout $duration $ADB -s $device $cmd
    if [ "$?" != "124" ]; then
      success=1
    fi
  done
}

adb_all () {
	cmd=$1

	if ! [ -f $ADB ]; then
		usage
		exit
	fi
	if ! [ -f $deviceslist ]; then
		usage
		exit
	fi
	for device in $(cat $DEVICE_LIST); do
		$ADB -s $device $cmd &
	done
	join
	sleep 3
}

adb_n () {
  num_devices=$1
  cmd=$2

  if ! [ -f $ADB ]; then
    usage
    exit
  fi
  if ! [ -f $deviceslist ]; then
    usage
    exit
  fi
  for device in $(cat $DEVICE_LIST | head -n $num_devices); do
    $ADB -s $device $cmd
  done
}

adb_retry_n () {
  timeout=$1
  num_devices=$2
  cmd=$3

  if ! [ -f $ADB ]; then
    usage
    exit
  fi
  if ! [ -f $deviceslist ]; then
    usage
    exit
  fi
  for device in $(cat $DEVICE_LIST | head -n $num_devices); do
    adb_retry $timeout $device "$cmd"
  done
}

adb_each () {
  cmd=$1
  if ! [ -f $ADB ]; then
    usage
    exit
  fi
  if ! [ -f $deviceslist ]; then
    usage
    exit
  fi
  for device in $(cat $DEVICE_LIST); do
    $ADB -s $device $cmd
  done
}

reboot_all () {
  printf "Rebooting all ADB devices"
  NUM_ADB_LINES=$(adb devices | wc -l)
  adb_all reboot
  sleep 5
  CURRENT_ADB_LINES=$(adb devices | wc -l)
  until [ $NUM_ADB_LINES -eq $CURRENT_ADB_LINES ]; do
    sleep 1
    printf "."
    CURRENT_ADB_LINES=$(adb devices | wc -l)
  done
  sleep 10
  echo "Done."
}

reboot_n () {
  num_devices=$1
  printf "Rebooting $num_devices ADB devices"
  NUM_ADB_LINES=$(adb devices | wc -l)
  adb_n $num_devices reboot
  sleep 5
  CURRENT_ADB_LINES=$(adb devices | wc -l)
  until [ $NUM_ADB_LINES -eq $CURRENT_ADB_LINES ]; do
    sleep 1
    printf "."
    CURRENT_ADB_LINES=$(adb devices | wc -l)
  done
  sleep 10
  echo "Done."
}

shell_all () {
  cmd=$1
  adb_all "shell $cmd"
}

installAPK () {
	in=$1
	if ! [ -f $in ]; then
		echo "[ERROR] There is no file : $in"
		exit
	fi
	adb_all "install $in"
}

pushFile () {
	file=$1
	dst=$2
	if  [ -f $file ] || [ -d $file ]; then
		adb_all "push $file $dst"
	else
		echo "[ERROR] There is no file : $file"
	fi

}

readDeviceList () {
	index=0
	for device in $(cat $DEVICE_LIST); do
		dev_ids[index]=$device
		index=$((index + 1))
	done
}

getDevice() {
  cat /tmp/device_info.txt | grep -w $1 | awk '{print $1}'
}

runShell() {
  device=$(getDevice $1)
  # echo "Device: $1($device) do $(echo $@ | sed "s/^$1 //g")"
  # echo ""
  eval adb -s $device shell $(echo $@ | sed "s/^$1 //g")
}

runADB() {
  device=$(getDevice $1)
  # echo "Device: $1($device) do $(echo $@ | sed "s/^$1 //g")"
  # echo ""
  eval adb -s $device $(echo $@ | sed "s/^$1 //g")
}

readDeviceList
