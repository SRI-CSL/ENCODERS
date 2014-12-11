CBMEN Security
--------------

Haggle's Security Manager supports four levels of security:

	LOW	: no digital signatures are used.
	MEDIUM	: only node descriptions are signed and verified.
	HIGH	: all data objects are signed and verified.
    VERYHIGH: all data objects with an 'Access' attribute are encrypted

The security level VERYHIGH is a shortcut for the setting
security_level="HIGH" and encrypt_file_payload="true" (see below).

The security level can be set to XXX in the Haggle config.xml with:

<SecurityManager security_level="XXX"/>

where XXX is LOW, MEDIUM, HIGH, or VERYHIGH.

As of 11/15/2012, the 'security' branch of CBMEN ENCODERS supports the 
following new functionality beyond what was available in version 0.4 of 
Haggle:


Background Signing
------------------

No special configuration is required. The Security Manager design has 
been modified to perform data object signing asynchronously in a 
Security Helper Task running in a separate thread, rather than in the 
main Haggle thread.


Optional Node Description Signing
---------------------------------

Node descriptions are signed by default. Support has been added to allow 
them to be disseminated without including the node's digital certificate 
or computing and including the digital signature of the node description.

This option is only useful when the security level is HIGH. It allows 
data objects with content to be signed while disabling the signing of 
node descriptions.

Node description signing can be disabled in the Haggle config.xml with:

<SecurityManager sign_node_descriptions="false"/>

Multiple Certificate Authorities
--------------------------------

In version 0.4 of Haggle, the CA keys were hard-coded. This has been changed.
Any node can now act as a CA. This can be enabled in the Haggle config.xml with:

<SecurityManager>
<Authority name = "SomeAuthority" />
</SecurityManager>

When a node is acting as a CA, it will sign public keys that it receives from
authenticated nodes and respond with the signed certificates. It will also
piggy back its own signed certificate in the response.

An authority has a 'name' that is used for scoping attributes. This allows
the mapping between physical, real world authorities to node IDs to be a
dynamic mapping.

A regular node needs to know which nodes it considers to be authorities,
so that it can appropriately request certificates and encryption
attributes from those nodes. The list of authorities can be specified
in the configuration as below:

<SecurityManager>
	<Authorities>
	    <Authority id="b0b646aa661722ef1cb619160c550004e87d361d" name="CBMEN" />
	</Authorities>
</SecurityManager>

Shared Secrets
--------------

In order to authenticate nodes and authorities, a shared secret scheme
is used. A node will encrypt and HMAC its SecurityDataRequests with this
shared secret, and an authority will act similarly for SecurityDataResponses.

Shared Secrets are set on a per-node basis. An authority should set them
for each node it intends to authorize, and a normal node should set them
for each authority it intends to communicate with.

Shared Secrets can be set in the configuration as below.

<SecurityManager>
	<SharedSecrets>
	    <Node id="681277cff0f4b5205a26b1e2314ae112be0ec792" shared_secret="Y2i9U5hKxFrPuhF8b9mLNg==" />
	    <Node id="3d17546eb3ffab4194128b85958a9c4fcfb830c4" shared_secret="this is a password" />
	</SharedSecrets>
</SecurityManager>

These shared secrets are passed through a key derivation function to produce the 
actual keys for the symmetric encryption and authentication.

Multiple Node Certificates
--------------------------

With the introduction of multiple certificate authorities, a node may possess
multiple certificates, each issued by a different authority. When node descriptions
are exchanged, all certificates are now piggy backed instead of just one. This is
so that the likelihood of the receiving node being able to verify the certificates
is increased.

Transitive Trust Relationships
------------------------------

With the move to multiple certificate authorities, it is no longer simple for a node to
know which nodes it can trust on the network. We use transitive trust relationships,
where two nodes will trust each other if they both have certificates signed by the same
authority.

When a node has not received a signed certificate from all its configured authorities,
it will periodically broadcast its public key to neighbouring nodes so that it can
receive the a signed certificate. It will also send its public key on meeting
an authority node for the first time. These are sent using the SecurityDataRequest
mechanism (detailed below).

An authority will respond to these broadcasts by sending back a signed certificate,
as well as the authority's own certificate. The node can then use this authority
certificate to verify any certificates it has received from other nodes, so that
it can start trusting them and verifying any data objects.

The requesting of certificates can be configured as below:

<SecurityManager certificate_signing_request_delay="15" 
		certificate_signing_first_request_delay="15" 
		certificate_signing_request_retries="2" />

The number of periodic certificate requests rounds (each generating
possibly a request for each neighbor) can be limited using 
certificate_signing_request_retries. This can be useful in a testbed
to limit all requests to occur during a warm-up phase.

The frequency of requests for certificate signature is specified
using certificate_signing_request_delay, which takes a value in 
seconds. The first request can be sent out earlier, facilitating testing,
using certificate_signing_first_request_delay.

