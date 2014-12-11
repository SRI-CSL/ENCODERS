# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Microbenchmark-Encryption-Uncached: 
Performs a micro-benchmark of the performance of un-cached encryption.
"""

import random

CATEGORIES=['microbenchmark', 'encryption']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.config.rsaKeyLength = env.config.microBenchmarkRSAKeyLength
    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    attributeList = ['A%s' % x for x in xrange(1, env.config.microBenchmarkNumAttributes+1)]
    namespacedAttributeList = ['ALICE.%s' % attribute for attribute in attributeList]
    ALICE.authorizeRoleForAttributes('ROLE1', attributeList, attributeList)

    ALICE.authorizeNodesForCertification('ALICE', 'BOB')
    ALICE.addRoleSharedSecret('ALICE.ROLE1')
    BOB.addRoleSharedSecret('ALICE.ROLE1')

    ALICE.createConfig(securityLevel='VERYHIGH', encryptFilePayload=True)
    BOB.createConfig(securityLevel='VERYHIGH', encryptFilePayload=True)

    env.config.testContentSize = env.config.microBenchmarkFileSize

    env.startAllNodes()
    env.limitCPU(env.config.microBenchmarkCPULimit)
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    commands = []
    for x in xrange(0, env.config.microBenchmarkNumFiles):
        attrs = namespacedAttributeList[:]
        random.shuffle(attrs)
        attrs = ' AND '.join(attrs)
        command = (env.config.microBenchmarkPublishDelay, [('object1', 'object1'), ('Access', attrs)], env.config.microBenchmarkFileSize)
        commands.append(command)
    duration = env.config.microBenchmarkPublishDelay * (2+env.config.microBenchmarkNumFiles)
    ALICE.batchSubscribe(duration, 'object1')
    BOB.batchPublish(duration, *commands)
    env.waitForBatchCommands(duration)
    env.stopAllNodes()

    results.expect('Empty Assertion', 0, 0)
