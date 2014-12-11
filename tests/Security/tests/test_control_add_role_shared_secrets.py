# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Control Add Role Shared Secrets: Tests whether the control message for adding a role shared secret
works correctly.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
BOB is not authorized for any roles.
Both nodes are started up.
A control message is used to authorize BOB for role 1.
Nodes are then shutdown.
The role shared secret should be persisted.

Nodes are then restarted.
The role shared secret should be loaded.
BOB will publish an object requiring encryption.
ALICE will subscribe to this object, and it should be received.
Nodes are then shutdown.
"""

CATEGORIES=['roles', 'control']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB')
    ALICE.authorizeRoleForAttributes('ROLE1', ['A1'], ['A2'])
    ALICE.authorizeRoleForAttributes('ROLE2', ['A2'], ['A1'])

    ALICE.createConfig(securityLevel='LOW', encryptFilePayload=True)
    BOB.createConfig(securityLevel='LOW', encryptFilePayload=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.dynamicAddRoleSharedSecrets(('ALICE.ROLE1', ALICE.roleSharedSecrets['ALICE.ROLE1']))
    env.stopAllNodes()

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.publishItem('object1', 'ALICE.A1')
    results.expect('Subscribing to object1 at ALICE', True, ALICE.subscribeItem('object1'))
    env.stopAllNodes()


    # Check whether control message added ROLE1 properly
    results.expect('Checking whether control message replaced ROLE1 exactly once', 1, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureRoleSharedSecrets}: Added shared secret for role ALICE.ROLE1'))

    # ROLE1 should be persisted twice.
    results.expect('Checking whether ROLE1 was persisted twice.', 2, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::onPrepareShutdown}: Storing shared secret for role: ALICE.ROLE1'))

    # ROLE1 should be never be loaded from the config.
    results.expect('Checking whether ROLE1 was never loaded from config.', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::configureRoleSharedSecrets}: Loaded shared secret for role ALICE.ROLE1'))

    # ROLE1 should not have the repository version replaced with that from the config.
    results.expect('Checking whether ROLE1 was never overwritten based on config.', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::postConfiguration}: Already have shared secret for role ALICE.ROLE1 from configuration, going to replace with shared secret from repository.'))
    
