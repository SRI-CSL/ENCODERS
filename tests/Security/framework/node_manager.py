# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Hasnain Lakhani (HL)

""" 

Contains the NodeManager for managing the security configuration for nodes in a test.

"""

import os
import base64
import functools
from config import SecurityConfig
from collections import OrderedDict
from xml.etree import ElementTree as et
import subprocess
import random
import shutil

HAVE_CCB = False
try:
    import ccb
    # We don't want the ccb output
    from collections import namedtuple
    mock = namedtuple('mock', ['log'])
    ccb.emb = mock(log=lambda s: None)
    HAVE_CCB = True
except ImportError:
    pass

# Taken from http://effbot.org/zone/element-lib.htm#prettyprint
def _indent(elem, level=0):
    i = "\n" + level*"    "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "    "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            _indent(elem, level+1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i

NULLOUT = open('/dev/null', 'w')
class Node(object):

    def __init__(self, name):
        self.name = name
        self.running = False
        self.haggleNodeID = None

        self.androidDeviceID = None

        self.coreDeviceID = None
        self.coreNode = None
        self.coreNodeName = None

        self.config = None
        self.securityConfig = None
        self.nodeSharedSecrets = {}
        self.roleSharedSecrets = {}
        self.authorities = []
        self.isAuthority = False
        self.certifiedNodes = []
        self.authorityAttributes = {}
        self.managedRoles = []

        self.publicKey = None
        self.privateKey = None
        self.myCerts = {}
        self.caCerts = {}

        self.issuedEncryptionAttributes = {}
        self.provisionedEncryptionAttributes = {}
        self.provisionedDecryptionAttributes = {}
        self.ccbState = {}

        self.additionalSecurityDataRequestAttributes = []
        self.additionalSecurityDataResponseAttributes = []

    def __repr__(self):
        output = 'Node "%s": ID %s' % (self.name, self.haggleNodeID)
        if self.androidDeviceID is not None:
            output += ' (device ID %s)' % self.androidDeviceID
        if self.coreDeviceID is not None:
            output += ' (node ID %s)' % self.coreDeviceID
        if self.config is not None:
            output += '\n%s\n' % self.config
        return output

class NodeManager(object):

    def __init__(self, baseConfig=None):
        self.nodes = OrderedDict()
        if baseConfig is None:
            self.baseConfig = SecurityConfig()
        else:
            self.baseConfig = baseConfig

        if os.path.exists(self.baseConfig.openSSLTmpCertFolder):
            shutil.rmtree(self.baseConfig.openSSLTmpCertFolder)
        if not os.path.exists(self.baseConfig.openSSLTmpCertFolder):
            os.makedirs(self.baseConfig.openSSLTmpCertFolder)
        with open(self.baseConfig.openSSLConfigFile, 'w') as f:
            f.write(self.baseConfig.openSSLConfigFileData)

    def addNode(self, name):
        if name in self.nodes:
            raise Exception('Already have node %s!' % name)
        node = Node(name)

        node.clearConfig = functools.partial(self.clearConfig, name)
        node.addNodeSharedSecret = functools.partial(self.addNodeSharedSecret, name)
        node.addRoleSharedSecret = functools.partial(self.addRoleSharedSecret, name)
        node.provisionForRole = functools.partial(self.provisionNodeForRole, name)
        node.setAuthority = functools.partial(self.setAuthority, name)
        node.addAuthorities = functools.partial(self.addAuthorities, name)
        node.authorizeNodesForCertification = functools.partial(self.authorizeNodesForCertification, name)
        node.authorizeRoleForAttributes = functools.partial(self.authorizeRoleForAttributes, name)
        node.generateKeyPair = functools.partial(self.generateKeyPair, name)
        node.signCertificateForNode = functools.partial(self.signCertificateForNode, name)
        node.preProvisionForRole = functools.partial(self.preProvisionNodeForRole, name)
        node.addAdditionalSecurityDataRequestAttribute = functools.partial(self.addAdditionalSecurityDataRequestAttribute, name)
        node.addAdditionalSecurityDataResponseAttribute = functools.partial(self.addAdditionalSecurityDataResponseAttribute, name)
        node.createSecurityConfig = functools.partial(self.createSecurityConfig, name)
        node.createConfig = functools.partial(self.createConfig, name)

        self.nodes[name] = node
        return node

    def addNodes(self, *names):
        return (self.addNode(name) for name in names)

    @property
    def haggleNodeIDs(self):
        return [node.haggleNodeID for node in self.nodes if node.haggleNodeID is not None]

    @property
    def androidDeviceIDs(self):
        return [node.androidDeviceID for node in self.nodes if node.androidDeviceID is not None]

    def __getitem__(self, name):
        if name not in self.nodes:
            return None 
        return self.nodes[name]

    def __len__(self):
        return len(self.nodes)

    def __contains__(self, name):
        return name in self.nodes

    def __iter__(self):
        return self.nodes.itervalues()

    def clearConfig(self, name):
        if name not in self.nodes:
            raise Exception("Can't clear config for nonexistent node %s!" % name)
        node = self.nodes[name]

        node.config = None
        node.securityConfig = None
        node.nodeSharedSecrets = {}
        node.roleSharedSecrets = {}
        node.authorities = []
        node.isAuthority = False
        node.certifiedNodes = []
        node.authorityAttributes = {}
        node.managedRoles = []
        node.publicKey = None
        node.privateKey = None
        node.myCerts = {}
        node.caCerts = {}
        node.issuedEncryptionAttributes = {}
        node.provisionedEncryptionAttributes = {}
        node.provisionedDecryptionAttributes = {}
        node.ccbState = {}
        node.additionalSecurityDataRequestAttributes = []
        node.additionalSecurityDataResponseAttributes = []

    def clearConfigs(self):
        for name in self.nodes.iterkeys():
            self.clearConfig(name)

    def addNodeSharedSecret(self, name1, name2):
        if name1 not in self.nodes:
            raise Exception("Can't add node shared secret for nonexistent node %s!" % name1)
        if name2 not in self.nodes:
            raise Exception("Can't add node shared secret for nonexistent node %s!" % name2)

        n1 = self.nodes[name1]
        n2 = self.nodes[name2]

        if name2 in n1.nodeSharedSecrets and name1 in n2.nodeSharedSecrets:
            return

        secret = base64.b64encode(open('/dev/urandom', 'r').read(16))
        n1.nodeSharedSecrets[n2] = secret
        n2.nodeSharedSecrets[n1] = secret

    def addRoleSharedSecret(self, name, fullName):
        if name not in self.nodes:
            raise Exception("Can't add role shared secret for nonexistent node %s!" % name)
        if fullName.find('.') == -1:
            raise Exception('Improperly scoped role! Must be AUTHORITYNAME.ROLENAME')
        else:
            authority, role = fullName.split('.')
            if len(authority) == 0 or len(role) == 0:
                raise Exception('Improperly scoped role! Must be AUTHORITYNAME.ROLENAME')

        if authority not in self.nodes:
            raise Exception("Can't add role shared secret for nonexistent authority %s!" % authority)

        node = self.nodes[name]
        authority = self.nodes[authority]

        if not authority.isAuthority:
            raise Exception("Can't authorize nodes for certification at non-authority node %s!" % authority.name)
        
        if fullName not in authority.managedRoles:
            raise Exception("Can't add role shared secret for nonexistent role %s!" % fullName)

        if fullName in authority.roleSharedSecrets:
            secret = authority.roleSharedSecrets[fullName]
        else:
            raise Exception('Shared secret for role %s not found!' % fullName)

        node.roleSharedSecrets[fullName] = secret

    def provisionNodeForRole(self, name, fullName):
        if name not in self.nodes:
            raise Exception("Can't provision for role on nonexistent node %s!" % name)
        if fullName.find('.') == -1:
            raise Exception('Improperly scoped role! Must be AUTHORITYNAME.ROLENAME')
        else:
            authority, role = fullName.split('.')
            if len(authority) == 0 or len(role) == 0:
                raise Exception('Improperly scoped role! Must be AUTHORITYNAME.ROLENAME')

        if authority not in self.nodes:
            raise Exception("Can't provision for role with nonexistent authority %s!" % authority)

        node = self.nodes[name]
        authority = self.nodes[authority]

        if not authority.isAuthority:
            raise Exception("Can't authorize nodes for certification at non-authority node %s!" % authority.name)
        
        if fullName not in authority.managedRoles:
            raise Exception("Can't provision for role with nonexistent role %s!" % fullName)

        node.roleSharedSecrets[fullName] = base64.b64encode(open('/dev/urandom', 'r').read(16))

    def setAuthority(self, name):
        if name not in self.nodes:
            raise Exception("Can't set nonexistent node %s as authority!" % name)

        self.nodes[name].isAuthority = True

        if HAVE_CCB:
            ccb.init(self.baseConfig.charmPersistenceData)
            self.nodes[name].ccbState = ccb.state

    def addAuthorities(self, name, *authorities):
        if name not in self.nodes:
            raise Exception("Can't add authorities for nonexistent node %s!" % name)

        for authority in authorities:
            if authority not in self.nodes:
                raise Exception("Can't add nonexistent node %s as authority!" % authority)
            if not self.nodes[authority].isAuthority:
                raise Exception("Can't add non-authority node %s as authority!" % authority)
            if authority not in self.nodes[name].authorities:
                self.nodes[name].authorities.append(self.nodes[authority])

    def authorizeNodesForCertification(self, authority, *names):
        if authority not in self.nodes:
            raise Exception("Can't authorize using nonexistent authority %s!" % authority)

        if not self.nodes[authority].isAuthority:
            raise Exception("Can't authorize nodes for certification at non-authority node %s!" % authority)

        for name in names:
            if name not in self.nodes:
                raise Exception("Can't authorize nonexistent node %s!" % name)

            if name not in self.nodes[authority].certifiedNodes:
                self.nodes[authority].certifiedNodes.append(self.nodes[name])

    def authorizeRoleForAttributes(self, authority, role, encryption, decryption):
        if authority not in self.nodes:
            raise Exception("Can't authorize using nonexistent authority node %s!" % authority)

        fullName = '%s.%s' % (authority, role)
        authority = self.nodes[authority]

        if not authority.isAuthority:
            raise Exception("Can't authorize nodes for certification at non-authority node %s!" % authority.name)

        if fullName not in authority.managedRoles:
            authority.managedRoles.append(fullName)

        if fullName not in authority.roleSharedSecrets:
            secret = base64.b64encode(open('/dev/urandom', 'r').read(16))
            authority.roleSharedSecrets[fullName] = secret

        for attr in encryption:
            if attr not in authority.authorityAttributes:
                authority.authorityAttributes[attr] = {'encryption': [], 'decryption': []}

            if fullName not in authority.authorityAttributes[attr]['encryption']:
                authority.authorityAttributes[attr]['encryption'].append(fullName)

        for attr in decryption:
            if attr not in authority.authorityAttributes:
                authority.authorityAttributes[attr] = {'encryption': [], 'decryption': []}

            if fullName not in authority.authorityAttributes[attr]['decryption']:
                authority.authorityAttributes[attr]['decryption'].append(fullName)

    def generateKeyPair(self, name):
        if name not in self.nodes:
            raise Exception("Can't generate key pair for nonexistent node %s!" % name)

        node = self.nodes[name]
        if node.publicKey is not None or node.privateKey is not None:
            raise Exception("Already have key pair for node %s!" % name)

        if node.haggleNodeID is None:
            raise Exception("Can't generate key pair for node %s without having haggleNodeID!" % node.name)

        dic = dict(size=self.baseConfig.rsaKeyLength, certFolder=self.baseConfig.openSSLTmpCertFolder, id=node.haggleNodeID)
        subprocess.call([c.format(**dic) for c in self.baseConfig.openSSLGenerateKeyPairAndSelfSignedCertCommandList], stdout=NULLOUT, stderr=NULLOUT)

        dic = dict(certFolder=self.baseConfig.openSSLTmpCertFolder, id=node.haggleNodeID)
        process = subprocess.Popen([c.format(**dic) for c in self.baseConfig.openSSLGetPublicKeyCommandList], stdout=subprocess.PIPE, stderr=NULLOUT)
        node.publicKey = process.communicate()[0]

        with open('%s/%s.pem' % (self.baseConfig.openSSLTmpCertFolder, node.haggleNodeID)) as f:
            node.privateKey = f.read()

    def signCertificateForNode(self, auth, name):
        if auth not in self.nodes:
            raise Exception("Can't sign certificate with nonexistent authority %s!" % auth)

        if name not in self.nodes:
            raise Exception("Can't sign certificate for nonexistent node %s!" % name)

        auth = self.nodes[auth]
        node = self.nodes[name]

        if auth.haggleNodeID is None:
            raise Exception("Can't sign certificate at authority node %s without having haggleNodeID!" % auth.name)
        if node.haggleNodeID is None:
            raise Exception("Can't sign certificate for node %s without having haggleNodeID!" % node.name)

        if auth.publicKey is None or auth.privateKey is None:
            raise Exception("Can't sign certificate at authority node %s without keypair!" % auth.name)
        if node.publicKey is None or node.privateKey is None:
            raise Exception("Can't sign certificate for node %s without keypair!" % node.name)

        if auth.haggleNodeID in node.myCerts:
            raise Exception("Already have signed certificate by authority %s at node %s!" % (auth.name, node.name))

        if auth.haggleNodeID in node.caCerts:
            raise Exception("Already have ca certificate for authority %s at node %s!" % (auth.name, node.name))

        if not auth.isAuthority:
            raise Exception("Can't sign certificate using non-authority node %s!" % auth.name)

        if os.path.exists(self.baseConfig.openSSLCAFolder):
            shutil.rmtree(self.baseConfig.openSSLCAFolder)
        if not os.path.exists(self.baseConfig.openSSLCAFolder):
            os.makedirs(self.baseConfig.openSSLCAFolder)
        with open(os.path.join(self.baseConfig.openSSLCAFolder, 'serial'), 'w') as f:
            f.write('01')
        with open(os.path.join(self.baseConfig.openSSLCAFolder, '.rand'), 'w') as f:
            f.write('%s' % random.randint(0, 1000))
        with open(os.path.join(self.baseConfig.openSSLCAFolder, 'index.txt'), 'w') as f:
            pass
        shutil.copy(os.path.join(self.baseConfig.openSSLTmpCertFolder, '%s.pem' % auth.haggleNodeID), os.path.join(self.baseConfig.openSSLCAFolder, 'cakey.pem'))
        shutil.copy(os.path.join(self.baseConfig.openSSLTmpCertFolder, '%s.crt' % auth.haggleNodeID), os.path.join(self.baseConfig.openSSLCAFolder, 'cacert.pem'))

        dic = dict(certFolder=self.baseConfig.openSSLTmpCertFolder, id=node.haggleNodeID)
        subprocess.call([c.format(**dic) for c in self.baseConfig.openSSLGenerateCertificateSigningRequestList], stdout=NULLOUT, stderr=NULLOUT)

        dic = dict(certFolder=self.baseConfig.openSSLTmpCertFolder, subject=node.haggleNodeID, configFile=self.baseConfig.openSSLConfigFile, issuer=auth.haggleNodeID)
        subprocess.call([c.format(**dic) for c in self.baseConfig.openSSLCASignCertificateCommandList], stdout=NULLOUT, stderr=NULLOUT)

        with open('%s/%s_%s.crt' % (self.baseConfig.openSSLTmpCertFolder, node.haggleNodeID, auth.haggleNodeID)) as f:
            node.myCerts[auth.haggleNodeID] = f.read()

        with open('%s/%s.crt' % (self.baseConfig.openSSLTmpCertFolder, auth.haggleNodeID)) as f:
            node.caCerts[auth.haggleNodeID] = f.read()

    def preProvisionNodeForRole(self, name, fullName):
        if not HAVE_CCB:
            raise Exception("Can't pre-provision for role without ccb module!")

        if name not in self.nodes:
            raise Exception("Can't pre-provision for role on nonexistent node %s!" % name)
        if fullName.find('.') == -1:
            raise Exception('Improperly scoped role! Must be AUTHORITYNAME.ROLENAME')
        else:
            authority, role = fullName.split('.')
            if len(authority) == 0 or len(role) == 0:
                raise Exception('Improperly scoped role! Must be AUTHORITYNAME.ROLENAME')

        if authority not in self.nodes:
            raise Exception("Can't pre-provision for role with nonexistent authority %s!" % authority)

        node = self.nodes[name]
        authority = self.nodes[authority]

        if not authority.isAuthority:
            raise Exception("Can't pre-provision for role with non-authority node %s!" % authority.name)
        
        if fullName not in authority.managedRoles:
            raise Exception("Can't pre-provision for role with nonexistent role %s!" % fullName)

        if node.haggleNodeID is None:
            raise Exception("Can't pre-provision for role at node %s without having haggleNodeID!" % node.name)

        ccb.state = authority.ccbState
        for name, acl in authority.authorityAttributes.iteritems():
            encryption = fullName in acl['encryption']
            decryption = fullName in acl['decryption']

            attr = ('%s.%s' % (authority.name, name)).upper()
            if encryption or decryption:
                if attr not in authority.issuedEncryptionAttributes:
                    ccb.authsetup([attr])
                    authority.issuedEncryptionAttributes[attr] = ccb.serialize(ccb.state['authority_pk'][attr])

            if encryption and attr not in node.provisionedEncryptionAttributes:
                node.provisionedEncryptionAttributes[attr] = authority.issuedEncryptionAttributes[attr]

            if decryption and attr not in node.provisionedDecryptionAttributes:
                node.provisionedDecryptionAttributes[attr] = ccb.keygen(attr, node.haggleNodeID, False)

    def addAdditionalSecurityDataRequestAttribute(self, nodeName, name, value, weight=1):

        if nodeName not in self.nodes:
            raise Exception("Can't add additional security data request attribute on nonexistent node %s!" % nodeName)

        self.nodes[nodeName].additionalSecurityDataRequestAttributes.append(dict(name=name, value=str(value), weight=str(weight)))

    def addAdditionalSecurityDataResponseAttribute(self, nodeName, name, value, weight=1):

        if nodeName not in self.nodes:
            raise Exception("Can't add additional security data response attribute on nonexistent node %s!" % nodeName)

        self.nodes[nodeName].additionalSecurityDataResponseAttributes.append(dict(name=name, value=str(value), weight=str(weight)))

    def createSecurityConfig(self, name, **kwargs):
        if name not in self.nodes:
            raise Exception("Can't create config for nonexistent node %s" % name)
        node = self.nodes[name]

        config = self.baseConfig.__class__()
        config.update(self.baseConfig.toDict())
        config.update(kwargs)

        if len(node.ccbState) > 0:
            if 'charmPersistenceData' in kwargs:
                raise Exception("Can not specify charmPersistenceData if using static attribute provisioning at an authority!")
            elif not HAVE_CCB:
                raise Exception("Can not save ccb state without ccb module!")
            else:
                ccb.state = node.ccbState
                config.charmPersistenceData = ccb.shutdown()
                ccb.init(config.charmPersistenceData)
                node.ccbState = ccb.state

        boolToStr = {True: 'true', False: 'false'}

        securityManager = et.Element('SecurityManager', dict(
                         signature_chaining=boolToStr[config.signatureChaining],
                         security_level=config.securityLevel,
                         encrypt_file_payload=boolToStr[config.encryptFilePayload],
                         charm_persistence_data=config.charmPersistenceData,
                         attribute_request_delay=str(config.attributeRequestDelay),
                         certificate_signing_request_delay=str(config.certificateSigningRequestDelay),
                         certificate_signing_request_retries=str(config.certificateSigningRequestRetries),
                         certificate_signing_first_request_delay=str(config.certificateSigningFirstRequestDelay),
                         max_outstanding_requests=str(config.maxOutstandingRequests),
                         sign_node_descriptions=boolToStr[config.signNodeDescriptions],
                         rsa_key_length=str(config.rsaKeyLength),
                         composite_security_data_requests=boolToStr[config.compositeSecurityDataRequests]))

        if config.tempFilePath is not None:
            if config.tempFilePath.find('XXXXXX') == -1:
                raise Exception('tempFilePath %s is invalid! needs to contain XXXXXX' % config.tempFilePath)
            else:
                securityManager.set('temp_file_path', config.tempFilePath)

        sharedSecrets = et.Element('SharedSecrets')
        for other, secret in node.nodeSharedSecrets.iteritems():
            if other.haggleNodeID is None:
                raise Exception('Can not add shared secret for node %s without having haggleNodeID!' % other.name)
            sharedSecrets.append(et.Element('Node', dict(id=other.haggleNodeID, shared_secret=secret)))

        for role, secret in node.roleSharedSecrets.iteritems():
            sharedSecrets.append(et.Element('Role', dict(name=role.upper(), shared_secret=secret)))

        authorities = et.Element('Authorities')
        for auth in node.authorities:
            if auth.haggleNodeID is None:
                raise Exception('Can not add authority node %s without having haggleNodeID!' % other.name)
            authorities.append(et.Element('Authority', dict(id=auth.haggleNodeID, name=auth.name.upper())))

        if node.isAuthority:
            authority = et.Element('Authority', dict(
                                   name=node.name.upper()))

            for other in node.certifiedNodes:
                certify = True
                child = et.Element('Node', dict(
                                   id=other.haggleNodeID,
                                   certify=boolToStr[certify]))
                authority.append(child)

            for role in node.managedRoles:
                certify = other in node.certifiedNodes
                child = et.Element('Role', dict(
                                   name=role.upper()))

                for name, acl in node.authorityAttributes.iteritems():
                    encryption = role in acl['encryption']
                    decryption = role in acl['decryption']

                    attrnode = et.Element('Attribute', dict(
                                          name=name.upper(),
                                          encryption=boolToStr[encryption],
                                          decryption=boolToStr[decryption]))

                    child.append(attrnode)
                authority.append(child)

        if node.publicKey is not None and node.privateKey is not None:
            keyPair = et.Element('KeyPair', {})
            privateKey = et.Element('PrivateKey')
            privateKey.text = node.privateKey
            keyPair.append(privateKey)
            publicKey = et.Element('PublicKey')
            publicKey.text = node.publicKey
            keyPair.append(publicKey)
            securityManager.append(keyPair)

        certificates = et.Element('Certificates', {})
        if len(node.myCerts) > 0:
            myCerts = et.Element('myCerts')
            for k,v in node.myCerts.iteritems():
                cert = et.Element('Certificate')
                cert.text = v
                myCerts.append(cert)
            certificates.append(myCerts)
        if len(node.caCerts) > 0:
            caCerts = et.Element('caCerts')
            for k,v in node.caCerts.iteritems():
                cert = et.Element('Certificate')
                cert.text = v
                caCerts.append(cert)
            certificates.append(caCerts)

        provisionedAttributes = et.Element('ProvisionedAttributes', {})
        if len(node.provisionedEncryptionAttributes) > 0:
            encryptionAttributes = et.Element('EncryptionAttributes', {})
            for k,v in node.provisionedEncryptionAttributes.iteritems():
                attr = et.Element('Attribute', dict(name=k))
                attr.text = v
                encryptionAttributes.append(attr)
            provisionedAttributes.append(encryptionAttributes)
        if len(node.provisionedDecryptionAttributes) > 0:
            decryptionAttributes = et.Element('DecryptionAttributes', {})
            for k,v in node.provisionedDecryptionAttributes.iteritems():
                attr = et.Element('Attribute', dict(name=k))
                attr.text = v
                decryptionAttributes.append(attr)
            provisionedAttributes.append(decryptionAttributes)
        if len(node.issuedEncryptionAttributes) > 0:
            issuedEncryptionAttributes = et.Element('IssuedEncryptionAttributes', {})
            for k,v in node.issuedEncryptionAttributes.iteritems():
                attr = et.Element('Attribute', dict(name=k))
                attr.text = v
                issuedEncryptionAttributes.append(attr)
            provisionedAttributes.append(issuedEncryptionAttributes)

        additionalSecurityDataRequestAttributes = et.Element('AdditionalSecurityDataRequestAttributes', {})
        if len(node.additionalSecurityDataRequestAttributes) > 0:
            for attr in node.additionalSecurityDataRequestAttributes:
                elem = et.Element('Attribute', attr)
                additionalSecurityDataRequestAttributes.append(elem)

        additionalSecurityDataResponseAttributes = et.Element('AdditionalSecurityDataResponseAttributes', {})
        if len(node.additionalSecurityDataResponseAttributes) > 0:
            for attr in node.additionalSecurityDataResponseAttributes:
                elem = et.Element('Attribute', attr)
                additionalSecurityDataResponseAttributes.append(elem)

        securityManager.append(provisionedAttributes)
        securityManager.append(certificates)
        securityManager.append(sharedSecrets)
        securityManager.append(authorities)
        securityManager.append(additionalSecurityDataRequestAttributes)
        securityManager.append(additionalSecurityDataResponseAttributes)

        if node.isAuthority:
            securityManager.append(authority)

        
        _indent(securityManager)
        node.securityConfig = et.tostring(securityManager)

    def createConfig(self, name, haggleConfigLocation=None, fullDebugging=True, **kwargs):
        if name not in self.nodes:
            raise Exception("Can't create config for nonexistent node %s" % name)
        node = self.nodes[name]

        if haggleConfigLocation is None and hasattr(self.baseConfig, 'haggleConfigLocation'):
            haggleConfigLocation = self.baseConfig.haggleConfigLocation
        # if 'haggleConfigLocation' in kwargs:
            # haggleConfigLocation = kwargs['haggleConfigLocation']

        if haggleConfigLocation is None or not os.path.exists(haggleConfigLocation):
            raise Exception('haggleConfigLocation %s does not exist!' % haggleConfigLocation)

        tree = et.ElementTree()
        tree.parse(haggleConfigLocation)
        root = tree.getroot()

        secs = root.findall('SecurityManager')
        for sec in secs:
            root.remove(sec)

        if fullDebugging:
            debugString = """
            <DebugManager>
                <DebugTrace enable="true" type="DEBUG2" flush="true" />
            </DebugManager>
            """
            debugs = root.findall('DebugManager')
            for debug in debugs:
                root.remove(debug)
            root.append(et.fromstring(debugString))

        node.createSecurityConfig(**kwargs)
        root.append(et.fromstring(node.securityConfig))
        _indent(root)
        node.config = et.tostring(root)
