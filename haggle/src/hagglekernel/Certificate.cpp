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
#include "Certificate.h"
#include "XMLMetadata.h"
#include "Trace.h"

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/opensslv.h>
#include <openssl/bn.h>
#include <openssl/err.h>

#include <string.h>
#include <base64.h>

// The reason for this function being a macro, is so that HAGGLE_DBG can 
// specify which function called writeErrors().
#if defined(DEBUG)
#define writeErrors(prefix) \
{ \
	unsigned long writeErrors_e; \
	char writeErrors_buf[256] = { 0 }; \
	do{ \
		writeErrors_e = ERR_get_error(); \
		if (writeErrors_e != 0) { \
			HAGGLE_DBG(prefix "%s\n", ERR_error_string(writeErrors_e, writeErrors_buf)); \
		} \
	}  while(writeErrors_e != 0); \
}
#else
#define writeErrors(prefix)
#endif

#define SERIAL_RAND_BITS  128

/* Taken from openssl certmodule.c */
static int certificate_set_serial(X509 *x)
{
        ASN1_INTEGER *sno = ASN1_INTEGER_new();
        BIGNUM *bn = NULL;
        int rv = 0;
        
        bn = BN_new();
        
        if (!bn) {
                ASN1_INTEGER_free(sno);
                return 0;
        }
        
        if (BN_pseudo_rand(bn, SERIAL_RAND_BITS, 0, 0) == 1 &&
            (sno = BN_to_ASN1_INTEGER(bn, sno)) != NULL &&
            X509_set_serialNumber(x, sno) == 1)
                rv = 1;
        
        BN_free(bn);
        ASN1_INTEGER_free(sno);
        
        return rv;
}

Certificate::Certificate(X509 *_x) : 
#ifdef DEBUG_LEAKS
	LeakMonitor(LEAK_TYPE_CERTIFICATE),
#endif
	stored(false), verified(false), hasSignature(true), x(_x), subject(""), issuer(""), 
	validity(""), pubKey(NULL), rsaPubKey(NULL), x509_PEM_str(NULL)
{
	char buf[200];
	
	pubKey = X509_get_pubkey(x);
	rsaPubKey = EVP_PKEY_get1_RSA(pubKey);

	X509_NAME *subject_name = X509_get_subject_name(x);
	
	if (X509_NAME_get_text_by_NID(subject_name, NID_commonName, buf, 200))
		subject = buf;
	
	X509_NAME *issuer_name = X509_get_issuer_name(x);
	
	if (X509_NAME_get_text_by_NID(issuer_name, NID_commonName, buf, 200))
		issuer = buf;
	
        //HAGGLE_DBG("Subject=\'%s\' issuer=\'%s\'\n", subject.c_str(), issuer.c_str());
	// TODO: set validity
}

Certificate::Certificate(const string& _subject, const string& _issuer, const string& _validity, const Node::Id_t _owner, RSA *_rsaPubKey) : 
#ifdef DEBUG_LEAKS
LeakMonitor(LEAK_TYPE_CERTIFICATE),
#endif
	stored(false), verified(false), hasSignature(false), x(NULL), subject(_subject), issuer(_issuer), validity(_validity), pubKey(NULL), rsaPubKey(NULL), x509_PEM_str(NULL)
{
	memcpy(owner, _owner, sizeof(Node::Id_t));
	
	x = X509_new();
	
	if (!x) {
		HAGGLE_ERR("Could not allocate X509 certificate struct\n");
		return;
	}
	
	X509_set_version(x, 2); 
	
	pubKey = EVP_PKEY_new();
	
	if (!pubKey) {
		X509_free(x);
		HAGGLE_ERR("Could not allocate X509 EVP_PKEY\n");
		return;
	}
	
	EVP_PKEY_assign_RSA(pubKey, RSAPublicKey_dup(_rsaPubKey));
	
	X509_set_pubkey(x, pubKey);
	rsaPubKey = EVP_PKEY_get1_RSA(pubKey);

	/* Set validity.
	 FIXME: currently hardcoded
	 */
	int days = 30;
	X509_gmtime_adj(X509_get_notBefore(x),0);
	X509_gmtime_adj(X509_get_notAfter(x),(long)60*60*24*days);

	X509_NAME *subject_name = X509_get_subject_name(x);
	
	/* Set subject */
	//X509_NAME_add_entry_by_txt(subname,"C", MBSTRING_ASC, "SE", -1, -1, 0); 
	X509_NAME_add_entry_by_txt(subject_name, "CN", MBSTRING_ASC, (const unsigned char *)subject.c_str(), -1, -1, 0); 
	X509_NAME_add_entry_by_txt(subject_name, "O", MBSTRING_ASC, (const unsigned char *)"Haggle", -1, -1, 0);  
	
	X509_set_subject_name(x, subject_name); 

	/* Set issuer */
	X509_NAME *issuer_name = X509_get_issuer_name(x);
	
	X509_NAME_add_entry_by_txt(issuer_name, "CN", MBSTRING_ASC, (const unsigned char *)issuer.c_str(), -1, -1, 0); 
	X509_NAME_add_entry_by_txt(issuer_name, "O", MBSTRING_ASC, (const unsigned char *)"Haggle", -1, -1, 0);  
	
	X509_set_issuer_name(x, issuer_name);
        
        //HAGGLE_DBG("Subject=\'%s\' issuer=\'%s\'\n", subject.c_str(), issuer.c_str());

        certificate_set_serial(x);
}

