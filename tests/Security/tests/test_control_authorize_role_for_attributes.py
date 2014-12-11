# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Control Authorize Role for Attributes: Tests whether the control message for 
authorizing roles for attributes works correctly.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
Both nodes are started up.
BOB is authorized for ROLE1.
BOB will publish a data object requiring encryption attribute A1
This should fail, as ROLE1 is not authorized for this attribute.
Control messages are used to authorize ROLE1 for encryption attribute A1 at ALICE.
BOB will publish a data object requiring encryption attribute A1.
This should be successfully received at ALICE.
Nodes are then shutdown.
"""

CATEGORIES=['roles', 'control']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.calculateHaggleNodeIDsExternally()
    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    ALICE.addNodeSharedSecret('BOB')

    ALICE.authorizeNodesForCertification('BOB')
    ALICE.authorizeRoleForAttributes('ROLE1', [], [])
    ALICE.addRoleSharedSecret('ALICE.ROLE1')
    BOB.addRoleSharedSecret('ALICE.ROLE1')

    ALICE.createConfig(securityLevel='HIGH', encryptFilePayload=True)
    BOB.createConfig(securityLevel='HIGH', encryptFilePayload=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.publishItem('object1', 'ALICE.A1')
    results.expect('Subscribing to object1 at ALICE', False, ALICE.subscribeItem('object1'))

    ALICE.dynamicAuthorizeRoleForAttributes('ALICE.ROLE1', ['A1'], ['A1'])
    env.sleep('Letting BOB request attributes.', 2*env.config.exchangeDelay)
    BOB.publishItem('object2', 'ALICE.A1')
    results.expect('Subscribing to object2 at ALICE', True, ALICE.subscribeItem('object2'))

    env.stopAllNodes()


    # Check whether control message authorized ROLE1 properly
    results.expect('Checking whether control message authorized ROLE1 for encryption attribute A1 exactly once at ALICE.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureRolesForAttributes}: Adding publicKeyACL entry ALICE.ROLE1:ALICE.A1.'))
    results.expect('Checking whether control message authorized ROLE1 for decryption attribute A1 exactly once at ALICE.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureRolesForAttributes}: Adding privateKeyACL entry ALICE.ROLE1:ALICE.A1.'))
