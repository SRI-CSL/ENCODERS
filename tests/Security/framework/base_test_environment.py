# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""

Base Test Environment class.

"""

import signal
import sys
import traceback
import functools
import time
from clitools import ProgressBar, Console
from node_manager import NodeManager

class BaseTestEnvironment(object):

    def __init__(self, config):
        self.config = config
        self.nodes = NodeManager(config)
        signal.signal(signal.SIGINT, self._signalHandler)
        sys.excepthook = self._exceptHook

    def _startTask(self, message, duration, report=True):
        ProgressBar.startTask(message, duration*self.config.sleepFactor, report)

    def _endTask(self):
        ProgressBar.endTask()

    def _signalHandler(self, signal, frame):
        Console.warning('ctrl-c received, cleaning up...')
        ProgressBar.setETA(0)
        try:
            self._shutdownEnvironment()
        except NotImplementedError:
            pass
        Console.info('Exiting after cleaning up.')
        sys.exit(1)

    def _exceptHook(self, type, value, tb):
        Console.fail('Uncaught exception of type: %s' % str(type))
        ProgressBar.setETA(0)
        traceback.print_exception(type, value, tb)
        try:
            self._shutdownEnvironment()
        except NotImplementedError:
            pass
        Console.info('Exiting after cleaning up.')
        sys.exit(1)
    
    def sleep(self, message, duration, report=True):
        self._startTask(message, duration, report)
        time.sleep(duration*self.config.sleepFactor)
        self._endTask()

    def setupEnvironment(self):
        self._setupEnvironment()

    def _setupEnvironment(self):
        raise NotImplementedError()

    def createNode(self, name):
        node = self._createNode(name)

        node.start = functools.partial(self.startNode, name)
        node.stop = functools.partial(self.stopNode, name)
        node.publishItem = functools.partial(self.publishItem, name)
        node.subscribeItem = functools.partial(self.subscribeItem, name)
        node.dynamicAddRoleSharedSecrets = functools.partial(self.dynamicAddRoleSharedSecretsAtNode, name)
        node.dynamicAddNodeSharedSecrets = functools.partial(self.dynamicAddNodeSharedSecretsAtNode, name)
        node.dynamicAddAuthorities = functools.partial(self.dynamicAddAuthoritiesAtNode, name)
        node.dynamicAuthorizeNodesForCertification = functools.partial(self.dynamicAuthorizeNodesForCertificationAtNode, name)
        node.dynamicAuthorizeRoleForAttributes = functools.partial(self.dynamicAuthorizeRoleForAttributesAtNode, name)
        node.countMatchingLinesInLog = functools.partial(self.countMatchingLinesInLog, name)
        node.deleteHaggleDB = functools.partial(self.deleteHaggleDBAtNode, name)
        node.batchPublish = functools.partial(self.batchPublishAtNode, name)
        node.batchSubscribe = functools.partial(self.batchSubscribeAtNode, name)

        return node

    def createNodes(self, *args):
        return (self.createNode(name) for name in args)

    def _createNode(self, name):
        raise NotImplementedError()
    
    def startNode(self, name):
        self._startNode(name)

    def startAllNodes(self):
        for node in self.nodes:
            self.startNode(node.name)

    def _startNode(self, name):
        raise NotImplementedError()

    def stopNode(self, name):
        self._stopNode(name)

    def stopAllNodes(self):
        for node in self.nodes:
            self.stopNode(node.name)

    def _stopNode(self, name):
        raise NotImplementedError()

    def calculateHaggleNodeIDs(self):
        self._calculateHaggleNodeIDs()

    def _calculateHaggleNodeIDs(self):
        raise NotImplementedError()

    def bootForHaggleNodeIDs(self):
        self.startAllNodes()
        self.sleep('Initial boot to gather IDs', self.config.haggleBootDelay)
        self.calculateHaggleNodeIDs()
        self.stopAllNodes()

    def calculateHaggleNodeIDsExternally(self):
        self._calculateHaggleNodeIDsExternally()

    def _calculateHaggleNodeIDsExternally(self):
        raise NotImplementedError()

    def shutdownEnvironment(self):
        for node in self.nodes:
            if node.running:
                self.stopNode(node.name)
        self._shutdownEnvironment()

    def _shutdownEnvironment(self):
        raise NotImplementedError()

    def deleteHaggleDBAtNode(self, name):
        self._deleteHaggleDBAtNode(name)

    def _deleteHaggleDBAtNode(self, name):
        raise NotImplementedError()

    def publishItem(self, name, descriptor, policy='', attributes=[]):
        self._publishItem(name, descriptor, policy, attributes)

    def _publishItem(self, name, descriptor, policy, attributes):
        raise NotImplementedError()

    def subscribeItem(self, name, descriptor):
        return self._subscribeItem(name, descriptor)

    def _subscribeItem(self, name, descriptor):
        raise NotImplementedError()

    def dynamicAddRoleSharedSecretsAtNode(self, name, *roles):
        return self._dynamicAddRoleSharedSecretsAtNode(name, *roles)

    def _dynamicAddRoleSharedSecretsAtNode(self, name, *roles):
        raise NotImplementedError()

    def dynamicAddNodeSharedSecretsAtNode(self, name, *nodes):
        return self._dynamicAddNodeSharedSecretsAtNode(name, *nodes)

    def _dynamicAddNodeSharedSecretsAtNode(self, name, *nodes):
        raise NotImplementedError()

    def dynamicAddAuthoritiesAtNode(self, name, *authorities):
        return self._dynamicAddAuthoritiesAtNode(name, *authorities)

    def _dynamicAddAuthoritiesAtNode(self, name, *authorities):
        raise NotImplementedError()

    def dynamicAuthorizeNodesForCertificationAtNode(self, name, *ids):
        return self._dynamicAuthorizeNodesForCertificationAtNode(name, *ids)

    def _dynamicAuthorizeNodesForCertificationAtNode(self, name, *ids):
        raise NotImplementedError()

    def dynamicAuthorizeRoleForAttributesAtNode(self, name, role, encryption, decryption):
        return self._dynamicAuthorizeRoleForAttributesAtNode(name, role, encryption, decryption)

    def _dynamicAuthorizeRoleForAttributesAtNode(self, name, role, encryption, decryption):
        raise NotImplementedError()

    def countMatchingLinesInLog(self, name, pattern):
        return self._countMatchingLinesInLog(name, pattern)

    def _countMatchingLinesInLog(self, name, pattern):
        raise NotImplementedError()

    def limitCPU(self, limit):
        return self._limitCPU(limit)

    def _limitCPU(self, limit):
        return self._limitCPU(limit)

    def batchPublishAtNode(self, name, duration, *commands):
        return self._batchPublishAtNode(name, duration, *commands)

    def _batchPublishAtNode(self, name, duration, *commands):
        raise NotImplementedError()

    def batchSubscribeAtNode(self, name, duration, descriptor):
        return self._batchSubscribeAtNode(name, duration, descriptor)

    def _batchSubscribeAtNode(self, name, duration, descriptor):
        raise NotImplementedError()

    def waitForBatchCommands(self, duration):
        self._startTask('Waiting for batch commands to complete', duration) 
        for node in self.nodes:
            for thread in node.threads:
                thread.join()
        self._endTask()

    def _cleanup(self):
        pass
