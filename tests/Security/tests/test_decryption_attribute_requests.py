# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Decryption Attribute Requests: Tests whether decryption attribute requests work correctly. Also tests whether decryption works correctly.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
ALICE will publish with a single encryption attribute, that will be subscribed to at BOB.
This should succeed.
There should be a request for the decryption attribute.
"""

CATEGORIES=['sanity', 'authorization']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB')
    ALICE.authorizeRoleForAttributes('ROLE1', [], ['A1'])
    ALICE.authorizeRoleForAttributes('ROLE2', ['A1'], [])
    ALICE.addRoleSharedSecret('ALICE.ROLE2')
    BOB.addRoleSharedSecret('ALICE.ROLE1')

    ALICE.createConfig(securityLevel='HIGH', encryptFilePayload=True)
    BOB.createConfig(securityLevel='HIGH', encryptFilePayload=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    ALICE.publishItem('object1', 'ALICE.A1')
    results.expect('Subscribing to object1 at BOB', True, BOB.subscribeItem('object1'))
    env.stopAllNodes()

    # Eager request should give it the key
    results.expect('Checking whether decryption never failed at BOB.', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::useCapability}: Missing some decryption attributes, can not decrypt capability for Data Object'))
    results.expect('Checking whether A1 was stored into bucket exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute private key ALICE.A1 into bucket.'))
    results.expect('Checking whether A1 was decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Decrypted attribute private key ALICE.A1 using role ALICE.ROLE1.'))
