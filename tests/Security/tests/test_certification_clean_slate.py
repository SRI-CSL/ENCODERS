# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Certification Clean Slate: Tests whether certification works correctly, even 
when nodes have not previously exchanged node descriptions.

The test uses a simple 3 node configuration, with ALICE as the authority node. 
All nodes are started up. They are allowed to exchange certificates.
BOB publishes an object, this should be received at ALICE successfully, and be signed.
EVE is then then shut down.

EVE has the haggle database cleared.
EVE is then booted up.
EVE publishes a data object, this should not be received at ALICE.
ALICE should detect the fact that EVE is now a new node with a different certificate, and reject the node description.
All nodes are then shut down.
"""

CATEGORIES=['certification']

def runTest(env, nodes, results, Console):
    ALICE, BOB, EVE = env.createNodes('ALICE', 'BOB', 'EVE')

    env.calculateHaggleNodeIDsExternally()
    BOB.deleteHaggleDB()
    ALICE.deleteHaggleDB()
    EVE.deleteHaggleDB()

    ALICE.addNodeSharedSecret('BOB')
    ALICE.addNodeSharedSecret('EVE')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')
    EVE.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB')
    ALICE.authorizeNodesForCertification('EVE')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')
    EVE.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    env.sleep('Letting nodes exchange certificates', env.config.exchangeDelay)

    BOB.publishItem('object1', '')
    results.expect('Subscribing to object1 at ALICE', True, ALICE.subscribeItem('object1'))
    EVE.stop()

    EVE.deleteHaggleDB()
    del EVE.nodeSharedSecrets[ALICE]
    EVE.createConfig(securityLevel='HIGH')

    
    EVE.start()
    env.sleep('Letting nodes exchange certificates', env.config.exchangeDelay)

    EVE.publishItem('object2', '')
    results.expect('Subscribing to object2 at ALICE', False, ALICE.subscribeItem('object2'))
    env.stopAllNodes()

    # We should be able to find the authority and send a security data request
    results.expect('Checking whether SecurityDataRequest went through at BOB', 0, BOB.countMatchingLinesInLog(
                   '{SecurityHelper::sendSecurityDataRequest}: No neighbours, not sending SecurityDataRequest!'))

    # Check whether node descriptions were verified with self signed certificates
    predicate = lambda c: c >= 1
    results.expect('Checking whether ALICE tried to verify node description for BOB without verified certificate', predicate, ALICE.countMatchingLinesInLog(
                   '{SecurityHelper::doTask}: Trying to verify node description \[[a-f0-9]*\] without verified certificate.'))
    results.expect('Checking whether ALICE tried to verify node description for BOB with self signed certificate', predicate, ALICE.countMatchingLinesInLog(
                   '{SecurityHelper::doTask}: No previously stored node description for node \[%s\]. Verifying node description \[[a-f0-9]*\] with self signed certificate.' % BOB.haggleNodeID))

    results.expect('Checking whether BOB tried to verify node description for ALICE without verified certificate', predicate, BOB.countMatchingLinesInLog(
                   '{SecurityHelper::doTask}: Trying to verify node description \[[a-f0-9]*\] without verified certificate.'))
    results.expect('Checking whether BOB tried to verify node description for ALICE with self signed certificate', predicate, BOB.countMatchingLinesInLog(
                   '{SecurityHelper::doTask}: No previously stored node description for node \[%s\]. Verifying node description \[[a-f0-9]*\] with self signed certificate.' % ALICE.haggleNodeID))
    results.expect('Checking whether BOB tried to verify node description for ALICE with previously stored certificate', predicate, BOB.countMatchingLinesInLog(
                   '{SecurityHelper::doTask}: Verifying node description \[[a-f0-9]*\] with previously stored certificate for node \[%s\]' % ALICE.haggleNodeID))

    # Check whether ALICE rejected the new node description from EVE
    results.expect('Checking whether ALICE rejected the new node description from EVE.', predicate, ALICE.countMatchingLinesInLog(
                   '{SecurityHelper::doTask}: ERROR: Node description [\[[a-f0-9]*\] from node \[%s\] has different certificates than the previously seen ones. Discarding node description.' % EVE.haggleNodeID), False, 'acceptable with a linear topology')
