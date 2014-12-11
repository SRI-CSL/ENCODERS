# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Composite Requests: Tests whether composite requests work correctly. Also tests whether encryption works.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
BOB will publish with a single encryption attribute, that will be subscribed to at ALICE.
This should succeed.
There should be a single composite request.
"""

CATEGORIES=['sanity', 'authorization', 'certification']

def runTest(env, nodes, results, Console):
    ALICE, BOB = env.createNodes('ALICE', 'BOB')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB')
    ALICE.authorizeRoleForAttributes('ROLE1', ['A1'], [])
    ALICE.authorizeRoleForAttributes('ROLE2', [], ['A1'])
    ALICE.addRoleSharedSecret('ALICE.ROLE2')
    BOB.addRoleSharedSecret('ALICE.ROLE1')

    ALICE.createConfig(securityLevel='HIGH', encryptFilePayload=True, compositeSecurityDataRequests=True)
    BOB.createConfig(securityLevel='HIGH', encryptFilePayload=True, compositeSecurityDataRequests=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.publishItem('object1', 'ALICE.A1')
    results.expect('Subscribing to object1 at ALICE', True, ALICE.subscribeItem('object1'))
    env.stopAllNodes()

    # Eager request should give it the key
    results.expect('Checking whether BOB enabled composite security data requests.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityManager::onConfig}: Enabling composite security data requests!"))
    results.expect('Checking whether BOB sent composite security data request.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityManager::generateCompositeRequest}: Generating composite request task"))
    results.expect('Checking whether encryption never failed at BOB', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'))
    results.expect('Checking whether A1 was stored into bucket exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute public key ALICE.A1 into bucket.'))
    results.expect('Checking whether A1 was decrypted using role exactly once', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Decrypted attribute public key ALICE.A1 using role ALICE.ROLE1.'))
