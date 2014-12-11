# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""

Mock Test Environment - Mock the test environments for validation and ETAs

"""

import functools
import inspect
from collections import namedtuple
from node_manager import NodeManager
from core_test_environment import CoreTestEnvironment
from android_test_environment import AndroidTestEnvironment
from clitools import Console

class MockTestEnvironment(object):

    def __init__(self, config):
        self.config = config
        self.nodes = NodeManager(config)
        self.eta = 0
        self.valid = True
        self.subscribing = False
        self.batchDelays = []
        self._addAllChecks()

    def _addCheck(self, func, action=None, duration=None):

        @functools.wraps(func)
        def _timerProxy(*args, **kwargs):
            self.subscribing = False
            callArgs = inspect.getcallargs(func, self, *args, **kwargs)
            if duration is not None:
                self.eta = self.eta + duration(callArgs) * self.config.sleepFactor
            if action is not None:
                return action(callArgs)

        setattr(self, func.__name__, _timerProxy)

    def _mockExpect(self, message, expected, actual, critical=True, warningMessage=''):
        if self.subscribing and expected:
            self.eta = self.eta - self.config.subscribeTimeout
            self.eta = self.eta + self.config.publishDelay

    @property
    def mockResultObject(self):
        klass = namedtuple('Result', 'expect')
        return klass(self._mockExpect)

    def _addAllChecks(self):
        raise NotImplementedError()

    def createNodeAction(self, args):
        node = self.nodes.addNode(args['name'])
        node.start = functools.partial(self.startNode, args['name'])
        node.stop = functools.partial(self.stopNode, args['name'])
        node.publishItem = functools.partial(self.publishItem, args['name'])
        node.subscribeItem = functools.partial(self.subscribeItem, args['name'])
        node.dynamicAddRoleSharedSecrets = functools.partial(self.dynamicAddRoleSharedSecretsAtNode, args['name'])
        node.dynamicAddNodeSharedSecrets = functools.partial(self.dynamicAddNodeSharedSecretsAtNode, args['name'])
        node.dynamicAddAuthorities = functools.partial(self.dynamicAddAuthoritiesAtNode, args['name'])
        node.dynamicAuthorizeNodesForCertification = functools.partial(self.dynamicAuthorizeNodesForCertificationAtNode, args['name'])
        node.dynamicAuthorizeRoleForAttributes = functools.partial(self.dynamicAuthorizeRoleForAttributesAtNode, args['name'])
        node.countMatchingLinesInLog = functools.partial(self.countMatchingLinesInLog, args['name'])
        node.deleteHaggleDB = functools.partial(self.deleteHaggleDBAtNode, args['name'])
        node.batchPublish = functools.partial(self.batchPublishAtNode, args['name'])
        node.batchSubscribe = functools.partial(self.batchSubscribeAtNode, args['name'])
        node.generateKeyPair = lambda: None # Do nothing - TODO: mock the subprocess calls there?
        node.signCertificateForNode = lambda name: None # Do nothing - TODO: mock the subprocess calls there?
        return node

    def createNodesAction(self, args):
        retval = []
        for name in args['args']:
            retval.append(self.createNodeAction({'name': name}))
        return retval

    def calculateHaggleNodeIDsAction(self, args):
        for (i, node) in enumerate(self.nodes):
            node.haggleNodeID = str(i)

    def subscribeItemAction(self, args):
        self.subscribing = True

class MockCoreTestEnvironment(MockTestEnvironment):

    def __init__(self, config):
        super(MockCoreTestEnvironment, self).__init__(config)
        slevel = Console.STDOUTLEVEL
        flevel = Console.FILELEVEL
        Console.STDOUTLEVEL = Console.WARNING
        Console.FILELEVEL = Console.WARNING
        config.autoDiscoverCore()
        Console.STDOUTLEVEL = slevel
        Console.FILELEVEL = flevel

    def _addAllChecks(self):
        self._addCheck(CoreTestEnvironment.setupEnvironment, duration = lambda args: 0)
        self._addCheck(CoreTestEnvironment.shutdownEnvironment, duration = lambda args: 1)
        self._addCheck(CoreTestEnvironment.sleep, duration = lambda args: args['duration'])
        
        self._addCheck(CoreTestEnvironment.createNode, action=self.createNodeAction, duration = lambda args: 0)
        self._addCheck(CoreTestEnvironment.createNodes, action=self.createNodesAction, duration = lambda args: 0)

        self._addCheck(CoreTestEnvironment.startNode, duration = lambda args: self.config.haggleBootDelay)
        self._addCheck(CoreTestEnvironment.startAllNodes, duration = lambda args: self.config.haggleBootDelay * len(self.nodes))
        self._addCheck(CoreTestEnvironment.stopNode, duration = lambda args: self.config.haggleStopDelay)
        self._addCheck(CoreTestEnvironment.stopAllNodes, duration = lambda args: self.config.haggleStopDelay * len(self.nodes))

        self._addCheck(CoreTestEnvironment.calculateHaggleNodeIDs, action=self.calculateHaggleNodeIDsAction, duration = lambda args: 0)
        self._addCheck(CoreTestEnvironment.bootForHaggleNodeIDs, action=self.calculateHaggleNodeIDsAction, duration = lambda args: (self.config.haggleBootDelay * 2 + self.config.haggleStopDelay) * len(self.nodes))
        self._addCheck(CoreTestEnvironment.calculateHaggleNodeIDsExternally, action=self.calculateHaggleNodeIDsAction, duration = lambda args: 0)
        self._addCheck(CoreTestEnvironment.deleteHaggleDBAtNode, duration = lambda args: self.config.deleteHaggleDBDelay)

        self._addCheck(CoreTestEnvironment.publishItem, duration = lambda args: 0)
        self._addCheck(CoreTestEnvironment.subscribeItem, action = self.subscribeItemAction, duration = lambda args: self.config.subscribeTimeout)
        self._addCheck(CoreTestEnvironment.dynamicAddRoleSharedSecretsAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(CoreTestEnvironment.dynamicAddNodeSharedSecretsAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(CoreTestEnvironment.dynamicAddAuthoritiesAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(CoreTestEnvironment.dynamicAuthorizeNodesForCertificationAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(CoreTestEnvironment.dynamicAuthorizeRoleForAttributesAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(CoreTestEnvironment.countMatchingLinesInLog, duration = lambda args: 0)
        self._addCheck(CoreTestEnvironment.limitCPU, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(CoreTestEnvironment.batchPublishAtNode, duration = lambda args: (0, self.batchDelays.append(args['duration']))[0])
        self._addCheck(CoreTestEnvironment.batchSubscribeAtNode, duration = lambda args: (0, self.batchDelays.append(args['duration']))[0])
        self._addCheck(CoreTestEnvironment.waitForBatchCommands, duration = lambda args: max(self.batchDelays))

class MockAndroidTestEnvironment(MockTestEnvironment):

    def __init__(self, config):
        super(MockAndroidTestEnvironment, self).__init__(config)
        slevel = Console.STDOUTLEVEL
        flevel = Console.FILELEVEL
        Console.STDOUTLEVEL = Console.WARNING
        Console.FILELEVEL = Console.WARNING
        config.autoDiscoverAndroid()
        Console.STDOUTLEVEL = slevel
        Console.FILELEVEL = flevel

    def _addAllChecks(self):
        self._addCheck(AndroidTestEnvironment.setupEnvironment, duration = lambda args: self.config.deviceRebootDelay + 2 + 5)
        self._addCheck(AndroidTestEnvironment.shutdownEnvironment, duration = lambda args: self.config.haggleStopDelay)
        self._addCheck(AndroidTestEnvironment.sleep, duration = lambda args: args['duration'])
        
        self._addCheck(AndroidTestEnvironment.createNode, action=self.createNodeAction, duration = lambda args: self.config.deviceInstallDelay)
        self._addCheck(AndroidTestEnvironment.createNodes, action=self.createNodesAction, duration = lambda args: self.config.deviceInstallDelay * len(self.nodes))

        self._addCheck(AndroidTestEnvironment.startNode, duration = lambda args: self.config.haggleBootDelay)
        self._addCheck(AndroidTestEnvironment.startAllNodes, duration = lambda args: self.config.haggleBootDelay * len(self.nodes))
        self._addCheck(AndroidTestEnvironment.stopNode, duration = lambda args: self.config.haggleStopDelay)
        self._addCheck(AndroidTestEnvironment.stopAllNodes, duration = lambda args: self.config.haggleStopDelay * len(self.nodes))

        self._addCheck(AndroidTestEnvironment.calculateHaggleNodeIDs, action=self.calculateHaggleNodeIDsAction, duration = lambda args: 0)
        self._addCheck(AndroidTestEnvironment.calculateHaggleNodeIDsExternally, action=self.calculateHaggleNodeIDsAction, duration = lambda args: 0)
        self._addCheck(AndroidTestEnvironment.bootForHaggleNodeIDs, action=self.calculateHaggleNodeIDsAction, duration = lambda args: (self.config.haggleBootDelay * 2 + self.config.haggleStopDelay) * len(self.nodes))
        self._addCheck(AndroidTestEnvironment.deleteHaggleDBAtNode, duration = lambda args: self.config.deleteHaggleDBDelay)

        self._addCheck(AndroidTestEnvironment.publishItem, duration = lambda args: 0)
        self._addCheck(AndroidTestEnvironment.subscribeItem, action = self.subscribeItemAction, duration = lambda args: self.config.subscribeTimeout)
        self._addCheck(AndroidTestEnvironment.dynamicAddRoleSharedSecretsAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(AndroidTestEnvironment.dynamicAddNodeSharedSecretsAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(AndroidTestEnvironment.dynamicAddAuthoritiesAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(AndroidTestEnvironment.dynamicAuthorizeNodesForCertificationAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(AndroidTestEnvironment.dynamicAuthorizeRoleForAttributesAtNode, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(AndroidTestEnvironment.countMatchingLinesInLog, duration = lambda args: 0)
        self._addCheck(AndroidTestEnvironment.limitCPU, duration = lambda args: self.config.dynamicConfigurationDelay)
        self._addCheck(AndroidTestEnvironment.batchPublishAtNode, duration = lambda args: (0, self.batchDelays.append(args['duration']))[0])
        self._addCheck(AndroidTestEnvironment.batchSubscribeAtNode, duration = lambda args: (0, self.batchDelays.append(args['duration']))[0])
        self._addCheck(AndroidTestEnvironment.waitForBatchCommands, duration = lambda args: max(self.batchDelays))
