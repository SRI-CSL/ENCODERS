# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""
Additional Security Data Attributes: Tests whether additional security data request attributes work.

The test uses a simple 2 node configuration, with ALICE as the authority node. 
BOB will publish with a single encryption attribute, that will be subscribed to at ALICE.
The Security Data Requests and Responses should contain the appropriate additional attributes.
"""

CATEGORIES=['sanity', 'authorization']

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

    ALICE.addAdditionalSecurityDataResponseAttribute('name', 'value', 5)
    ALICE.addAdditionalSecurityDataResponseAttribute('ContentOrigin', '%%replace_current_node_name%%')
    ALICE.addAdditionalSecurityDataResponseAttribute('ContentCreationTime', '%%replace_current_time%%')

    BOB.addAdditionalSecurityDataRequestAttribute('name', 'value', 3)
    BOB.addAdditionalSecurityDataRequestAttribute('ContentOrigin', '%%replace_current_node_name%%')
    BOB.addAdditionalSecurityDataRequestAttribute('ContentCreationTime', '%%replace_current_time%%')

    ALICE.createConfig(securityLevel='HIGH', encryptFilePayload=True, compositeSecurityDataRequests=True)
    BOB.createConfig(securityLevel='HIGH', encryptFilePayload=True, compositeSecurityDataRequests=True)

    env.startAllNodes()
    env.sleep('Letting nodes boot', env.config.exchangeDelay)
    BOB.publishItem('object1', '')
    results.expect('Subscribing to object1 at ALICE', True, ALICE.subscribeItem('object1'))
    env.stopAllNodes()



    results.expect('Checking whether ALICE configured AdditionalSecurityDataResponseAttributes.', 1, ALICE.countMatchingLinesInLog(
                   "{SecurityManager::onConfig}: Configuring AdditionalSecurityDataResponseAttributes"))
    results.expect('Checking whether ALICE configured AdditionalSecurityDataRequestAttributes.', 1, ALICE.countMatchingLinesInLog(
                   "{SecurityManager::onConfig}: Configuring AdditionalSecurityDataRequestAttributes"))

    results.expect('Checking whether BOB configured AdditionalSecurityDataRequestAttributes.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityManager::onConfig}: Configuring AdditionalSecurityDataRequestAttributes"))
    results.expect('Checking whether BOB configured AdditionalSecurityDataResponseAttributes.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityManager::onConfig}: Configuring AdditionalSecurityDataResponseAttributes"))

    results.expect('Checking whether ALICE configured AdditionalSecurityDataResponseAttributes.', 1, ALICE.countMatchingLinesInLog(
                   "{SecurityManager::configureAdditionalAttributes}: Configuring additional attribute name=value:5"))
    results.expect('Checking whether ALICE configured AdditionalSecurityDataResponseAttributes.', 1, ALICE.countMatchingLinesInLog(
                   "{SecurityManager::configureAdditionalAttributes}: Configuring additional attribute ContentOrigin=%%replace_current_node_name%%:1"))
    results.expect('Checking whether ALICE configured AdditionalSecurityDataResponseAttributes.', 1, ALICE.countMatchingLinesInLog(
                   "{SecurityManager::configureAdditionalAttributes}: Configuring additional attribute ContentCreationTime=%%replace_current_time%%:1"))

    results.expect('Checking whether BOB configured AdditionalSecurityDataRequestAttributes.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityManager::configureAdditionalAttributes}: Configuring additional attribute name=value:3"))
    results.expect('Checking whether BOB configured AdditionalSecurityDataRequestAttributes.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityManager::configureAdditionalAttributes}: Configuring additional attribute ContentOrigin=%%replace_current_node_name%%:1"))
    results.expect('Checking whether BOB configured AdditionalSecurityDataRequestAttributes.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityManager::configureAdditionalAttributes}: Configuring additional attribute ContentCreationTime=%%replace_current_time%%:1"))

    results.expect('Checking whether ALICE added AdditionalSecurityDataResponseAttributes.', 1, ALICE.countMatchingLinesInLog(
                   "{SecurityHelper::addAdditionalAttributes}: Adding additionalAttribute name=value:5"))
    results.expect('Checking whether ALICE added AdditionalSecurityDataResponseAttributes.', 1, ALICE.countMatchingLinesInLog(
                   "{SecurityHelper::addAdditionalAttributes}: Adding additionalAttribute ContentOrigin=n0:1"))
    results.expect('Checking whether ALICE added AdditionalSecurityDataResponseAttributes.', 1, ALICE.countMatchingLinesInLog(
                   "{SecurityHelper::addAdditionalAttributes}: Adding additionalAttribute ContentCreationTime=[0-9]*:1"))

    results.expect('Checking whether BOB added AdditionalSecurityDataRequestAttributes.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityHelper::addAdditionalAttributes}: Adding additionalAttribute name=value:3"))
    results.expect('Checking whether BOB added AdditionalSecurityDataRequestAttributes.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityHelper::addAdditionalAttributes}: Adding additionalAttribute ContentOrigin=n1:1"))
    results.expect('Checking whether BOB added AdditionalSecurityDataRequestAttributes.', 1, BOB.countMatchingLinesInLog(
                   "{SecurityHelper::addAdditionalAttributes}: Adding additionalAttribute ContentCreationTime=[0-9]*:1"))
