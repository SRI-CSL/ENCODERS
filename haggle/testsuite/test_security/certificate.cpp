/* Copyright 2008 Uppsala University
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

#include "testhlp.h"
#include <libcpphaggle/Thread.h>
#include "utils.h"
#include <haggleutils.h>
#include <Certificate.h>
#include <Node.h>
#include <DataObject.h>

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

using namespace haggle;
const char ca_public_key[] =
"-----BEGIN PUBLIC KEY-----\n\
MFwwDQYJKoZIhvcNAQEBBQADSwAwSAJBAPR0eI6KaW618dwjFQtNkm9YwUeDOVqb\n\
Nbh3V7EkdVrB5g4SxY82budnqC3nsN3N9CFlPTw/NwR6oWOqcqqYpGUCAwEAAQ==\n\
-----END PUBLIC KEY-----\n";

const char ca_private_key[] = 
"-----BEGIN RSA PRIVATE KEY-----\n\
MIIBOgIBAAJBAPR0eI6KaW618dwjFQtNkm9YwUeDOVqbNbh3V7EkdVrB5g4SxY82\n\
budnqC3nsN3N9CFlPTw/NwR6oWOqcqqYpGUCAwEAAQJBAO4j2J3jsLotfSQa+RE9\n\
zH20VPW5nFHsCfVeLYtgHQL/Ig3Ff1GkYuGHBXElFaoMbjml2PRieniSIKxF9RD+\n\
4wECIQD9z32obXTpJDzGnWzJQnBRrwE/PXUYQrVqYmzHueXYcQIhAPaQUcsYaAWy\n\
1QMNp7TpKbsfZ24dmlgh8DoK6v8yoqU1AiAGWfLjDBoo22dJ8RaP0sHMyXxWgMs1\n\
WDYB+4SNWvGNgQIgE7LsFgHZLbtf8WKB555JSz3zEYUj866idsCwjbsJ65ECIBK+\n\
S/IbEYjSBXB4R/Xh7A12WJ0xAi8IEAn9rG/hTnTG\n\
-----END RSA PRIVATE KEY-----\n";

typedef enum {
	KEY_TYPE_PRIVATE,
	KEY_TYPE_PUBLIC,
} KeyType_t;

static RSA *stringToRSAKey(const char *keyStr, KeyType_t type = KEY_TYPE_PUBLIC)
{
	RSA *key = NULL;

	BIO *bp = BIO_new_mem_buf(const_cast<char *>(keyStr), -1);
        
        if (!bp) {
		fprintf(stderr, "Could not allocate BIO\n");
		return NULL;
	}

	if (type == KEY_TYPE_PUBLIC) {
		if (!PEM_read_bio_RSA_PUBKEY(bp, &key, NULL, NULL)) {
			fprintf(stderr, "Could not read public key from PEM string\n");
		}
	} else if (type == KEY_TYPE_PRIVATE) {
		if (!PEM_read_bio_RSAPrivateKey(bp, &key, NULL, NULL)) {
			fprintf(stderr, "Could not read private key from PEM string\n");
		}
	}

	BIO_free(bp);

	return key;
}

#if 0
static const char *RSAPrivKeyToString(RSA *key)
{
	static char buffer[2000];
	
	BIO *bp = BIO_new(BIO_s_mem());

	if (!bp)
		return NULL;
	

	if (!PEM_write_bio_RSAPrivateKey(bp, key, NULL, NULL, 0, NULL, NULL)) {
		BIO_free(bp);
		return NULL;
	}

	int len = BIO_read(bp, buffer, sizeof(buffer));
	
	BIO_free(bp);

	if (len <= 0)
		return NULL;

	buffer[len] = '\0';

	//printf("Key string:\n%s\n", buffer);

	return buffer;
}
#endif // 0

static bool signDataObject(DataObjectRef& dObj, RSA *key)
{
	unsigned char *signature;
	
	if (!key || !dObj) 
		return false;
	
	unsigned int siglen = RSA_size(key);
	
	signature = (unsigned char *)malloc(siglen);
	
	if (!signature)
		return false;
	
	if (RSA_sign(NID_sha1, dObj->getId(), sizeof(DataObjectId_t), signature, &siglen, key) != 1) {
		free(signature);
		return false;
	}

        //NodeId_t id = { 6 };
	
	dObj->setSignature("foo", signature, siglen);
	
	// Do not free the allocated signature as it is now owned by the data object...
	
	return true;
}

static bool verifyDataObject(DataObjectRef& dObj, CertificateRef& cert) 
{
	RSA *key = cert->getPubKey();
	
	// Cannot verify without signature
	if (!dObj->getSignature()) {
		fprintf(stderr, "No signature in data object, cannot verify\n");
		return false;
	}
	
	if (RSA_verify(NID_sha1, dObj->getId(), sizeof(DataObjectId_t), 
		       const_cast<unsigned char *>(dObj->getSignature()), dObj->getSignatureLength(), key) != 1) {
		unsigned char buf[10000];
		dObj->getRawMetadata(buf, sizeof(buf));
		fprintf(stderr, "Signature is invalid:\n%s\n", buf);
		dObj->setSignatureStatus(DataObject::SIGNATURE_INVALID);
		return false;
	}

        printf("Signature is valid\n");
	dObj->setSignatureStatus(DataObject::SIGNATURE_VALID);
	
	return true;
}


#if defined(OS_WINDOWS)
int haggle_test_metadata(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Certificate test: ");
#if 1
	try {
		bool success = true, tmp_succ = true;
		Node::Id_t id = { 8 }; // Just create any node id
                RSA *privKey = NULL;
                RSA *ca_privKey = stringToRSAKey(ca_private_key, KEY_TYPE_PRIVATE);
                RSA *ca_pubKey = stringToRSAKey(ca_public_key, KEY_TYPE_PUBLIC);

                if (!ca_privKey || !ca_pubKey) {
                        fprintf(stderr, "could not create CA keys\n");
                        return 0;
                }

                /* This function must be called to load crypto
                 * algorithms used for signing and verification of
                 * certificates. */
                OpenSSL_add_all_algorithms();
                ERR_load_crypto_strings();
                print_over_test_str(1, "Creating certificate...");

                Certificate *cert = Certificate::create("foo", "issuer", "forever", id, &privKey);

                if (!cert) {
                        print_pass(false);
                        return 0;
                }
                print_pass(true);

                print_over_test_str(1, "Signing certificate...");

                tmp_succ = cert->sign(ca_privKey);                       
                
                success &= tmp_succ;
                print_pass(tmp_succ);

                //printf("\n%s\n", cert->toString().c_str());
                //cert->print();

                print_over_test_str(1, "Verifying certificate...");

                tmp_succ = cert->verifySignature(ca_pubKey);
                
                success &= tmp_succ;
                print_pass(tmp_succ);

                const char *pem = cert->toPEM();
                //cert->writePEM("/tmp/cert.pem");

                
                if (!pem) {
                        fprintf(stderr, "Could not convert certificate to PEM string\n");
                        delete cert;
                        return 0;
                }
                //printf("PEM string:\n%s\n", pem);
                //printf("PEM string:\n%s\n", cert->toPEM());
                
                Certificate *cert2 = Certificate::fromPEM(pem);
                //Certificate *cert2 = Certificate::readPEM("/tmp/cert.pem");
                
                if (!cert2) {
                        fprintf(stderr, "Could not convert PEM string to certificate\n");
                        delete cert;
                        return 0;
                }

                //const char *pem2 = cert2->toPEM();

                if (*cert != *cert2) {
                        printf("Certificate created from PEM is NOT identical to the source certificate!!!\n");
                } else {
                        printf("Certificate created from PEM is identical to the source certificate!\n");
                }
                
                //printf("PEM2\n");
                //cert2->print();
                
                //cert2->sign(ca_privKey);       


                print_over_test_str(1, "Verifying certificate created from PEM string...");

                tmp_succ = cert2->verifySignature(ca_pubKey);
                
                success &= tmp_succ;
                print_pass(tmp_succ);

                //printf("\n%s\n", cert2->toString().c_str());

                Metadata *m = cert2->toMetadata();

                if (!m) {
                        fprintf(stderr, "Could not convert certificate to metadata\n");
                        delete cert;
                        delete cert2;
                        return 0;
                }
                
                CertificateRef cert3 = Certificate::fromMetadata(*m);
                
                delete m;

                if (!cert) {
                        fprintf(stderr, "Could not convert metadata to certificate\n");
                        delete cert;
                        delete cert2;
                        return 0;
                }


                print_over_test_str(1, "Verifying certificate created from metadata...");
                tmp_succ = cert3->verifySignature(ca_pubKey);
                
                success &= tmp_succ;
                print_pass(tmp_succ);

                //printf("\n%s\n", cert3->toString().c_str());

                DataObjectRef dObj = DataObject::create();
                
                dObj->addAttribute("foo", "bar");
                signDataObject(dObj, privKey);

                print_over_test_str(1, "Verifying signature in data object...");
                tmp_succ = verifyDataObject(dObj, cert3);
                
                success &= tmp_succ;
                print_pass(tmp_succ);

                delete cert;
                delete cert2;

		return success ? 0 : 1;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
#else
	return 0;
#endif
}

