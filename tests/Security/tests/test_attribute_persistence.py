# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Attribute Persistence: Tests whether encryption and decryption attributes are persisted correctly after restarts.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
BOB will publish with a single encryption attribute, that will be subscribed to at ALICE.
ALICE will publish with a single encryption attribute, that will be subscribed to at BOB.
Both should succeed.
There should be requests for the encryption attribute, as well as the decryption attribute.

Nodes are then restarted.
BOB publishes again with the same attribute, and it should be received at ALICE.
ALICE will publish again with the same attribute, and it should be received at BOB.
No additional requests for attributes should be made.
"""

CATEGORIES=['sanity', 'authorization']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB')
    ALICE.authorizeRoleForAttributes('ROLE1', ['A1'], ['A2'])
    ALICE.authorizeRoleForAttributes('ROLE2', ['A1', 'A2'], ['A1', 'A2'])
    ALICE.addRoleSharedSecret('ALICE.ROLE2')
    BOB.addRoleSharedSecret('ALICE.ROLE1')

    ALICE.createConfig(securityLevel='LOW', encryptFilePayload=True)
    BOB.createConfig(securityLevel='LOW', encryptFilePayload=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.publishItem('object1', 'ALICE.A1')
    results.expect('Subscribing to object1 at ALICE', True, ALICE.subscribeItem('object1'))
    ALICE.publishItem('object2', 'ALICE.A2')
    results.expect('Subscribing to object2 at BOB', True, BOB.subscribeItem('object2'))
    env.stopAllNodes()

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.publishItem('object3', 'ALICE.A1')
    results.expect('Subscribing to object3 at ALICE', True, ALICE.subscribeItem('object3'))
    ALICE.publishItem('object4', 'ALICE.A2')
    results.expect('Subscribing to object4 at BOB', True, BOB.subscribeItem('object4'))
    env.stopAllNodes()


    # Eager request should give it the key
    results.expect('Checking whether encryption never failed at BOB', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'))
    results.expect('Checking whether A1 was stored into bucket exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute public key ALICE.A1 into bucket.'))
    results.expect('Checking whether A1 was decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Decrypted attribute public key ALICE.A1 using role ALICE.ROLE1.'))

    # Eager request should give it the key.
    results.expect('Checking whether decryption never failed at BOB', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::useCapability}: Missing some decryption attributes, can not decrypt capability for Data Object'))
    results.expect('Checking whether A2 was stored into bucket exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute private key ALICE.A2 into bucket.'))
    results.expect('Checking whether A2 was decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Decrypted attribute private key ALICE.A2 using role ALICE.ROLE1.'))

    # Twice because nodes were shutdown twice.
    results.expect('Checking whether A1 was persisted exactly twice', 2, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::onPrepareShutdown}: Storing encryption attribute: ALICE.A1'))
    results.expect('Checking whether A2 was persisted exactly twice', 2, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::onPrepareShutdown}: Storing decryption attribute: ALICE.A2'))

    # Once because the attributes should only be there on second boot up
    results.expect('Checking whether A1 was loaded exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::onRepositoryData}: Read encryption attribute: ALICE.A1'))
    results.expect('Checking whether A2 was loaded exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityManager::onRepositoryData}: Read decryption attribute: ALICE.A2'))
