# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Authority Name Map Persistence: Tests whether the shared secrets for nodes are persisted
correctly after restarts.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
BOB has ALICE configured as an authority.
Both nodes are started up.
BOB has the ID for ALICE added again through the use of a control message.
BOB has a fake authority added through the use of a control message.
Nodes are then shutdown.
All authority names should be persisted at BOB.

Nodes are then restarted.
Nodes are then shutdown.
All authority names should be persisted at BOB.
"""

import hashlib

CATEGORIES=['authorities', 'control']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')
    EVE_ID = hashlib.sha1('EVE').hexdigest()

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.dynamicAddAuthorities(('ALICE', ALICE.haggleNodeID))
    BOB.dynamicAddAuthorities(('EVE', EVE_ID))
    env.stopAllNodes()

    env.startAllNodes()
    env.stopAllNodes()

    # Check whether control message added authorities properly
    results.expect('Checking whether control message replaced authority ALICE exactly once at BOB', 1, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureAuthorities}: Replaced id for authority ALICE \[id=%s\]' % ALICE.haggleNodeID))
    results.expect('Checking whether control message added authority EVE exactly once at BOB', 1, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureAuthorities}: Added id for authority EVE \[id=%s\]' % EVE_ID))

    # Checking whether authority request was not made
    results.expect('Checking whether BOB did not generate certificate signature request.', 0, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureAuthorities}: New authority node \[id=%s\]. Updating encryption attribute request queues and checking whether we need certificates signed.' % EVE_ID))

    # Check whether BOB persisted the name map correctly.
    results.expect('Checking whether BOB persisted authorityNameMap twice.', 2, BOB.countMatchingLinesInLog(
                   '{SecurityManager::onPrepareShutdown}: Storing authorityNameMap'))

    # Check whether ALICE's mapping was loaded from config twice
    results.expect("Checking whether BOB loaded ALICE's authority ID from config twice.", 2, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureAuthorities}: Loaded id for authority ALICE \[id=%s\]' % ALICE.haggleNodeID))

    # Check whether ALICE's mapping from config was overwritten with that from repository.
    results.expect("Checking whether BOB replaced ALICE's authority ID from config with that from repository once.", 1, BOB.countMatchingLinesInLog(
                   "{SecurityManager::postConfiguration}: Already have id \[%s\] for authority ALICE from configuration, going to replace with id \[%s\] from repository." % (ALICE.haggleNodeID, ALICE.haggleNodeID)))

    # Check whether EVE's mapping was loaded from the repository once.
    results.expect("Checking whether BOB loaded EVE's authority ID from the repository once.", 1, BOB.countMatchingLinesInLog(
                   "{SecurityManager::postConfiguration}: Loaded authority EVE \[id=%s\] from repository." % EVE_ID))
