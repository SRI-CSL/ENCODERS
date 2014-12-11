# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Declarative Certification: Tests whether declarative certification works correctly.

The test uses a simple 3 node configuration, with ALICE as the authority node. 
All nodes are booted up, and allowed to exchange certificates.
BOB is authorized for certification, so it should receive a signed certificate.
EVE is not authorized for certification, so it should not receive a signed certificate.
"""

CATEGORIES=['sanity', 'certification']

def runTest(env, nodes, results, Console):
    ALICE, BOB, EVE = env.createNodes('ALICE', 'BOB', 'EVE')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')
    ALICE.addNodeSharedSecret('EVE')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')
    EVE.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')
    EVE.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    env.sleep('Letting nodes exchange certificates', env.config.exchangeDelay)
    env.stopAllNodes()

    results.expect('Checking whether signed certificate was received at BOB.', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::handleSecurityDataResponse}: Saved signed certificate issued by %s' % ALICE.haggleNodeID))

    results.expect('Checking whether signed certificate was not received at EVE.', 0, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::handleSecurityDataResponse}: Saved signed certificate issued by %s' % ALICE.haggleNodeID))

    results.expect('Checking whether ALICE signed certificate for BOB.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityHelper::signCertificate}: Signing certificate for id=%s' % BOB.haggleNodeID))

    predicate = lambda c: c >= 1
    results.expect('Checking whether ALICE did not sign certificate for EVE.', predicate, ALICE.countMatchingLinesInLog(
                   '{SecurityHelper::signCertificate}: Node %s is not authorized for certification, ignoring certificate signing request.' % EVE.haggleNodeID))
