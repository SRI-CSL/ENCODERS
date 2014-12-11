# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Control Add Authorities: Tests whether the control message for adding an authority
works correctly.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
Both nodes are started up.
BOB will publish a data object.
This should not be received at ALICE, due to a signature verification failure.
Control messages are used to add ALICE as an authority for BOB.
BOB will publish a data object.
This should be successfully received at ALICE.
Nodes are then shutdown.
"""

CATEGORIES=['authorities', 'control']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.calculateHaggleNodeIDsExternally()
    ALICE.setAuthority()

    ALICE.addNodeSharedSecret('BOB')
    ALICE.authorizeNodesForCertification('BOB')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.publishItem('object1', '')
    results.expect('Subscribing to object1 at ALICE', False, ALICE.subscribeItem('object1'))

    BOB.dynamicAddAuthorities(('ALICE', ALICE.haggleNodeID))
    env.sleep('Letting nodes exchange certificates', 2*env.config.exchangeDelay)
    BOB.publishItem('object2', '')
    results.expect('Subscribing to object2 at ALICE', True, ALICE.subscribeItem('object2'))

    env.stopAllNodes()


    # Check whether control message added shared secrets properly
    results.expect('Checking whether control message added authority exactly once at BOB', 1, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureAuthorities}: Added id for authority ALICE \[id=%s\]' % ALICE.haggleNodeID))

    # Checking whether authority request was made
    results.expect('Checking whether BOB generated certificate signature request.', 1, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureAuthorities}: New authority node \[id=%s\]. Updating encryption attribute request queues and checking whether we need certificates signed.' % ALICE.haggleNodeID))
