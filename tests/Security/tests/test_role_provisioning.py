# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Role Provisioning: Tests whether role provisioning works correctly.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
BOB is authorized for role 1.
BOB has an incorrect shared secret configured for role 2.
Both nodes are started up.
BOB should receive, and be able to decrypt, attributes for role 1.
BOB should receive, and not be able to decrypt, attributes for role 2.

BOB is now configured with a correct shared secret for role2, through a control message.
BOB should publish a data object requiring encryption.
Bob should receive, and be able to decrypt, attributes for role 2.
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
    ALICE.authorizeRoleForAttributes('ROLE2', ['A3'], ['A4'])
    ALICE.authorizeRoleForAttributes('ROLE3', [], ['A3'])
    ALICE.addRoleSharedSecret('ALICE.ROLE3')
    BOB.addRoleSharedSecret('ALICE.ROLE1')
    BOB.provisionForRole('ALICE.ROLE2')

    ALICE.createConfig(securityLevel='LOW', encryptFilePayload=True)
    BOB.createConfig(securityLevel='LOW', encryptFilePayload=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)

    BOB.dynamicAddRoleSharedSecrets(('ALICE.ROLE2', ALICE.roleSharedSecrets['ALICE.ROLE2']))

    BOB.publishItem('object1', 'ALICE.A3')
    results.expect('Subscribing to object1 at ALICE', True, ALICE.subscribeItem('object1'))
    env.stopAllNodes()


    results.expect('Checking whether A1 was stored into bucket exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute public key ALICE.A1 into bucket.'))
    results.expect('Checking whether A1 was decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Decrypted attribute public key ALICE.A1 using role ALICE.ROLE1.'))

    results.expect('Checking whether A2 was stored into bucket exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute private key ALICE.A2 into bucket.'))
    results.expect('Checking whether A2 was decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Decrypted attribute private key ALICE.A2 using role ALICE.ROLE1.'))

    results.expect('Checking whether A3 was stored into bucket exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute public key ALICE.A3 into bucket.'))
    results.expect('Checking whether A3 was not decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Could not decrypt attribute public key ALICE.A3!'))
    results.expect('Checking whether A3 was decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::decryptAttributeBuckets}: Decrypted attribute public key ALICE.A3 using role ALICE.ROLE2.'))

    results.expect('Checking whether A4 was stored into bucket exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute private key ALICE.A4 into bucket.'))
    results.expect('Checking whether A4 was not decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Could not decrypt attribute private key ALICE.A4!'))
    results.expect('Checking whether A4 was decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::decryptAttributeBuckets}: Decrypted attribute private key ALICE.A4 using role ALICE.ROLE2.'))

    results.expect('Checking whether control message replaced ROLE2 exactly once', 1, BOB.countMatchingLinesInLog(
                   '{SecurityManager::configureRoleSharedSecrets}: Replaced shared secret for role ALICE.ROLE2'))

    # Control message is sent before publishing, so encryption should never fail.
    results.expect('Checking whether encryption never failed at BOB', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'))
