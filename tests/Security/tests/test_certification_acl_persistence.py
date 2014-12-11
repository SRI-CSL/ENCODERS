# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Certification ACL Persistence: Tests whether the list of nodes authorized for certification
is persisted correctly across restarts.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
ALICE has authorized BOB for certification in the configuration.
Both nodes are started up.
ALICE authorizes BOB for certification, again, through the use of a control message.
ALICE authorizes a fake node EVE for certification, through the use of a control message.
Nodes are then shutdown.
All nodes should be correctly authorized for certification at ALICE.

Nodes are then restarted.
Nodes are then shutdown.
All nodes should be correctly authorized for certification at ALICE.
"""

import hashlib

CATEGORIES=['authority', 'control']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')
    EVE_ID = hashlib.sha1('EVE').hexdigest()

    env.calculateHaggleNodeIDsExternally()
    
    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    ALICE.addNodeSharedSecret('BOB')
    ALICE.authorizeNodesForCertification('BOB')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    ALICE.dynamicAuthorizeNodesForCertification(BOB.haggleNodeID)
    ALICE.dynamicAuthorizeNodesForCertification(EVE_ID)
    env.stopAllNodes()

    env.startAllNodes()
    env.stopAllNodes()

    # Check whether control message authorized BOB and EVE properly
    # Twice from config, once from control message
    results.expect('Checking whether ALICE authorized BOB for certification exactly thrice', 3, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureNodesForCertification}: Adding Node %s to the list of nodes which can be certified by this authority.' % BOB.haggleNodeID))
    results.expect('Checking whether ALICE authorized EVE for certification exactly once.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::configureNodesForCertification}: Adding Node %s to the list of nodes which can be certified by this authority.' % EVE_ID))

    # Check whether ALICE persisted the certification ACL correctly.
    results.expect('Checking whether ALICE persisted certificationACL twice.', 2, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::onPrepareShutdown}: Storing certificationACL'))

    # Check whether ALICE did not add BOB again to certificationACL based on repository data.
    results.expect('Checking whether ALICE did not authorize BOB again based on repository data.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityManager::postConfiguration}: Node %s was already authorized for certification based on configuration.' % BOB.haggleNodeID))

    # Check whether EVE's authorization was loaded from the repository once.
    results.expect("Checking whether ALICE authorized EVE for certification based on repository data exactly once.", 1, ALICE.countMatchingLinesInLog(
                   "{SecurityManager::postConfiguration}: Adding Node %s to the list of nodes which can be certified by this authority." % EVE_ID))
