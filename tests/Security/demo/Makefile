# Usage:
#	make ping
#		Checks that the two devices can talk to each other.
#
#	make pubsub
#		Check thats one device can publish and the other can subscribe.
#		No encryption is used.
#
#	make abepubsub
#		Two nodes are placed in Open Certification Mode and Open Authorization Mode, 
#               one is made into an Authority and subscribes to an interest, while the 
#               other publishes content with an attribute that matches the interest and 
#               an Access policy. The data object should be encrypted at the publisher 
#               and decrypted at the subscriber.
#
#	make restart
#		All connected devices have their Haggles refreshed, reconfigured as
#		users in Open Certification Mode and Open Authorization, and 
#               started as a service.
#
#	Note: BSD tail (with -r support) assumed.
#
# Ashish Gehani <ashish.gehani@sri.com>

SHELL = /bin/sh

# Assume at least one device is connected. The default device is the first.
DEVICE = $(shell adb devices|cut -f1|head -2|tail -1)

# Get a list of all devices.
DEVICE_LIST = $(shell adb devices|cut -f1|tail -n+2|tail -r|tail -n+2|tail -r)

# The default address that will be given to the device with the 'network' rule.
ADDRESS = 192.168.0.1

# The default configuration that Haggle will use.
CONFIG_AUTHORITY = cfg/config-flood-direct-coding-udptcp-encryption-auth.xml
CONFIG_USER = cfg/config-flood-direct-coding-udptcp-encryption-user.xml
CONFIG_USER_CLOSED = cfg/config-flood-direct-coding-udptcp-encryption-closed.xml

# Android default action
ACTION = android.intent.action.MAIN

# Location of the Haggle test application.
LOCAL_HAGGLETEST = bin/haggletest
HAGGLETEST = /data/haggletest

# Location of the ARP helper tool.
LOCAL_ARPHELPER = bin/arphelper
ARPHELPER = /system/etc/arphelper

# Details needed to install, configure, start, and uninstall Haggle.
HAGGLE_APK = apk/Haggle-debug.apk
HAGGLE_APP = org.haggle.kernel
HAGGLE_COMPONENT = org.haggle.kernel.Haggle
HAGGLE_INTENT = -a ${ACTION} -n ${HAGGLE_APP}/${HAGGLE_COMPONENT}
DATA_DIR = /data/data/${HAGGLE_APP}/files

# Details needed to install, use, and uninstall Mesh networking.
MESH_APK = apk/MeshApp-edited.apk
MESH_APP = android.tether
MESH_COMPONENT = .MainActivity
MESH_INTENT = -a ${ACTION} -n ${MESH_APP}/${MESH_COMPONENT}
MESH_OPTIONS = --activity-brought-to-front
MESH_DELAY = 20

# Details needed to install and uninstall PhotoShare.
PHOTO_APK = apk/PhotoShare-debug.apk
PHOTO_APP = org.haggle.PhotoShare

# Details needed to install and uninstall KeepScreen.
SCREEN_APK = apk/KeepScreen.apk
SCREEN_APP = com.example.keepscreen

# Global Charm parameters.
PICKLE = dat/charm_state.pickle

all:
	@echo 'make < network | ifconfig | uninstall | install | config | start >'

reboot:
	adb -s ${DEVICE} reboot

unlock:
	adb -s ${DEVICE} shell input keyevent 82

sync:
	$(eval TIME = $(shell date +%Y%m%d.%H%M%S ))
	adb -s ${DEVICE} shell date -s ${TIME}

network:
	adb -s ${DEVICE} shell busybox ifconfig eth0 down
	adb -s ${DEVICE} shell busybox ifconfig eth0 ${ADDRESS}

address:
	$(eval ADDRESS = $(shell adb -s ${DEVICE} shell busybox ifconfig | grep -A 1 eth0 | grep inet | cut -f2 -d':' | cut -f1 -d' ' ))
	@echo 'Device: '${DEVICE}'     IP: '${ADDRESS}

ifconfig:
	adb -s ${DEVICE} shell busybox ifconfig

uninstall:
	adb -s ${DEVICE} uninstall ${APP}

install:
	adb -s ${DEVICE} install ${APK}

