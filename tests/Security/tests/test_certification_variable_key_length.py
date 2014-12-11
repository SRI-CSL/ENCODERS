# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Certification Variable Key Length: Tests whether variable key sizes work correctly.

The test uses a simple 3 node configuration, with ALICE as the authority node. 
All nodes have their keypair specified in the configuration.
ALICE signs certificates for BOB, statically, at configuration time.
All nodes are started up. There should be no certificate exchange.
ALICE publishes an object, this should be received at BOB and EVE successfully, and be signed.
BOB publishes an object, this should be received at ALICE and EVE successfully, and be signed.
EVE publishes an object, this should be received at ALICE and BOB successfully, and be signed.
"""

CATEGORIES=['certification']

def runTest(env, nodes, results, Console):
    ALICE, BOB, EVE = env.createNodes('ALICE', 'BOB', 'EVE')

    env.config.rsaKeyLength = 2048
    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')
    ALICE.addNodeSharedSecret('EVE')

    ALICE.generateKeyPair()
    BOB.generateKeyPair()

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')
    EVE.addAuthorities('ALICE')

    ALICE.signCertificateForNode('ALICE')
    ALICE.signCertificateForNode('BOB')
    ALICE.authorizeNodesForCertification('EVE')

    ALICE.createConfig(securityLevel='HIGH')
    BOB.createConfig(securityLevel='HIGH')
    EVE.createConfig(securityLevel='HIGH')

    env.startAllNodes()
    env.sleep('Letting nodes boot up', 1)

    ALICE.publishItem('object1', '')
    results.expect('Subscribing to object1 at BOB', True, BOB.subscribeItem('object1'))
    results.expect('Subscribing to object1 at EVE', True, EVE.subscribeItem('object1'))

    BOB.publishItem('object2', '')
    results.expect('Subscribing to object2 at ALICE', True, ALICE.subscribeItem('object2'))
    results.expect('Subscribing to object2 at EVE', True, EVE.subscribeItem('object2'))

    EVE.publishItem('object3', '')
    results.expect('Subscribing to object3 at ALICE', True, ALICE.subscribeItem('object3'))
    results.expect('Subscribing to object3 at BOB', True, BOB.subscribeItem('object3'))

    env.stopAllNodes()

    # Check whether keypairs were loaded properly.
    for node in [ALICE, BOB]:
        results.expect('Checking whether %s loaded private key from config.' % node.name, 1, node.countMatchingLinesInLog(
                       '{SecurityManager::onConfig}: Loaded private key from config.'))
        results.expect('Checking whether %s loaded public key from config.' % node.name, 1, node.countMatchingLinesInLog(
                       '{SecurityManager::onConfig}: Loaded public key from config.'))

    # Check whether myCerts were generated properly at BOB
    for node in [BOB]:
        results.expect('Checking whether %s generated own certificate.' % node.name, 1, node.countMatchingLinesInLog(
                       "{SecurityManager::onConfig}: Generated node's own certificate based on public and private key."))
        results.expect('Checking whether %s signed certificate.' % node.name, 1, node.countMatchingLinesInLog(
                       "{SecurityManager::onConfig}: Signed node's own certificate."))
        results.expect('Checking whether %s verified certificate.' % node.name, 1, node.countMatchingLinesInLog(
                       "{SecurityManager::onConfig}: Verified node's own certificate."))

    # Check whether myCert was not generated at ALICE
    for node in [ALICE]:
        results.expect('Checking whether %s did not generate own certificate.' % node.name, 0, node.countMatchingLinesInLog(
                       "{SecurityManager::onConfig}: Generated node's own certificate based on public and private key."))
        results.expect('Checking whether %s did not sign certificate.' % node.name, 0, node.countMatchingLinesInLog(
                       "{SecurityManager::onConfig}: Signed node's own certificate."))
        results.expect('Checking whether %s did not verify certificate.' % node.name, 0, node.countMatchingLinesInLog(
                       "{SecurityManager::onConfig}: Verified node's own certificate."))
        results.expect('Checking whether %s loaded myCert from config.' % node.name, 1, node.countMatchingLinesInLog(
                       "{SecurityManager::configureCertificates}: Loaded myCert from config!"))

    # Check whether myCert was generated at EVE
    results.expect('Checking whether EVE generated own certificate.', 1, EVE.countMatchingLinesInLog(
                   "{SecurityManager::postConfiguration}: No repository entries and no keypair found, generating new certificate and keypair"))

    # Check whether certificates were loaded properly
    for node in [BOB]:
        results.expect('Checking whether %s loaded its certificate signed by ALICE.' % node.name, 1, node.countMatchingLinesInLog(
                       '{SecurityManager::configureCertificates}: Loaded certificate for %s issued by %s.' % (node.haggleNodeID, ALICE.haggleNodeID)))
        results.expect("Checking whether %s loaded ALICE's authority certificate." % node.name, 1, node.countMatchingLinesInLog(
                       '{SecurityManager::configureCertificates}: Loaded certificate for %s issued by %s.' % (ALICE.haggleNodeID, ALICE.haggleNodeID)))

    # Check whether no certificate signature requests were made
    results.expect('Checking whether BOB did not request a signed certificate from ALICE.', 0, node.countMatchingLinesInLog(
                       '{SecurityManager::generateCertificateSigningRequest}: Still need signed certificate from ALICE: %s' % ALICE.haggleNodeID))

    # Check whether EVE requested a certificate
    results.expect('Checking whether signed certificate was received at EVE.', 1, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::handleSecurityDataResponse}: Saved signed certificate issued by %s' % ALICE.haggleNodeID))

    results.expect('Checking whether ALICE signed certificate for EVE.', 1, ALICE.countMatchingLinesInLog(
                   '{SecurityHelper::signCertificate}: Signing certificate for id=%s' % EVE.haggleNodeID))

    # Check whether RSA Key length was set to 2048
    for node in [ALICE, BOB, EVE]:
        results.expect('Checking whether %s set RSA key length to 2048.' % node.name, 1, node.countMatchingLinesInLog(
                       '{SecurityManager::onConfig}: Setting RSA Key Length to 2048.'))