Certificate::~Certificate()
{
	if (rsaPubKey)
		RSA_free(rsaPubKey);

	if (pubKey) {
		EVP_PKEY_free(pubKey);
	}
	
	if (x)
		X509_free(x);
	
	if (x509_PEM_str)
		free(x509_PEM_str);
}

// Should somehow autodetect the OpenSSL capabilities/version. 
// One problem is MacOS X, because the headers say OpenSSL version 0.9.8j, but
// the library is 0.9.7
#if defined(OS_MACOSX)
// RSA_generate_key() is deprecated and removed in the Android OpenSSL version
#define HAVE_RSA_GENERATE_KEY_EX 0
#else
#define HAVE_RSA_GENERATE_KEY_EX 1
#endif

Certificate *Certificate::create(const string subject, const string issuer, const string validity, const Node::Id_t owner, RSA **privKey, size_t keyLength)
{
        Certificate *c = NULL;
	RSA *pubKey, *keyPair;
	
#if HAVE_RSA_GENERATE_KEY_EX
        BIGNUM *e;

        e = BN_new();
        
        if (!e)
                return NULL;

        // The exponent is an odd number, typically 3, 17 or 65537.
        if (BN_set_word(e, 65537) == 0) {
                BN_free(e);
                return NULL;
        }

	keyPair = RSA_new();

        if (!keyPair) {
                BN_free(e);
                return NULL;
        }

	if (RSA_generate_key_ex(keyPair, keyLength, e, NULL) == -1) {
		BN_free(e);
                goto out;
        }
	
        BN_free(e);
#else
	// RSA_generate_key is deprecated, but MacOS X seems to bundle an old version of OpenSSL
	// with only the old function.
	keyPair = RSA_generate_key(keyLength, RSA_F4, NULL, NULL);
	
	if (!keyPair)
		return NULL;
#endif
	*privKey = RSAPrivateKey_dup(keyPair);
	
	if (!*privKey) 
                goto out;
	
	pubKey = RSAPublicKey_dup(keyPair);
	
	if (!pubKey) {
                RSA_free(*privKey);
		*privKey = NULL;
                goto out;
        }
	
        c = new Certificate(subject, issuer, validity, owner, pubKey);
	
	RSA_free(pubKey);
	
        if (!c) {
                RSA_free(*privKey);
        }
out:           
	RSA_free(keyPair);
	
	return c;
}

Certificate *Certificate::create(const string subject, const string issuer, const string validity, const Node::Id_t owner, const string pubKeyFile)
{
	FILE *f = fopen(pubKeyFile.c_str(), "r");

	if (!f) {
		HAGGLE_ERR("Could not open public key file %s\n", pubKeyFile.c_str());
		return NULL;
	}

	RSA *pubKey = PEM_read_RSAPublicKey(f, NULL, NULL, NULL);

	fclose(f);

	if (!pubKey) {
		HAGGLE_ERR("Could not read RSA public key\n");
		return NULL;
	}

	Certificate *c = new Certificate(subject, issuer, validity, owner, pubKey);
	
	RSA_free(pubKey);
        
	return c;
}

