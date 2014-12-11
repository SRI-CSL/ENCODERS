# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""

Core Test Environment - Test Environment for CORE

"""

import sys
import os
import time
import subprocess
import shutil
import random
import threading
import glob
from base_test_environment import BaseTestEnvironment
from clitools import Console
import imp

NULLOUT = open('/dev/null', 'w')
class CoreTestEnvironment(BaseTestEnvironment):

    def __init__(self, config):
        super(CoreTestEnvironment, self).__init__(config)

    def _setupEnvironment(self):
        sys.path.append(self.config.coredLocation)
        imp.load_source('cored', self.config.coredLocation)
        from core import pycore
        from core.api import coreapi
        from core.mobility import BasicRangeModel
        from core.service import CoreService, addservice
        from core.misc.ipaddr import IPv4Prefix
        from haggle_core_service import HaggleService

        self.pycore = pycore
        self.haggleService = HaggleService

        Console.log('Creating core session')

        self.haggleService.updateFrameworkConfig(self.config)
        self.session = pycore.Session(persistent=True)
        addservice(self.haggleService)

        self.wlan = self.session.addobj(cls=self.pycore.nodes.WlanNode, name='wlan', objid=999)
        values = BasicRangeModel.getdefaultvalues()
        self.wlan.setmodel(BasicRangeModel, values)
        self.wlan.model.range = self.config.coreWLANRange
        self.wlan.setposition(x=self.config.coreWLANX, y=self.config.coreWLANY)

        Console.log('Added wlan node with range %s' % self.wlan.model.range)

    def _createNode(self, name):
        node = self.nodes.addNode(name)
        node.coreDeviceID = len(self.nodes) - 1

        if self.config.coreLinearTopology:
            node_x = (node.coreDeviceID * (self.config.coreWLANRange - self.config.coreRangeMargin)) + self.config.coreEdgeOffset
            node_y = self.config.coreEdgeOffset
        else:
            node_x = random.randint(self.config.coreMinX, self.config.coreMaxX)
            node_y = random.randint(self.config.coreMinY, self.config.coreMaxY)

        node.coreNodeName = 'n%d' % node.coreDeviceID
        node.coreNode = self.session.addobj(cls=self.pycore.nodes.CoreNode, name=node.coreNodeName, objid=node.coreDeviceID)
        node.coreNode.type = 'PC'
        node.coreNode.setposition(x=node_x, y=node_y)
        node.coreNode.newnetif(self.wlan, ['10.0.0.%d/24' % (node.coreDeviceID + 1)])
        self.session.services.addservicestonode(node.coreNode, 'PC', 'DefaultRoute|HaggleService', False)
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

        self.session.services.bootnodeservices(node.coreNode)
        self.sleep('Starting haggle on node %s' % node.name, self.config.haggleBootDelay)

        node.running = True

    def _stopNode(self, name):
        if name not in self.nodes:
            raise Exception("Can't stop nonexistent node %s!" % name)

        node = self.nodes[name]

        if not node.running:
            raise Exception("Can't stop already stopped node %s!" % name)

        self.session.services.stopnodeservice(node.coreNode, self.haggleService)
        self.sleep('Stopping haggle on node %s' % node.name, self.config.haggleStopDelay)

        node.running = False

    def _calculateHaggleNodeIDs(self):
        for node in self.nodes:
            node.coreNode.cmd(['/bin/su', '-l', '%s' % self.config.whoami, '-c', 'cp .Haggle/haggle.log %s' % self.config.tmpLogLocation])
            grep = subprocess.Popen(['grep', 'calcId', self.config.tmpLogLocation], stdout=subprocess.PIPE)
            data = grep.communicate()[0]
            for line in data.split('\n')[:-1]:
                line = line.strip()
                id = line[-41:-1]
                node.haggleNodeID = id

            status, externalID = node.coreNode.cmdresult(['/bin/su', '-c', self.config.coreExternalHaggleNodeIDCommand])
            externalID = externalID.split()[0]
            if node.haggleNodeID != externalID:
                Console.warning('HaggleNodeID %s should match externally computed ID %s at node %s' % (node.haggleNodeID, externalID, node.name))
            Console.log('Node %s has haggle ID %s' % (node.name, node.haggleNodeID))

    def _calculateHaggleNodeIDsExternally(self):
        for node in self.nodes:
            status, externalID = node.coreNode.cmdresult(['/bin/su', '-c', self.config.coreExternalHaggleNodeIDCommand])
            node.haggleNodeID = externalID.split()[0]
            Console.log('Node %s has externally computed haggle ID %s' % (node.name, node.haggleNodeID))

    def _shutdownEnvironment(self):
        if not os.path.isdir(self.config.logDirectory):
            os.makedirs(self.config.logDirectory)
        logDirectory = os.path.abspath(self.config.logDirectory)

        for node in self.nodes:
            node.coreNode.cmd(['/bin/su', '-l', self.config.whoami, '-c', 'cp .Haggle/haggle.log /tmp/n%s_haggle.log' % node.coreDeviceID])
            shutil.move('/tmp/n%s_haggle.log' % node.coreDeviceID, '%s/%s_haggle.log' % (logDirectory, node.name))
            node.coreNode.cmd(['/bin/su', '-l', self.config.whoami, '-c', 'cp .Haggle/haggle.db /tmp/n%s_haggle.db' % node.coreDeviceID])
            shutil.move('/tmp/n%s_haggle.db' % node.coreDeviceID, '%s/%s_haggle.db' % (logDirectory, node.name))
            node.coreNode.cmd(['/bin/su', '-l', self.config.whoami, '-c', 'cp .Haggle/config.xml /tmp/n%s_config.xml' % node.coreDeviceID])
            shutil.move('/tmp/n%s_config.xml' % node.coreDeviceID, '%s/%s_config.xml' % (logDirectory, node.name))

        for filename in (glob.glob('/tmp/*_batchpubs.*') + glob.glob('/tmp/*_batchsubs_*.log')):
            shutil.move(filename, '%s/%s' % (logDirectory, os.path.basename(filename)))

        os.popen('chown -R %s:%s %s' % (self.config.whoami, self.config.whoami, logDirectory))

        self._cleanup()

    def _cleanup(self):
        self._startTask('Cleaning up', 1, False)
        self.session.shutdown()
        time.sleep(1)
        os.popen('rm -rf /tmp/pycore.* %s %s %s %s %s' % (self.config.tmpLogLocation, self.config.tmpConfigLocation, self.config.openSSLTmpCertFolder, self.config.openSSLCAFolder, self.config.openSSLConfigFile))
        subprocess.call([c for c in self.config.coreCleanupLimitCPUCommandList], stdout=NULLOUT, stderr=NULLOUT)
        self._endTask()

    def _deleteHaggleDBAtNode(self, name):
        if name not in self.nodes:
            raise Exception("Can't publish at nonexistent node %s!" % name)

        node = self.nodes[name]
        self._startTask('Deleting Haggle DB at %s' % (node.name, ), self.config.deleteHaggleDBDelay)
        status, result = node.coreNode.cmdresult(['rm', '-f', 'home.%s..Haggle./haggle.db' % self.config.whoami])
        self._endTask()

    def _publishItem(self, name, descriptor, policy, attributes):
        if name not in self.nodes:
            raise Exception("Can't publish at nonexistent node %s!" % name)

        node = self.nodes[name]
        subprocess.call(['dd', 'if=/dev/urandom', 'of=%s/%s.txt' % (self.config.coreHaggleDataDirectory, descriptor), 
                        'bs=%s' % self.config.testContentSize, 'count=1'], stdout=NULLOUT, stderr=NULLOUT)

        publish_string = ''
        if policy == '':
            publish_string = '%s -q pub file %s/%s.txt %s %s' % (self.config.coreHaggleTest, self.config.coreHaggleDataDirectory, descriptor, descriptor, ' '.join(attributes))
        else:
            publish_string = "%s -q pub file %s/%s.txt %s %s Access='%s'" % (self.config.coreHaggleTest, self.config.coreHaggleDataDirectory, descriptor, descriptor, ' '.join(attributes), policy)

        self._startTask('Publishing at %s: %s' % (node.name, publish_string), self.config.publishDelay)
        status, result = node.coreNode.cmdresult(['/bin/su', '-l', self.config.whoami, '-c', publish_string])
        self._endTask()

        # Console.info('status: %s' % status)

    def _subscribeItem(self, name, descriptor):
        if name not in self.nodes:
            raise Exception("Can't subscribe at nonexistent node %s!" % name)

        node = self.nodes[name]
        subscribe_string = '%s -c -s %s -w 1 sub %s' % (self.config.coreHaggleTest, self.config.subscribeTimeout * self.config.sleepFactor, descriptor)

        self._startTask('Subscribing at %s: %s' % (node.name, subscribe_string), self.config.subscribeTimeout)
        status, result = node.coreNode.cmdresult(['/bin/su', '-l', self.config.whoami, '-c', subscribe_string])
        self._endTask()

        for line in result.split('\n'):
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

        role_string = '%s addRoleSharedSecrets ' % self.config.coreHaggleTest
        for (name, ss) in roles:
            role_string += '%s=%s ' % (name, ss)
        role_string = role_string[:-1]

        self._startTask('Adding role shared secrets at %s: %s' % (node.name, role_string), self.config.dynamicConfigurationDelay)
        status, result = node.coreNode.cmdresult(['/bin/su', '-l', self.config.whoami, '-c', role_string])
        self._endTask()

    def _dynamicAddNodeSharedSecretsAtNode(self, name, *nodes):
        if name not in self.nodes:
            raise Exception("Can't add node shared secrets at nonexistent node %s!" % name)

        node = self.nodes[name]

        node_string = '%s addNodeSharedSecrets ' % self.config.coreHaggleTest
        for (id, ss) in nodes:
            node_string += '%s=%s ' % (id, ss)
        node_string = node_string[:-1]

        self._startTask('Adding node shared secrets at %s: %s' % (node.name, node_string), self.config.dynamicConfigurationDelay)
        status, result = node.coreNode.cmdresult(['/bin/su', '-l', self.config.whoami, '-c', node_string])
        self._endTask()

    def _dynamicAddAuthoritiesAtNode(self, name, *authorities):
        if name not in self.nodes:
            raise Exception("Can't add authorities at nonexistent node %s!" % name)

        node = self.nodes[name]

        auth_string = '%s addAuthorities ' % self.config.coreHaggleTest
        for (name, id) in authorities:
            auth_string += '%s=%s ' % (name, id)
        auth_string = auth_string[:-1]

        self._startTask('Adding authorities at %s: %s' % (node.name, auth_string), self.config.dynamicConfigurationDelay)
        status, result = node.coreNode.cmdresult(['/bin/su', '-l', self.config.whoami, '-c', auth_string])
        self._endTask()

    def _dynamicAuthorizeNodesForCertificationAtNode(self, name, *ids):
        if name not in self.nodes:
            raise Exception("Can't authorize nodes for certification at nonexistent node %s!" % name)

        node = self.nodes[name]

        add_string = '%s authorizeNodesForCertification ' % self.config.coreHaggleTest
        for id in ids:
            add_string += '%s ' % id
        add_string = add_string[:-1]

        self._startTask('Authorizing nodes for certification at %s: %s' % (node.name, add_string), self.config.dynamicConfigurationDelay)
        status, result = node.coreNode.cmdresult(['/bin/su', '-l', self.config.whoami, '-c', add_string])
        self._endTask()

    def _dynamicAuthorizeRoleForAttributesAtNode(self, name, role, encryption, decryption):
        if name not in self.nodes:
            raise Exception("Can't authorize role for attributes at nonexistent node %s!" % name)

        node = self.nodes[name]

        role_string = '%s authorizeRoleForAttributes %s %s %s ' % (self.config.coreHaggleTest, role, len(encryption), len(decryption))
        for attr in (encryption + decryption):
            role_string += '%s ' % attr
        role_string = role_string[:-1]

        self._startTask('Authorizing role for attributes at %s: %s' % (node.name, role_string), self.config.dynamicConfigurationDelay)
        status, result = node.coreNode.cmdresult(['/bin/su', '-l', self.config.whoami, '-c', role_string])
        self._endTask()

    def _countMatchingLinesInLog(self, name, pattern):
        if name not in self.nodes:
            raise Exception("Can't count lines in log for nonexistent node %s!" % name)

        node = self.nodes[name]
        node.coreNode.cmd(['/bin/su', '-l', '%s' % self.config.whoami, '-c', 'cp .Haggle/haggle.log %s' % self.config.tmpLogLocation])
        grep = subprocess.Popen(['grep', '-c', '-P', pattern, self.config.tmpLogLocation], stdout=subprocess.PIPE)
        data = grep.communicate()[0]
        for line in data.split('\n')[:-1]:
            line = line.strip()
            return int(line)

    def _limitCPU(self, limit):
        if limit >= 100:
            Console.warning("Swallowing limitcpu >= 100 command!")

        Console.log("Limiting CPU to %s" % limit)
        process = subprocess.Popen([c for c in self.config.coreHagglePIDCommandList], stdout=subprocess.PIPE, stderr=NULLOUT)
        pids = process.communicate()[0]
        pids = [int(p) for p in pids.split('\n')[:-1]]
        for pid in pids:
            dic = dict(pid=pid, cpulimit=limit)
            subprocess.call([c.format(**dic) for c in self.config.coreLimitCPUCommandList], stdout=NULLOUT, stderr=NULLOUT)

    def _asyncAction(self, node, command):
        status, result = node.coreNode.cmdresult(['/bin/su', '-l', self.config.whoami, '-c', command])

    def _batchPublishAtNode(self, name, duration, *commands):
        if name not in self.nodes:
            raise Exception("Can't batch publish at nonexistent node %s!" % name)

        node = self.nodes[name]
        duration = duration * self.config.sleepFactor

        fileSizes = [command[2] for command in commands]
        uniqFileSizes = list(set(fileSizes))
        for fileSize in uniqFileSizes:
            subprocess.call(['dd', 'if=/dev/urandom', 'of=%s/%s_%s.txt' % (self.config.coreHaggleDataDirectory, name, fileSize), 
                            'bs=%s' % fileSize, 'count=1'], stdout=NULLOUT, stderr=NULLOUT)

        with open('/tmp/%s_batchpubs.txt' % name, 'w') as output:
            for command in commands:
                (wait, attribute, fileSize) = command
                attributeString = ';'.join('%s=%s' % (k,v) for (k,v) in attribute)
                commandString = '%.2f,pub,"%s","%s/%s_%s.txt"\n' % (wait*self.config.sleepFactor, attributeString, self.config.coreHaggleDataDirectory, name, fileSize)
                output.write(commandString)

        publishString = '%s -p %s -j %s -f /tmp/%s_batchpubs.log -g /tmp/%s_batchpubs.txt appPUB' % (self.config.coreHaggleTest, duration, name, name, name)

        Console.log('Batch publishing at %s: %s' % (name, publishString))
        thread = threading.Thread(target=self._asyncAction, args=(node,publishString))
        node.threads.append(thread)
        thread.start()

    def _batchSubscribeAtNode(self, name, duration, descriptor):
        if name not in self.nodes:
            raise Exception("Can't batch subscribe at nonexistent node %s!" % name)

        node = self.nodes[name]
        duration = duration * self.config.sleepFactor

        subscribeString = '%s -d -a -f /tmp/%s_batchsubs_%s.log -c -s %s %s sub %s=%s' % (self.config.coreHaggleTest, name, descriptor, duration, descriptor, descriptor, descriptor)

        Console.log('Batch subscribing at %s: %s' % (name, subscribeString))
        thread = threading.Thread(target=self._asyncAction, args=(node,subscribeString))
        node.threads.append(thread)
        thread.start()
