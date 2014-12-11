# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Control Authorize Nodes for Certification: Tests whether the control message for 
authorizing nodes for certification works correctly.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
Both nodes are started up.
BOB will publish a data object.
This should not be received at ALICE, due to a signature verification failure.
Control messages are used to authorize BOB for certification at ALICE.
BOB will publish a data object.
This should be successfully received at ALICE.
Nodes are then shutdown.
"""

CATEGORIES=['authority', 'control']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.calculateHaggleNodeIDsExternally()
    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    ALICE.addNodeSharedSecret('BOB')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH', certificateSigningRequestDelay=1)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.publishItem('object1', '')
    results.expect('Subscribing to object1 at ALICE', False, ALICE.subscribeItem('object1'))

    ALICE.dynamicAuthorizeNodesForCertification(BOB.haggleNodeID)
    env.sleep('Letting nodes exchange certificates', 2*env.config.exchangeDelay)
    BOB.publishItem('object2', '')
    results.expect('Subscribing to object2 at ALICE', True, ALICE.subscribeItem('object2'))

    env.stopAllNodes()


    # Check whether control message authorized BOB properly
    results.expect('Checking whether control message authorized BOB for certification exactly once at ALICE.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureNodesForCertification}: Adding Node %s to the list of nodes which can be certified by this authority.' % BOB.haggleNodeID))