bool Certificate::createDigest(unsigned char digest[SHA_DIGEST_LENGTH], const string data) const
{
	SHA_CTX ctx;
        unsigned char *b, *bp;
        int len;
        
        len = i2d_PublicKey(pubKey, NULL);
        
        if (len < 0)
                return false;
        
        bp = b = new unsigned char[len+1];
        
        if (!b)
                return false;

        len = i2d_PublicKey(pubKey, &bp);
  
        if (len < 0) {
                delete [] b;
                return false;
        }

	SHA1_Init(&ctx);

	SHA1_Update(&ctx, data.c_str(), data.length());
	SHA1_Update(&ctx, subject.c_str(), subject.length());
	SHA1_Update(&ctx, issuer.c_str(), issuer.length());;
	SHA1_Update(&ctx, validity.c_str(), validity.length());
	SHA1_Update(&ctx, b, len);

	SHA1_Final(digest, &ctx);

        delete [] b;

	return true;
}

RSA *Certificate::getPubKey()
{
	return rsaPubKey;
}

void Certificate::printPubKey() const
{
	char key_str[5000];
	BIO *bp = BIO_new(BIO_s_mem());
	
	if (!bp)
		return;

	RSA_print(bp, rsaPubKey, 0);
	
	memset(key_str, '\0', sizeof(key_str));
	BIO_read(bp, key_str, sizeof(key_str));
	
	BIO_free(bp);
}

const RSA *Certificate::getPubKey() const
{
	return rsaPubKey;
}

// CBMEN, HL, Begin
EVP_PKEY *Certificate::getEVPPubKey()
{
    return pubKey;
}

const EVP_PKEY *Certificate::getEVPPubKey() const
{
    return pubKey;
}
// CBMEN, HL, End

bool Certificate::isSigned() const
{
        return hasSignature;
}

bool Certificate::isOwner(const Node::Id_t owner) const
{
	if (memcmp(owner, this->owner, sizeof(Node::Id_t)) == 0)
		return true;
	
	return false;
}

bool Certificate::isSubject(const string subject) const
{
	return this->subject == subject;
}

bool Certificate::sign(EVP_PKEY *key)
{
	bool res = false;
	
	if (key && X509_sign(x, key, EVP_sha1())) 
		hasSignature = res = true;
	else {
		writeErrors("");
	}

	return res;
}

bool Certificate::sign(RSA *key)
{
	bool res = false;
	
	if (!key)
		return false;

	EVP_PKEY *pkey = EVP_PKEY_new();
	
	if (!pkey) {
		HAGGLE_ERR("Could not allocate EVP_PKEY\n");
		writeErrors("");
		return false;
	}
	
	EVP_PKEY_set1_RSA(pkey, key);
	
        res = sign(pkey);
	
	EVP_PKEY_free(pkey);
	
	return res;
}

bool Certificate::verifySignature(EVP_PKEY *key)
{
	bool res = false;
	
	if (!key)
		return false;

	if (verified)
		return true;

	// X509 apparently returns 0 or -1 on failure, and 1 on success:
	if (X509_verify(x, key) == 1) {
		verified = res = true;
	} else {
		writeErrors("");		
	}
		
	return res;
}

bool Certificate::verifySignature(RSA *key)
{
	bool res = false;
	
	if (!key)
		return false;

	if (verified)
		return true;
	
	EVP_PKEY *pkey = EVP_PKEY_new();
	
	if (!pkey) {
		HAGGLE_ERR("Could not allocate EVP_PKEY\n");
		writeErrors("");
		return false;
	}
	
	EVP_PKEY_set1_RSA(pkey, key);
	
        res = verifySignature(pkey);
	
	EVP_PKEY_free(pkey);
		
	return res;
}

/**
	Convert certificate to a human readable string.
 */
#define MAX_CERT_STR_SIZE 10000

string Certificate::toString() const
{
        char x509_str[MAX_CERT_STR_SIZE] = { 0 };
	
	if (!x)
		return x509_str;
		
	BIO *bp = BIO_new(BIO_s_mem());
	
	if (!bp)
		return x509_str;
	
	memset(x509_str, '\0', MAX_CERT_STR_SIZE);
		
	if (X509_print(bp, x))
		BIO_read(bp, x509_str, MAX_CERT_STR_SIZE);
	
	BIO_free(bp);
	
	return x509_str;
}