Trust Bootstrapping
-------------------

At a high enough security level, all node descriptions must be signed.
This has implications for routing when a node first joins a network,
as it can not verify node descriptions and thus not communicate with any
other nodes on the network. This poses a problem as it is unable to request
certificates from authorities, hindering its participation in the network.

To facilitate trust bootstrapping, we introduce a secondary check that is
performed when signature verification fails for a node description due to the
lack of a verified certificate.

If no previously stored certificates are present for the given node, 
the node description is verified using the self signed certificates present 
in the node description. These are then stored.

If a previously stored (unverified) certificate is present for the given node,
a check is made to ensure that the certificate in the node description is the same,
and then the node description is verified using the given certificate.

This scheme allows nodes to receive node descriptions and communicate on the network
so that they can receive authority certificates and begin trusting other
nodes on the network and then participate fully on the network. The trust model
is similar to that used by SSH, where it is assumed that the first node that claims
an identity is the legitimate one. 

Declarative Certification
-------------------------

It is possible for an authority to configure the nodes that it authorizes
for certification. Any authorized nodes will receive a signed certificate
if they ask for it, while other nodes will not receive any signed certificates.

This can be configured in the config, as in the following snippet:

<SecurityManager>
	<Authority name="CBMEN">
		<Node id="eb257bb83e5e2922953fc5fd16ab641ac6af7d93" certify="true" />
	</Authority>
</SecurityManager>

Any nodes that have certify=true in the config will be authorized for certification.
They must also have the appropriate shared secret configured so that they can
make a request for certificate signature.

Signature Chaining
------------------

When this functionality is enabled, DataObjects have a "chain" of signatures, 
which lets the receiver trace the path that the DataObject took from the sender. 

Before sending out an object, a node signs it and adds its signature to the chain.
The signature chain is in the metadata in the below format:

<SignatureChain hops="2">
<Signature hop="0" signee="..."> .... </Signature>
<Signature hop="1" signee="..."> .... </Signature>
</Signaturechain>

Signature chaining can be enabled in the Haggle config.xml with:

<SecurityManager signature_chaining="true" />

SecurityDataRequests and SecurityDataResponses
----------------------------------------------

In order to implement the new Security functionality, communication between nodes
and authorities is required. This is implemented by nodes sending out 
SecurityDataRequests, which are data objects with the 'SecurityDataRequest' attribute.
The file payload for these objects is encrypted and HMAC'd with a shared secret, which
is used for authentication and confidentiality. Upon receiving a request, an authority
can generate an appropriate SecurityDataResponse and send it back.

The maximum number of outstanding security data requests can be specified in the configuration:

<SecurityManager max_outstanding_requests="3"/>

The Haggle kernel uses temporary files for the SecurityDataRequests and SecurityDataResponses,
these files are created using the 'mkstemp' function. The path for these files can be 
configured using the 'temp_file_path' parameter. It must contain the string "XXXXXX" 
and must be in a location writeable by the haggle user.

An example configuration is below:

<SecurityManager temp_file_path="[path]" />

The default path (if none is specified) are below:

#ifdef OS_ANDROID
#define TEMP_FILE_PATH "/data/data/org.haggle.kernel/files/haggletmpsecdata.XXXXXX"
#else
#define TEMP_FILE_PATH "/tmp/haggletmpsecdata.XXXXXX"
#endif

Charm Setup (on Linux and Android)
---------------------------------

For the Attributed Based Encryption functionality (described below), it is
required that the Charm libraries be installed. Note that they are only
required if the security level is set to "VERYHIGH".

Linux
-----

To setup Charm on Linux, switch to the 'charm' directory and follow
the instructions in the INSTALL.sri file. Charm is invoked by Haggle via the 
Charm Crypto Bridge (CCB) (in the 'ccb' directory).

Android
-------

Charm is bundled with the APK, no setup is necessary.

Charm Configuration
-------------------

Charm requires certain global parameters to be synchronized across all devices
in order for encryption to work correctly.

To generate a persistence file with global parameters, navigate to the
'~/cbmen-encoders/ccb/python/' folder. Then, open a python shell and 
run the following lines:

>>> import ccb
>>> ccb.init()
>>> ccb.shutdown()

It will return a string. Copy the value inside the quotes and save it to a file somewhere.

A sample persistence file, with just a set of global parameters, is provided in the 
'~/cbmen-encoders/ccb/charm_state.orig' file. Note that all nodes must have the same 
global parameters configured or encryption/decryption will fail.

The global parameters can be set in the configuration file as below:

<SecurityManager charm_persistence_data="[data]"/>    

The default parameters currently used are specified in SecurityManager.h:

