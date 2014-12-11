# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Certification New Node with Authority Reboot: Tests whether certification works 
    correctly when a new node joins the network, and an authority is down
    but brought back up later.

The test uses a simple 4 node configuration, with ALICE as the authority node. 

ALICE authorizes all nodes for certification.
ALICE, BOB, and EVE are booted up, and allowed to exchange certificates.

BOB publishes a data object, and it should successfully be received at EVE.
EVE publishes a data object, and it should successfully be received at BOB.

ALICE is shut down.
MALLORY is booted up.
MALLORY publishes a data object, it should not be received at BOB due to missing certificates.

ALICE is booted up.
MALLORY should make certificate signature requests, and receive signed certificates.

MALLORY publishes a data object, and it should successfully be received at BOB and EVE.
EVE publishes a data object, and it should successfully be received at MALLORY.
"""

CATEGORIES=['certification']

def runTest(env, nodes, results, Console):
    ALICE, BOB, EVE, MALLORY = env.createNodes('ALICE', 'BOB', 'EVE', 'MALLORY')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')
    ALICE.addNodeSharedSecret('EVE')
    ALICE.addNodeSharedSecret('MALLORY')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')
    EVE.addAuthorities('ALICE')
    MALLORY.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB', 'EVE', 'MALLORY')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')
    EVE.createConfig(securityLevel='HIGH')
    MALLORY.createConfig(securityLevel='HIGH')

    ALICE.start()
    BOB.start()
    EVE.start()
    env.sleep('Letting nodes exchange certificates', env.config.exchangeDelay)

    BOB.publishItem('object1', '')
    results.expect('Subscribing to object1 at EVE', True, EVE.subscribeItem('object1'))

    EVE.publishItem('object2', '')
    results.expect('Subscribing to object2 at BOB', True, BOB.subscribeItem('object2'))

    env.stopNode('ALICE')
    MALLORY.start()
    env.sleep('Letting MALLORY boot', env.config.exchangeDelay)

    MALLORY.publishItem('object3', '')
    results.expect('Subscribing to object3 at BOB', False, BOB.subscribeItem('object3'))

    ALICE.start()
    env.sleep('Letting ALICE boot and MALLORY receive certificates', env.config.exchangeDelay)

    MALLORY.publishItem('object4', '')
    results.expect('Subscribing to object4 at EVE', True, EVE.subscribeItem('object4'))
    results.expect('Subscribing to object4 at BOB', True, BOB.subscribeItem('object4'))

    EVE.publishItem('object5', '')
    results.expect('Subscribing to object5 at MALLORY', True, MALLORY.subscribeItem('object5'))


    env.stopAllNodes()

    predicate = lambda c: c >= 1
    for node in [BOB, EVE, MALLORY]:
        results.expect('Checking whether signed certificate was received at %s.' % node.name, predicate, node.countMatchingLinesInLog( 
                   '{SecurityHelper::handleSecurityDataResponse}: Saved signed certificate issued by %s' % ALICE.haggleNodeID))
        results.expect('Checking whether ALICE signed certificate for %s.' % node.name, predicate, ALICE.countMatchingLinesInLog(
                   '{SecurityHelper::signCertificate}: Signing certificate for id=%s' % node.haggleNodeID))
