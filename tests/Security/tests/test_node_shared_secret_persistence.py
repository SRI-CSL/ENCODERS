# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Node Shared Secret Persistence: Tests whether the shared secrets for nodes are persisted
correctly after restarts.

The test uses a simple 3 node configuration, with ALICE as the authority node. 
ALICE and BOB have node shared secrets configured in the config.
All 3 nodes are started up.
ALICE and EVE have their node shared secrets configured through the use of a control message.
Nodes are then shutdown.
All node shared secrets should be persisted.

ALICE and BOB have their node shared secrets changed in the config.
Nodes are then restarted.
ALICE and EVE have their node shared secrets changed through the use of a control message.
Nodes are then shutdown.
All node shared secrets should be persisted.
"""

CATEGORIES=['nodes', 'control']

def runTest(env, nodes, results, Console):
    ALICE, BOB, EVE = env.createNodes('ALICE', 'BOB', 'EVE')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')
    EVE.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB', 'EVE')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')
    EVE.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)

    ALICE.addNodeSharedSecret('EVE')
    EVE.dynamicAddNodeSharedSecrets((ALICE.haggleNodeID, EVE.nodeSharedSecrets[ALICE]))
    ALICE.dynamicAddNodeSharedSecrets((EVE.haggleNodeID, ALICE.nodeSharedSecrets[EVE]))

    env.stopAllNodes()

    del ALICE.nodeSharedSecrets[BOB]
    del ALICE.nodeSharedSecrets[EVE]
    del BOB.nodeSharedSecrets[ALICE]
    del EVE.nodeSharedSecrets[ALICE]
    ALICE.addNodeSharedSecret('BOB')
    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    ALICE.addNodeSharedSecret('EVE')
    EVE.dynamicAddNodeSharedSecrets((ALICE.haggleNodeID, EVE.nodeSharedSecrets[ALICE]))
    ALICE.dynamicAddNodeSharedSecrets((EVE.haggleNodeID, ALICE.nodeSharedSecrets[EVE]))
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    env.stopAllNodes()


    # Check whether shared secrets were loaded from the config properly
    results.expect('Checking whether shared secret was loaded from config twice at ALICE', 2, ALICE.countMatchingLinesInLog( 
                   '{SecurityManager::configureNodeSharedSecrets}: Loaded shared secret for node %s' % BOB.haggleNodeID))
    results.expect('Checking whether shared secret was loaded from config twice at BOB', 2, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::configureNodeSharedSecrets}: Loaded shared secret for node %s' % ALICE.haggleNodeID))

    # Check whether shared secret from config was overwritten by that in the repository
    results.expect('Checking whether shared secret from repository overwrote the one from config at ALICE', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::postConfiguration}: Already have shared secret for node %s from configuration, going to replace with shared secret from repository.' % BOB.haggleNodeID))
    results.expect('Checking whether shared secret from repository overwrote the one from config at BOB', 1, BOB.countMatchingLinesInLog(
                   '{SecurityManager::postConfiguration}: Already have shared secret for node %s from configuration, going to replace with shared secret from repository.' % ALICE.haggleNodeID))

    # Check whether control message added shared secrets properly
    results.expect('Checking whether control message added node shared secret exactly once at ALICE', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureNodeSharedSecrets}: Added shared secret for node %s' % EVE.haggleNodeID))
    results.expect('Checking whether control message added node shared secret exactly once at EVE', 1, EVE.countMatchingLinesInLog(
                   '{SecurityManager::configureNodeSharedSecrets}: Added shared secret for node %s' % ALICE.haggleNodeID))

    # Check whether control message replaced shared secrets properly
    results.expect('Checking whether control message replaced node shared secret exactly once at ALICE', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureNodeSharedSecrets}: Replaced shared secret for node %s' % EVE.haggleNodeID))
    results.expect('Checking whether control message replaced node shared secret exactly once at EVE', 1, EVE.countMatchingLinesInLog(
                   '{SecurityManager::configureNodeSharedSecrets}: Replaced shared secret for node %s' % ALICE.haggleNodeID))

    # All shared secrets should be persisted twice
    results.expect('Checking whether shared secret for BOB was persisted twice at ALICE', 2, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::onPrepareShutdown}: Storing shared secret for node: %s' % BOB.haggleNodeID))
    results.expect('Checking whether shared secret for EVE was persisted twice at ALICE', 2, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::onPrepareShutdown}: Storing shared secret for node: %s' % EVE.haggleNodeID))
    results.expect('Checking whether shared secret for ALICE was persisted twice at BOB', 2, BOB.countMatchingLinesInLog(
                   '{SecurityManager::onPrepareShutdown}: Storing shared secret for node: %s' % ALICE.haggleNodeID))
    results.expect('Checking whether shared secret for ALICE was persisted twice at EVE', 2, EVE.countMatchingLinesInLog(
                   '{SecurityManager::onPrepareShutdown}: Storing shared secret for node: %s' % ALICE.haggleNodeID))

    # Check whether shared secrets were loaded correctly from repository
    results.expect('Checking whether shared secret for EVE was loaded once at ALICE', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::postConfiguration}: Loading shared secret for node %s from repository.' % EVE.haggleNodeID))
    results.expect('Checking whether shared secret for ALICE was loaded once at EVE', 1, EVE.countMatchingLinesInLog(
                   '{SecurityManager::postConfiguration}: Loading shared secret for node %s from repository.' % ALICE.haggleNodeID))
