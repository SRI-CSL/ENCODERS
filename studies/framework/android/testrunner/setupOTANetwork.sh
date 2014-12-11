#!/bin/bash

# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Tim McCarthy (TTM)

. $ANDROID_TESTRUNNER/adbtool.sh

# global variables
fname="/tmp/device_ids.txt" # place in /tmp where everyone has access
fname_info="/tmp/device_info.txt"
fname_iplist="/tmp/device_iplist.txt"
unset id_device_list # initialize to empty list
unset id_list        # initialize to empty list

# check if adb is running
check_adb_running () {
  pid=`pidof -s adb`
  if [ -z $pid ]; then
    echo ""
    echo "adb is NOT running.  exiting..."
    echo ""
    sleep 1
    exit
  fi
}

unset n
# read file and extract device id(s)
extract_device_ids_from_file () {
  if [ ! -e $fname ]; then
    echo ""
    echo "$fname not found. exiting..."
    echo ""
    exit
  fi

  c=0

  for device in $(cat $fname | head -n $devLimit); do
    id_list[c++]=$device
  done
}

# print id_list
print_list () {
  idLen=${#id_list[@]}

  if [ $idLen = 0 ]; then
   echo""
   echo "id_list is empty[len=$idLen]"
   echo ""
   exit
  fi

  echo "device id(s):"
  for ((j=0; j < idLen; j++)); do
    echo "${id_list[j]}"
  done
  echo ""
}

# turn on SAIC mesh app
turn_on_saic_mesh () {
  idLen=${#id_list[@]}
  for ((j=0; j < idLen; j++)); do
   echo "Turn on SAIC Mesh App: " "${id_list[j]}"
   adb_retry 20 ${id_list[j]} shell svc wifi disable
   adb_retry 20 ${id_list[j]} shell input keyevent 82
   sleep 1
   adb_retry 20 ${id_list[j]} shell am start -a android.intent.action.MAIN -n android.tether/.MainActivity --activity-brought-to-front
   sleep 2
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 53 506
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 54 568
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 48 50
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 50 5
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 57 0
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 0 2 0
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 0 0 0
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 53 506
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 54 568
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 48 0
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 50 5
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 3 57 0
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 0 2 0
   adb_retry 20 ${id_list[j]} shell sendevent /dev/input/event0 0 0 0
   sleep 2
  done
  sleep 10
  echo ""
}

unset ip_address

# pip (partial ip)
pip=192.168.1.

# starting IP
d=10

# set ip address for each device
set_ip_address ()
{
  idLen=${#id_list[@]}

  # assign ip address
  for ((i=0; i < idLen; i++)); do
    ip=$pip$d
    adb_retry 10 ${id_list[i]} shell busybox ifconfig bat0 down
    adb_retry 10 ${id_list[i]} shell busybox ifconfig eth0 down
    adb_retry 10 ${id_list[i]} shell busybox ifconfig eth0 $ip
    adb_retry 10 ${id_list[i]} shell ip route add default dev eth0
    adb_retry 10 ${id_list[i]} shell netcfg
    adb_retry 10 ${id_list[i]} shell ip route show

    ip_address[i]=$ip
    let d=d+1
  done
}

# display device id(s) with associated ip(s)
display_ids_with_ips ()
{
  idLen=${#id_list[@]}
  usbdev="usb"
  rm $fname_info &> /dev/null
  rm $fname_iplist &> /dev/null
  echo ""
  echo "device id           ip address          usb id"
  echo "----------------    --------------      --------"
  for ((i=0; i < idLen; i++)); do
    usbId=$usbdev$i
    echo "${id_list[i]}    ${ip_address[i]}      ${usbId}"
    echo "${id_list[i]}    ${ip_address[i]}      ${usbId}" >> $fname_info
    echo "${ip_address[i]}" >> $fname_iplist
  done
}

if [ -n $1 ]; then
  devLimit=$1
else
  devLimit=$(cat $fname | wc -l)
fi

# execute functions
check_adb_running
extract_device_ids_from_file
print_list
turn_on_saic_mesh
set_ip_address
display_ids_with_ips
