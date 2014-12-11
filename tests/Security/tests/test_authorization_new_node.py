# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Authorization New Node: Tests whether authorization works correctly when a new node joins the network.

The test uses a simple 4 node configuration, with ALICE as the authority node. 

ALICE authorizes all nodes for attributes.
ALICE, BOB, and EVE are booted up, and allowed to receive attributes.

BOB publishes a data object requiring encryption, and it should successfully be received at EVE.
EVE publishes a data object requiring encryption, and it should successfully be received at BOB.

MALLORY is booted up.
It should make attribute requests, and receive attribute.

MALLORY publishes a data object requiring encryption, and it should successfully be received at BOB and EVE.
EVE publishes a data object requiring encryption, and it should successfully be received at MALLORY.
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

    MALLORY.start()
    env.sleep('Letting MALLORY request attributes', 2*env.config.exchangeDelay)

    MALLORY.publishItem('object3', 'ALICE.A1')
    results.expect('Subscribing to object3 at EVE', True, EVE.subscribeItem('object3'))
    results.expect('Subscribing to object3 at BOB', True, BOB.subscribeItem('object3'))

    EVE.publishItem('object4', 'ALICE.A1')
    results.expect('Subscribing to object4 at MALLORY', True, MALLORY.subscribeItem('object4'))


    env.stopAllNodes()

    for node in [BOB, EVE, MALLORY]:
        results.expect('Checking whether encryption never failed at %s' % node.name, 0, node.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'), False, 'acceptable with a linear topology')
        results.expect('Checking whether decryption never failed at %s' % node.name, 0, node.countMatchingLinesInLog( 
                   '{SecurityHelper::useCapability}: Missing some decryption attributes, can not decrypt capability for Data Object'), False, 'acceptable with a linear topology')
