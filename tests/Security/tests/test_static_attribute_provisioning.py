# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Static Attribute Provisioning: Tests whether static attribute provisioning works correctly.

The test uses a simple 3 node configuration, with ALICE as the authority node. 
ALICE authorizes ROLE1, ROLE2, and ROLE3 for cryptographic attributes.
BOB and EVE are pre-provisioned for ROLE1 and ROLE2, respectively.
All nodes are started up. There should be no attribute exchange for ROLE1 and ROLE2.

ALICE should publish an object requiring encryption; this should be received at BOB and EVE without
having to request the decryption attribute.

BOB will publish a data object requiring encryption, it will make a request for the attribute and publish it;
this should be received at ALICE.

EVE will publish a data object requiring encryption, it will make a request for the attribute and publish it;
this should be received at ALICE.
"""

CATEGORIES=['authorization']

def runTest(env, nodes, results, Console):
    ALICE, BOB, EVE = env.createNodes('ALICE', 'BOB', 'EVE')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')
    ALICE.addNodeSharedSecret('EVE')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')
    EVE.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB', 'EVE')
    ALICE.authorizeRoleForAttributes('ROLE1', ['A2'], ['A1'])
    ALICE.authorizeRoleForAttributes('ROLE2', ['A2'], ['A1'])
    ALICE.authorizeRoleForAttributes('ROLE3', ['A1', 'A2'], ['A1', 'A2'])

    BOB.preProvisionForRole('ALICE.ROLE1')
    EVE.preProvisionForRole('ALICE.ROLE2')

    ALICE.addRoleSharedSecret('ALICE.ROLE3')


    ALICE.createConfig(securityLevel='LOW', encryptFilePayload=True)
    BOB.createConfig(securityLevel='LOW', encryptFilePayload=True)
    EVE.createConfig(securityLevel='LOW', encryptFilePayload=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot up', 1)

    ALICE.publishItem('object1', 'ALICE.A1')
    results.expect('Subscribing to object1 at BOB', True, BOB.subscribeItem('object1'))
    results.expect('Subscribing to object1 at EVE', True, EVE.subscribeItem('object1'))

    BOB.publishItem('object2', 'ALICE.A2')
    results.expect('Subscribing to object2 at ALICE', True, ALICE.subscribeItem('object2'))

    EVE.publishItem('object3', 'ALICE.A2')
    results.expect('Subscribing to object3 at ALICE', True, ALICE.subscribeItem('object3'))

    env.stopAllNodes()

    # Check whether ALICE pre-provisioned issuedEncAttrs properly.

    results.expect('Checking whether IssuedEncryptionAttributes were loaded from config at ALICE.', 1, ALICE.countMatchingLinesInLog( 
                   '{SecurityManager::onConfig}: Loading IssuedEncryptionAttributes from config!'))
    results.expect('Checking whether IssuedEncryptionAttributes were loaded from config at ALICE.', 1, ALICE.countMatchingLinesInLog( 
                   '{SecurityManager::configureProvisionedAttributes}: Loaded attribute ALICE.A1 from config with value %s.' % ALICE.issuedEncryptionAttributes['ALICE.A1'].replace('/', '\\/').replace('+', '\\+')))
    results.expect('Checking whether IssuedEncryptionAttributes were loaded from config at ALICE.', 1, ALICE.countMatchingLinesInLog( 
                   '{SecurityManager::configureProvisionedAttributes}: Loaded attribute ALICE.A2 from config with value %s.' % ALICE.issuedEncryptionAttributes['ALICE.A2'].replace('/', '\\/').replace('+', '\\+')))
    results.expect('Checking whether encryption never failed at ALICE.', 0, ALICE.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'))
    results.expect('Checking whether ALICE never requested attributes.', 0, ALICE.countMatchingLinesInLog( 
                   '{SecurityHelper::requestSpecificKeys}: Authority: ALICE, attribute: A1'))
    results.expect('Checking whether IssuedEncryptionAttributes were loaded from config at ALICE.', 1, ALICE.countMatchingLinesInLog( 
                   '{SecurityHelper::startPython}: Added pubKeyFromConfig ALICE.A1'))
    results.expect('Checking whether IssuedEncryptionAttributes were loaded from config at ALICE.', 1, ALICE.countMatchingLinesInLog( 
                   '{SecurityManager::onConfig}: Inserting IssuedEncryptionAttribute ALICE.A1 into pubKeysFromConfig!'))
    results.expect('Checking whether IssuedEncryptionAttributes were loaded from config at ALICE.', 1, ALICE.countMatchingLinesInLog( 
                   '{SecurityHelper::startPython}: Added pubKeyFromConfig ALICE.A2'))
    results.expect('Checking whether IssuedEncryptionAttributes were loaded from config at ALICE.', 1, ALICE.countMatchingLinesInLog( 
                   '{SecurityManager::onConfig}: Inserting IssuedEncryptionAttribute ALICE.A2 into pubKeysFromConfig!'))

    # They should be pre-provisioned with the attributes
    for node in [BOB, EVE]:
        results.expect('Checking whether encryption never failed at %s' % node.name, 0, node.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object')) 
        results.expect('Checking whether decryption never failed at %s.' % node.name, 0, node.countMatchingLinesInLog( 
                   '{SecurityHelper::useCapability}: Missing some decryption attributes, can not decrypt capability for Data Object'))
        results.expect('Checking whether EncryptionAttributes were loaded from config at %s.' % node.name, 1, node.countMatchingLinesInLog( 
                   '{SecurityManager::onConfig}: Loading EncryptionAttributes from config!'))
        results.expect('Checking whether EncryptionAttributes were loaded from config at %s.' % node.name, 1, node.countMatchingLinesInLog( 
                   '{SecurityManager::configureProvisionedAttributes}: Loaded attribute ALICE.A2 from config with value %s.' % node.provisionedEncryptionAttributes['ALICE.A2'].replace('/', '\\/').replace('+', '\\+')))
        results.expect('Checking whether EncryptionAttributes were loaded from config at %s.' % node.name, 1, node.countMatchingLinesInLog( 
                   '{SecurityHelper::startPython}: Added pubKeyFromConfig ALICE.A2'))
        results.expect('Checking whether DecryptionAttributes were loaded from config at %s.' % node.name, 1, node.countMatchingLinesInLog( 
                   '{SecurityManager::onConfig}: Loading DecryptionAttributes from config!'))
        results.expect('Checking whether DecryptionAttributes were loaded from config at %s.' % node.name, 1, node.countMatchingLinesInLog( 
                   '{SecurityManager::configureProvisionedAttributes}: Loaded attribute ALICE.A1 from config with value %s.' % node.provisionedDecryptionAttributes['ALICE.A1'].replace('/', '\\/').replace('+', '\\+')))
        results.expect('Checking whether DecryptionAttributes were loaded from config at %s.' % node.name, 1, node.countMatchingLinesInLog( 
                   '{SecurityHelper::startPython}: Added privKeyFromConfig ALICE.A1'))
