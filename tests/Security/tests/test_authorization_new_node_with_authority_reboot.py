# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Authorization New Node with Authority Reboot: Tests whether authorization works 
    correctly when a new node joins the network, and an authority is down
    but brought back up later.

The test uses a simple 4 node configuration, with ALICE as the authority node. 

ALICE authorizes all nodes for attributes.
ALICE, BOB, and EVE are booted up, and allowed to receive attributes.

BOB publishes a data object requiring encryption, and it should successfully be received at EVE.
EVE publishes a data object requiring encryption, and it should successfully be received at BOB.

ALICE is shut down.
MALLORY is booted up.
MALLORY publishes a data object requiring encryption, it should not be received at BOB due to missing attributes.

ALICE is booted up.
MALLORY should make attribute requests, and receive them.

MALLORY publishes a data object, and it should successfully be received at BOB and EVE.
EVE publishes a data object, and it should successfully be received at MALLORY.
"""

CATEGORIES=['authorization']

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
    ALICE.authorizeRoleForAttributes('ROLE1', ['A1'], ['A1'])

    BOB.addRoleSharedSecret('ALICE.ROLE1')
    EVE.addRoleSharedSecret('ALICE.ROLE1')
    MALLORY.addRoleSharedSecret('ALICE.ROLE1')

    ALICE.createConfig(securityLevel='LOW', encryptFilePayload=True)
    BOB.createConfig(securityLevel='LOW', encryptFilePayload=True)
    EVE.createConfig(securityLevel='LOW', encryptFilePayload=True)
    MALLORY.createConfig(securityLevel='LOW', encryptFilePayload=True)

    ALICE.start()
    BOB.start()
    EVE.start()
    env.sleep('Letting nodes request attributes', env.config.exchangeDelay)

    BOB.publishItem('object1', 'ALICE.A1')
    results.expect('Subscribing to object1 at EVE', True, EVE.subscribeItem('object1'))

    EVE.publishItem('object2', 'ALICE.A1')
    results.expect('Subscribing to object2 at BOB', True, BOB.subscribeItem('object2'))

    env.stopNode('ALICE')
    MALLORY.start()
    env.sleep('Letting MALLORY boot', env.config.exchangeDelay)

    MALLORY.publishItem('object3', 'ALICE.A1')
    results.expect('Subscribing to object3 at BOB', False, BOB.subscribeItem('object3'))

    ALICE.start()
    env.sleep('Letting ALICE boot and MALLORY request attributes', env.config.exchangeDelay)

    MALLORY.publishItem('object4', 'ALICE.A1')
    results.expect('Subscribing to object4 at EVE', True, EVE.subscribeItem('object4'))
    results.expect('Subscribing to object4 at BOB', True, BOB.subscribeItem('object4'))

    EVE.publishItem('object5', 'ALICE.A1')
    results.expect('Subscribing to object5 at MALLORY', True, MALLORY.subscribeItem('object5'))


    env.stopAllNodes()

    results.expect('Checking whether encryption never failed at BOB', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'), False, 'acceptable with a linear topology')
    results.expect('Checking whether encryption never failed at EVE', 0, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'), False, 'acceptable with a linear topology')

    # Can be retried
    predicate = lambda c: c >= 1
    results.expect('Checking whether encryption failed at least once at MALLORY', predicate, MALLORY.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'))

    results.expect('Checking whether decryption never failed at BOB', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::useCapability}: Missing some decryption attributes, can not decrypt capability for Data Object'), False, 'acceptable with a linear topology')
    results.expect('Checking whether decryption never failed at EVE', 0, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::useCapability}: Missing some decryption attributes, can not decrypt capability for Data Object'), False, 'acceptable with a linear topology')
    results.expect('Checking whether decryption never failed at MALLORY', 0, MALLORY.countMatchingLinesInLog( 
                   '{SecurityHelper::useCapability}: Missing some decryption attributes, can not decrypt capability for Data Object'), False, 'acceptable with a linear topology')
