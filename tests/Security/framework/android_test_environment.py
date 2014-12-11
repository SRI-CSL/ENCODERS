# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""

Android Test Environment - Test Environment for Android

"""

import sys
import os
import time
import subprocess
import shutil
import threading
import hashlib
import re
import glob
from base_test_environment import BaseTestEnvironment
from clitools import Console

NULLOUT = open('/dev/null', 'w')
class AndroidTestEnvironment(BaseTestEnvironment):

    def __init__(self, config):
        super(AndroidTestEnvironment, self).__init__(config)
        self.deviceIDs = []
        self.timenow = None

    def _setupEnvironment(self):

        Console.log('Finding devices before starting test.')
        self.deviceIDs = self._allDevicesPopulate()
        self._allDevicesClean()
        self._allDevicesReboot()

        self.sleep('Waiting for devices to reboot', self.config.deviceRebootDelay)

        deviceIDs = self._allDevicesPopulate()
        if len(deviceIDs) != len(self.deviceIDs):
            Console.warning('One or more devices did not connect after reboot. Please replug devices.')
            raw_input('Press enter to continue...')
            deviceIDs = self._allDevicesPopulate()
            if len(deviceIDs) != len(self.deviceIDs):
                raise Exception('Unable to bring all devices back online!')

        self.deviceIDs = deviceIDs

        self._allDevicesUnlock()
        self._allDevicesSyncTime()
        self.sleep('Waiting for devices to sync time', 1, False)
        self._allDevicesMesh()
        self.sleep('Waiting to setup mesh on devices', 1, False)

    def _createNode(self, name):
        node = self.nodes.addNode(name)

        if len(self.nodes) > len(self.deviceIDs):
            raise Exception('Have more nodes than devices, please re-run test with more devices plugged in!')

        node.androidDeviceID = self.deviceIDs[len(self.nodes) - 1]
        Console.log('Assigning device %s to node %s' % (node.name, node.androidDeviceID))

        self._startTask('Assigning device %s to node %s' % (node.name, node.androidDeviceID), self.config.deviceInstallDelay, False)
        address = '%s.%s' % (self.config.deviceMeshBaseIP, len(self.nodes))
        self._deviceNetwork(node.androidDeviceID, address)
        self._deviceInstall(node.androidDeviceID)
        self._endTask()
        node.threads = []
        return node

    def _startNode(self, name):
        if name not in self.nodes:
            raise Exception("Can't start nonexistent node %s!" % name)

        node = self.nodes[name]

        if node.running:
            raise Exception("Can't start already running node %s!" % name)

        if node.config is None:
            self.nodes.createConfig(node.name)

        with open(self.config.tmpConfigLocation, 'w') as out:
            out.write(node.config)

        self._deviceConfigure(node.androidDeviceID)
        self._deviceStart(node.androidDeviceID)
        self.sleep('Starting haggle on node %s' % node.name, self.config.haggleBootDelay)

        node.running = True

    def _stopNode(self, name):
        if name not in self.nodes:
            raise Exception("Can't stop nonexistent node %s!" % name)

        node = self.nodes[name]

        if not node.running:
            raise Exception("Can't stop already stopped node %s!" % name)

        self._deviceStop(node.androidDeviceID)
        self.sleep('Stopping haggle on node %s' % node.name, self.config.haggleStopDelay)

        node.running = False

    def _calculateHaggleNodeIDs(self):
        for node in self.nodes:
            self._deviceSaveFiles(node.androidDeviceID, self.config.tmpLogLocation, self.config.tmpConfigLocation, self.config.tmpDBLocation)
            grep = subprocess.Popen(['grep', 'calcId', self.config.tmpLogLocation], stdout=subprocess.PIPE)
            data = grep.communicate()[0]
            for line in data.split('\n')[:-1]:
                line = line.strip()
                id = line[-41:-1]
                node.haggleNodeID = id

            externalID = hashlib.sha1(node.androidDeviceID).hexdigest()
            if node.haggleNodeID != externalID:
                Console.warning('HaggleNodeID %s should match externally computed ID %s at node %s' % (node.haggleNodeID, externalID, node.name))
            Console.log('Node %s has haggle ID %s' % (node.name, node.haggleNodeID))

    def _calculateHaggleNodeIDsExternally(self):
        for node in self.nodes:
            externalID = hashlib.sha1(node.androidDeviceID).hexdigest()
            node.haggleNodeID = externalID.split()[0]
            Console.log('Node %s has externally computed haggle ID %s' % (node.name, node.haggleNodeID))

    def _shutdownEnvironment(self):
        if not os.path.isdir(self.config.logDirectory):
            os.makedirs(self.config.logDirectory)
        logDirectory = os.path.abspath(self.config.logDirectory)
        self._cleanup()
        for node in self.nodes:
            self._deviceSaveFiles(node.androidDeviceID, '%s/%s_haggle.log' % (logDirectory, node.name), '%s/%s_config.xml' % (logDirectory, node.name), '%s/%s_haggle.db' % (logDirectory, node.name))
        for filename in (glob.glob('/tmp/*_batchpubs.*') + glob.glob('/tmp/*_batchsubs_*.log')):
            shutil.move(filename, '%s/%s' % (logDirectory, os.path.basename(filename)))

    def _cleanup(self):
        self._startTask('Cleaning up', self.config.haggleStopDelay + 1, False)
        self._allDevicesStop()
        time.sleep(self.config.haggleStopDelay)
        os.popen('rm -rf %s %s %s %s %s' % (self.config.tmpLogLocation, self.config.tmpConfigLocation, self.config.openSSLTmpCertFolder, self.config.openSSLCAFolder, self.config.openSSLConfigFile))
        self._endTask()

    def _deleteHaggleDBAtNode(self, name):
        if name not in self.nodes:
            raise Exception("Can't publish at nonexistent node %s!" % name)

        node = self.nodes[name]
        self._startTask('Deleting Haggle DB at %s' % (node.name, ), self.config.deleteHaggleDBDelay)
        os.popen('%s -s %s shell rm -f %s' % (self.config.adbPath, node.androidDeviceID, self.config.deviceHaggleDBFile))
        self._endTask()

    def _publishItem(self, name, descriptor, policy, attributes):
        if name not in self.nodes:
            raise Exception("Can't publish at nonexistent node %s!" % name)

        node = self.nodes[name]
        subprocess.call([self.config.adbPath, '-s', node.androidDeviceID, 'shell', 
                        'dd', 'if=/dev/urandom', 'of=%s/%s.txt' % (self.config.deviceHaggleDataDirectory, descriptor), 
                        'bs=%s' % self.config.testContentSize, 'count=1'], stdout=NULLOUT, stderr=NULLOUT)

        publish_string = ''
        if policy == '':
            publish_string = '%s -q pub file %s/%s.txt %s %s' % (self.config.deviceHaggleTest, self.config.deviceHaggleDataDirectory, descriptor, descriptor, ' '.join(attributes))
        else:
            publish_string = "%s -q pub file %s/%s.txt %s %s Access='%s'" % (self.config.deviceHaggleTest, self.config.deviceHaggleDataDirectory, descriptor, descriptor, ' '.join(attributes), policy)

        self._startTask('Publishing at %s: %s' % (node.name, publish_string), self.config.publishDelay)
        os.popen('%s -s %s shell %s' % (self.config.adbPath, node.androidDeviceID, publish_string))
        time.sleep(self.config.publishDelay * self.config.sleepFactor)
        self._endTask()

    def _subscribeItem(self, name, descriptor):
        if name not in self.nodes:
            raise Exception("Can't subscribe at nonexistent node %s!" % name)

        node = self.nodes[name]
        subscribe_string = '%s -c -s %s -w 1 sub %s' % (self.config.deviceHaggleTest, self.config.subscribeTimeout * self.config.sleepFactor, descriptor)

        self._startTask('Subscribing at %s: %s' % (node.name, subscribe_string), self.config.subscribeTimeout)
        result = os.popen('%s -s %s shell %s' % (self.config.adbPath, node.androidDeviceID, subscribe_string))
        self._endTask()

        for line in result:
            if line.find('Number of data objects received') != -1:
                objectCount = line.split()[-1]
                if objectCount == '0':
                    Console.log('Failed to receive %s at %s!' % (descriptor, node.name))
                    return False
                else:
                    Console.log('Successfully received %s at %s!' % (descriptor, node.name))
                    return True

        raise Exception('haggletest failed at node %s!' % node.name)

    def _dynamicAddRoleSharedSecretsAtNode(self, name, *roles):
        if name not in self.nodes:
            raise Exception("Can't add role shared secrets at nonexistent node %s!" % name)

        node = self.nodes[name]

        role_string = '%s addRoleSharedSecrets ' % self.config.deviceHaggleTest
        for (name, ss) in roles:
            role_string += '%s=%s ' % (name, ss)
        role_string = role_string[:-1]

        self._startTask('Adding role shared secrets at %s: %s' % (node.name, role_string), self.config.dynamicConfigurationDelay)
        os.popen('%s -s %s shell %s' % (self.config.adbPath, node.androidDeviceID, role_string))
        time.sleep(self.config.dynamicConfigurationDelay * self.config.sleepFactor)
        self._endTask()

    def _dynamicAddNodeSharedSecretsAtNode(self, name, *nodes):
        if name not in self.nodes:
            raise Exception("Can't add node shared secrets at nonexistent node %s!" % name)

        node = self.nodes[name]

        node_string = '%s addNodeSharedSecrets ' % self.config.deviceHaggleTest
        for (id, ss) in nodes:
            node_string += '%s=%s ' % (id, ss)
        node_string = node_string[:-1]

        self._startTask('Adding node shared secrets at %s: %s' % (node.name, node_string), self.config.dynamicConfigurationDelay)
        os.popen('%s -s %s shell %s' % (self.config.adbPath, node.androidDeviceID, node_string))
        time.sleep(self.config.dynamicConfigurationDelay * self.config.sleepFactor)
        self._endTask()

    def _dynamicAddAuthoritiesAtNode(self, name, *authorities):
        if name not in self.nodes:
            raise Exception("Can't add authorities at nonexistent node %s!" % name)

        node = self.nodes[name]

        auth_string = '%s addAuthorities ' % self.config.deviceHaggleTest
        for (name, id) in authorities:
            auth_string += '%s=%s ' % (name, id)
        auth_string = auth_string[:-1]

        self._startTask('Adding authorities at %s: %s' % (node.name, auth_string), self.config.dynamicConfigurationDelay)
        os.popen('%s -s %s shell %s' % (self.config.adbPath, node.androidDeviceID, auth_string))
        time.sleep(self.config.dynamicConfigurationDelay * self.config.sleepFactor)
        self._endTask()

    def _dynamicAuthorizeNodesForCertificationAtNode(self, name, *ids):
        if name not in self.nodes:
            raise Exception("Can't authorize nodes for certification at nonexistent node %s!" % name)

        node = self.nodes[name]

        add_string = '%s authorizeNodesForCertification ' % self.config.deviceHaggleTest
        for id in ids:
            add_string += '%s ' % id
        add_string = add_string[:-1]

        self._startTask('Authorizing nodes for certification at %s: %s' % (node.name, add_string), self.config.dynamicConfigurationDelay)
        os.popen('%s -s %s shell %s' % (self.config.adbPath, node.androidDeviceID, add_string))
        time.sleep(self.config.dynamicConfigurationDelay * self.config.sleepFactor)
        self._endTask()

    def _dynamicAuthorizeRoleForAttributesAtNode(self, name, role, encryption, decryption):
        if name not in self.nodes:
            raise Exception("Can't authorize role for attributes at nonexistent node %s!" % name)

        node = self.nodes[name]

        role_string = '%s authorizeRoleForAttributes %s %s %s ' % (self.config.deviceHaggleTest, role, len(encryption), len(decryption))
        for attr in (encryption + decryption):
            role_string += '%s ' % attr
        role_string = role_string[:-1]

        self._startTask('Authorizing role for attributes at %s: %s' % (node.name, role_string), self.config.dynamicConfigurationDelay)
        os.popen('%s -s %s shell %s' % (self.config.adbPath, node.androidDeviceID, role_string))
        time.sleep(self.config.dynamicConfigurationDelay * self.config.sleepFactor)
        self._endTask()

    def _countMatchingLinesInLog(self, name, pattern):
        if name not in self.nodes:
            raise Exception("Can't count lines in log for nonexistent node %s!" % name)

        node = self.nodes[name]
        subprocess.call([self.config.adbPath, '-s', node.androidDeviceID, 'pull', self.config.deviceHaggleLogFile, self.config.tmpLogLocation], stdout=NULLOUT, stderr=NULLOUT)
        grep = subprocess.Popen(['grep', '-c', '-P', pattern, self.config.tmpLogLocation], stdout=subprocess.PIPE)
        data = grep.communicate()[0]
        for line in data.split('\n')[:-1]:
            line = line.strip()
            return int(line)

    def _limitCPU(self, limit):
        Console.log("Swallowing limitcpu command on android")

    def _asyncAction(self, node, command, duration, copyFrom, copyTo):
        os.popen('%s -s %s shell %s' % (self.config.adbPath, node.androidDeviceID, command))
        # time.sleep(duration)
        subprocess.call([self.config.adbPath, '-s', node.androidDeviceID, 'pull', copyFrom, copyTo], stdout=NULLOUT, stderr=NULLOUT)

    def _batchPublishAtNode(self, name, duration, *commands):
        if name not in self.nodes:
            raise Exception("Can't batch publish at nonexistent node %s!" % name)

        node = self.nodes[name]
        duration = duration * self.config.sleepFactor

        fileSizes = [command[2] for command in commands]
        uniqFileSizes = list(set(fileSizes))
        for fileSize in uniqFileSizes:
            subprocess.call([self.config.adbPath, '-s', node.androidDeviceID, 'shell', 
                        'dd', 'if=/dev/urandom', 'of=%s/%s_%s.txt' % (self.config.deviceHaggleDataDirectory, name, fileSize), 
                        'bs=%s' % fileSize, 'count=1'], stdout=NULLOUT, stderr=NULLOUT)

        with open('/tmp/%s_batchpubs.txt' % name, 'w') as output:
            for command in commands:
                (wait, attribute, fileSize) = command
                attributeString = ';'.join('%s=%s' % (k,v) for (k,v) in attribute)
                commandString = '%.2f,pub,"%s","%s/%s_%s.txt"\n' % (wait*self.config.sleepFactor, attributeString, self.config.deviceHaggleDataDirectory, name, fileSize)
                output.write(commandString)

        subprocess.call([self.config.adbPath, '-s', node.androidDeviceID, 'push', '/tmp/%s_batchpubs.txt' % name, '%s/%s_batchpubs.txt' % (self.config.deviceHaggleDataDirectory, name)], stdout=NULLOUT, stderr=NULLOUT)
        uid = os.popen('%s -s %s %s' % (self.config.adbPath, node.androidDeviceID, self.config.deviceGetHaggleUIDCommand)).readline().replace('\n', '')
        os.popen('%s -s %s shell chown -R %s %s' % (self.config.adbPath, node.androidDeviceID, uid, self.config.deviceHaggleDataDirectory))

        publishString = '%s -p %s -j %s -f %s/%s_batchpubs.log -g %s/%s_batchpubs.txt appPUB' % (self.config.deviceHaggleTest, duration, name, self.config.deviceHaggleDataDirectory, name, self.config.deviceHaggleDataDirectory, name)

        Console.log('Batch publishing at %s: %s' % (name, publishString))
        thread = threading.Thread(target=self._asyncAction, args=(node, publishString, duration, '%s/%s_batchpubs.log' % (self.config.deviceHaggleDataDirectory, name), '/tmp/%s_batchpubs.log' % name))
        node.threads.append(thread)
        thread.start()

    def _batchSubscribeAtNode(self, name, duration, descriptor):
        if name not in self.nodes:
            raise Exception("Can't batch subscribe at nonexistent node %s!" % name)

        node = self.nodes[name]
        duration = duration * self.config.sleepFactor

        subscribeString = '%s -d -a -f %s/%s_batchsubs_%s.log -c -s %s %s sub %s=%s' % (self.config.deviceHaggleTest, self.config.deviceHaggleDataDirectory, name, descriptor, duration, descriptor, descriptor, descriptor)

        Console.log('Batch subscribing at %s: %s' % (name, subscribeString))
        thread = threading.Thread(target=self._asyncAction, args=(node, subscribeString, duration, '%s/%s_batchsubs_%s.log' % (self.config.deviceHaggleDataDirectory, name, descriptor), '/tmp/%s_batchsubs_%s.log' % (name, descriptor)))
        node.threads.append(thread)
        thread.start()

    def _deviceNetwork(self, deviceID, address):
        os.popen('%s -s %s shell busybox ifconfig bat0 down' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell busybox ifconfig eth0 down' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell busybox ifconfig eth0 %s' % (self.config.adbPath, deviceID, address))

    def _deviceMesh(self, deviceID):
        os.popen('%s -s %s shell svc wifi disable' % (self.config.adbPath, deviceID))
        time.sleep(1)
        os.popen('%s -s %s shell am start -a android.intent.action.MAIN -n android.tether/.MainActivity --activity-brought-to-front' % (self.config.adbPath, deviceID))
        time.sleep(2)
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 53 506' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 54 568' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 48 50' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 50 5' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 57 0' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 0 2 0' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 0 0 0' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 53 506' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 54 568' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 48 0' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 50 5' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 3 57 0' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 0 2 0' % (self.config.adbPath, deviceID))
        os.popen('%s -s %s shell sendevent /dev/input/event0 0 0 0' % (self.config.adbPath, deviceID))
        time.sleep(2)

    def _deviceSyncTime(self, deviceID):
        os.popen('%s -s %s shell date -s %s' % (self.config.adbPath, deviceID, self.timenow))
        os.popen('%s -s %s shell "mount -o remount,exec /dev/block/vold/179:0 /mnt/sdcard"' % (self.config.adbPath, deviceID))

    def _deviceUninstall(self, deviceID):
        os.popen('%s -s %s uninstall %s' % (self.config.adbPath, deviceID, self.config.deviceHaggleApp))

    def _deviceInstall(self, deviceID):
        subprocess.call([self.config.adbPath, '-s', deviceID, 'install', '-r', self.config.deviceHaggleAPK], stdout=NULLOUT, stderr=NULLOUT)
        subprocess.call([self.config.adbPath, '-s', deviceID, 'shell', 'mkdir', '-p', self.config.deviceHaggleDataDirectory], stdout=NULLOUT, stderr=NULLOUT)

    def _deviceConfigure(self, deviceID):
        os.popen('%s -s %s shell mkdir -p %s' % (self.config.adbPath, deviceID, self.config.deviceHaggleDataDirectory))
        subprocess.call([self.config.adbPath, '-s', deviceID, 'push', self.config.tmpConfigLocation, self.config.deviceHaggleConfigFile], stdout=NULLOUT, stderr=NULLOUT)
        uid = os.popen('%s -s %s %s' % (self.config.adbPath, deviceID, self.config.deviceGetHaggleUIDCommand)).readline().replace('\n', '')
        os.popen('%s -s %s shell chown -R %s %s' % (self.config.adbPath, deviceID, uid, self.config.deviceHaggleDataDirectory))

    def _deviceStart(self, deviceID):
        os.popen('%s -s %s shell %s' % (self.config.adbPath, deviceID, self.config.deviceHaggleIntent))

    def _deviceStop(self, deviceID):
        os.popen('%s -s %s shell %s -x nop' % (self.config.adbPath, deviceID, self.config.deviceHaggleTest))

    def _deviceReboot(self, deviceID):
        os.popen('%s -s %s reboot' % (self.config.adbPath, deviceID))

    def _deviceUnlock(self, deviceID):
        os.popen('%s -s %s shell input keyevent 82' % (self.config.adbPath, deviceID))

    def _deviceClean(self, deviceID):
        os.popen('%s -s %s shell rm %s/haggle.db' % (self.config.adbPath, deviceID, self.config.deviceHaggleDataDirectory))
        os.popen('%s -s %s shell rm %s/haggle.log' % (self.config.adbPath, deviceID, self.config.deviceHaggleDataDirectory))
        os.popen('%s -s %s shell rm %s/trace.log' % (self.config.adbPath, deviceID, self.config.deviceHaggleDataDirectory))
        os.popen('%s -s %s shell rm /sdcard/haggle/*' % (self.config.adbPath, deviceID))

    def _deviceSaveFiles(self, deviceID, logPath, configPath, dbPath):
        subprocess.call([self.config.adbPath, '-s', deviceID, 'pull', self.config.deviceHaggleLogFile, logPath], stdout=NULLOUT, stderr=NULLOUT)
        subprocess.call([self.config.adbPath, '-s', deviceID, 'pull', self.config.deviceHaggleConfigFile, configPath], stdout=NULLOUT, stderr=NULLOUT)
        subprocess.call([self.config.adbPath, '-s', deviceID, 'pull', self.config.deviceHaggleDBFile, dbPath], stdout=NULLOUT, stderr=NULLOUT)

    def _allDevicesPopulate(self):
        deviceIDs = []
        stream = os.popen(self.config.deviceGetADBDeviceListCommand)
        for line in stream:
            line = line.strip()
            deviceIDs.append(line.split()[0])
            Console.log('Found device ID %s' % deviceIDs[-1])
        return deviceIDs

    def _allDevicesClean(self):
        Console.log('Removing any existing haggle files from all devices!')
        for deviceID in self.deviceIDs:
            self._deviceClean(deviceID)
            self._deviceUninstall(deviceID)

    def _allDevicesAction(self, deviceIDs, target, message):
        threads = []
        for deviceID in deviceIDs:
            Console.log(message % deviceID)
            thread = threading.Thread(target=target, args=(deviceID,))
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()

    def _allDevicesReboot(self):
        Console.log('Rebooting all devices!')
        self._allDevicesAction(self.deviceIDs, self._deviceReboot, 'Rebooting device %s')

    def _allDevicesUnlock(self):
        Console.log('Unlocking all devices!')
        self._allDevicesAction(self.deviceIDs, self._deviceUnlock, 'Unlocking device %s')

    def _allDevicesMesh(self):
        Console.log('Setting up environment for haggle on all devices!')
        self._allDevicesAction(self.deviceIDs, self._deviceMesh, 'Disabling wifi and launching mesh on device %s')

    def _allDevicesStop(self):
        Console.log('Stopping all devices!')
        self._allDevicesAction(self.deviceIDs, self._deviceStop, 'Stopping device %s')

    def _allDevicesSyncTime(self):
        self.timenow = os.popen("date +%Y%m%d.%H%M%S").readline().replace('\n', '')
        Console.log('Syncing time on all devices! Setting to %s' % self.timenow)
        self._allDevicesAction(self.deviceIDs, self._deviceSyncTime, 'Syncing time on device %s') 
