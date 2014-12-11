/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _CERTIFICATE_H
#define _CERTIFICATE_H

#include <libcpphaggle/String.h>
#include "Attribute.h"
#include "Metadata.h"
#include "Debug.h"
#include "Node.h"

#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <stdio.h>

using namespace haggle;

/**
	A class that implements certificates by wrapping the openssl x509 API.
 
 */
#ifdef DEBUG_LEAKS
class Certificate : public LeakMonitor
#else
class Certificate
#endif
{
	// Whether this certificate is stored or not.
	bool stored;
	// to define who is the owner of this certificate
	Node::Id_t owner;
	// to define if the certificate is verified or not
	bool verified;
	// True if Certificate is signed
	bool hasSignature;

	X509 *x; // The X509 certificate
	
	// The subject that this certificate is for
	string subject;
	string issuer;
	string validity; // TODO: should this be a string or something else?
	
	// The public key associated with this certificate
	EVP_PKEY *pubKey;
	RSA *rsaPubKey;
	// Certificate in PEM format
	char *x509_PEM_str;
	
	bool createDigest(unsigned char digest[SHA_DIGEST_LENGTH], const string data) const;
	
	Certificate(X509 *_x);
public: // CBMEN, HL
	Certificate(const string& _subject, const string& _issuer, const string& _validity, const Node::Id_t _owner, RSA *_pubKey);
	~Certificate();
	/**
	 Create an attribute certificate. The private/public keypair will be generated.
	 
	 @param subject the subject to generate a certificate for
	 @param issuer the issuer of the certificate
	 @param validity for how long the certificate is valid.
	 @param privKey a pointer to a RSA key pointer which will hold the generated private key
	 @param keyLength the length of the key to create (in bits)
	 
	 @returns a pointer to a new certificate for the attribute, or NULL on failure. The certificate must
	 be deleted by the creator.
	 */
	static Certificate *create(const string subject, const string issuer, const string validity, const Node::Id_t owner, RSA **privKey, size_t keyLength);
	/**
	 Create an attribute certificate with a public key stored in a file.
	 
	 @param subject the subject to generate a certificate for
	 @param issuer the issuer of the certificate
	 @param validity for how long the certificate is valid.
	 @param privKeyFile the filename of a file holding the public key of the certificate
	 
	 @returns a pointer to a new certificate for the attribute, or NULL on failure. The certificate must
	 be deleted by the creator.
	 */
	static Certificate *create(const string subject, const string issuer, const string validity, const Node::Id_t owner, const string pubKeyFile);	
	void setStored(bool _stored = true) { stored = _stored; }
	bool isStored() const { return stored; }
	bool isSigned(void) const;
	/**
	 Sign the certificate using the private key of the issuer (certificate authority).
	 */ 
	bool sign(EVP_PKEY *key);
	bool sign(RSA *key);
	/**
	 Verify the signature in the certificate using the public key of the issuer (certificate authority).
	 If the verification is successful, the certificate will be marked as verified and isVerified() will
	 return true.
	 */
	bool verifySignature(EVP_PKEY *key);
	bool verifySignature(RSA *key);
	bool isOwner(const Node::Id_t owner) const;
	bool isSubject(const string subject) const;
        RSA *getPubKey();
        const RSA *getPubKey() const;
    EVP_PKEY *getEVPPubKey(); // CBMEN, HL
    const EVP_PKEY *getEVPPubKey() const; // CBMEN, HL
	void printPubKey() const;
        bool isVerified() const { return verified; }
	const unsigned char *getOwnerId() const { return owner; }
	const string& getIssuer() const { return issuer; }	
	const string& getSubject() const { return subject; }
        int writePEM(const char *file);
        int writePEM(FILE *fp = stdout);
        static Certificate *readPEM(const char *file);
        static Certificate *readPEM(FILE *fp);
	const char *toPEM() const;
	string toString() const;
	Metadata *toMetadata() const;
        static Certificate *fromPEM(const char *pem);
	static Certificate *fromPEM(const string& pem);
	static Certificate *fromMetadata(const Metadata& m);
        void print(FILE *fp = stdout);
	friend bool operator==(const Certificate& c1, const Certificate& c2);
	friend bool operator!=(const Certificate& c1, const Certificate& c2);
};

#include <libcpphaggle/Reference.h>

typedef Reference<Certificate> CertificateRef; 
typedef ReferenceList<Certificate> CertificateRefList;

#endif
