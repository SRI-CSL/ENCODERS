Overview
--------

The framework comes with a generator for the SecurityManager config section of haggle configuration files. This
makes it easy to declaratively specify authorization and certification, and can also be used in a standalone manner in the Python shell 
to generate configurations for provisioning. For example of such usage, please see config_generator.py

There are two ways to generate configuration files. The recommended approach is to declaratively specify it using a JSON description.
The other approach is to use the node manager API, which is used by the unit tests in the unit test framework, and also by the 
declarative specification method. Instructions for both the declarative specification, and the node manager API are below.

Configuration Generator - Declarative JSON specification
--------------------------------------------------------

For an example of this, view/edit the haggleConfigLocation, fullDebugging, and config variables at the top of the 
config_generator.py file. Then run the config_generator.py to generate sample config files.

For an explanation of the haggleConfigLocation and fullDebugging parameters, view the documentation
for the node.createConfig() API call.

In the declarative JSON declaration, the following sections are supported:

defaultSecurityParameters (optional)
====================================

This is a dictionary containing the default parameters that are to go into the SecurityManager config
section of the haggle config.xml file. For a list of supported parameters, view the documentation for
the node.createSecurityConfig() API call.

perNodeSecurityParameters (optional)
====================================

This is a dictionary which can contain a dictionary per specified node, with the node name as the key and
the dictionary as the value. The dictionary contains parameters that are to go into the SecurityManager
config section of the haggle config.xml file for this node. These will override the defaultSecurityParameters. 
For a list of supported parameters, view the documentation for the node.createSecurityConfig() API call.

haggleNodeIDs (required)
========================

This is a dictionary containing mappings from node names to haggleNodeIDs. An entry must be present
for each node.

staticKeyPairs (optional)
=========================

This is a list containing the names of nodes that have their key pairs generated statically.

authorities (optional)
======================

This is a dictionary which can contain a dictionary per authority, with the node name as the key
and the dictionary as the value. If a node is specified in this dictionary, it will become an authority.

An explanation of the sections in the authority dictionary is below.

    certifiedNodes (optional)
    -=-=-=-=-=-=-=-=-=-=-=-=-

    This is a list containing the names of nodes that are authorized for certification at this authority.

    staticallyCertifiedNodes (optional)
    -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    This is a list containing the names of nodes that will have their certificates statically signed by this authority.

    roles (optional)
    -=-=-=-=-=-=-=-=

    This is a dictionary which can contain a dictionary per specified role, with the role name as the key and
    the dictionary as the value. The dictionary contains the following sections for the role:

        encryptionAttributes (optional)
        -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        This is a list of encryption attributes that this role is authorized for.

        decryptionAttributes (optional)
        -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

        This is a list of decryption attributes that this role is authorized for.

        authorizedNodes (optional)
        -=-=-=-=-=-=-=-=-=-=-=-=-=

        This is a list containing the names of nodes that are authorized for this role.

        provisionedNodes (optional)
        -=-=-=-=-=-=-=-=-=-=-=-=-=-

        This is a list containing the names of nodes that are to be provisioned for this role;
        they will be configured with an incorrect shared secret for the role so that they will
        request attributes from the authority for faster provisioning.

        preProvisionedNodes (optional)
        -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        This is a list containing the names of nodes that are to be pre-provisioned for this role;
        they will be configured with the encryption and decryption attributes for this role in
        their configuration.

Configuration Generator - Node Manager API
------------------------------------------

For an example of this, check the generateConfigsWithAPI() function in config_generator.py

Note that the haggleNodeIDs should be set before creating the configuration, or an exception will be thrown. If running
test code, this just means that env.calculateHaggleNodeIDs() or env.bootForHaggleNodeIDs(bootTime) should be called. Otherwise,
for each node, its haggleNodeID attribute should be set.

The NodeManager object is used to create and manage the list of nodes in an environment. Each Node object is configured
separately, and once all security configuration has been specified, the configurations can be generated.
The NodeManager is a dictionary-like object, so calling nodes[name] should return the node with the given name.

The following methods are supported on the Node Manager:

    nodes.addNode(name)

        Adds a node with the given name, and returns the node object

    nodes.addNodes(*names)

        Adds nodes with the given names, and returns the node objects

    nodes.clearConfigs()

        Clears the configuration on all nodes

The following configuration methods are supported on each node:

    node.addNodeSharedSecret(name, type='base64')

        Generates a unique shared secret to be used between this node and the other node identified by name.
        The other node will also have this shared secret set. 
        The type of the shared secret is indicated by type ('base64' or 'pbkdf2').

    node.addRoleSharedSecret(roleName, type='base64')

        Gives the node the shared secret for the given roleName, which must be properly scoped.
        This role must already have been created using authorizeRoleForAttributes
        The type of the shared secret is indicated by type ('base64' or 'pbkdf2').

    node.provisionForRole(roleName, type='base64')

        Provisions the node for the given roleName, which must be properly scoped. It will request
        and receive attributes for the role, but it will not be able to decrypt and use them. This is useful for provisioning.
        The type of the shared secret is indicated by type ('base64' or 'pbkdf2').

    node.setAuthority()

        Declares the node to be an authority

    node.addAuthorities(*names)

        Declares every node named in the argument list as an authority for this node.

    node.authorizeNodesForCertification(*names)

        Authorizes every node named in the argument list for certification at this authority.

    node.authorizeRoleForAttributes(role, encryption, decryption, type='base64')

        Authorizes the given role (which should not be scoped, as it will automatically be scoped by the node manager) for the listed encryption and decryption attributes at this authority.
        The type of the shared secret used for this role is indicated by type ('base64' or 'pbkdf2').

    node.generateKeyPair()

        Statically generates a keypair for the node to use for certification.

    node.signCertificateForNode(name)

        Statically generate a certificate for the node identified by name at this authority.

    node.preProvisionForRole(roleName)

        Pre-provisions the node for the given roleName, which must be properly scoped. It will have the authorized encryption and decryption
        attributes present in the configuration.

    node.createSecurityConfig(**kwargs)

        Creates the SecurityManager section of Haggle config file for the node and store it on the node object, in the properties mentioned
        in the next section.

        All the security configuration parameters from the SecurityConfig object are valid keyword arguments. 
        Specifically, the following keyword arguments will have an effect:

            1. signatureChaining
            2. securityLevel
            3. encryptFilePayload
            4. certificateSigningRequestDelay
            5. certificateSigningRequestRetries
            6. certificateSigningFirstRequestDelay
            7. attributeRequestDelay
            8. maxOutstandingRequests
            9. charmPersistenceData
            10. signNodeDescriptions
            11. tempFilePath
            12. rsaKeyLength

    node.createConfig(haggleConfigLocation=None, fullDebugging=True, **kwargs)

        Creates the full Haggle config file for the node and store it on the node object, in the properties mentioned in the next
        section. It will automatically call createSecurityConfig() to create the security section.

        haggleConfigLocation should be a path to a base haggle config.xml file containing configurations for all other sections.
        It needs to be set on the config object (will automatically be set by test framework) or specified as an argument.
        If fullDebugging is set, the debug section from the base config file will be changed to enable a trace level of DEBUG2.

The following properties are present on each node:

    node.haggleNodeID

        This is the haggle node ID for this node

    node.securityConfig

        This is the security manager section of the haggle config.xml for this node

    node.config

        This is the haggle config.xml for this node