#define CHARM_PERSISTENCE_DATA "eJyrVkosLcnIL8osqYwvzlayUqiu1VFQKi1OLULiIpQUwMXSC0AspXQgqWRole6aaxpq4unn7GribaGd61YZGWXkH1GRlpbtXJZXFlqkX+Fcbp6T7xpSZWHhGZCZ5RpkFpCabZoUUO7ka5lRYJBXVFiV4eQSHFaemeur7+SfbmKUnlLuaqtUWwsACb01cg=="

Attribute Based Encryption
--------------------------

If the configuration parameter "encrypt_file_payload" is set to true as
in the following configuration file excerpt

<SecurityManager security_level="LOW" encrypt_file_payload="true" />

and a DataObject is received from an application that has the
"Access" attribute, it will be encrypted with this policy. A
symmetric key is generated and used to encrypt the data file; this key
is encrypted using MA-ABE and the resulting capability is added to the
DataObject's metadata.

On the receiving end, this capability is used to extract the symmetric key which is used
to decrypt the file and it is then sent on to the receiving application.

Key Distribution
----------------

The SecurityDataRequest/SecurityDataResponse mechanisms are used for key distribution.
There are two types of requests, lazy (specific) and eager (all). When an outgoing object
needs to be encrypted and the appropriate public keys are not present, a SecurityDataRequest
will be sent out for the specific public keys that are needed. Similarly, when an object needs
to be decrypted and the appropriate private keys are not present, a SecurityDataRequest will be
sent out for those specific private keys. 

The above lazy mechanism is sufficient for meeting functionality requirements; but an eager
mechanism has also been implemented to improve performance. When a node interacts with a new
authority (after receiving a certificate signature), it will send requests to that authority
to get all its public keys and private keys. Receiving this information early on will save
having to request the keys when they are needed later on.

When a node meets another node, it sends outstanding attribute requests in case
the new neighbor is an authority. Pending attributes will also be periodically requested
from neighboring authorities. The frequency of these security data requests can be configured with
the "attribute_request_delay" option in the SecurityManager section of the configuration:

<SecurityManager attribute_request_delay="60" />

Policy Specifications
---------------------

Policies can be any logical formula over the attributes. The 'or', 'and', 'OR', and 'AND' operators
are allowed, along with paranetheses '(' and ')'. Attribute identifiers (and authority names) must be
alphanumeric.

Attributes in the policy must be namespaced with the authority name, to ensure that attribute names
are globally unique. The '.' character is used for namespacing.

The following is an example of a valid policy string:

Authority1.Attribute1 OR (Authority2.Attribute1 AND (Authority3.Attribute2 or Authority3.Attribute9))

Declarative Authorization
------------------------

Nodes can take on various "roles", which are the basic unit of access control.
Each role has an associated shared secret. The shared secrets can be configured
as below:

<SecurityManager>
	<SharedSecrets>
        <Role name="ALICE.ROLE1" shared_secret="NdatNqem0rv52HoAf7y5Ww==" />
        <Role name="ALICE.ROLE3" shared_secret="PY0sFAALsWD99RZbv0kexA==" />
        <Role name="ALICE.ROLE2" shared_secret="esgZ2+tDbY2YrIxFOW9LJQ==" />
	</SharedSecrets>
</SecurityManager>

When a node requests encryption or decryption attributes, it sends along the
list of roles that it is configured for. The authority uses this list
to perform access control. It will send respond with all the attributes
that it is authorized for, encrypted with the appropriate role shared secrets.
The node will decrypt the encryption or decryption attributes that it can,
and use them. Note that this means that a node can configure an incorrect
shared secret for a role in order to request attributes; so that they do
not have to be requested later when the correct shared secret is entered.

Authorities can declare access control lists for encryption and decryption
attributes on a per-role basis. A sample configuration snippet is below:

<SecurityManager>
	<Authority name="CBMEN">
        <Role name="CBMEN.ROLE1">
            <Attribute decryption="false" encryption="true" name="A1" />
            <Attribute decryption="true" encryption="false" name="A2" />
        </Role>
        <Role name="CBMEN.ROLE2">
            <Attribute decryption="false" encryption="true" name="A1" />
            <Attribute decryption="true" encryption="false" name="A2" />
        </Role>
        <Role name="CBMEN.ROLE3">
            <Attribute decryption="false" encryption="true" name="A1" />
            <Attribute decryption="true" encryption="false" name="A2" />
        </Role>
	</Authority>
</SecurityManager>

Note that roles are scoped by the authority name, and the role name
must be prefixed by the authority name. This is to ensure that role names are
globally unique. The '.' character is used for namespacing.

Within each Role entry, Attribute entries are used to scope access to the
attributes used for ABE encryption. For each Attribute (identified by the
"name" property), the "encryption" and "decryption" properties are used to
specify whether the given role has access to the encryption and decryption
keys for the given attribute.

Testing
=======

Please look at the following directories in the cbmen-encoders-eval
repository for tests:

    tests/Security
    tests/Signing
