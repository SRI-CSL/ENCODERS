# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

"""

Contains examples for generating the SecurityManager section of the haggle config.xml.
For detailed instructions, please see README-config_generator

"""

import json
from framework.config import SecurityConfig
from framework.node_manager import NodeManager

haggleConfigLocation = '../../haggle/resources/config-flood-direct-thirdpartynd-noshortcut.xml'
fullDebugging = False
config = """
{
    "defaultSecurityParameters":    {
        "securityLevel":        "HIGH",
        "encryptFilePayload":   true
    },

    "perNodeSecurityParameters":    {
        "ALICE":    {
            "securityLevel":    "VERYHIGH"
        }
    },

    "haggleNodeIDs":    {
        "ALICE":    "ID1",
        "BOB":      "ID2",
        "EVE":      "ID3"
    },

    "staticKeyPairs":   [
        "ALICE",
        "BOB"
    ],

    "additionalSecurityDataRequestAttributes":  [
        {
            "name": "testing",
            "value": "7",
            "weight": 5
        },
        {
            "name": "ContentOrigin",
            "value": "%%replace_current_node_name%%"
        },
        {
            "name": "ContentCreationTime",
            "value": "%%replace_current_time%%"
        }
    ],

    "additionalSecurityDataResponseAttributes":  [
        {
            "name": "testing",
            "value": "1",
            "weight": 2
        },
        {
            "name": "ContentOrigin",
            "value": "%%replace_current_node_name%%"
        },
        {
            "name": "ContentCreationTime",
            "value": "%%replace_current_time%%"
        }
    ],

    "authorities":      {
        "ALICE":    {
            "certifiedNodes":   [
                "BOB",
                "EVE"
            ],
            "staticallyCertifiedNodes":     [
                "BOB"
            ],
            "roles":    {
                "role1":    {
                    "encryptionAttributes": [
                        "A1",
                        "A2"
                    ],
                    "decryptionAttributes": [
                        "A1",
                        "A3"
                    ],
                    "authorizedNodes":      [
                        "ALICE"
                    ],
                    "provisionedNodes":     [
                        "BOB"
                    ],
                    "preProvisionedNodes":  [
                        "EVE"
                    ]
                }
            }
        }
    }
}
"""

def convert(input):
    if isinstance(input, dict):
        return {convert(key): convert(value) for key, value in input.iteritems()}
    elif isinstance(input, list):
        return [convert(element) for element in input]
    elif isinstance(input, unicode):
        return input.encode('ascii')
    else:
        return input

def generateConfigsWithAPI():
    nodes = NodeManager()
    ALICE = nodes.addNode('ALICE')
    (BOB, EVE) = nodes.addNodes('BOB', 'EVE')
    ALICE.haggleNodeID = 'ID1'
    BOB.haggleNodeID = 'ID2'
    EVE.haggleNodeID = 'ID3'
    ALICE.addNodeSharedSecret('BOB')
    EVE.addNodeSharedSecret('ALICE')
    ALICE.setAuthority()
    EVE.setAuthority()
    BOB.addAuthorities('ALICE')
    EVE.addAuthorities('ALICE')
    ALICE.authorizeNodesForCertification('BOB', 'EVE')
    ALICE.authorizeRoleForAttributes('ROLE1', ['A1'], [])
    ALICE.authorizeRoleForAttributes('ROLE2', ['A2'], ['A1'])
    EVE.authorizeRoleForAttributes('ROLE1', ['A3', 'A1'], ['A1', 'A4'])
    BOB.addRoleSharedSecret('ALICE.ROLE1')
    EVE.addRoleSharedSecret('ALICE.ROLE2')
    BOB.provisionForRole('ALICE.ROLE2')
    BOB.preProvisionForRole('ALICE.ROLE1')
    BOB.preProvisionForRole('EVE.ROLE1')
    ALICE.createConfig(haggleConfigLocation=haggleConfigLocation, securityLevel='LOW', encryptFilePayload=False, signNodeDescriptions=True)
    BOB.createSecurityConfig(securityLevel='LOW', encryptFilePayload=False, tempFilePath='/tmp/haggletmpsecdata.XXXXXX')
    EVE.createSecurityConfig(securityLevel='LOW', encryptFilePayload=False)
    print BOB.securityConfig
    print EVE.securityConfig
    EVE.clearConfig()
    nodes.clearConfigs()

