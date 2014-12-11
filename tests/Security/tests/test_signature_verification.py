# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Signature Verification: Tests whether signatures are verified correctly.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
Both nodes are started up. They are allowed to exchange certificates.
BOB publishes an object, this should be received at ALICE successfully, and be signed.
"""

CATEGORIES=['sanity', 'certification']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    env.sleep('Letting nodes exchange certificates', env.config.exchangeDelay)

    BOB.publishItem('object1', '')
    results.expect('Subscribing to object1 at ALICE', True, ALICE.subscribeItem('object1'))
    env.stopAllNodes()
