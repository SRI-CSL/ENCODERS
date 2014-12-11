# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Microbenchmark-Base: 
Performs a micro-benchmark of the baseline performance.
"""

import random

CATEGORIES=['microbenchmark', 'base']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.config.rsaKeyLength = env.config.microBenchmarkRSAKeyLength
    env.calculateHaggleNodeIDsExternally()
    # ALICE.addNodeSharedSecret('BOB')

    # ALICE.setAuthority()
    # BOB.addAuthorities('ALICE')

    # ALICE.authorizeNodesForCertification('ALICE', 'BOB')

    ALICE.createConfig(securityLevel='LOW')
    BOB.createConfig(securityLevel='LOW')

    env.config.testContentSize = env.config.microBenchmarkFileSize

    env.startAllNodes()
    env.limitCPU(env.config.microBenchmarkCPULimit)
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    commands = []
    for x in xrange(0, env.config.microBenchmarkNumFiles):
        command = (env.config.microBenchmarkPublishDelay, [('object1', 'object1')], env.config.microBenchmarkFileSize)
        commands.append(command)
    duration = env.config.microBenchmarkPublishDelay * (2+env.config.microBenchmarkNumFiles)
    ALICE.batchSubscribe(duration, 'object1')
    BOB.batchPublish(duration, *commands)
    env.waitForBatchCommands(duration)
    env.stopAllNodes()

    results.expect('Empty Assertion', 0, 0)