def generateSecurityConfigsFromJSON(cfg):
    cfg = convert(cfg)
    baseConfig = SecurityConfig()
    baseConfig.update(cfg.get('defaultSecurityParameters', {}))
    nodes = NodeManager(baseConfig)

    haggleNodeIDs = cfg.get('haggleNodeIDs', {})
    if len(haggleNodeIDs) == 0:
        raise Exception('Need to have at least one haggleNodeID specified!')

    for name, ID in haggleNodeIDs.items():
        node = nodes.addNode(name)
        node.haggleNodeID = ID

    staticKeyPairs = cfg.get('staticKeyPairs', [])
    for name in staticKeyPairs:
        if name not in nodes:
            raise Exception('Can not statically generate key pair at nonexistent node %s!' % name)
        nodes[name].generateKeyPair()

    additionalSecurityDataRequestAttributes = cfg.get('additionalSecurityDataRequestAttributes', [])
    for attr in additionalSecurityDataRequestAttributes:
        for node in nodes:
            node.addAdditionalSecurityDataRequestAttribute(**attr)

    additionalSecurityDataResponseAttributes = cfg.get('additionalSecurityDataResponseAttributes', [])
    for attr in additionalSecurityDataResponseAttributes:
        for node in nodes:
            node.addAdditionalSecurityDataResponseAttribute(**attr)

    authorities = cfg.get('authorities', {})
    for name, section in authorities.items():
        if name not in nodes:
            raise Exception('Can not make nonexistent node %s an authority!' % name)

        auth = nodes[name]
        auth.setAuthority()

        for other in section.get('certifiedNodes', []):
            if other not in nodes:
                raise Exception('Can not certify nonexistent node %s for certification at authority %s!' (other, auth.name))

            node = nodes[other]
            node.addAuthorities(auth.name)
            auth.addNodeSharedSecret(node.name)
            auth.authorizeNodesForCertification(node.name)

        for other in section.get('staticallyCertifiedNodes', []):
            if other not in nodes:
                raise Exception('Can not statically certify nonexistent node %s for certification at authority %s!' (other, auth.name))

            node = nodes[other]

            if auth not in node.authorities:
                node.addAuthorities(auth.name)
                auth.addNodeSharedSecret(node.name)

            auth.signCertificateForNode(node.name)

        for roleName, entry in section.get('roles', {}).items():
            fullName = '%s.%s' % (auth.name, roleName)
            if len(entry.get('encryptionAttributes', []) + entry.get('decryptionAttributes', [])) == 0:
                raise Exception("Role %s needs to be authorized for at least one attribute!" % (fullName))

            auth.authorizeRoleForAttributes(roleName, entry.get('encryptionAttributes', []), entry.get('decryptionAttributes', []))

            for other in entry.get('authorizedNodes', []):
                if other not in nodes:
                    raise Exception('Can not authorize nonexistent node %s for role %s at authority %s!' (other, roleName, auth.name))
                node = nodes[other]
                node.addRoleSharedSecret(fullName)

            for other in entry.get('provisionedNodes', []):
                if other not in nodes:
                    raise Exception('Can not provision nonexistent node %s for role %s at authority %s!' (other, roleName, auth.name))
                node = nodes[other]
                node.provisionForRole(fullName)

            for other in entry.get('preProvisionedNodes', []):
                if other not in nodes:
                    raise Exception('Can not pre-provision nonexistent node %s for role %s at authority %s!' (other, roleName, auth.name))
                node = nodes[other]
                node.preProvisionForRole(fullName)

    return nodes

def generateConfigsFromJSON(cfg, haggleConfigLocation, fullDebugging=False):
    nodes = generateSecurityConfigsFromJSON(cfg)

    for node in nodes:
        node.createConfig(haggleConfigLocation, fullDebugging, **cfg.get('perNodeSecurityParameters', {}).get(node.name, {}))
        with open('config.xml.%s' % node.name, 'w') as out:
            out.write(node.config)

if __name__ == "__main__":
    generateConfigsFromJSON(json.loads(config), haggleConfigLocation, fullDebugging)
