# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Declarative Authorization: Tests whether declarative authorization works correctly.

The test uses a simple 3 node configuration, with ALICE as the authority node. 
All nodes are booted up.


BOB will try to publish a data object requiring encryption, and it will make a request for an encryption attribute.
It should successfully receive the encryption attribute, as it is authorized for it.
EVE will subscribe to this object, and it will make a request for a decryption attribute.
It should successfully receive the decryption attribute, as it is authorized for it.

EVE will try to publish a data object requiring encryption, and it will make a request for an encryption attribute.
It should not receive the encryption attribute, as it is not authorized for it.

EVE will try to publish a data object requiring encryption, and it will make a request for an encryption attribute.
It should receive the attribute, as it is authorized for it.
BOB will subscribe to this data object, and it will make a request for a decryption attribute.
It should not receive the attribute, as it is not authorized for it.
"""

CATEGORIES=['sanity', 'authorization']

def runTest(env, nodes, results, Console):
    ALICE, BOB, EVE = env.createNodes('ALICE', 'BOB', 'EVE')

    env.calculateHaggleNodeIDsExternally()
    ALICE.addNodeSharedSecret('BOB')
    ALICE.addNodeSharedSecret('EVE')

    ALICE.setAuthority()
    BOB.addAuthorities('ALICE')
    EVE.addAuthorities('ALICE')

    ALICE.authorizeNodesForCertification('BOB', 'EVE')
    ALICE.authorizeRoleForAttributes('ROLE1', ['A1'], [])
    ALICE.authorizeRoleForAttributes('ROLE2', ['A2'], ['A1'])
    BOB.addRoleSharedSecret('ALICE.ROLE1')
    EVE.addRoleSharedSecret('ALICE.ROLE2')


    ALICE.createConfig(securityLevel='LOW', encryptFilePayload=True)
    BOB.createConfig(securityLevel='LOW', encryptFilePayload=True)
    EVE.createConfig(securityLevel='LOW', encryptFilePayload=True)

    env.startAllNodes()
    env.sleep('Letting nodes exchange certificates', env.config.exchangeDelay)

    BOB.publishItem('object1', 'ALICE.A1')
    results.expect('Subscribing to object1 at EVE', True, EVE.subscribeItem('object1'))

    EVE.publishItem('object2', 'ALICE.A1')

    EVE.publishItem('object3', 'ALICE.A2')
    results.expect('Subscribing to object3 at BOB', False, BOB.subscribeItem('object3'))


    env.stopAllNodes()

    results.expect('Checking whether encryption never failed at BOB', 0, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'), False, 'acceptable with a linear topology')
    results.expect('Checking whether A1 was stored into bucket exactly once at BOB', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute public key ALICE.A1 into bucket.'), False, 'acceptable with a linear topology')
    results.expect('Checking whether A1 was decrypted using role exactly once at BOB', 1, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Decrypted attribute public key ALICE.A1 using role ALICE.ROLE1.'), False, 'acceptable with a linear topology')
    results.expect('Checking whether decryption never failed at EVE', 0, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::useCapability}: Missing some decryption attributes, can not decrypt capability for Data Object'), False, 'acceptable with a linear topology')
    results.expect('Checking whether A1 was stored into bucket exactly once at EVE', 1, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute private key ALICE.A1 into bucket.'), False, 'acceptable with a linear topology')
    results.expect('Checking whether A1 was decrypted using role exactly once at EVE', 1, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Decrypted attribute private key ALICE.A1 using role ALICE.ROLE2.'), False, 'acceptable with a linear topology')

    # Initial failure for A1
    results.expect('Checking whether encryption failed exactly once at EVE', 1, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::generateCapability}: Missing some encryption attributes, can not generate capability for Data Object'), False, 'acceptable with a linear topology')
    results.expect('Checking whether A2 was stored into bucket exactly once at EVE', 1, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Saving encrypted attribute public key ALICE.A2 into bucket.'), False, 'acceptable with a linear topology')
    results.expect('Checking whether A2 was decrypted using role exactly once at EVE', 1, EVE.countMatchingLinesInLog( 
                   '{SecurityHelper::saveReceivedKeys}: Decrypted attribute public key ALICE.A2 using role ALICE.ROLE2.'), False, 'acceptable with a linear topology')

    # Changed to > 1, sometimes the interest match for haggletest -x nop succeeds and we try decryption
    predicate = lambda c: c >= 1
    results.expect('Checking whether decryption failed at least once at BOB', predicate, BOB.countMatchingLinesInLog( 
                   '{SecurityHelper::useCapability}: Missing some decryption attributes, can not decrypt capability for Data Object'))

    results.expect('Checking whether ALICE sent encryption attribute A1 to BOB at least once', predicate, ALICE.countMatchingLinesInLog( 
                   '{SecurityHelper::send(All|Specific)PublicKeys}: %s is authorized for attribute public key ALICE.A1 through role ALICE.ROLE1, sending.' % BOB.haggleNodeID))
    results.expect('Checking whether ALICE sent encryption attribute A2 to EVE at least once', predicate, ALICE.countMatchingLinesInLog( 
                   '{SecurityHelper::send(All|Specific)PublicKeys}: %s is authorized for attribute public key ALICE.A2 through role ALICE.ROLE2, sending.' % EVE.haggleNodeID))
    results.expect('Checking whether ALICE sent decryption attribute A1 to EVE at least once', predicate, ALICE.countMatchingLinesInLog( 
                   '{SecurityHelper::send(All|Specific)PrivateKeys}: %s is authorized for attribute private key ALICE.A1 through role ALICE.ROLE2, sending.' % EVE.haggleNodeID))
    
    # Initial failure, one retry while requesting A2 (as we request all pending attributes in a lazy request), one retry due to waiting queues.
    results.expect('Checking whether ALICE refused to send encryption attribute A1 to EVE exactly once', 1, ALICE.countMatchingLinesInLog( 
                   '{SecurityHelper::sendSpecificPublicKeys}: %s is not authorized for attribute public key ALICE.A1, not sending.' % EVE.haggleNodeID), False, 'acceptable with a linear topology')

    results.expect('Checking whether ALICE refused to send decryption attribute A2 to BOB exactly once', 1, ALICE.countMatchingLinesInLog( 
                   '{SecurityHelper::sendSpecificPrivateKeys}: %s is not authorized for attribute private key ALICE.A2, not sending.' % BOB.haggleNodeID), False, 'acceptable with a linear topology')

