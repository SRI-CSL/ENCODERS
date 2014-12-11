#!/system/bin/sh

# A script to set adhoc mode on android devices.
# Usage: adhoc.sh [start|stop] nodeNR

NETDEV=eth0
MODULE_PATH=/system/lib/modules
POST_INSERT_CMD=
IP_PREFIX="192.168.1"

if ls $MODULE_PATH/wlan.ko 2>/dev/null; then
    NETDEV=tiwlan0
    MODULE=wlan
    HOSTNAME=Magic
    POST_INSERT_CMD="wlan_loader -f /system/etc/wifi/Fw1251r1c.bin -e /proc/calibration -i /data/local/tiwlan.ini"
elif ls $MODULE_PATH/bcm4329.ko 2>/dev/null; then
    NETDEV=eth0
    MODULE=bcm4329
    HOSTNAME=Android
    POST_INSERT_CMD="iwconfig eth0 mode ad-hoc essid HaggleHoc txpower 4"
else
    echo "Unknown network device..."
    exit
fi

case "$2" in
"")
NODE=1 
;;
*)
NODE=$2
;;
esac

case "$1" in
'start')

echo "Shutting down interface $NETDEV"
ifconfig $NETDEV down 2>/dev/null
sleep 1
echo "Removing module $MODULE"
rmmod $MODULE 2>/dev/null

echo "Node IP is $IP_PREFIX.$NODE"

echo "Setting hostname $HOSTNAME-$NODE"
echo "$HOSTNAME-$NODE" > /proc/sys/kernel/hostname

insmod $MODULE_PATH/$MODULE.ko

eval $POST_INSERT_CMD

ifconfig $NETDEV $IP_PREFIX.$NODE netmask 255.255.255.0
ifconfig $NETDEV up 

;;
'stop')
echo "shutting down $NETDEV"
ifconfig $NETDEV down 2>/dev/null
sleep 1
rmmod $MODULE 2>/dev/null
;;
*)
echo "Usage: $0 [start|stop] [ node # ]"
;;
esac