static char *X509ToPEMAlloc(X509 *x)
{
	char *x509_str = NULL;
	size_t plen  = 0;
        int len = 0;

	if (!x)
		return NULL;
	
	BIO *bp = BIO_new(BIO_s_mem());
	
	if (!bp)
		return NULL;
	
	if (!PEM_write_bio_X509_AUX(bp, x))
		goto done;
	
	/* Get the length of the data written */	
	plen = BIO_ctrl_pending(bp);	
	
	if (plen == 0)
		goto done;
	
	/* Allocate enough memory to hold the PEM string */
	x509_str = (char *)malloc(plen + 1);
	
	if (!x509_str)
		goto done;
	
	len = BIO_read(bp, x509_str, plen);
	
	if (len <= 0) {
		free(x509_str);
		x509_str = NULL;
	} else {
                x509_str[len] = '\0';
        }	
done:
	BIO_free(bp);
	
	return x509_str;
}

int Certificate::writePEM(const char *file)
{
        FILE *fp;
        int res = 0;

        if (!file)
                return -1;

        if (!x)
                return -2;
        
        fp = fopen(file, "w");

        if (!fp)
                return -3;

        res = writePEM(fp);

        fclose(fp);

        return res;
}

int Certificate::writePEM(FILE *fp)
{
        int res = 0;

        if (!x)
                return -4;
        
        if (!fp)
                return -5;

        res = PEM_write_X509(fp, x);

        if (res == 0) {
                writeErrors("");
                res = -6;
        } else
                res = 0;

        return res;
}

Certificate *Certificate::readPEM(const char *file)
{
        FILE *fp;
        Certificate *c = NULL;

        if (!file || strlen(file) == 0)
                return NULL;

        fp = fopen(file, "r");

        if (!fp)
                return NULL;
        
        c = readPEM(fp);

        fclose(fp);
        
        return c;
}

Certificate *Certificate::readPEM(FILE *fp)
{
        X509 *x = NULL;
        Certificate *c = NULL;

        if (!fp)
                return NULL;
        
        x = PEM_read_X509_AUX(fp, NULL, 0, NULL);

        if (x) {
		c = new Certificate(x);
        }
        
        return c;
}

const char *Certificate::toPEM() const
{
        if (!x)
                return NULL;

	if (!x509_PEM_str)	
		const_cast<Certificate *>(this)->x509_PEM_str = X509ToPEMAlloc(x);
	
	return x509_PEM_str;
}

Metadata *Certificate::toMetadata() const
{
        Metadata *m;

	if (!x)
		return NULL;
	
        m = new XMLMetadata("Certificate");
        
        if (!m)
                return NULL;

	if (!x509_PEM_str) {
		
		const_cast<Certificate *>(this)->x509_PEM_str = X509ToPEMAlloc(x);
	
		if (!x509_PEM_str) {
			delete m;
			return NULL;
		}
	}
	
	m->setContent(x509_PEM_str);
	
        return m;
}

Certificate *Certificate::fromPEM(const char *pem)
{
	X509 *x = NULL;
	Certificate *c = NULL;
        int ret = 0;

	BIO *bp = BIO_new(BIO_s_mem());
	
	if (!bp)
		return NULL;
	
        ret = BIO_puts(bp, pem);
        
	if (!ret)
		goto done;

        x = PEM_read_bio_X509_AUX(bp, NULL, 0, NULL);
        
	if (x) {
		c = new Certificate(x);
	}
done:
	BIO_free(bp);
	
	return c;
}

Certificate *Certificate::fromPEM(const string& pem)
{
	return fromPEM(pem.c_str());
}

Certificate *Certificate::fromMetadata(const Metadata& m)
{
	if (m.getName() != "Certificate")
		return NULL;
	
        return fromPEM(m.getContent());
}

void Certificate::print(FILE *fp)
{
        X509_print_fp(fp, x);
}

bool operator==(const Certificate& c1, const Certificate& c2)
{
	return (X509_cmp(c1.x, c2.x) == 0 && c1.subject == c2.subject && /* c1.validity == c2.validity && */ c1.issuer == c2.issuer);
}

bool operator!=(const Certificate& c1, const Certificate& c2)
{
	return !(c1 == c2);
}
