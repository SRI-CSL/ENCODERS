# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Certificate Signature: Tests whether certificate signature requests work correctly.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
Both nodes are started up.
BOB should request a certificate signature from ALICE, and should receive the signed certificate.
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
    env.stopAllNodes()

    results.expect('Checking whether signed certificate was received.', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::handleSecurityDataResponse}: Saved signed certificate issued by %s' % ALICE.haggleNodeID))