empty:
	adb -s ${DEVICE} shell rm /sdcard/haggle/*

remove_ccb:
	adb -s ${DEVICE} shell rm /sdcard/ccb/data/*
	adb -s ${DEVICE} shell rm /sdcard/ccb/data/scripts/*
	adb -s ${DEVICE} shell rm /data/data/out

mesh:
	adb -s ${DEVICE} shell am start ${MESH_INTENT} ${MESH_OPTIONS}
	sleep ${MESH_DELAY}
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 53 506
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 54 568
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 48 50
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 50 5
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 57 0
	adb -s ${DEVICE} shell sendevent /dev/input/event0 0 2 0
	adb -s ${DEVICE} shell sendevent /dev/input/event0 0 0 0
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 53 506
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 54 568
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 48 0
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 50 5
	adb -s ${DEVICE} shell sendevent /dev/input/event0 3 57 0
	adb -s ${DEVICE} shell sendevent /dev/input/event0 0 2 0
	adb -s ${DEVICE} shell sendevent /dev/input/event0 0 0 0
	sleep ${MESH_DELAY}

config:
	adb -s ${DEVICE} shell mkdir -p ${DATA_DIR}
	adb -s ${DEVICE} push ${CONFIG} ${DATA_DIR}/config.xml

pickle:
	adb -s ${DEVICE} push ${PICKLE} ${DATA_DIR}/

start:
	adb -s ${DEVICE} shell am startservice ${HAGGLE_INTENT}

stop:
	adb -s ${DEVICE} shell ${HAGGLETEST} -x nop

log:
	adb -s ${DEVICE} shell cat ${DATA_DIR}/haggle.log | less

# Assume below that exactly two phones are connected.

SOURCE_ADDRESS = 192.168.0.1
DESTINATION_ADDRESS = 192.168.0.2
CONTENT_LOCATION = /data/data/org.haggle.kernel/files/plain.txt
CONTENT_SIZE = 100
CONTENT_DESCRIPTION = something
DELAY = 30
AUTHORITY_NAME = cbmen
ABE_ATTRIBUTE = Access

setup_two_devices:
	@echo '--- Get device names. ---'
	$(eval SOURCE_DEVICE = $(shell adb devices|cut -f1|head -2|tail -1))
	$(eval DESTINATION_DEVICE = $(shell adb devices|cut -f1|head -3|tail -1))
	@echo 'Source: '${SOURCE_DEVICE}
	@echo 'Destination: '${DESTINATION_DEVICE}
	@echo ''
	@echo '--- Uninstall old Haggles. ---'
	make uninstall DEVICE=${SOURCE_DEVICE} APP=${HAGGLE_APP}
	make uninstall DEVICE=${DESTINATION_DEVICE} APP=${HAGGLE_APP}
	@echo ''
	@echo '--- Reboot devices. ---'
	make reboot DEVICE=${SOURCE_DEVICE}
	make reboot DEVICE=${DESTINATION_DEVICE}
	@echo ''
	@echo '--- Wait for devices to initialize. ---'
	sleep ${DELAY}
	@echo ''
	@echo '--- Please wake the devices. Then press a key. ---'
	read
	@echo ''
	@echo '--- Unlock devices. ---'
	make unlock DEVICE=${SOURCE_DEVICE}
	make unlock DEVICE=${DESTINATION_DEVICE}	
	@echo ''
	@echo '--- Start meshes. ---'
	make mesh DEVICE=${SOURCE_DEVICE}
	make mesh DEVICE=${DESTINATION_DEVICE}
	@echo ''
	@echo '--- Configure network addresses. ---'
	make network DEVICE=${SOURCE_DEVICE} ADDRESS=${SOURCE_ADDRESS}
	make network DEVICE=${DESTINATION_DEVICE} ADDRESS=${DESTINATION_ADDRESS}
	@echo ''
	@echo '--- Install new Haggles. ---'
	make install DEVICE=${SOURCE_DEVICE} APK=${HAGGLE_APK}
	make install DEVICE=${DESTINATION_DEVICE} APK=${HAGGLE_APK}
	@echo ''
	@echo '--- Configure the Haggles. ---'
	make config DEVICE=${SOURCE_DEVICE} CONFIG=${CONFIG_USER}
	make config DEVICE=${DESTINATION_DEVICE} CONFIG=${CONFIG_AUTHORITY}
	@echo ''
	@echo '--- Copy the Charm parameters. ---'
	make pickle DEVICE=${SOURCE_DEVICE}
	make pickle DEVICE=${DESTINATION_DEVICE}
	@echo ''
	@echo '--- Start the Haggles. ---'
	make start DEVICE=${SOURCE_DEVICE}
	make start DEVICE=${DESTINATION_DEVICE}
	@echo ''

ping:
	make setup_two_devices
	@echo '--- Source pings destination. ---'
	$(eval SOURCE_DEVICE = $(shell adb devices|cut -f1|head -2|tail -1))
	adb -s ${SOURCE_DEVICE} shell ping ${DESTINATION_ADDRESS}

publish:
	adb -s ${DEVICE} shell dd if=/dev/zero of=${CONTENT_LOCATION} bs=${CONTENT_SIZE} count=1
	adb -s ${DEVICE} shell ${HAGGLETEST} -q pub file ${CONTENT_LOCATION} ${CONTENT_DESCRIPTION} ${POLICY}

subscribe:
	adb -s ${DEVICE} shell ${HAGGLETEST} -w 1 sub ${CONTENT_DESCRIPTION}

pubsub:
	make setup_two_devices
	@echo '--- Wait for source to initialize. ---'
	sleep ${DELAY}
	@echo ''
	$(eval SOURCE_DEVICE = $(shell adb devices|cut -f1|head -2|tail -1))
	$(eval DESTINATION_DEVICE = $(shell adb devices|cut -f1|head -3|tail -1))
	@echo '--- Source publishes content with description. ---'
	make publish DEVICE=${SOURCE_DEVICE} ${POLICY}
	@echo ''
	@echo '--- Destination subscribes to interest. ---'
	make subscribe DEVICE=${DESTINATION_DEVICE}
	@echo ''

abepubsub:
	make pubsub POLICY="${ABE_ATTRIBUTE}=${AUTHORITY_NAME}.${CONTENT_DESCRIPTION}"

source:
	$(eval SOURCE_DEVICE = $(shell adb devices|cut -f1|head -2|tail -1))
	adb -s ${SOURCE_DEVICE} shell

destination:
	$(eval DESTINATION_DEVICE = $(shell adb devices|cut -f1|head -3|tail -1))
	adb -s ${DESTINATION_DEVICE} shell

# Assume below that any number of (upto 255) devices are connected.

ADDRESS_PREFIX = 192.168.0.

address_all:
	@echo '--- Listing IP addresses of all devices. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make address DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

remove_ccb_all:
	@echo '--- Remove CCB from all devices. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make remove_ccb DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

uninstall_all:
	@echo '--- Uninstall from all devices. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make uninstall DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

install_all:
	@echo '--- Install on all devices. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make install DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

reboot_all:
	@echo '--- Reboot all devices. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make reboot DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

sync_all:
	@echo '--- Sync all devices. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make sync DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

unlock_all:
	@echo '--- Unlock all devices. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make unlock DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

empty_all:
	@echo '--- Empty files from all devices. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make empty DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

mesh_all:
	@echo '--- Start mesh on all devices. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make mesh DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

network_all:
	@echo '--- Configure network addresses. ---' ; \
	COUNT=1 ; for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make network DEVICE=$${CURRENT_DEVICE} ADDRESS=${ADDRESS_PREFIX}$$COUNT; \
		((COUNT = COUNT + 1)) ; \
	done
	@echo ''

start_all:
	@echo '--- Start the Haggles. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make start DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

stop_all:
	@echo '--- Stop the Haggles. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make stop DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

configure_all:
	@echo '--- Configure all Haggles as users. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make config DEVICE=$${CURRENT_DEVICE} CONFIG=${CONFIG_USER} ; \
	done
	@echo ''

pickle_all:
	@echo '--- Copy the default Charm parameters. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		make pickle DEVICE=$${CURRENT_DEVICE} ; \
	done
	@echo ''

remove_apps:
	@echo '--- Remove PhotoShare. ---' ; \
	make uninstall_all APP=${PHOTO_APP}
	@echo '--- Remove Haggle. ---' ; \
	make uninstall_all APP=${HAGGLE_APP}
	@echo '--- Remove Mesh. ---' ; \
	make uninstall_all APP=${MESH_APP}
	@echo '--- Remove KeepScreen. ---' ; \
	make uninstall_all APP=${SCREEN_APP}

add_helpers:
	@echo '--- Install helpers. ---' ; \
	for CURRENT_DEVICE in ${DEVICE_LIST} ; do \
		adb -s $${CURRENT_DEVICE} shell mount -o remount,rw -t yaffs2 /dev/block/mtdblock3 /system ; \
		adb -s $${CURRENT_DEVICE} push ${LOCAL_ARPHELPER} ${ARPHELPER} ; \
		adb -s $${CURRENT_DEVICE} shell chmod ugo+x ${ARPHELPER} ; \
		adb -s $${CURRENT_DEVICE} shell chmod +s ${ARPHELPER} ; \
		adb -s $${CURRENT_DEVICE} push ${LOCAL_HAGGLETEST} ${HAGGLETEST} ; \
		adb -s $${CURRENT_DEVICE} shell chmod ugo+x ${HAGGLETEST} ; \
	done
	@echo ''

add_apps:
	@echo '--- Add KeepScreen. ---' ; \
	make install_all APK=${SCREEN_APK}
	@echo '--- Add Mesh. ---' ; \
	make install_all APK=${MESH_APK}
	@echo '--- Add Haggle. ---' ; \
	make install_all APK=${HAGGLE_APK}
	@echo '--- Add PhotoShare. ---' ; \
	make install_all APK=${PHOTO_APK}

restart:
	make uninstall_all APP=${HAGGLE_APP}
	make empty_all
	make reboot_all
	@echo '--- Wait for devices to initialize. ---'
	sleep ${DELAY}
	@echo ''
	@echo '--- Please wake the devices. Then press a key. ---'
	read
	@echo ''
	make unlock_all
	make sync_all
	make mesh_all
	make network_all
	make install_all APK=${HAGGLE_APK}
	make configure_all
	make pickle_all
	make start_all
