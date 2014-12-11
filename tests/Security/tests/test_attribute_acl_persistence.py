# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Attribute Persistence: Tests whether the access contro lists for attributes are
persisted correctly across restarts.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
ALICE has authorized ROLE1 for encryption attribute A1 and decryption attribute A2.
Both nodes are started up.
ALICE authorizes ROLE1 for encryption attribute A1 and decryption attribute A3, through the use of a control message.
Nodes are then shutdown.
The publicKeyACL and privateKeyACL should be persisted correctly, at ALICE.

Nodes are then restarted.
Nodes are then shutdown.
The publicKeyACL and privateKeyACL should be persisted correctly, at ALICE.
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

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    ALICE.dynamicAuthorizeRoleForAttributes('ALICE.ROLE1', ['A1'], ['A3'])
    env.stopAllNodes()

    env.startAllNodes()
    env.stopAllNodes()

    # Check whether control message authorized ROLE1 properly
    # Twice from config, once from control message
    results.expect('Checking whether ALICE authorized ROLE1 for encryption attribute A1 exactly thrice', 3, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureRolesForAttributes}: Adding publicKeyACL entry ALICE.ROLE1:ALICE.A1.'))

    # Twice from config
    results.expect('Checking whether ALICE authorized ROLE1 for decryption attribute A2 exactly twice', 2, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureRolesForAttributes}: Adding privateKeyACL entry ALICE.ROLE1:ALICE.A2.'))

    # Once from control message
    results.expect('Checking whether ALICE authorized ROLE1 for decryption attribute A3 exactly once', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureRolesForAttributes}: Adding privateKeyACL entry ALICE.ROLE1:ALICE.A3.'))

    # Check whether ALICE persisted the ACLs correctly.
    results.expect('Checking whether ALICE persisted publicKeyACL twice.', 2, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::onPrepareShutdown}: Storing publicKeyACL'))
    results.expect('Checking whether ALICE persisted privateKeyACL twice.', 2, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::onPrepareShutdown}: Storing privateKeyACL'))

    # Check whether entries were loaded correctly based on repository data
    results.expect('Checking whether ALICE did not authorize ROLE1 for encryption attribute A1 again based on repository data.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::postConfiguration}: publicKeyACL entry ALICE.ROLE1:ALICE.A1 was already present based on on configuration.'))
    results.expect('Checking whether ALICE did not authorize ROLE1 for decryption attribute A2 again based on repository data.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::postConfiguration}: privateKeyACL entry ALICE.ROLE1:ALICE.A2 was already present based on on configuration.'))
    results.expect('Checking whether ALICE authorized ROLE1 for decryption attribute A3 based on repository data.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::postConfiguration}: Adding privateKeyACL entry ALICE.ROLE1:ALICE.A3.'))

