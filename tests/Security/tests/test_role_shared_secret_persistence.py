# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Role Shared Secret Persistence: Tests whether the shared secrets for roles are persisted
correctly after restarts.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
BOB is authorized for role 1, and provisioned for role 2.
Both nodes are started up.
A control message is used to authorize Bob for roles 2 and 4.
Nodes are then shutdown.
All four role shared secrets should be persisted.

BOB is authorized for roles 1 and 3.
Nodes are then restarted.
Five role shared secrets should be loaded.
Nodes are then shutdown.
All five role shared secrets should be persisted.
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
    ALICE.authorizeRoleForAttributes('ROLE2', ['A1'], ['A2'])
    ALICE.authorizeRoleForAttributes('ROLE3', ['A1'], ['A2'])
    ALICE.authorizeRoleForAttributes('ROLE4', ['A1'], ['A2'])
    BOB.addRoleSharedSecret('ALICE.ROLE1')
    BOB.provisionForRole('ALICE.ROLE2')

    ALICE.createConfig(securityLevel='LOW', encryptFilePayload=True)
    BOB.createConfig(securityLevel='LOW', encryptFilePayload=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.dynamicAddRoleSharedSecrets(('ALICE.ROLE2', ALICE.roleSharedSecrets['ALICE.ROLE2']),
                       ('ALICE.ROLE4', ALICE.roleSharedSecrets['ALICE.ROLE4']))
    env.stopAllNodes()

    del BOB.roleSharedSecrets['ALICE.ROLE2']
    BOB.addRoleSharedSecret('ALICE.ROLE3')
    ALICE.createConfig(securityLevel='LOW', encryptFilePayload=True)
    BOB.createConfig(securityLevel='LOW', encryptFilePayload=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    env.stopAllNodes()


    # ROLE1, ROLE2 and ROLE4 should be persisted twice.
    results.expect('Checking whether ROLE1 was persisted twice.', 2, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::onPrepareShutdown}: Storing shared secret for role: ALICE.ROLE1'))
    results.expect('Checking whether ROLE2 was persisted twice.', 2, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::onPrepareShutdown}: Storing shared secret for role: ALICE.ROLE2'))
    results.expect('Checking whether ROLE4 was persisted twice.', 2, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::onPrepareShutdown}: Storing shared secret for role: ALICE.ROLE4'))

    # ROLE3 should be persisted once.
    results.expect('Checking whether ROLE3 was persisted once.', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::onPrepareShutdown}: Storing shared secret for role: ALICE.ROLE3'))

    # ROLE1 should be loaded from the config twice.
    results.expect('Checking whether ROLE1 was loaded from config twice.', 2, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::configureRoleSharedSecrets}: Loaded shared secret for role ALICE.ROLE1'))

    # ROLE2 and ROLE3 should be loaded from the config once.
    results.expect('Checking whether ROLE2 was loaded from config once.', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::configureRoleSharedSecrets}: Loaded shared secret for role ALICE.ROLE2'))
    results.expect('Checking whether ROLE3 was loaded from config once.', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::configureRoleSharedSecrets}: Loaded shared secret for role ALICE.ROLE3'))

    # ROLE4 should never be loaded from the config.
    results.expect('Checking whether ROLE4 was never loaded from config.', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::configureRoleSharedSecrets}: Loaded shared secret for role ALICE.ROLE4'))

    # ROLE1 should not have the repository version replaced with that from the config.
    results.expect('Checking whether ROLE1 was overwritten once based on config.', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::postConfiguration}: Already have shared secret for role ALICE.ROLE1 from configuration, going to replace with shared secret from repository.'))

    # ROLE2 should be loaded from the repository once.
    results.expect('Checking whether ROLE2 was loaded from the repository once.', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::postConfiguration}: Loading shared secret for role ALICE.ROLE2 from repository.'))

    # postConfiguration should not touch ROLE3 as it was not in the repository.
    results.expect('Checking whether ROLE3 was not overwritten based on config.', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::postConfiguration}: Replacing shared secret for role ALICE.ROLE3 with shared secret from repository.'))
    results.expect('Checking whether ROLE3 was not loaded from the repository.', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::postConfiguration}: Loading shared secret for role ALICE.ROLE3 from repository.'))

    # ROLE4 should be loaded from the repository once.
    results.expect('Checking whether ROLE4 was loaded from the repository once.', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::postConfiguration}: Loading shared secret for role ALICE.ROLE4 from repository.'))

    # Check whether control message added roles properly
    results.expect('Checking whether control message replaced ROLE2 exactly once', 1, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureRoleSharedSecrets}: Replaced shared secret for role ALICE.ROLE2'))
    results.expect('Checking whether control message added ROLE4 exactly once', 1, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureRoleSharedSecrets}: Added shared secret for role ALICE.ROLE4'))
