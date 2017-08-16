/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
 *   Joshua Joy (JJ, jjoy)
 *   Ashish Gehani (AG)
 *   Hasnain Lakhani (HL)
 */

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

#include "SecurityManager.h"
#include "Certificate.h"
#include "Utility.h"
#include "DataObject.h"
#include "Metadata.h"
#include "XMLMetadata.h"
#include "CharmCryptoBridge.h"

#include <libcpphaggle/Exception.h>
#include <haggleutils.h>
#include <stdio.h>
#include <ctype.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

// SW: need to include these to properly free crypto resources during shutdown
#include <openssl/engine.h>
#include <openssl/conf.h>

// This include is for the HAGGLE_ATTR_CONTROL_NAME attribute name.
// TODO: remove dependency on libhaggle header
#include "../libhaggle/include/libhaggle/ipc.h"

// The reason for this function being a macro, is so that HAGGLE_DBG can
// specify which function called writeErrors().
#if defined(DEBUG)
#define writeErrors(prefix) \
{ \
    unsigned long writeErrors_e; \
    char writeErrors_buf[256]; \
    do { \
        writeErrors_e = ERR_get_error(); \
        if (writeErrors_e != 0) \
            HAGGLE_DBG(prefix "%s\n", \
                ERR_error_string(writeErrors_e, writeErrors_buf)); \
    } while(writeErrors_e != 0); \
}
#else
#define writeErrors(prefix)
#endif

typedef ccb_api::CharmCryptoBridge bridge; // CBMEN, HL

typedef enum {
    KEY_TYPE_PRIVATE,
    KEY_TYPE_PUBLIC,
} KeyType_t;

static RSA *stringToRSAKey(const char *keyStr, KeyType_t type = KEY_TYPE_PUBLIC)
{
    RSA *key = NULL;

    //HAGGLE_DBG("trying to convert:\n%s\n", keyStr);

    BIO *bp = BIO_new_mem_buf(const_cast<char *>(keyStr), -1);

        if (!bp) {
        HAGGLE_ERR("Could not allocate BIO\n");
        return NULL;
    }

    if (type == KEY_TYPE_PUBLIC) {
        if (!PEM_read_bio_RSA_PUBKEY(bp, &key, NULL, NULL)) {
            HAGGLE_ERR("Could not read public key from PEM string\n");
        }
    } else if (type == KEY_TYPE_PRIVATE) {
        if (!PEM_read_bio_RSAPrivateKey(bp, &key, NULL, NULL)) {
            HAGGLE_ERR("Could not read private key from PEM string\n");
        }
    }

    BIO_free(bp);

    return key;
}

static const char *RSAKeyToString(RSA *key, KeyType_t type = KEY_TYPE_PUBLIC) // CBMEN, HL
{
    static char buffer[2000];

    BIO *bp = BIO_new(BIO_s_mem());

    if (!bp)
        return NULL;

    // CBMEN, HL, Begin
    if (type == KEY_TYPE_PRIVATE) {
        if (!PEM_write_bio_RSAPrivateKey(bp, key, NULL, NULL, 0, NULL, NULL)) {
            BIO_free(bp);
            return NULL;
        }
   } else {
        if (!PEM_write_bio_RSA_PUBKEY(bp, key)) {
            BIO_free(bp);
            return NULL;
        }
   }
   // CBMEN, HL, End

    int len = BIO_read(bp, buffer, sizeof(buffer));

    BIO_free(bp);

    if (len <= 0)
        return NULL;

    buffer[len] = '\0';

    //HAGGLE_DBG("Key string:\n%s\n", buffer);

    return buffer;
}

#if 0
static bool generateKeyPair(int num, unsigned long e, RSA **keyPair)
{
    if (!keyPair)
        return false;

    *keyPair = RSA_generate_key(num, e, NULL, NULL);

    if (*keyPair == NULL)
        return false;
    else
        return true;
}
#endif

// CBMEN, HL, Begin
List<string> getAttributes(string policy) {
    string delimiters = "( )";
    List<string> tokens;
    List<string> result;
    size_t prev = 0;
    size_t next = 0;
    size_t i = 0;

    for ( ; next < policy.length(); next++) {
        if (delimiters.find(policy[next]) != string::npos) {
            tokens.push_back(policy.substr(prev, next-prev));
            prev = next+1;
        }
    }

    if (prev < policy.length()) {
        tokens.push_back(policy.substr(prev));
    }

    for (List<string>::iterator it = tokens.begin(); it != tokens.end(); it++) {
        string token = *it;
        bool valid = true;
        prev = 0;

        for (i = 0; i < token.length(); i++) {

            if (token[i] == '.') {
                if (prev != 0) {
                    valid = false;
                    break;
                }
                prev = i;
            }
            else if (!(isalnum(token[i]) || token[i] == '.')) {
                valid = false;
                break;
            } else {
                token[i] = toupper(token[i]);
            }
        }

        if (prev == 0 || !valid)
            continue;

        result.push_back(token);
    }

    return result;
}

Pair<string, string> getAuthorityAndAttribute(string str) {
    size_t delimiter = 0;
    bool valid = true;

    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '.') {
            if (delimiter != 0) {
                valid = false;
                break;
            }
            delimiter = i;
        } else if (!(isalnum(str[i]) || str[i] == '.')) {
            valid = false;
            break;
        } else {
            str[i] = toupper(str[i]);
        }
    }

    if (delimiter == 0 || !valid)
        return make_pair("", "");

    return make_pair(str.substr(0, delimiter), str.substr(delimiter+1));
}

bool hashMapToMetadata(Metadata *m, string tag, string key, HashMap<string, string>& map) {

    for (HashMap<string, string>::iterator it = map.begin(); it != map.end(); it++) {

        Metadata *dm = new XMLMetadata(tag, (*it).second);
        dm->setParameter(key, (*it).first);

        if (!m->addMetadata(dm))
            return false;
    }

    return true;
}

HashMap<string, string> metadataToHashMap(Metadata *m, string tag, string key) {

    HashMap<string, string> result;

    Metadata *dm = m->getMetadata(tag);
    while (dm) {
        result.insert(make_pair(dm->getParameter(key), dm->getContent()));
        dm = m->getNextMetadata();
    }

    return result;
}
// CBMEN, HL, End

SecurityTask::SecurityTask(const SecurityTaskType_t _type,
               DataObjectRef _dObj,
               CertificateRef _cert,
               NodeRefList *_targets, // CBMEN, AG
               unsigned char *_AESKey) : // CBMEN, HL
    type(_type), completed(false), dObj(_dObj),
    privKey(NULL), cert(_cert),
    targets(_targets ? _targets->copy() : NULL), // CBMEN, AG
    AESKey(_AESKey) // CBMEN, HL
{
}

 SecurityTask::~SecurityTask()
 {
   if(targets) delete targets;
 }

SecurityHelper::SecurityHelper(SecurityManager *m,
                   const EventType _etype) :
    ManagerModule<SecurityManager>(m, "SecurityHelper"),
    taskQ("SecurityHelper"), etype(_etype)
{
}

SecurityHelper::~SecurityHelper()
{
    if (isRunning())
        stop();

    while (!taskQ.empty()) {
        SecurityTask *task = NULL;
        taskQ.retrieve(&task);
        delete task;
    }

}

// CBMEN, HL, Begin
bool SecurityHelper::deriveSecretKeys(string secret)
{
    size_t AES_KEY_LENGTH = 16;
    size_t HMAC_KEY_LENGTH = 16;
    size_t TMP_KEY_LENGTH = 16;
    unsigned char salt[SHA_DIGEST_LENGTH];
    unsigned char tmp[TMP_KEY_LENGTH];
    unsigned char derived[AES_KEY_LENGTH + HMAC_KEY_LENGTH];
    unsigned char *AESKey = NULL;
    unsigned char *HMACKey = NULL;

    AESKey = (unsigned char *) malloc(AES_KEY_LENGTH * sizeof(unsigned char));
    if (!AESKey) {
        HAGGLE_ERR("Couldn't allocate AESKey!\n");
        return false;
    }

    HMACKey = (unsigned char *) malloc(HMAC_KEY_LENGTH * sizeof(unsigned char));
    if (!HMACKey) {
        HAGGLE_ERR("Couldn't allocate HMACKey!\n");
        goto err_aes;
    }

    SHA1((unsigned char *) secret.c_str(), secret.length(), salt);
    if (PKCS5_PBKDF2_HMAC_SHA1(secret.c_str(), secret.length(), salt, sizeof(salt), PBKDF2_ITERATIONS, TMP_KEY_LENGTH, tmp) != 1) {
        HAGGLE_ERR("Error deriving tmp key from secret!\n");
        goto err_hmac;
    }

    if (PKCS5_PBKDF2_HMAC_SHA1((char *) tmp, sizeof(tmp), salt, sizeof(salt), 1, sizeof(derived), derived) != 1) {
        HAGGLE_ERR("Error deriving derived key from tmp key!\n");
        goto err_hmac;
    }

    memcpy(AESKey, derived, AES_KEY_LENGTH);
    memcpy(HMACKey, derived + AES_KEY_LENGTH, HMAC_KEY_LENGTH);

    getManager()->derivedSharedSecretMap.insert(make_pair(secret, make_pair(AESKey, HMACKey)));
    return true;

    err_hmac:
        free(HMACKey);
    err_aes:
        free(AESKey);
        return false;
}

bool SecurityHelper::encryptString(unsigned char *plaintext, size_t ptlen,
    unsigned char **ciphertext, size_t *ctlen,
    string secret) {

    unsigned char iv[AES_BLOCK_SIZE];
    unsigned char ecount[AES_BLOCK_SIZE];
    unsigned int num;
    string result = "";
    AES_KEY key;
    unsigned char *AESKey = NULL;
    unsigned char *HMACKey = NULL;
    HashMap<string, Pair<unsigned char *, unsigned char *> >::iterator it;

    // Lookup keys
    it = getManager()->derivedSharedSecretMap.find(secret);
    if (it == getManager()->derivedSharedSecretMap.end()) {
        if (!deriveSecretKeys(secret)) {
            HAGGLE_ERR("Error deriving shared secrets!\n");
            return false;
        }
        it = getManager()->derivedSharedSecretMap.find(secret);
    }
    AESKey = (*it).second.first;
    HMACKey = (*it).second.second;

    // Init crypto state
    num = 0;
    memset(ecount, 0, AES_BLOCK_SIZE);
    RAND_bytes(iv, 8);
    memset((void *)(iv + 8), 0, 8);

    if (AES_set_encrypt_key(AESKey, 128, &key)) {
        HAGGLE_ERR("AES_set_encrypt_key failed.\n");
        goto cleanup;
    }

    *ctlen = ptlen + AES_BLOCK_SIZE + SHA_DIGEST_LENGTH;
    *ciphertext = (unsigned char *)malloc(sizeof(unsigned char) * *ctlen);
    if (!*ciphertext) {
        HAGGLE_ERR("Couldn't allocate memory.\n");
        goto cleanup;
    }

    memcpy((void *)(*ciphertext + SHA_DIGEST_LENGTH), iv, AES_BLOCK_SIZE);
    AES_ctr128_encrypt(plaintext, (*ciphertext + SHA_DIGEST_LENGTH + AES_BLOCK_SIZE), ptlen, &key, iv, ecount, &num);

    if (!HMAC(EVP_sha1(), HMACKey, 16, *ciphertext + SHA_DIGEST_LENGTH, ptlen + AES_BLOCK_SIZE, *ciphertext, NULL)) {
        HAGGLE_ERR("Error creating HMAC.\n");
        goto err_ciphertext;
    }

    memset(&key, 0, sizeof(key));
    return true;

err_ciphertext:
    free(*ciphertext);
cleanup:
    memset(&key, 0, sizeof(key));
    return false;
}

bool SecurityHelper::encryptStringBase64(string plaintext,
    string secret,
    string &ciphertext) {

    unsigned char *ct = NULL;
    size_t ctlen;
    char *b64 = NULL;

    if (!encryptString((unsigned char *) plaintext.c_str(), plaintext.length(), &ct, &ctlen, secret))
        return false;

    if (base64_encode_alloc((char *) ct, ctlen, &b64) > 0) {
        ciphertext = string(b64);
        free(b64);
        free(ct);
        return true;
    } else {
        HAGGLE_ERR("Error encoding base64!\n");
        return false;
    }
    return false;
}

bool SecurityHelper::decryptString(unsigned char *ciphertext, size_t ctlen,
    unsigned char **plaintext, size_t *ptlen,
    string secret) {

    unsigned char iv[AES_BLOCK_SIZE];
    unsigned char ecount[AES_BLOCK_SIZE];
    unsigned char hash[SHA_DIGEST_LENGTH];
    unsigned int num;
    string result = "";
    AES_KEY key;
    unsigned char *AESKey = NULL;
    unsigned char *HMACKey = NULL;
    HashMap<string, Pair<unsigned char *, unsigned char *> >::iterator it;

    // Lookup keys
    it = getManager()->derivedSharedSecretMap.find(secret);
    if (it == getManager()->derivedSharedSecretMap.end()) {
        if (!deriveSecretKeys(secret)) {
            HAGGLE_ERR("Error deriving shared secrets!\n");
            return false;
        }
        it = getManager()->derivedSharedSecretMap.find(secret);
    }
    AESKey = (*it).second.first;
    HMACKey = (*it).second.second;

    if (AES_set_encrypt_key(AESKey, 128, &key)) {
        HAGGLE_ERR("AES_set_encrypt_key failed.\n");
        goto cleanup;
    }

    *ptlen = ctlen - AES_BLOCK_SIZE - SHA_DIGEST_LENGTH;
    *plaintext = (unsigned char *)malloc(*ptlen);
    if (!*plaintext) {
        HAGGLE_ERR("Couldn't allocate memory.\n");
        goto cleanup;
    }

    // Init crypto state
    num = 0;
    memset(ecount, 0, AES_BLOCK_SIZE);
    memcpy(iv, (void *)(ciphertext + SHA_DIGEST_LENGTH), AES_BLOCK_SIZE);

    AES_ctr128_encrypt(ciphertext + SHA_DIGEST_LENGTH + AES_BLOCK_SIZE, *plaintext, *ptlen, &key, iv, ecount, &num);

    if (!HMAC(EVP_sha1(), HMACKey, 16, ciphertext + SHA_DIGEST_LENGTH, *ptlen + AES_BLOCK_SIZE, hash, NULL)) {
        HAGGLE_ERR("Error computing HMAC\n");
        goto err_plaintext;
    }

    if (memcmp(hash, ciphertext, SHA_DIGEST_LENGTH) != 0) {
        HAGGLE_DBG("HMAC doesn't match.\n"); // change from ERR to DBG as we may expect failures.
        goto err_plaintext;
    }

    memset(&key, 0, sizeof(key));
    return true;

err_plaintext:
    free(*plaintext);
cleanup:
    memset(&key, 0, sizeof(key));
    return false;
}

bool SecurityHelper::decryptStringBase64(string ciphertext,
    string secret,
    string &plaintext) {

    unsigned char *ct = NULL;
    size_t ctlen;
    unsigned char *pt = NULL;
    size_t ptlen;
    char *terminated = NULL;

    base64_decode_context ctx;
    base64_decode_ctx_init(&ctx);

    if (base64_decode_alloc(&ctx, ciphertext.c_str(), ciphertext.length(), (char **) &ct, &ctlen)) {
        if (!decryptString(ct, ctlen, &pt, &ptlen, secret)) {
            if (ct) {
                free(ct);
            }
            return false;
        }

        terminated = (char *) malloc((ptlen + 1) * sizeof(char));

        if (!terminated) {
            HAGGLE_ERR("Error allocating memory!\n");
            free(ct);
            free(pt);
            return false;
        }

        memcpy(terminated, pt, ptlen);
        terminated[ptlen] = '\0';

        // We need to null terminate, and the string() constructor taking a length is private
        plaintext = string((char *)terminated);
        free(ct);
        free(pt);
        free(terminated);

        return true;
    } else {
        HAGGLE_ERR("Error decoding base64!\n");
        return false;
    }
}
// CBMEN, HL, End

// CBMEN, AG, Begin

bool SecurityHelper::startPython()
{
    Mutex::AutoLocker l(getManager()->ccbMutex);

    if (getManager()->pythonRunning)
        return true;

// CBMEN, HL, Begin
#ifdef OS_ANDROID

    // Check whether Python has been unzipped
    if (access(PYTHON_UNZIP_COMPLETE_FILE, F_OK) == -1) {
        HAGGLE_DBG("Python unzip not complete yet, checking whether it's in progress.\n");

        if (access(PYTHON_UNZIP_IN_PROGRESS_FILE, F_OK) == -1) {
            HAGGLE_ERR("Python unzip not complete and not in progress, something is wrong. Please delete /data/data/org.haggle.kernel/python or reinstall Haggle. Can not start Python without risking segfault!\n");
            return false;
        } else {
            bool done = false;
            for (size_t i = 0; i < PYTHON_UNZIP_WAIT_TIME; i++) {
                Watch w;
                w.waitTimeout(1000);

                if (access(PYTHON_UNZIP_COMPLETE_FILE, F_OK) == 0) {
                    HAGGLE_DBG("Python unzip now complete, can start.\n");
                    done = true;
                    break;
                }
            }
            if (!done) {
                HAGGLE_ERR("Waited too long for unzip to complete, can not start Python without risking segfault!.\n");
                return false;
            }
        }
    } else {
        HAGGLE_DBG("Python has been unzipped, can start.\n");
    }

#endif
// CBMEN, HL, End

    if(getManager()->securityConfigured) {

        // CBMEN, HL, Begin

#ifdef OS_ANDROID
#if PY_MAJOR_VERSION >= 3
setenv("EXTERNAL_STORAGE", "/mnt/sdcard/com.googlecode.python3forandroid", true);
setenv("PY34A", "/data/data/com.googlecode.python3forandroid/files/python3", true);
setenv("PY4A_EXTRAS", "/mnt/sdcard/com.googlecode.python3forandroid/extras", true);
setenv("PYTHONPATH", "/mnt/sdcard/com.googlecode.python3forandroid/extras/python3:/data/data/com.googlecode.python3forandroid/files/python3/lib/python3.2/lib-dynload:/mnt/sdcard/com.googlecode.python3forandroid/extras/python3/site-packages", true);
setenv("TEMP", "/mnt/sdcard/com.googlecode.python3forandroid/extras/python3/tmp", true);
setenv("HOME", "/sdcard", true);
setenv("PYTHON_EGG_CACHE", "/mnt/sdcard/com.googlecode.python3forandroid/extras/python3/tmp", true);
setenv("PYTHONHOME", "/data/data/com.googlecode.python3forandroid/files/python3", true);
setenv("LD_LIBRARY_PATH", (getenv("LD_LIBRARY_PATH") + string(":/data/data/com.googlecode.python3forandroid/files/python3/lib")).c_str(), true);
setenv("LD_RUN_PATH", (getenv("LD_LIBRARY_PATH") + string(":/data/data/com.googlecode.python3forandroid/files/python3/lib")).c_str(), true);
#else
#define PY4A_BASE "/data/data/org.haggle.kernel/python"
setenv("PY34A", PY4A_BASE, true);
setenv("PYTHONPATH", PY4A_BASE "lib/python2.7:" PY4A_BASE "lib/python2.7/lib-dynload:" PY4A_BASE "lib/python2.7/site-packages", true);
setenv("TEMP", PY4A_BASE "lib/python2.7/tmp", true);
setenv("HOME", "/sdcard", true);
setenv("PYTHON_EGG_CACHE", PY4A_BASE "lib/python2.7/tmp", true);
setenv("PYTHONHOME", PY4A_BASE, true);
LOG("Set all paths\n");
#endif
#endif

        if (bridge::startPython()) {
            HAGGLE_ERR("Error starting Python.\n");
            HAGGLE_ERR("\tCharm persistence data\t: %s\n", getManager()->charmPersistenceData.c_str());

            getManager()->pythonRunning = false;
            return false;
        } else {
            HAGGLE_DBG("Started Python.\n");

            if (bridge::init(getManager()->charmPersistenceData)) {
                HAGGLE_ERR("Error calling init.\n");
                HAGGLE_ERR("\tCharm persistence data\t: %s\n", getManager()->charmPersistenceData.c_str());
                return false;
            }

            getManager()->pythonRunning = true;
            bridge::set_gid(getManager()->kernel->getThisNode()->getIdStr());

            // CBMEN, HL - we may have received attributes in config that we need to give to charm
            for (HashMap<string, string>::iterator it = getManager()->pubKeysFromConfig.begin(); it != getManager()->pubKeysFromConfig.end(); it++) {
                HashMap<string, string>::iterator mit = getManager()->pubKeys.find((*it).first);
                if (mit == getManager()->pubKeys.end()) {
                    if (!bridge::addAuthorityPK((*it).first, (*it).second)) {
                        HAGGLE_DBG("Added pubKeyFromConfig %s\n", (*it).first.c_str());
                        getManager()->pubKeys.insert(make_pair((*it).first, ""));
                    }
                }
            }

            for (HashMap<string, string>::iterator it = getManager()->privKeysFromConfig.begin(); it != getManager()->privKeysFromConfig.end(); it++) {
                HashMap<string, string>::iterator mit = getManager()->privKeys.find((*it).first);
                if (mit == getManager()->privKeys.end()) {
                    if (!bridge::addUserSK((*it).first, (*it).second)) {
                        HAGGLE_DBG("Added privKeyFromConfig %s\n", (*it).first.c_str());
                        getManager()->privKeys.insert(make_pair((*it).first, ""));
                    }
                }
            }

            decryptAttributeBuckets(); // CBMEN, HL - We may have new roles in config/control message
            return true;
        }

    // CBMEN, HL, End
    }

    return false;
}

// CBMEN, AG, End

bool SecurityHelper::signDataObject(DataObjectRef& dObj, RSA *key)
{
    unsigned char *signature;

    if (!dObj) {
      HAGGLE_ERR("data object to sign is NULL\n");
      return false;
    }

    if(!key) {
      HAGGLE_ERR("cannot sign data object [%s] - private key missing\n", dObj->getIdStr());
      return false;
    }

    unsigned int siglen = RSA_size(key);

    signature = (unsigned char *)malloc(siglen);

    if (!signature)
        return false;

    HAGGLE_DBG("signing data object [%s], siglen=%u\n", dObj->getIdStr(), siglen);

    memset(signature, 0, siglen);

    // CBMEN, HL, MOS, Begin
    DataHash_t toSign;
    memcpy(toSign, dObj->getId(), sizeof(DataHash_t));
    // CBMEN, HL, MOS, End

    if (RSA_sign(NID_sha1, toSign, sizeof(DataHash_t), signature, &siglen, key) != 1) { // CBMEN, HL
        free(signature);
        return false;
    }

    dObj->setSignature(getManager()->getKernel()->getThisNode()->getIdStr(), signature, siglen);

    // Assume that our own signature is valid
    dObj->setSignatureStatus(DataObject::SIGNATURE_VALID);

    // Do not free the allocated signature as it is now owned by the data object...

    if (getManager()->signatureChaining)    // CBMEN, HL, AG
        return updateSignatureChain(dObj, key);

    return true;
}

bool SecurityHelper::verifyDataObject(DataObjectRef& dObj, CertificateRef& cert) const
{
    RSA *key;

    // Cannot verify without signature
    if (!dObj->getSignature()) {
        HAGGLE_ERR("No signature in data object, cannot verify\n");
        return false;
    }
    writeErrors("(not this): ");

    key = cert->getPubKey();

    // CBMEN, HL, MOS, Begin
    DataHash_t toVerify;
    memcpy(toVerify, dObj->getId(), sizeof(DataHash_t));
    // CBMEN, HL, MOS, End

    if (RSA_verify(NID_sha1, toVerify, sizeof(DataHash_t),  // CBMEN, HL
               const_cast<unsigned char *>(dObj->getSignature()), dObj->getSignatureLength(), key) != 1) {
        char *raw;
        size_t len;
        writeErrors("");
        dObj->getRawMetadataAlloc((unsigned char **)&raw, &len);
        if (raw) {
            HAGGLE_DBG("Signature is invalid:\n%s\n", raw);
            free(raw);
        }
        dObj->setSignatureStatus(DataObject::SIGNATURE_INVALID);

        return false;
    }

    HAGGLE_DBG("Signature is valid\n");
    dObj->setSignatureStatus(DataObject::SIGNATURE_VALID);

    return true;
}

// CBMEN, AG, Begin

string getPolicy(string rawPolicy) {
    string delimiter = "_";
    string replacement = " ";
    size_t prev = 0;
    size_t next = 0;
    string policy = "";

    for ( ; next < rawPolicy.length(); next++) {
        if (delimiter.find(rawPolicy[next]) != string::npos) {
            policy += rawPolicy.substr(prev, next-prev);
            policy += replacement;
            prev = next+1;
        }
    }
    policy += rawPolicy.substr(prev, rawPolicy.length() - prev);
    HAGGLE_DBG("Raw policy received: %s\t Policy that will be used: %s\n", rawPolicy.c_str(), policy.c_str());
    return policy;
}

// CBMEN, AG, End

// CBME, HL, AG, Begin
bool SecurityHelper::updateSignatureChain(DataObjectRef& dObj, RSA *key) {
    string idStr = getManager()->kernel->getThisNode()->getIdStr();
    int hop = 0;
    int hops = 0;
    Metadata *m, *sig, *lastsig;
    char *base64_signature = NULL;
    const char *param = NULL;
    unsigned char *signature = NULL;
    unsigned int siglen = RSA_size(key);
    DataHash_t toSign;
    bool toFree = true;

    signature = (unsigned char *) malloc(siglen);
    if (!signature)
        return false;
    memset(signature, 0, siglen);

    dObj.lock();
    m = dObj->getMetadata()->getMetadata("SignatureChain");
    if (!m) {
        m = dObj->getMetadata()->addMetadata("SignatureChain");
        if (!m) {
            HAGGLE_ERR("Couldn't allocate metadata.\n");
            free(signature);
            dObj.unlock();
            return false;
        }
    }

    param = m->getParameter("hops");
    if (param) {
        char *endptr = NULL;
        int tmp;
        tmp = strtoul(param, &endptr, 10);

        if (endptr && endptr != param) {
            hops = tmp;
        }
    }

    lastsig = NULL;
    sig = m->getMetadata("Signature");
    while (sig) {
        hop++;
        lastsig = sig;
        sig = m->getNextMetadata();
    }

    if (hop != hops) {
        HAGGLE_ERR("Number of signatures in the chain doesn't match for Data Object [%s]\n", dObj->getIdStr());
        free(signature);
        dObj.unlock();
        return false;
    }

    if (hop == 0) {
        free(signature);
        signature = (unsigned char *)dObj->getSignature();
        siglen = dObj->getSignatureLength();
        toFree = false;
        HAGGLE_DBG("Adding initial chain signature for Data Object [%s]\n", dObj->getIdStr());
    } else if (hop == 1 && idStr == lastsig->getParameter("signee")) {
        // Update our previous signature
        free(signature);
        signature = (unsigned char *)dObj->getSignature();
        siglen = dObj->getSignatureLength();
        toFree = false;
        m->removeMetadata("Signature");
        hops--;
        hop--;
        HAGGLE_DBG("Updating previous chain signature for Data Object [%s]\n", dObj->getIdStr());
    } else {
        SHA1((const unsigned char *)lastsig->getContent().c_str(), lastsig->getContent().length(), toSign);
        if (RSA_sign(NID_sha1, toSign, sizeof(DataHash_t), signature, &siglen, key) != 1) {
            free(signature);
            dObj.unlock();
            return false;
        }
    }

    if (base64_encode_alloc((char *)signature, siglen, &base64_signature) > 0) {
        sig = m->addMetadata("Signature", base64_signature);
        sig->setParameter("hop", hop);
        sig->setParameter("signee", idStr.c_str());
        m->setParameter("hops", hops+1);
        free(base64_signature);
    } else {
        HAGGLE_ERR("Error allocating base 64 signature\n");
        if (toFree)
            free(signature);
        dObj.unlock();
        return false;
    }

    if (toFree)
        free(signature);

    dObj.unlock();
    return true;
}

// CBMEN, HL, AG, End

// CBMEN, HL, Begin
bool SecurityHelper::generateCapability(DataObjectRef& dObj)
{
    Mutex::AutoLocker l(getManager()->ccbMutex);
    unsigned char *key = NULL;
    char *b64_key = NULL;
    size_t keysize = 16 * sizeof(unsigned char);
    Metadata *m = NULL;

    string rawPolicy = dObj->getAttribute(DATAOBJECT_ATTRIBUTE_ABE_POLICY)->getValue(); // CBMEN, AG
    string policy = getPolicy(rawPolicy); // CBMEN, AG
    string capability;
    Pair<string, unsigned char *> cacheVal;
    cacheVal.second = NULL;

    HashMap<string, Pair<string, unsigned char *> >::iterator it = getManager()->policyCache.find(policy);

    if (it != getManager()->policyCache.end())
        cacheVal = (*it).second;

    if (cacheVal.second != NULL) {
        capability = cacheVal.first;
        key = cacheVal.second;

        HAGGLE_DBG("Using cached encryption key and capability for Data Object [%s].\n", dObj->getIdStr());
    } else {
        // Do we have all the pubkeys?
        HAGGLE_DBG("Checking attributes for policy %s\n", policy.c_str());
        List<string> attributes = getAttributes(policy);
        List<string> notfound;
        for (List<string>::iterator it = attributes.begin(); it != attributes.end(); it++) {
            HAGGLE_DBG("Checking for attribute %s\n", (*it).c_str());
            if (getManager()->pubKeys.find(*it) == getManager()->pubKeys.end()) {
                HAGGLE_DBG("Attribute %s not found!\n", (*it).c_str());
                notfound.push_back(*it);
            }
        }

        if (notfound.size() > 0) {
            HAGGLE_DBG("Missing some encryption attributes, can not generate capability for Data Object [%s].\n", dObj->getIdStr()); // CBMEN, AG
            NodeRefList targets;
        if(getManager()->tooManyOutstandingSecurityDataRequests()) return false; // MOS // CBMEN, AG, HL
            requestSpecificKeys(notfound, dObj, targets.copy(), true);
            return false;
        }

        key = (unsigned char *) malloc(keysize);
        if (!key)
            return false;
        RAND_bytes((unsigned char *) key, keysize);
        HAGGLE_DBG("Key generated, sending capability generation request to charm for Data Object [%s].\n", dObj->getIdStr());

        if (base64_encode_alloc((char *)key, keysize, &b64_key) <= 0) {
            HAGGLE_ERR("Couldn't encode key to send it to Charm for Data Object [%s].\n", dObj->getIdStr());
            return false;
        }

        if (!b64_key) {
            HAGGLE_ERR("Couldn't allocate memory.\n");
            return false;
        }

        if (bridge::encrypt(string(b64_key), policy, capability)) {
            HAGGLE_ERR("Error retrieving capability from Charm for Data Object [%s].\n", dObj->getIdStr());
            free(b64_key);
            return false;
        }
        HAGGLE_DBG("Successfully retrieved capability from Charm for Data Object [%s].\n", dObj->getIdStr());
        free(b64_key);

        getManager()->policyCache.insert(make_pair(policy, make_pair(capability, key)));
    }

    dObj.lock(); // MOS

    m = dObj->getMetadata();
    if (!m) {
        HAGGLE_ERR("Couldn't get metadata to add capability.\n");
    dObj.unlock();
        return false;
    }

    m = m->getMetadata(DATAOBJECT_METADATA_ABE);
    if (!m) {
        m = new XMLMetadata(DATAOBJECT_METADATA_ABE);
        if (!m) {
            HAGGLE_ERR("Couldn't allocate ABE metadata.\n");
        dObj.unlock();
            return false;
        }
    }

    m->addMetadata(DATAOBJECT_METADATA_ABE_CAPABILITY, capability);
    dObj->getMetadata()->addMetadata(m);

    dObj.unlock();

    HAGGLE_DBG("Enqueuing encryption task for Data Object [%s].\n", dObj->getIdStr());
    addTask(new SecurityTask(SECURITY_TASK_ENCRYPT_DATAOBJECT, dObj, NULL, NULL, key));

    return true;
}

bool SecurityHelper::useCapability(DataObjectRef& dObj, NodeRefList *targets)
{
    Mutex::AutoLocker l(getManager()->ccbMutex);
    unsigned char *key = NULL;
    size_t keylen = 0;
    string capability, tmp;
    const Metadata *m = NULL;
    FILE *fin;
    size_t len;
    unsigned char *raw = NULL;
    char *b64_capability = NULL;
    string inputFilePath = dObj->getEncryptedFilePath();

    HashMap<string, string>::iterator cit = getManager()->dataObjectCapabilityCache.find(dObj->getIdStr());
    if (cit != getManager()->dataObjectCapabilityCache.end()) {
        capability = (*cit).second;
        HAGGLE_DBG("Read cached capability for Data Object [%s]\n", dObj->getIdStr());
        HAGGLE_DBG("Capability is %s\n", capability.c_str());
    } else {
        HAGGLE_DBG("Reading capability from file for Data Object [%s], going to read from \"%s\".\n",
                    dObj->getIdStr(), inputFilePath.c_str());
        fin = fopen(inputFilePath.c_str(), "rb");
        if (!fin) {
            HAGGLE_ERR("Failed to open input file.\n");
            return false;
        }

        if (!fread(&len, sizeof(len), 1, fin)) {
            HAGGLE_ERR("Error reading capability length.\n");
            fclose(fin);
            return false;
        }

        HAGGLE_DBG("Capability length is %u\n", len);
        raw = (unsigned char *) malloc(len * sizeof(unsigned char));
        if (!raw) {
            HAGGLE_ERR("Failed to allocate buffer for capability\n");
            fclose(fin);
            return false;
        }

        if (!fread(raw, len, 1, fin)) {
            HAGGLE_ERR("Error reading capability from file.\n");
            fclose(fin);
            free(raw);
            return false;
        }

        if (base64_encode_alloc((char *)raw, len, &b64_capability) <= 0) {
            HAGGLE_ERR("Error base64 encoding capability.\n");
            fclose(fin);
            free(raw);
            return false;
        }

        capability = string((char *) b64_capability);
        HAGGLE_DBG("Capability is %s\n", capability.c_str());

        fclose(fin);
        free(raw);
        free(b64_capability);

        getManager()->dataObjectCapabilityCache.insert(make_pair(dObj->getIdStr(), capability));
    }

    HashMap<string, unsigned char *>::iterator it = getManager()->capabilityCache.find(capability);
    if (it != getManager()->capabilityCache.end())
        key = (*it).second;

    if (key == NULL) {
        base64_decode_context ctx;
        base64_decode_ctx_init(&ctx);
        HAGGLE_DBG("Sending capability using request to charm for Data Object [%s].\n", dObj->getIdStr());

        if (bridge::decrypt(capability, tmp)) {
            HAGGLE_DBG("Charm could not decrypt capability for Data Object [%s].\n", dObj->getIdStr());

            // Do we have all privkeys?
            string rawPolicy = dObj->getAttribute(DATAOBJECT_ATTRIBUTE_ABE_POLICY)->getValue(); // CBMEN, AG
            string policy = getPolicy(rawPolicy); // CBMEN, AG
            List<string> attributes = getAttributes(policy);
            List<string> notfound;
            for (List<string>::iterator it = attributes.begin(); it != attributes.end(); it++) {
                HAGGLE_DBG("Checking for Attribute %s\n", (*it).c_str());
                if (getManager()->privKeys.find(*it) == getManager()->privKeys.end()) {
                    HAGGLE_DBG("Attribute %s not found!\n", (*it).c_str());
                    notfound.push_back(*it);
                }
            }

            if (notfound.size() > 0) {
                HAGGLE_DBG("Missing some decryption attributes, can not decrypt capability for Data Object [%s].\n", dObj->getIdStr());
        if(getManager()->tooManyOutstandingSecurityDataRequests()) return false; // MOS // CBMEN, AG, HL
                requestSpecificKeys(notfound, dObj, targets->copy(), false);
            }

            return false;
        }

        if (!base64_decode_alloc(&ctx, tmp.c_str(), tmp.length(), (char **)&key, &keylen)) {
            HAGGLE_ERR("Couldn't decode capability.\n");
            return false;
        }

        if (!key) {
            HAGGLE_ERR("Couldn't allocate memory.\n");
            return false;
        }

        if (keylen != 16) {
            HAGGLE_ERR("Key is of wrong length.\n");
            free(key);
            return false;
        }

        getManager()->capabilityCache.insert(make_pair(capability, key));
    } else {
        HAGGLE_DBG("Using cached decryption key for DataObject [%s].\n", dObj->getIdStr());
    }

    HashMap<string, string>::iterator it2 = (getManager()->pendingDecryption).find(dObj->getIdStr());
    if (it2 != (getManager()->pendingDecryption).end()) {
      HAGGLE_DBG("Data object [%s] is already pending decryption.\n", dObj->getIdStr());
    } else {
      (getManager()->pendingDecryption).insert(make_pair(dObj->getIdStr(), ""));
      HAGGLE_DBG("Enqueueing decryption task for Data Object [%s].\n", dObj->getIdStr());
      addTask(new SecurityTask(SECURITY_TASK_DECRYPT_DATAOBJECT, dObj, NULL, targets, key));
    } // CBMEN, AG, End

    return true;
}

bool SecurityHelper::encryptDataObject(DataObjectRef& dObj, unsigned char *AESKey)
{
    unsigned char iv[AES_BLOCK_SIZE];
    unsigned int num;
    unsigned char ecount[AES_BLOCK_SIZE];
    AES_KEY key;
    FILE *fin, *fout;
    unsigned char *in, *out;
    unsigned int readLen, writeLen, totalWritten;
    bool goOn = true;
    unsigned char *hash = dObj->getEncryptedFileHash();
    unsigned char *raw = NULL;
    size_t len = 0;
    base64_decode_context bctx;
    SHA_CTX ctx;
    Metadata *m, *abem;

    string inputFilePath = dObj->getFilePath();
    string outputFilePath = dObj->getEncryptedFilePath();
    if (outputFilePath.length() == 0) {
        outputFilePath = dObj->createEncryptedFilePath();
    }
    HAGGLE_DBG("Encrypting Data Object [%s], going to read unencrypted data from \"%s\" and write encrypted data to \"%s\".\n",
                dObj->getIdStr(), inputFilePath.c_str(), outputFilePath.c_str());

    // Init crypto variables
    num = 0;
    memset(ecount, 0, AES_BLOCK_SIZE);
    RAND_bytes((unsigned char *)iv, 8);
    memset((void *)(iv + 8), 0, 8);
    SHA1_Init(&ctx);

    if (AES_set_encrypt_key(AESKey, 128, &key)) {
        HAGGLE_ERR("AES_set_encrypt_key failed.\n");
        return false;
    }

    fin = fopen(inputFilePath.c_str(), "rb");
    if (!fin) {
        HAGGLE_ERR("Failed to open input file.\n");
        goto err_fin;
    }
    fout = fopen(outputFilePath.c_str(), "wb");
    if (!fout)
    {
        HAGGLE_ERR("Failed to open output file.\n");
        goto err_fout;
    }

    in = (unsigned char *) malloc(4096);
    if (!in) {
        HAGGLE_ERR("Failed to allocate input buffer.\n");
        goto err_in;
    }
    out = (unsigned char *) malloc(4096);
    if (!out) {
        HAGGLE_ERR("Failed to allocate output buffer.\n");
        goto err_out;
    }

    m = dObj->getMetadata();
    if (!m) {
        HAGGLE_ERR("Data Object [%s] has no metadata, can't read ABE Metadata.\n", dObj->getIdStr());
        goto err_m;
    }

    abem = m->getMetadata(DATAOBJECT_METADATA_ABE);
    if (!abem) {
        HAGGLE_ERR("Data Object [%s] has no ABE metadata!\n", dObj->getIdStr());
        goto err_m;
    }

    abem = abem->getMetadata(DATAOBJECT_METADATA_ABE_CAPABILITY);
    if (!abem) {
        HAGGLE_ERR("Data Object [%s] has no ABE capability metadata!\n", dObj->getIdStr());
        goto err_m;
    }

    base64_decode_ctx_init(&bctx);
    if (!base64_decode_alloc(&bctx, abem->getContent().c_str(), abem->getContent().length(), (char **) &raw, &len)) {
        HAGGLE_ERR("Error base64 decoding capability!\n");
        goto err_m;
    }

    if (!fwrite(&len, sizeof(len), 1, fout)) {
        HAGGLE_ERR("Couldn't write capability length to file.\n");
        goto err_capability;
    }
    SHA1_Update(&ctx, &len, sizeof(len));
    totalWritten = sizeof(len);
    HAGGLE_DBG("Wrote capability length %u to file.\n", len);

    if (!fwrite(raw, len, 1, fout)) {
        HAGGLE_ERR("Couldn't write capability to file.\n");
        goto err_capability;
    }
    SHA1_Update(&ctx, raw, len);
    totalWritten += len;
    HAGGLE_DBG("Wrote capability to file: %s\n", abem->getContent().c_str());

    // Write the IV
    if (!fwrite(iv, AES_BLOCK_SIZE, 1, fout))
    {
        HAGGLE_ERR("Couldn't write IV to file.\n");
        goto err_iv;
    }
    SHA1_Update(&ctx, iv, 16);
    totalWritten += AES_BLOCK_SIZE;

    do {
        readLen = fread(in, 1, 4096, fin);

        AES_ctr128_encrypt(in, out, readLen, &key, iv, ecount, &num);
        SHA1_Update(&ctx, out, readLen);

        writeLen = fwrite(out, 1, readLen, fout);
        totalWritten += writeLen;

        if (feof(fin) || ferror(fin) || ferror(fout)) {
            goOn = false;
        }
    } while (goOn);

    SHA1_Final(hash, &ctx);

    HAGGLE_DBG("Encryption complete, wrote %u bytes for Data Object [%s].\n", totalWritten, dObj->getIdStr());

    dObj->setEncryptedData(hash,totalWritten); // MOS

    dObj.lock(); // MOS

    if (!dObj->addAttribute(DATAOBJECT_ATTRIBUTE_ABE, "true")) {
        HAGGLE_ERR("Couldn't add ABE attribute to DataObject [%s].\n", dObj->getIdStr());
        goto err_attr;
    }

    m->removeMetadata(DATAOBJECT_METADATA_ABE);
    dObj->setABEStatus(DataObject::ABE_DECRYPTION_DONE); // Since we also have the plaintext

    if(dObj->getSignature() != NULL) {
        HAGGLE_ERR("Newly encrypted data object [%s] is already signed.\n", dObj->getIdStr());
        goto err_attr;
    }

    // We have to sign it again to tie it to the hash
    // of the ciphertext
    if(getManager()->securityLevel >= SECURITY_LEVEL_HIGH) // MOS - skip signing if turned off
      signDataObject(dObj, getManager()->privKey);

    memset(&key, 0, sizeof(key));
    dObj.unlock();
    free(raw);
    free(out);
    free(in);
    fclose(fin);
    fclose(fout);
    return true;

err_attr:
    dObj.unlock();

err_iv:
err_capability:
    free(raw);

err_m:
    free(out);

err_out:
    free(in);

err_in:
    fclose(fout);

err_fout:
    fclose(fin);
    memset(&key, 0, sizeof(key));

err_fin:
    return false;
}

bool SecurityHelper::decryptDataObject(DataObjectRef& dObj, unsigned char *AESKey)
{
    unsigned char iv[AES_BLOCK_SIZE];
    unsigned int num;
    unsigned char ecount[AES_BLOCK_SIZE];
    AES_KEY key;
    FILE *fin, *fout;
    unsigned char *in, *out;
    unsigned int readLen, writeLen, totalWritten;
    bool goOn = true;
    DataHash_t hash;
    SHA_CTX ctx;
    size_t len;
    unsigned char *raw = NULL;

    string inputFilePath = dObj->getEncryptedFilePath();
    string outputFilePath = dObj->getFilePath();
    if (outputFilePath.length() == 0) {
        dObj->createFilePath();
        outputFilePath = dObj->getFilePath();
    }
    HAGGLE_DBG("Decrypting Data Object [%s], going to read encrypted data from \"%s\" and write unencrypted data to \"%s\".\n",
                dObj->getIdStr(), inputFilePath.c_str(), outputFilePath.c_str());

    // Init crypto variables
    num = 0;
    memset(ecount, 0, AES_BLOCK_SIZE);
    SHA1_Init(&ctx);

    if (AES_set_encrypt_key(AESKey, 128, &key)) {
        HAGGLE_ERR("AES_set_encrypt_key failed.\n");
        return false;
    }

    fin = fopen(inputFilePath.c_str(), "rb");
    if (!fin) {
        HAGGLE_ERR("Failed to open input file.\n");
        goto err_fin;
    }
    fout = fopen(outputFilePath.c_str(), "wb");
    if (!fout)
    {
        HAGGLE_ERR("Failed to open output file.\n");
        goto err_fout;
    }

    in = (unsigned char *) malloc(4096);
    if (!in) {
        HAGGLE_ERR("Failed to allocate input buffer.\n");
        goto err_in;
    }
    out = (unsigned char *) malloc(4096);
    if (!out) {
        HAGGLE_ERR("Failed to allocate output buffer.\n");
        goto err_out;
    }

    // Read past the capability
    if (!fread(&len, sizeof(len), 1, fin)) {
        HAGGLE_ERR("Error reading capability length.\n");
        goto err_out;
    }

    raw = (unsigned char *) malloc(len * sizeof(unsigned char));
    if (!raw) {
        HAGGLE_ERR("Failed to allocate buffer for capability\n");
        goto err_out;
    }

    if (!fread(raw, len, 1, fin)) {
        HAGGLE_ERR("Error reading capability from file.\n");
        goto err_raw;
    }

    // Read the IV
    if (!fread(iv, AES_BLOCK_SIZE, 1, fin))
    {
        HAGGLE_ERR("Couldn't read IV from file.\n");
        goto err_iv;
    }

    totalWritten = 0;
    do {
        readLen = fread(in, 1, 4096, fin);

        AES_ctr128_encrypt(in, out, readLen, &key, iv, ecount, &num);
        SHA1_Update(&ctx, out, readLen);

        writeLen = fwrite(out, 1, readLen, fout);
        totalWritten += writeLen;

        if (feof(fin) || ferror(fin) || ferror(fout)) {
            goOn = false;
        }
    } while (goOn);

    SHA1_Final(hash, &ctx);

    dObj->setData(hash, totalWritten); // MOS

    HAGGLE_DBG("Decryption complete, wrote %u bytes for Data Object [%s].\n", totalWritten, dObj->getIdStr());

    dObj->setABEStatus(DataObject::ABE_DECRYPTION_DONE);

    memset(&key, 0, sizeof(key));
    free(raw);
    free(out);
    free(in);
    fclose(fin);
    fclose(fout);
    return true;

err_iv:
    memset(&key, 0, sizeof(key));
    free(raw);

err_raw:
    free(out);

err_out:
    free(in);

err_in:
    fclose(fout);

err_fout:
    fclose(fin);

err_fin:

    return false;
}

void SecurityHelper::addAdditionalAttributes(DataObjectRef& dObj, List<Attribute>& attributes) {
    for (List<Attribute>::iterator it = attributes.begin(); it != attributes.end(); it++) {
        Attribute attr = *it;
        Attribute add = attr;

        if (attr.getValue() == "%%replace_current_time%%") {
            char tmp[30];
            snprintf(tmp, 30, "%llu", (unsigned long long)time(NULL));
            add = Attribute(attr.getName(), string(tmp), attr.getWeight());
        } else if (attr.getValue() == "%%replace_current_node_name%%") {
            add = Attribute(attr.getName(), getManager()->kernel->getThisNode()->getName(), attr.getWeight());
        }

        HAGGLE_DBG("Adding additionalAttribute %s\n", add.getString().c_str());
        dObj->addAttribute(add);
    }
}

void SecurityHelper::sendSecurityDataRequest(DataObjectRef& dObj) {

    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    Mutex::AutoLocker l2(getManager()->outstandingRequestsMutex); // CBMEN, AG

    Metadata *m, *dm;
    string type;
    unsigned char *plaintext, *ciphertext;
    size_t ptlen, ctlen;
    unsigned long num;
    string target;
    NodeRefList neighList;
    NodeRef responseNode = NULL;
    DataObjectRef dObjRequest;
    char filepath[60] = "";
    FILE *fp;
    int fd = -1;
    string secret;
    HashMap<string, string>::iterator it; // CBMEN, HL, AG

    if (!dObj->getAttribute("SecurityDataRequest"))
        goto error;

    if (dObj->getAttribute("target"))
        target = dObj->getAttribute("target")->getValue();

     // CBMEN, HL, AG, Begin

    it = getManager()->rawNodeSharedSecretMap.find(target);
    if (it == getManager()->rawNodeSharedSecretMap.end()) {
        HAGGLE_ERR("Couldn't find shared secret for node %s\n", target.c_str());
        goto error;
    }

    secret = (*it).second;

    // CBMEN, HL, AG, End

    m = new XMLMetadata("security");
    if (!m)
        goto error;

    dm = m->addMetadata("subject", getManager()->kernel->getThisNode()->getIdStr());
    if (!dm)
        goto err_m;

    type = dObj->getAttribute("SecurityDataRequest")->getValue();
    if (type == "certificate_signature") {
        dm = m->addMetadata("pubkey", RSAKeyToString(getManager()->pubKey, KEY_TYPE_PUBLIC));
        if (!dm)
            goto err_m;
    } else if (type == "pubkey_specific" || type == "pubkey_all") {
        dm = dObj->getMetadata()->getMetadata("PublicKeys")->copy();
        if (!dm)
            goto err_m;
        if (!m->addMetadata(dm))
            goto err_m;
        dm = dObj->getMetadata()->getMetadata("roles")->copy();
        if (!dm)
            goto err_m;
        if (!m->addMetadata(dm))
            goto err_m;
    } else if (type == "privkey_specific" || type == "privkey_all") {
        dm = dObj->getMetadata()->getMetadata("PrivateKeys")->copy();
        if (!dm)
            goto err_m;
        if (!m->addMetadata(dm))
            goto err_m;
        dm = dObj->getMetadata()->getMetadata("roles")->copy();
        if (!dm)
            goto err_m;
        if (!m->addMetadata(dm))
            goto err_m;
    } else if (type == "composite") {
        dm = m->addMetadata("pubkey", RSAKeyToString(getManager()->pubKey, KEY_TYPE_PUBLIC));
        if (!dm)
            goto err_m;
        dm = dObj->getMetadata()->getMetadata("PublicKeys")->copy();
        if (!dm)
            goto err_m;
        if (!m->addMetadata(dm))
            goto err_m;
        dm = dObj->getMetadata()->getMetadata("PrivateKeys")->copy();
        if (!dm)
            goto err_m;
        if (!m->addMetadata(dm))
            goto err_m;
        dm = dObj->getMetadata()->getMetadata("roles")->copy();
        if (!dm)
            goto err_m;
        if (!m->addMetadata(dm))
            goto err_m;
    }

    if (!m->getRawAlloc(&plaintext, &ptlen))
        goto err_m;

    if (!encryptString(plaintext, ptlen, &ciphertext, &ctlen, secret)) // CBMEN, HL, AG
        goto err_plaintext;

    strncpy(filepath, getManager()->tempFilePath.c_str(), sizeof(filepath));
    if ((fd = mkstemp(filepath)) == -1 ||
        (fp = fdopen(fd, "wb+")) == NULL) {
        if (fd != -1) {
            unlink(filepath);
            close(fd);
        }
        HAGGLE_ERR("Couldn't create temporary file.\n");
        goto err_ciphertext;
    }

    fwrite(ciphertext, 1, ctlen, fp);
    if (ferror(fp)) {
        HAGGLE_ERR("Couldn't write out to temporary file\n");
        goto err_fp;
    }
    fclose(fp);

    dObjRequest = DataObject::create(filepath, "");
    if (!dObjRequest) {
        HAGGLE_ERR("Error creating data object.\n");
        goto err_fp;
    }
    dObjRequest->addAttribute("SecurityDataRequest", type);
    dObjRequest->addAttribute("SecurityDataRequestAuthority", target);
    dObjRequest->addAttribute("SecurityDataRequestSubject", getManager()->kernel->getThisNode()->getIdStr());
    dObjRequest->setPersistent(true);
    addAdditionalAttributes(dObjRequest, getManager()->additionalSecurityDataRequestAttributes);

    if (getManager()->isAuthority && target == getManager()->kernel->getThisNode()->getIdStr()) {
        HAGGLE_DBG("SecurityDataRequest is for self, issuing SECURITY_TASK_HANDLE_SECURITY_DATA_REQUEST\n");
        addTask(new SecurityTask(SECURITY_TASK_HANDLE_SECURITY_DATA_REQUEST, dObjRequest));
    } else {
        if (target != "")
            responseNode = getManager()->kernel->getNodeStore()->retrieve(target);

        if (responseNode) {
            HAGGLE_DBG("Forwarding SecurityDataRequest to node %s.\n", target.c_str());
            getManager()->kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_FORWARD, dObjRequest, responseNode));
            // MOS - don't use forward
            // HAGGLE_DBG("Sending SecurityDataRequest to node %s.\n", target.c_str());
        // getManager()->kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObjRequest, responseNode));
        getManager()->outstandingSecurityDataRequests += 1;
        HAGGLE_DBG2("Outstanding security data requests: %d\n", getManager()->outstandingSecurityDataRequests);
        } else {
            // MOS - limit to defined neighbors to avoid unnecessary sig verification failures
            num = getManager()->kernel->getNodeStore()->retrieveDefinedNeighbors(neighList);
            if (num > 0) {
                HAGGLE_DBG("Sending SecurityDataRequest to neighbours.\n");
                getManager()->kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObjRequest, neighList));
            getManager()->outstandingSecurityDataRequests += neighList.size();
                HAGGLE_DBG2("Outstanding security data requests: %d\n", getManager()->outstandingSecurityDataRequests);
            } else {
                HAGGLE_DBG("No neighbours, not sending SecurityDataRequest!\n");
            }
        }
    }

    free(ciphertext);
    free(plaintext);
    delete m;
    dObj = NULL;

    return;

err_fp:
    fclose(fp);
err_ciphertext:
    free(ciphertext);
err_plaintext:
    free(plaintext);
err_m:
    delete m;
error:
    dObj = NULL;
    HAGGLE_DBG("Error while trying to send security data request.\n");
}

void SecurityHelper::handleSecurityDataRequest(DataObjectRef& dObj) {

    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    string type = dObj->getAttribute("SecurityDataRequest")->getValue();
    Metadata *m, *dm, *response;
    string subject;
    CertificateRef cert;
    DataObjectRef dObjResponse;
    unsigned char *plaintext, *ciphertext, *buf;
    size_t len, ptlen, ctlen;
    NodeRefList neighList;
    NodeRef responseNode;
    FILE *fin, *fout;
    unsigned char *inbuf;
    char filepath[60] = "";
    int fd = -1;
    bool success = false;
    string secret;
    string securityDataRequestSubject;
    HashMap<string, string>::iterator it;

    if (dObj->getAttribute("SecurityDataRequestSubject"))
        securityDataRequestSubject = dObj->getAttribute("SecurityDataRequestSubject")->getValue();

    // CBMEN, HL - Do we need this for composite requests?
    if (type == "privkey_specific" || type == "privkey_all") {

    if(getManager()->securityLevel >= SECURITY_LEVEL_HIGH &&
        securityDataRequestSubject != getManager()->kernel->getThisNode()->getIdStr()) { // CBMEN, AG - Skip verification if turned off

      CertificateRef cert = getManager()->retrieveCertificate(dObj->getSignee(), getManager()->certStore);

      if (cert) {
            if (!cert->isVerified()) {
                HAGGLE_ERR("Can't verify SecurityDataRequest signature without verified certificate\n");
                return;
            }
            if (!verifyDataObject(dObj, cert)) {
                HAGGLE_ERR("Signature verification failed for SecurityDataRequest\n");
                return;
            }
      } else {
            HAGGLE_ERR("Don't have certificate to verify SecurityDataRequest.\n");
            return;
      }
    }
    }

    fin = fopen(dObj->getFilePath().c_str(), "rb");
    if (!fin) {
        HAGGLE_ERR("Failed to open input file.\n");
        return;
    }

    inbuf = (unsigned char *)malloc(sizeof(unsigned char) * dObj->getDataLen());
    if (!inbuf) {
        HAGGLE_ERR("Couldn't allocate memory.\n");
        goto err_fin;
    }

    if (fread(inbuf, 1, dObj->getDataLen(), fin) < dObj->getDataLen() || ferror(fin)) {
        HAGGLE_ERR("Couldn't read from file.\n");
        goto err_inbuf;
    }

    it = getManager()->rawNodeSharedSecretMap.find(securityDataRequestSubject);
    if (it == getManager()->rawNodeSharedSecretMap.end()) {
        HAGGLE_ERR("Couldn't find shared secret for node %s\n", subject.c_str());
        goto err_inbuf;
    }

    secret = (*it).second;
    if (!decryptString(inbuf, dObj->getDataLen(), &plaintext, &ptlen, secret)) {
        HAGGLE_DBG("Decryption failed, ignoring request \n");
        goto err_inbuf;
    }

    {
    char *pt = (char*)malloc(ptlen+1);
    memcpy(pt,plaintext,ptlen); pt[ptlen] = 0;
    HAGGLE_DBG("Received Request: \n%s\n", pt);
    free(pt);
    }

    m = new XMLMetadata;
    if (!m)
        goto err_plaintext;

    response = new XMLMetadata("security");
    if (!response)
        goto err_plaintext;

    if (!m->initFromRaw(plaintext, ptlen))
        goto err_response;

    dm = m->getMetadata("subject");
    if (!dm)
        goto err_response;

    subject = dm->getContent();
    if (subject != securityDataRequestSubject) {
        HAGGLE_ERR("Subject inside encrypted Security Data Request does not match subject outside; dropping request.\n"); // CBMEN, AG
        goto err_response;
    }

    dm = response->addMetadata("subject", subject);
    if (!dm)
        goto err_response;

    dm = response->addMetadata("type", type);
    if (!dm)
        goto err_response;

    dm = response->addMetadata("authorityName", getManager()->authorityName);
    if (!dm)
        goto err_response;

    dm = response->addMetadata("authorityID", getManager()->kernel->getThisNode()->getIdStr());
    if (!dm)
        goto err_response;

    if (type == "certificate_signature") {
        success = signCertificate(subject, m, response);
    } else {
        bool succ = true;
        if (!getManager()->pythonRunning) {
            if (!startPython()) {
                HAGGLE_ERR("Need Python to handle security data requests involving ABE keys.\n");
                goto err_response;
            }
        }
        if (type == "pubkey_specific") {
            succ &= sendSpecificPublicKeys(subject, m, response);
        }
        if (type == "privkey_specific") {
            succ &= sendSpecificPrivateKeys(subject, m, response);
        }
        if (type == "pubkey_all" || type == "composite") {
            succ &= sendAllPublicKeys(subject, m, response);
        }
        if (type == "privkey_all" || type == "composite") {
            succ &= sendAllPrivateKeys(subject, m, response);
        }
        if (type == "composite") {
            succ &= signCertificate(subject, m, response);
        }
        success = succ;
    }

    if (!success)
        goto err_response;

    if (!response->getRawAlloc(&buf, &len))
        goto err_response;

    if (!encryptString(buf, len, &ciphertext, &ctlen, secret))
        goto err_buf;

    strncpy(filepath, getManager()->tempFilePath.c_str(), sizeof(filepath));
    if ((fd = mkstemp(filepath)) == -1 ||
        (fout = fdopen(fd, "wb+")) == NULL) {
        if (fd != -1) {
            unlink(filepath);
            close(fd);
        }
        HAGGLE_ERR("Couldn't create temporary file.\n");
        goto err_ciphertext;
    }

    fwrite(ciphertext, 1, ctlen, fout);
    if (ferror(fout)) {
        HAGGLE_ERR("Couldn't write out to temporary file\n");
        goto err_fout;
    }
    fclose(fout);

    dObjResponse = DataObject::create(filepath, "");
    dObjResponse->setPersistent(true);
    if (!dObjResponse) {
        HAGGLE_ERR("Error creating data object.\n");
        goto err_fout;
    }
    dObjResponse->addAttribute("SecurityDataResponse", subject);
    dObjResponse->addAttribute("authorityID", getManager()->kernel->getThisNode()->getIdStr());
    addAdditionalAttributes(dObjResponse, getManager()->additionalSecurityDataResponseAttributes);

    if (getManager()->isAuthority && subject == getManager()->kernel->getThisNode()->getIdStr()) {
        HAGGLE_DBG("SecurityDataResponse is for self, issuing SECURITY_TASK_HANDLE_SECURITY_DATA_RESPONSE\n");
        addTask(new SecurityTask(SECURITY_TASK_HANDLE_SECURITY_DATA_RESPONSE, dObjResponse));
    } else {

        // Forward directly if we can
        responseNode = getManager()->kernel->getNodeStore()->retrieve(subject);
        if (responseNode) {
            HAGGLE_DBG("Forwarding SecurityDataResponse to node %s.\n", subject.c_str());
            getManager()->kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_FORWARD, dObjResponse, responseNode));
                // HAGGLE_DBG("Sending SecurityDataResponse to node %s.\n", subject.c_str());
                // getManager()->kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObjResponse, responseNode));

                HashMap<string, DataObjectRef>::iterator it = getManager()->securityDataResponseQueue.find(subject);
                while (it != getManager()->securityDataResponseQueue.end()) {
                    getManager()->securityDataResponseQueue.erase(it);
                    it = getManager()->securityDataResponseQueue.find(subject);
                }
        } else {
            // We can't, so broadcast it
          /*
            num = getManager()->kernel->getNodeStore()->retrieveNeighbors(neighList);
            if (num > 0) {
                HAGGLE_DBG("Sending SecurityDataResponse to neighbours.\n");
                getManager()->kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObjResponse, neighList));
            } else {
                HAGGLE_DBG("No neighbours, not sending SecurityDataResponse!\n");
            }
          */
            HAGGLE_DBG("Target %s for SecurityDataResponse not in node store, inserting response into queue!\n", subject.c_str());

            HashMap<string, DataObjectRef>::iterator it = getManager()->securityDataResponseQueue.find(subject);
            while (it != getManager()->securityDataResponseQueue.end()) {
                getManager()->securityDataResponseQueue.erase(it);
                it = getManager()->securityDataResponseQueue.find(subject);
            }

            getManager()->securityDataResponseQueue.insert(make_pair(subject, dObjResponse));

            // Try sending it to the node we got it from
            InterfaceRef iface = dObj->getRemoteInterface();
            NodeRef actualNbr = getManager()->getKernel()->getNodeStore()->retrieve(iface, true);

            if (actualNbr && actualNbr->getType() == Node::TYPE_PEER) {
                HAGGLE_DBG("Sending SecurityDataResponse to node %s for further forwarding.\n", actualNbr->getIdStr());
                getManager()->kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, dObjResponse, actualNbr));
            }
        }
    }

    fclose(fin);
    free(inbuf);
    free(buf);
    free(plaintext);
    free(ciphertext);
    delete response;
    delete m;
    dObj->setStored(false);
    dObj->deleteData();

    return;

err_fout:
    fclose(fout);
err_ciphertext:
    free(ciphertext);
err_buf:
    free(buf);
err_response:
    delete response;
    delete m;
err_plaintext:
    free(plaintext);
err_inbuf:
    free(inbuf);
err_fin:
    fclose(fin);
    dObj->setStored(false);
    dObj->deleteData();
    HAGGLE_DBG("Error while trying to handle security data request.\n");
}

void SecurityHelper::handleSecurityDataResponse(DataObjectRef& dObj) {
    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    string type;
    Metadata *m, *dm;
    string subject, authName, authID;
    CertificateRef cert, issuer_cert;
    HashMap<string, string>::iterator authMapIt;
    unsigned char *plaintext;
    size_t ptlen;
    FILE *fin;
    unsigned char *inbuf;

    // CBMEN, HL, AG, Begin

    string authorityID;
    string secret;
    HashMap<string, string>::iterator it;

    if (dObj->getAttribute("authorityID"))
        authorityID = dObj->getAttribute("authorityID")->getValue();

    if(authorityID.length() != (MAX_NODE_ID_STR_LEN - 1)) {
        HAGGLE_ERR("Invalid authority ID length!\n");
        return;
    }

    if(getManager()->securityLevel >= SECURITY_LEVEL_HIGH) { // CBMEN, AG - Only check if signee will be available
        if(authorityID != dObj->getSignee() &&
           (getManager()->isAuthority && authorityID != getManager()->kernel->getThisNode()->getIdStr())) {
            HAGGLE_ERR("Invalid authority ID inside response, dropping!\n");
            return;
        }
    }

    it = getManager()->rawNodeSharedSecretMap.find(authorityID);
    if (it == getManager()->rawNodeSharedSecretMap.end()) {
        HAGGLE_ERR("Couldn't find shared secret for node %s\n", subject.c_str());
        return;
    }

    secret = (*it).second;

    // CBMEN, HL, AG, End

    fin = fopen(dObj->getFilePath().c_str(), "rb");
    if (!fin) {
        HAGGLE_ERR("Failed to open input file.\n");
        return;
    }

    inbuf = (unsigned char *)malloc(sizeof(unsigned char) * dObj->getDataLen());
    if (!inbuf) {
        HAGGLE_ERR("Couldn't allocate memory.\n");
        goto err_fin;
    }

    if (fread(inbuf, 1, dObj->getDataLen(), fin) < dObj->getDataLen() || ferror(fin)) {
        HAGGLE_ERR("Couldn't read from file.\n");
        goto err_fin;
    }

    if (!decryptString(inbuf, dObj->getDataLen(), &plaintext, &ptlen, secret)) { // CBMEN, HL, AG
        HAGGLE_DBG("Decryption failed, ignoring response \n");
        goto err_inbuf;
    }

    {
    char *pt = (char*)malloc(ptlen+1);
    memcpy(pt,plaintext,ptlen); pt[ptlen] = 0;
    HAGGLE_DBG("Received Response: \n%s\n", (char *)pt);
    free(pt);
    }

    m = new XMLMetadata;
    if (!m)
        goto err_plaintext;

    if (!m->initFromRaw(plaintext, ptlen))
        goto err_m;

    dm = m->getMetadata("subject");
    if (!dm)
        goto err_m;

    subject = dm->getContent();
    if (subject != getManager()->kernel->getThisNode()->getIdStr())
        goto err_m;

    dm = m->getMetadata("type");
    if (!dm)
        goto err_m;

    type = dm->getContent();

    dm = m->getMetadata("authorityID");
    if (!dm)
        goto err_m;

    authID = dm->getContent();

    // CBMEN, HL, AG, Begin

    if (authID != authorityID) {
        HAGGLE_ERR("authorityID inside encrypted payload does not match authorityID outside payload, dropping response.\n");
        goto err_m;
    }

    // CBMEN, HL, AG, End

    dm = m->getMetadata("authorityName");
    if (!dm)
        goto err_m;

    authName = dm->getContent();

    // CBMEN, HL - Do we need this for composite requests?
    if (type == "privkey_specific" || type == "pubkey_specific"
        || type == "privkey_all" || type == "pubkey_all") {

    if(getManager()->securityLevel >= SECURITY_LEVEL_HIGH &&
        subject != getManager()->kernel->getThisNode()->getIdStr()) { // CBMEN, AG - Skip verification if turned off

      CertificateRef cert = getManager()->retrieveCertificate(authID, getManager()->caCerts);

      if (cert) {
            if (!cert->isVerified()) {
                HAGGLE_ERR("Can't verify SecurityDataResponse signature without verified certificate\n");
                return;
            }
            if (!verifyDataObject(dObj, cert)) {
                HAGGLE_ERR("Signature verification failed for SecurityDataResponse.\n");
                goto err_m;
            }
      } else {
            HAGGLE_ERR("Don't have certificate to verify SecurityDataResponse.\n");
            goto err_m;
      }
    }
    }

    if (type == "certificate_signature") {

        dm = m->getMetadata("certificate");
        if (!dm)
            goto err_m;

        cert = Certificate::fromPEM(dm->getContent());
        if (!cert || cert->getSubject() != subject || cert->getIssuer() != authID)
            goto err_m;

        getManager()->storeCertificate(authID, cert, getManager()->myCerts, true);
        HAGGLE_DBG("Saved signed certificate issued by %s\n", authID.c_str());

        dm = m->getMetadata("issuer_certificate");
        if (!dm)
            goto err_m;

        issuer_cert = Certificate::fromPEM(dm->getContent());
        if (!issuer_cert || issuer_cert->getSubject() != authID || issuer_cert->getIssuer() != authID)
            goto err_m;

        getManager()->storeCertificate(authID, issuer_cert, getManager()->caCerts, true);
        HAGGLE_DBG("Saved CA certificate for %s\n", authID.c_str());

        // Now that we have this certificate, verify any certificates that we can
        addTask(new SecurityTask(SECURITY_TASK_VERIFY_AUTHORITY_CERTIFICATE, NULL, issuer_cert));

        getManager()->kernel->getThisNode()->setNodeDescriptionCreateTime();
        getManager()->kernel->getThisNode()->setNodeDescriptionUpdateTime();
        getManager()->kernel->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));
        requestAllKeys(authName, authID, true);
        requestAllKeys(authName, authID, false);
    } else if (type == "pubkey_specific" || type == "privkey_specific"
                || type == "pubkey_all" || type == "privkey_all") {

        if (!getManager()->pythonRunning) {
            if (!startPython()) {
                HAGGLE_ERR("Need Python to handle security data responses involving ABE keys.\n");
                goto err_m;
            }
        }

        bool isPublic = false;

        if (type[1] == 'u')
            isPublic = true;

        if (!saveReceivedKeys(m, isPublic)){
            HAGGLE_DBG("Response of size zero may have been received.\n");
            goto err_m;
        }

        if (!updateWaitingQueues(isPublic))
            goto err_m;
    } else if (type == "composite") {
        dm = m->getMetadata("certificate");
        if (!dm)
            goto err_m;

        cert = Certificate::fromPEM(dm->getContent());
        if (!cert || cert->getSubject() != subject || cert->getIssuer() != authID)
            goto err_m;

        getManager()->storeCertificate(authID, cert, getManager()->myCerts, true);
        HAGGLE_DBG("Saved signed certificate issued by %s\n", authID.c_str());

        dm = m->getMetadata("issuer_certificate");
        if (!dm)
            goto err_m;

        issuer_cert = Certificate::fromPEM(dm->getContent());
        if (!issuer_cert || issuer_cert->getSubject() != authID || issuer_cert->getIssuer() != authID)
            goto err_m;

        getManager()->storeCertificate(authID, issuer_cert, getManager()->caCerts, true);
        HAGGLE_DBG("Saved CA certificate for %s\n", authID.c_str());

        // Now that we have this certificate, verify any certificates that we can
        addTask(new SecurityTask(SECURITY_TASK_VERIFY_AUTHORITY_CERTIFICATE, NULL, issuer_cert));

        getManager()->kernel->getThisNode()->setNodeDescriptionCreateTime();
        getManager()->kernel->getThisNode()->setNodeDescriptionUpdateTime();
        getManager()->kernel->addEvent(new Event(EVENT_TYPE_NODE_DESCRIPTION_SEND));

        if (!getManager()->pythonRunning) {
            if (!startPython()) {
                HAGGLE_ERR("Need Python to handle security data responses involving ABE keys.\n");
                goto err_m;
            }
        }

        bool err = false;
        if (!saveReceivedKeys(m, true)){
            HAGGLE_DBG("Response of size zero may have been received.\n");
            err = true;
        }

        if (!updateWaitingQueues(true))
            err = true;

        if (!saveReceivedKeys(m, false)){
            HAGGLE_DBG("Response of size zero may have been received.\n");
            err = true;
        }

        if (!updateWaitingQueues(false))
            err = true;

        if (err)
            goto err_m;
    }

    delete m;
    free(inbuf);
    free(plaintext);
    fclose(fin);
    dObj->setStored(false);
    dObj->deleteData();

    return;

err_m:
    delete m;
err_plaintext:
    free(plaintext);
err_inbuf:
    free(inbuf);
err_fin:
    fclose(fin);
    dObj->setStored(false);
    dObj->deleteData();
    HAGGLE_DBG("Error while trying to handle security data response.\n");
}

void SecurityHelper::requestSpecificKeys(List<string>& keys, DataObjectRef& dObj, NodeRefList *targets, bool publicKeys) {

    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    HashMap<string, string>* map = &(getManager()->pubKeysNeeded);
    HashMap<DataObjectRef, NodeRefList*>* waitingMap = &(getManager()->waitingForPubKey);
    string request_type = "pubkey_specific";
    string tag = "PublicKeys";
    List<string> toIssue;

    if (!publicKeys) {
        HAGGLE_DBG("We need decryption attributes!\n"); // CBMEN, AG
        map = &(getManager()->privKeysNeeded);
        request_type = "privkey_specific";
        tag = "PrivateKeys";
        waitingMap = &(getManager()->waitingForPrivKey);
    }

    for (List<string>::iterator it = keys.begin(); it != keys.end(); it++) {

        string fullName = *it;
        Pair<string, string> tuple = getAuthorityAndAttribute(fullName);
        HashMap<string, string>::iterator check = map->find(fullName);

        HAGGLE_DBG("Authority: %s, attribute: %s\n", tuple.first.c_str(), tuple.second.c_str());

        if (check == map->end())
            map->insert(make_pair(fullName, ""));
    }

    if (dObj) { // CBMEN, AG

    waitingMap->insert(make_pair(dObj, targets));
    HAGGLE_DBG("Inserted Data Object ID %s into Waiting Map %p.\n", dObj->getIdStr(), waitingMap);

    } // CBMEN, AG

    // CBMEN, HL, AG, Begin

    HashMap<string, HashMap<string, string> > keysByAuthority;
    for (HashMap<string, string>::iterator it = map->begin(); it != map->end(); it++) {
        string fullName = (*it).first;
        Pair<string, string> tuple = getAuthorityAndAttribute(fullName);

        HashMap<string, HashMap<string, string> >::iterator kit = keysByAuthority.find(tuple.first);
        if (kit == keysByAuthority.end()) {
            keysByAuthority.insert(make_pair(tuple.first, HashMap<string, string>()));
        }

        kit = keysByAuthority.find(tuple.first);
        if (kit != keysByAuthority.end()) {
            (*kit).second.insert(make_pair(fullName, ""));
        } else {
            HAGGLE_ERR("This should never happen, we must always have a matching entry for authority %s\n", tuple.first.c_str());
        }
    }

    for (HashMap<string, HashMap<string, string> >::iterator kit = keysByAuthority.begin();
         kit != keysByAuthority.end(); kit++)
    {
        // TODO: uncomment this when eager self-requests work
        // if ((*kit).first == getManager()->authorityName)
        //     continue;

        string authID;
        HashMap<string, string>::iterator it = getManager()->authorityNameMap.find((*kit).first);
        if (it == getManager()->authorityNameMap.end()) {
            HAGGLE_ERR("Couldn't find the node ID for authority \"%s\"\n", (*kit).first.c_str());
            continue;
        }
        authID = (*it).second;

        DataObjectRef dObjRequest = DataObject::create();

        if (!dObjRequest) {
            HAGGLE_ERR("Couldn't create data object\n");
            continue;
        }

        dObjRequest->addAttribute("SecurityDataRequest", request_type);
        dObjRequest->setPersistent(false);
        dObjRequest->addAttribute("target", authID);

        Metadata *m;
        m = dObjRequest->getMetadata()->addMetadata(tag);

        if (!(m && hashMapToMetadata(m, "Key", "name", (*kit).second))) {
            HAGGLE_ERR("Couldn't create metadata.\n");
            dObjRequest = NULL;
            continue;
        }

        m = dObjRequest->getMetadata()->addMetadata("roles");
        HashMap<string, string> roles;
        for (HashMap<string, string>::iterator rit = getManager()->rawRoleSharedSecretMap.begin();
                rit != getManager()->rawRoleSharedSecretMap.end(); rit++) {

            Pair<string, string> thePair = getAuthorityAndAttribute((*rit).first);

            if (thePair.first == "" || thePair.first != (*it).first)
                continue;

            if (roles.find((*rit).first) == roles.end())
                roles.insert(make_pair((*rit).first, ""));
        }

        if (!(m && hashMapToMetadata(m, "Role", "name", roles))) {
            HAGGLE_ERR("Couldn't create metadata.\n");
            dObjRequest = NULL;
            continue;
        }

        addTask(new SecurityTask(SECURITY_TASK_SEND_SECURITY_DATA_REQUEST, dObjRequest));
    }

    // CBMEN, HL, AG, End
}

void SecurityHelper::requestAllKeys(string authName, string authID, bool publicKeys) {

    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    HashMap<string, string>* map = &(getManager()->pubKeys);
    HashMap<string, string> request;
    string request_type = "pubkey_all";
    string tag = "PublicKeys";

    if (!publicKeys) {
        map = &(getManager()->privKeys);
        request_type = "privkey_all";
        tag = "PrivateKeys";
    }

    for (HashMap<string, string>::iterator it = map->begin(); it != map->end(); it++) {

        string fullName = (*it).first;
        Pair<string, string> tuple = getAuthorityAndAttribute(fullName);

        if (tuple.first == authName)
            request.insert(make_pair(fullName, ""));
    }

    DataObjectRef dObjRequest = DataObject::create();

    if (!dObjRequest) {
        HAGGLE_ERR("Couldn't create data object\n");
        return;
    }

    dObjRequest->addAttribute("SecurityDataRequest", request_type);
    dObjRequest->addAttribute("target", authID);
    dObjRequest->setPersistent(false);

    Metadata *m;
    m = dObjRequest->getMetadata()->addMetadata(tag);

    if (!(m && hashMapToMetadata(m, "Key", "name", *map))) {
        HAGGLE_ERR("Couldn't create metadata.\n");
        dObjRequest = NULL;
        return;
    }

    m = dObjRequest->getMetadata()->addMetadata("roles");
    HashMap<string, string> roles;
    for (HashMap<string, string>::iterator rit = getManager()->rawRoleSharedSecretMap.begin();
            rit != getManager()->rawRoleSharedSecretMap.end(); rit++) {

        Pair<string, string> thePair = getAuthorityAndAttribute((*rit).first);

        if (thePair.first == "" || thePair.first != authName)
            continue;

        if (roles.find((*rit).first) == roles.end())
            roles.insert(make_pair((*rit).first, ""));
    }

    if (!(m && hashMapToMetadata(m, "Role", "name", roles))) {
        HAGGLE_ERR("Couldn't create metadata.\n");
        dObjRequest = NULL;
        return;
    }

    addTask(new SecurityTask(SECURITY_TASK_SEND_SECURITY_DATA_REQUEST, dObjRequest));
}

bool SecurityHelper::signCertificate(string subject, Metadata *m, Metadata *response) {
    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    Metadata *dm;
    const char *myPEM;
    const char *certPEM;
    RSA *pubKey;

    // CBMEN, HL - Support access control list for certification

    if (getManager()->certificationACL.find(subject) == getManager()->certificationACL.end()) {
        HAGGLE_DBG("Node %s is not authorized for certification, ignoring certificate signing request.\n", subject.c_str());
        return false;
    }

    CertificateRef cert = getManager()->retrieveCertificate(subject, getManager()->caCertCache);
    if (cert) {
        HAGGLE_DBG("Sending cached signed certificate\n");
    } else {

        dm = m->getMetadata("pubkey");
        if (!dm)
            return false;

        pubKey = stringToRSAKey(dm->getContent().c_str(), KEY_TYPE_PUBLIC);
        if (!pubKey)
            return false;

        HAGGLE_DBG("Signing certificate for id=%s\n", subject.c_str());
        cert = new Certificate(subject.c_str(),
            getManager()->getKernel()->getThisNode()->getIdStr(), "forever",
            getManager()->getKernel()->getThisNode()->strIdToRaw(subject.c_str()), // TODO: This call is not thread-safe
            pubKey);

        RSA_free(pubKey); // CBMEN, HL - free leak

        if (cert) {
            if (!cert->sign(getManager()->privKey)) {
                HAGGLE_ERR("Signing of certificate failed \n");
                return false;
            }
        }

        // Verify the signature so it gets marked as verified
        cert->verifySignature(getManager()->pubKey);

        // Save it now so we don't have to wait until we receive an updated
        // node description before we save it
        getManager()->storeCertificate(cert->getSubject()+"|"+cert->getIssuer(),
                                       cert, getManager()->remoteSignedCerts, true); // CBMEN, AG

        // Look up the self signed certificate, and verify it if needed
        CertificateRef scert = getManager()->retrieveCertificate(cert->getSubject(), getManager()->certStore);
        if (scert) {
            if (!scert->isVerified()) {
                scert->verifySignature(cert->getPubKey());
                if (scert->isVerified()) {
                    HAGGLE_DBG("Data objects from %s can now be verified\n", scert->getSubject().c_str());
                }
            }
        }

        // Save it in the cache
        getManager()->storeCertificate(subject, cert, getManager()->caCertCache, true);
    }

    myPEM = getManager()->myCert->toPEM();
    if (!myPEM)
        return false;

    dm = response->addMetadata("issuer_certificate", myPEM);
    if (!dm)
        return false;

    certPEM = cert->toPEM();
    if (!cert)
        return false;

    dm = response->addMetadata("certificate", certPEM);
    if (!dm)
        return false;

    return true;
}

bool SecurityHelper::sendSpecificPublicKeys(string subject, Metadata *m, Metadata *response) {

    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    Metadata *dm;
    HashMap<string, string> toReturn;
    string ct, roleName;
    HashMap<string, string>::iterator kit;

    dm = m->getMetadata("PublicKeys");
    if (!dm)
        return false;

    HashMap<string, string> request = metadataToHashMap(dm, "Key", "name");

    dm = m->getMetadata("roles");
    if (!dm)
        return false;

    HashMap<string, string> roles = metadataToHashMap(dm, "Role", "name");

    for (HashMap<string, string>::iterator it = request.begin(); it != request.end(); it++) {
        string fullName = (*it).first;
        Pair<string, string> tuple = getAuthorityAndAttribute(fullName);

        if (tuple.first != getManager()->authorityName)
            continue;

        HashMap<string, string>::iterator mit = getManager()->issuedPubKeys.find(fullName);
        if (mit == getManager()->issuedPubKeys.end()) {
            List<string> toIssue;
            toIssue.push_back(fullName);
            issuePublicKeys(toIssue);
            mit = getManager()->issuedPubKeys.find(fullName);
        }

        bool authorized = false;
        for (HashMap<string, string>::iterator rit = roles.begin(); rit != roles.end(); rit++) {
            roleName = (*rit).first;
            Pair<string, string> thePair = getAuthorityAndAttribute(roleName);

            if (thePair.first != getManager()->authorityName)
                continue;

            if (getManager()->publicKeyACL.find(roleName + ":" + fullName) != getManager()->publicKeyACL.end()) {
                kit = getManager()->rawRoleSharedSecretMap.find(roleName);
                if (kit == getManager()->rawRoleSharedSecretMap.end()) {
                    continue;
                } else {
                    if (encryptStringBase64((*mit).second, (*kit).second, ct))
                        toReturn.insert(make_pair((*mit).first, ct));
                    authorized = true;
                    HAGGLE_DBG("%s is authorized for attribute public key %s through role %s, sending.\n", subject.c_str(), fullName.c_str(), roleName.c_str());
                    // Don't break. Can send multiple times. Might have decryption work with one role but not other
                }
            }
        }

        if (!authorized)
            HAGGLE_DBG("%s is not authorized for attribute public key %s, not sending.\n", subject.c_str(), fullName.c_str());
    }

    dm = response->addMetadata("PublicKeys");
    if (!dm)
        return false;

    return hashMapToMetadata(dm, "Key", "name", toReturn);
}

bool SecurityHelper::sendSpecificPrivateKeys(string subject, Metadata *m, Metadata *response) {

    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    Metadata *dm;
    HashMap<string, string> toReturn;
    string ct, roleName, privkey;
    HashMap<string, string>::iterator kit;

    dm = m->getMetadata("PrivateKeys");
    if (!dm)
        return false;

    HashMap<string, string> request = metadataToHashMap(dm, "Key", "name");

    dm = m->getMetadata("roles");
    if (!dm)
        return false;

    HashMap<string, string> roles = metadataToHashMap(dm, "Role", "name");

    for (HashMap<string, string>::iterator it = request.begin(); it != request.end(); it++) {
        string fullName = (*it).first;
        Pair<string, string> tuple = getAuthorityAndAttribute(fullName);

        if (tuple.first != getManager()->authorityName)
            continue;

        bool authorized = false;
        bool issued = false;
        for (HashMap<string, string>::iterator rit = roles.begin(); rit != roles.end(); rit++) {
            roleName = (*rit).first;
            Pair<string, string> thePair = getAuthorityAndAttribute(roleName);

            if (thePair.first != getManager()->authorityName)
                continue;

            if (getManager()->privateKeyACL.find(roleName + ":" + fullName) != getManager()->privateKeyACL.end()) {
                kit = getManager()->rawRoleSharedSecretMap.find(roleName);
                if (kit == getManager()->rawRoleSharedSecretMap.end()) {
                    continue;
                } else {
                    if (!issued) {
                        if (issuePrivateKey(fullName, subject, privkey, false)) {
                            issued = true;
                        } else {
                            HAGGLE_ERR("Error issuing attribute private key %s for %s.\n", fullName.c_str(), subject.c_str());
                            continue;
                        }
                    }
                    if (encryptStringBase64(privkey, (*kit).second, ct))
                        toReturn.insert(make_pair(fullName, ct));
                    authorized = true;
                    HAGGLE_DBG("%s is authorized for attribute private key %s through role %s, sending.\n", subject.c_str(), fullName.c_str(), roleName.c_str());
                    // Don't break. Can send multiple times. Might have decryption work with one role but not other
                }
            }
        }

        if (!authorized)
            HAGGLE_DBG("%s is not authorized for attribute private key %s, not sending.\n", subject.c_str(), fullName.c_str());
    }

    dm = response->addMetadata("PrivateKeys");
    if (!dm)
        return false;

    return hashMapToMetadata(dm, "Key", "name", toReturn);
}

bool SecurityHelper::sendAllPublicKeys(string subject, Metadata *m, Metadata *response) {

    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    Metadata *dm;
    HashMap<string, string> toReturn;
    string ct, roleName, fullName;
    HashMap<string, string>::iterator kit;

    dm = m->getMetadata("PublicKeys");
    if (!dm)
        return false;

    HashMap<string, string> have = metadataToHashMap(dm, "Key", "name");

    dm = m->getMetadata("roles");
    if (!dm)
        return false;

    HashMap<string, string> roles = metadataToHashMap(dm, "Role", "name");

    // Iterate over all entries in our ACL
    for (HashMap<string, string>::iterator ait = getManager()->publicKeyACL.begin(); ait != getManager()->publicKeyACL.end(); ait++) {
        string entry = (*ait).first;

        // Check whether the role is authorized
        for (HashMap<string, string>::iterator rit = roles.begin(); rit != roles.end(); rit++) {
            roleName = (*rit).first;
            Pair<string, string> thePair = getAuthorityAndAttribute(roleName);

            if (thePair.first != getManager()->authorityName)
                continue;

            if (entry.substr(0, roleName.length()) == roleName) {
                fullName = entry.substr(roleName.length()+1);

                // Check whether the node already has it
                HashMap<string, string>::iterator hit = have.find(fullName);
                if (hit != have.end())
                    continue;

                // Issue if needed
                HashMap<string, string>::iterator mit = getManager()->issuedPubKeys.find(fullName);
                if (mit == getManager()->issuedPubKeys.end()) {
                    List<string> list;
                    list.push_back(fullName);

                    if (!issuePublicKeys(list)) {
                        continue;
                    }

                    mit = getManager()->issuedPubKeys.find(fullName);
                }

                kit = getManager()->rawRoleSharedSecretMap.find(roleName);
                if (kit == getManager()->rawRoleSharedSecretMap.end()) {
                    continue;
                }

                if (mit != getManager()->issuedPubKeys.end()) {
                    HAGGLE_DBG("%s is authorized for attribute public key %s through role %s, sending.\n", subject.c_str(), fullName.c_str(), roleName.c_str());
                    encryptStringBase64((*mit).second, (*kit).second, ct);
                    toReturn.insert(make_pair((*mit).first, ct));
                    // Don't break. Can send multiple times. Might have decryption work with one role but not other
                }
            }
        }
    }

    dm = response->addMetadata("PublicKeys");
    if (!dm)
        return false;

    return hashMapToMetadata(dm, "Key", "name", toReturn);
}

bool SecurityHelper::sendAllPrivateKeys(string subject, Metadata *m, Metadata *response) {

    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    Metadata *dm;
    HashMap<string, string> toReturn;
    string ct, roleName, fullName, privkey;
    HashMap<string, string>::iterator kit;

    dm = m->getMetadata("PrivateKeys");
    if (!dm)
        return false;

    HashMap<string, string> have = metadataToHashMap(dm, "Key", "name");

    dm = m->getMetadata("roles");
    if (!dm)
        return false;

    HashMap<string, string> roles = metadataToHashMap(dm, "Role", "name");

    // Iterate over all entries in our ACL
    for (HashMap<string, string>::iterator ait = getManager()->privateKeyACL.begin(); ait != getManager()->privateKeyACL.end(); ait++) {
        string entry = (*ait).first;

        // Check whether the role is authorized
        for (HashMap<string, string>::iterator rit = roles.begin(); rit != roles.end(); rit++) {
            roleName = (*rit).first;
            Pair<string, string> thePair = getAuthorityAndAttribute(roleName);

            if (thePair.first != getManager()->authorityName)
                continue;

            if (entry.substr(0, roleName.length()) == roleName) {
                fullName = entry.substr(roleName.length()+1);

                // Check whether the node already has it
                HashMap<string, string>::iterator hit = have.find(fullName);
                if (hit != have.end())
                    continue;

                kit = getManager()->rawRoleSharedSecretMap.find(roleName);
                if (kit == getManager()->rawRoleSharedSecretMap.end()) {
                    continue;
                }

                if (issuePrivateKey(fullName, subject, privkey, false)) {
                    HAGGLE_DBG("%s is authorized for attribute private key %s through role %s, sending.\n", subject.c_str(), fullName.c_str(), roleName.c_str());
                    encryptStringBase64(privkey, (*kit).second, ct);
                    toReturn.insert(make_pair(fullName, ct));
                    // Don't break. Can send multiple times. Might have decryption work with one role but not other
                } else {
                    HAGGLE_ERR("Error issuing attribute private key %s for %s.\n", fullName.c_str(), subject.c_str());
                }
            }
        }
    }

    dm = response->addMetadata("PrivateKeys");
    if (!dm)
        return false;

    return hashMapToMetadata(dm, "Key", "name", toReturn);
}

bool SecurityHelper::decryptAttributeKey(string ciphertext, string& roleName, string& plaintext) {
    Mutex::AutoLocker l(getManager()->dynamicConfigurationMutex);
    for (HashMap<string, string>::iterator rit = getManager()->rawRoleSharedSecretMap.begin();
         rit != getManager()->rawRoleSharedSecretMap.end(); rit++) {

        roleName = (*rit).first;
        if (decryptStringBase64(ciphertext, (*rit).second, plaintext)) {
            return true;
        }
    }
    return false;
}

void SecurityHelper::decryptAttributeBuckets() {
    Mutex::AutoLocker l(getManager()->ccbMutex);
    string fullName, key, roleName, plaintext;
    HashMap<string, string>::iterator hit;

    for (HashMap<string, string>::iterator it = getManager()->pubKeyBucket.begin(); it != getManager()->pubKeyBucket.end(); it++) {
        fullName = (*it).first;
        key = (*it).second;

        hit = getManager()->pubKeys.find(fullName);
        if (hit != getManager()->pubKeys.end())
            continue;

        if (decryptAttributeKey(key, roleName, plaintext)) {
            HAGGLE_DBG("Decrypted attribute public key %s using role %s.\n", fullName.c_str(), roleName.c_str());
            if (!bridge::addAuthorityPK(fullName, plaintext)) {
                getManager()->pubKeys.insert(make_pair(fullName, ""));
            }
        } else {
            HAGGLE_DBG("Could not decrypt attribute public key %s!\n", fullName.c_str());
        }
    }

    for (HashMap<string, string>::iterator it = getManager()->privKeyBucket.begin(); it != getManager()->privKeyBucket.end(); it++) {
        fullName = (*it).first;
        key = (*it).second;

        hit = getManager()->privKeys.find(fullName);
        if (hit != getManager()->privKeys.end())
            continue;

        if (decryptAttributeKey(key, roleName, plaintext)) {
            HAGGLE_DBG("Decrypted attribute private key %s using role %s.\n", fullName.c_str(), roleName.c_str());
            if (!bridge::addUserSK(fullName, plaintext)) {
                getManager()->privKeys.insert(make_pair(fullName, ""));
            }
        } else {
            HAGGLE_DBG("Could not decrypt attribute private key %s!\n", fullName.c_str());
        }
    }
}

bool SecurityHelper::saveReceivedKeys(Metadata *m, bool publicKeys) {

    Mutex::AutoLocker l(getManager()->ccbMutex);
    Metadata *dm;
    size_t saved = 0;
    string roleName, plaintext;
    HashMap<string, string>::iterator hit;
    string tag = "PublicKeys";

    if (!publicKeys)
        tag = "PrivateKeys";

    dm = m->getMetadata(tag);
    if (!dm)
        return false;

    HashMap<string, string> response = metadataToHashMap(dm, "Key", "name");
    string gid = getManager()->kernel->getThisNode()->getIdStr();

    for (HashMap<string, string>::iterator it = response.begin(); it != response.end(); it++) {
        string fullName = (*it).first;
        string key = (*it).second;
        bool error = false;
        bool found = false;

        if (publicKeys) {
            HAGGLE_DBG("Saving encrypted attribute public key %s into bucket.\n", fullName.c_str());

            // Can save multiple times. Might have decryption work with one role but not other
            hit = getManager()->pubKeyBucket.find(fullName);
            while (hit != getManager()->pubKeyBucket.end() && (*hit).first == fullName) {
                if ((*hit).second == key) {
                    found = true;
                    break;
                }
                ++hit;
            }
            if (!found)
                getManager()->pubKeyBucket.insert(make_pair(fullName, key));

            hit = getManager()->pubKeys.find(fullName);
            if (hit != getManager()->pubKeys.end()) {
                HAGGLE_DBG("Already have decrypted attribute public key %s.\n", fullName.c_str());
                continue;
            }

            if (decryptAttributeKey(key, roleName, plaintext)) {
                HAGGLE_DBG("Decrypted attribute public key %s using role %s.\n", fullName.c_str(), roleName.c_str());
                if (!bridge::addAuthorityPK(fullName, plaintext)) {
                    getManager()->pubKeys.insert(make_pair(fullName, ""));
                    saved++;
                } else {
                    error = true;
                }
            } else {
                HAGGLE_DBG("Could not decrypt attribute public key %s!\n", fullName.c_str());
            }
        } else {
            HAGGLE_DBG("Saving encrypted attribute private key %s into bucket.\n", fullName.c_str());

            // Can save multiple times. Might have decryption work with one role but not other
            hit = getManager()->privKeyBucket.find(fullName);
            while (hit != getManager()->privKeyBucket.end() && (*hit).first == fullName) {
                if ((*hit).second == key) {
                    found = true;
                    break;
                }
                ++hit;
            }
            if (!found)
                getManager()->privKeyBucket.insert(make_pair(fullName, key));

            hit = getManager()->privKeys.find(fullName);
            if (hit != getManager()->privKeys.end()) {
                HAGGLE_DBG("Already have decrypted attribute private key %s.\n", fullName.c_str());
                continue;
            }

            if (decryptAttributeKey(key, roleName, plaintext)) {
                HAGGLE_DBG("Decrypted attribute private key %s using role %s.\n", fullName.c_str(), roleName.c_str());
                if (!bridge::addUserSK(fullName, plaintext)) {
                    getManager()->privKeys.insert(make_pair(fullName, ""));
                    saved++;
                } else {
                    error = true;
                }
            } else {
                HAGGLE_DBG("Could not decrypt attribute private key %s!\n", fullName.c_str());
            }
        }

        if (error) {
            HAGGLE_ERR("Error saving received key %s\n", fullName.c_str());
            continue;
        }

        if (publicKeys) {
            getManager()->pubKeysNeeded.erase(fullName);
        } else {
            getManager()->privKeysNeeded.erase(fullName);
        }
    }

    if (saved > 0)
        return true;
    else
        return false;
}

bool SecurityHelper::issuePublicKeys(List<string>& names) {
    Mutex::AutoLocker l(getManager()->ccbMutex);
    Map<string, string> sk, pk;
    if (bridge::authsetup(names, sk, pk)) {
        HAGGLE_ERR("Error generating attribute pubkeys\n");
        return false;
    }
    for (Map<string, string>::iterator it = pk.begin(); it != pk.end(); it++) {
        HAGGLE_DBG("Issued attribute public key %s\n", (*it).first.c_str());
        getManager()->issuedPubKeys.insert(*it);
    }
    return true;
}

bool SecurityHelper::issuePrivateKey(string fullName, string subject, string& privkey, bool self) {
    Mutex::AutoLocker l(getManager()->ccbMutex);
    try {

        HashMap<string, string>::iterator mit = getManager()->issuedPubKeys.find(fullName);
        if (mit == getManager()->issuedPubKeys.end()) {
            List<string> list;
            list.push_back(fullName);

            if (!issuePublicKeys(list))
                return false;
        }

        HAGGLE_DBG("Issuing attribute private key %s for %s\n", fullName.c_str(), subject.c_str());

        if (bridge::keygen(fullName, subject, self, privkey)) {
            HAGGLE_ERR("Error generating privkey %s\n", fullName.c_str());
            return false;
        }

        return true;
    } catch (Exception& e) {
        HAGGLE_ERR("Error generating attribute privkey %s\n", fullName.c_str());
        return false;
    }
}

bool SecurityHelper::updateWaitingQueues(bool publicKeys) {
    // If publicKeys = true, we're working with the public key queue
    // if publicKeys = false, we're working with the private key queue
    HashMap<DataObjectRef, NodeRefList*>* map = &(getManager()->waitingForPubKey);
    SecurityTaskType_t type = SECURITY_TASK_GENERATE_CAPABILITY;

    HAGGLE_DBG("Updating waiting queues. Public: %d, Map: %p, Map Size: %d\n", publicKeys, map, map->size());
    if (!publicKeys) {
        map = &(getManager()->waitingForPrivKey);
        type = SECURITY_TASK_USE_CAPABILITY;
        HAGGLE_DBG("Map: %p, Map size: %d\n", map, map->size());
    }

    for (HashMap<DataObjectRef, NodeRefList*>::iterator it = map->begin(); it != map->end(); it++) {
        HAGGLE_DBG("Re-queued DataObject %s\n", (*it).first->getIdStr());
        addTask(new SecurityTask(type, (*it).first, NULL, (*it).second));
        delete (*it).second;
    }

    *map = HashMap<DataObjectRef, NodeRefList*>();

    return true;
}

// Verify all certificates issued by this authority.
// This should NEVER be called accidentally with a non-authority cert.
bool SecurityHelper::verifyAuthorityCertificate(CertificateRef caCert) {
    Mutex::AutoLocker l(getManager()->certStoreMutex);
    if (!caCert) {
        HAGGLE_ERR("Can't verify null caCert!\n");
        return false;
    }

    string authID = caCert->getIssuer();
    if (!caCert->isVerified()) {
        HAGGLE_DBG("Verifying authority certificate for node %s\n", authID.c_str());
        caCert->verifySignature(caCert->getPubKey());
        if (!caCert->isVerified()) {
            HAGGLE_ERR("Could not verify authority certificate for node %s!\n", authID.c_str());
            return false;
        }
    }

    HAGGLE_DBG("Verifying certificates issued by authority %s!\n", authID.c_str());

    for (CertificateStore_t::iterator it = getManager()->myCerts.begin(); it != getManager()->myCerts.end(); it++) {
        CertificateRef& cert = (*it).second;
        if (cert->getIssuer() == authID && !cert->isVerified()) {
            HAGGLE_DBG("Will try to verify this node's certificate using CA certificate of %s\n",
                       cert->getSubject().c_str(), cert->getIssuer().c_str());
            addTask(new SecurityTask(SECURITY_TASK_VERIFY_CERTIFICATE, NULL, cert));
        }
    }

    for (CertificateStore_t::iterator it = getManager()->remoteSignedCerts.begin(); it != getManager()->remoteSignedCerts.end(); it++) {
        CertificateRef& cert = (*it).second;
        if (cert->getIssuer() == authID && !cert->isVerified()) {
            HAGGLE_DBG("Will try to verify certificate for %s using CA certificate of %s\n",
                       cert->getSubject().c_str(), cert->getIssuer().c_str());
            addTask(new SecurityTask(SECURITY_TASK_VERIFY_CERTIFICATE, NULL, cert));
        }
    }

    return true;
}
// CBMEN, HL, End

void SecurityHelper::doTask(SecurityTask *task)
{
        switch (task->type) {

        case SECURITY_TASK_GENERATE_CERTIFICATE:
            HAGGLE_DBG("Creating certificate for id=%s\n",
                       getManager()->getKernel()->getThisNode()->getIdStr());

            if (strlen(getManager()->getKernel()->getThisNode()->getIdStr()) == 0) {
                HAGGLE_ERR("Certificate creation failed: This node's id is not valid!\n");
                break;
            }
            task->cert = Certificate::create(getManager()->getKernel()->getThisNode()->getIdStr(),
                                             getManager()->getKernel()->getThisNode()->getIdStr(), "forever", // CBMEN, HL
                                             getManager()->getKernel()->getThisNode()->getId(),
                                             &task->privKey,
                                             getManager()->rsaKeyLength);
            break;

            // CBMEN, HL, Begin
        case SECURITY_TASK_VERIFY_CERTIFICATE:
            HAGGLE_DBG("Verifying certificate\n"); // CBMEN, AG

            if (task->cert) {
                Mutex::AutoLocker l(getManager()->certStoreMutex);
                if (!task->cert->isVerified()){
                    // Look up caCert directly first, in case it's an authority certificate that is already in caCerts
                    CertificateRef caCert = getManager()->retrieveCertificate(task->cert->getIssuer(), getManager()->caCerts);
                    if (!caCert) {
                        if (task->cert->getIssuer() == task->cert->getSubject()) {
                            HAGGLE_DBG("Need to verify self signed certificate for %s, looking up attestation certificate!\n", task->cert->getSubject().c_str());
                            for (CertificateStore_t::iterator it = getManager()->remoteSignedCerts.begin(); it != getManager()->remoteSignedCerts.end(); it++) {
                                CertificateRef cert = (*it).second;
                                // Make sure it's a verified certificate, has the same subject, and that the public key matches
                                if (cert->isVerified() && cert->getSubject() == task->cert->getSubject() &&
                                    EVP_PKEY_cmp(cert->getEVPPubKey(), task->cert->getEVPPubKey()) == 1) {
                                    HAGGLE_DBG("Will try to verify self signed certificate for %s using attestation certificate issued by %s\n",
                                               cert->getSubject().c_str(), cert->getIssuer().c_str());
                                    caCert = cert;
                                    break;
                                }
                            }
                            HAGGLE_DBG("No attestation certificate found for %s!\n", task->cert->getSubject().c_str());
                        }
                    }

                    if (caCert && caCert->isVerified()) {
                        task->cert->verifySignature(caCert->getPubKey());

                        if (task->cert->isVerified()) {
                            // CBMEN, HL - Move check here in order to avoid race condition when verifying
                            // node descriptions. Otherwise signature verification will fail as SecurityHelper
                            // will try verifying the node description before SecurityManager's onSecurityTaskComplete
                            // verifies the self signed certificate

                            // Verify the self-signed certificate of the subject, if present and not previously verified
                            CertificateRef cert = getManager()->retrieveCertificate(task->cert->getSubject(), getManager()->certStore);
                            if (cert) {
                                if (!cert->isVerified()) {
                                    cert->verifySignature(task->cert->getPubKey());
                                    if (cert->isVerified()) {
                                        HAGGLE_DBG("Data objects from %s can now be verified\n", cert->getSubject().c_str());
                                    } else {
                                        HAGGLE_DBG("Unable to verify certificate from %s\n", cert->getSubject().c_str());
                                    }
                                }
                            }
                        } else {
                            HAGGLE_ERR("Signature verification failed for certificate for %s issued by %s; being verified with caCert for %s issued by %s\n", task->cert->getSubject().c_str(), task->cert->getIssuer().c_str(), caCert->getSubject().c_str(), caCert->getIssuer().c_str());
                        }
                    } else {
                        HAGGLE_DBG("Can't verify certificate for %s due to lack of CA certificate from authority %s\n",
                                   task->cert->getSubject().c_str(), task->cert->getIssuer().c_str()); // CBMEN, AG
                    }
                }
            }
            // CBMEN, HL, End
            break;

        case SECURITY_TASK_VERIFY_DATAOBJECT:
            // Check for a certificate:
            task->cert = getManager()->retrieveCertificate(task->dObj->getSignee(), getManager()->certStore); // CBMEN, HL

            if (task->cert) {
                if (task->cert->isVerified()) { // CBMEN, AG
                    HAGGLE_DBG("Verifying Data Object [%s]\n", task->dObj->getIdStr());
                    verifyDataObject(task->dObj, task->cert);
                } else if (task->dObj->isNodeDescription()) {
                    HAGGLE_DBG("Trying to verify node description [%s] without verified certificate.\n", task->dObj->getIdStr());
                    NodeRef oldNode = getManager()->kernel->getNodeStore()->retrieve(task->dObj->getSignee());
                    if (oldNode) {
                        DataObjectRef oldNodeDescription = oldNode->getDataObject();
                        oldNodeDescription.lock();
                        Metadata *m = oldNodeDescription->getMetadata();
                        Metadata *dm = NULL;
                        bool haveOldCertificates = false;
                        if (m) {
                            m = m->getMetadata("Security");
                            if (m) {
                                dm = m->getMetadata("Certificate");
                                if (dm) {
                                    haveOldCertificates = true;
                                    bool found = false;
                                    do {
                                        CertificateRef oldCert = Certificate::fromPEM(dm->getContent());
                                        if (oldCert == task->cert) {
                                            HAGGLE_DBG("Verifying node description [%s] with previously stored certificate for node [%s]\n", task->dObj->getIdStr(), task->dObj->getSignee().c_str());
                                            verifyDataObject(task->dObj, task->cert);
                                            found = true;
                                            break;
                                        }
                                    } while ((dm = m->getNextMetadata()));

                                    if (!found) {
                                        HAGGLE_ERR("Node description [%s] from node [%s] has different certificates than the previously seen ones. Discarding node description.\n", task->dObj->getIdStr(), task->dObj->getSignee().c_str());
                                    }
                                }
                            }
                        }
                        oldNodeDescription.unlock();

                        if (!haveOldCertificates) {
                            HAGGLE_DBG("No previously stored certificates for node [%s]. Verifying node description [%s] with self signed certificate.\n", task->dObj->getSignee().c_str(), task->dObj->getIdStr());
                            verifyDataObject(task->dObj, task->cert);
                        }
                    } else {
                        HAGGLE_DBG("No previously stored node description for node [%s]. Verifying node description [%s] with self signed certificate.\n", task->dObj->getSignee().c_str(), task->dObj->getIdStr());
                        verifyDataObject(task->dObj, task->cert);
                    }
                } else {
                    HAGGLE_DBG("Could not verify Data Object [%s] since signer's certificate is not verified\n", task->dObj->getIdStr()); // CBMEN, AG
                    HAGGLE_DBG("Signer is %s with certificate issued by %s\n", task->cert->getSubject().c_str(), task->cert->getIssuer().c_str()); // CBMEN, HL
                }
            } else {
                HAGGLE_DBG("Could not verify Data Object [%s] due to lack of certificate. Can happen if the node description is delayed\n", task->dObj->getIdStr());
            }
            break;

        case SECURITY_TASK_SIGN_DATAOBJECT:

            if (signDataObject(task->dObj, getManager()->privKey)) { // CBMEN, AG, Begin
                HAGGLE_DBG("Successfully signed Data Object [%s]\n",
                           task->dObj->getIdStr());
            } else {
                HAGGLE_DBG("Signing of Data Object [%s] failed!\n",
                           task->dObj->getIdStr());
            } // CBMEN, AG, End

            break;

            // CBMEN, HL, Begin

        case SECURITY_TASK_GENERATE_CAPABILITY:

            if (!getManager()->pythonRunning) { // CBMEN, AG, Begin
                if(!startPython()) {
                    HAGGLE_ERR("Need Python to generate capability\n");
                    break;
                }
            } // CBMEN, AG, End

            if (generateCapability(task->dObj)) {
                HAGGLE_DBG("Capability successfully generated for Data Object [%s]\n", task->dObj->getIdStr());
            } else {
                HAGGLE_DBG("Couldn't generate capability for Data Object [%s]\n", task->dObj->getIdStr());
            }
            break;

        case SECURITY_TASK_USE_CAPABILITY:

        {
            bool failed = false;
            if (!getManager()->pythonRunning) { // CBMEN, AG, Begin
                if(!startPython()) {
                    HAGGLE_ERR("Need Python to use capability\n");
                    failed = true;
                }
            }

            if (!failed && useCapability(task->dObj, task->targets)) {
                HAGGLE_DBG("Successfully used capability for Data Object [%s].\n", task->dObj->getIdStr());
            } else {
                HAGGLE_DBG("Couldn't use capability for Data Object [%s].\n", task->dObj->getIdStr());
                failed = true;
            }

            // if (failed) {
            //     // Tell the ApplicationManager we couldn't decrypt it,
            //     // so that it clears its pendingDO list and can shutdown properly.
            //     HAGGLE_DBG("Issuing EVENT_TYPE_DATAOBJECT_SEND_FAILURE for Data Object [%s]\n", task->dObj->getIdStr());
            //     for (NodeRefList::iterator it = task->targets->begin(); it != task->targets->end(); it++) {
            //         getManager()->kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, task->dObj, *it));
            //     }
            // }
        }
            break;

            case SECURITY_TASK_ENCRYPT_DATAOBJECT:
                if (encryptDataObject(task->dObj, task->AESKey)) {
                    HAGGLE_DBG("Finished encrypting - new encrypted Data Object [%s] is replacing plaintext object\n", task->dObj->getIdStr());
#ifdef DEBUG
  if(Trace::trace.getTraceType() <= TRACE_TYPE_DEBUG1) {
    task->dObj->print(NULL, true); // MOS - NULL means print to debug trace
  }
#endif
                } else {
                    HAGGLE_ERR("Couldn't encrypt Data Object [%s].\n", task->dObj->getIdStr());
                }
            break;

            case SECURITY_TASK_DECRYPT_DATAOBJECT:
            if (decryptDataObject(task->dObj, task->AESKey)) {
                    HAGGLE_DBG("Finished decrypting - Data Object [%s] is now in decrypted state\n", task->dObj->getIdStr());
                } else {
                    HAGGLE_ERR("Couldn't decrypt Data Object [%s].\n", task->dObj->getIdStr());
                    // We need to prevent it from being sent up somehow.
                }
            break;

        case SECURITY_TASK_SEND_SECURITY_DATA_REQUEST:

      if(getManager()->tooManyOutstandingSecurityDataRequests()) break; // MOS

            if (task->dObj) {
                HAGGLE_DBG("Sending SecurityDataRequest\n");
                sendSecurityDataRequest(task->dObj);
            }

            break;

        case SECURITY_TASK_HANDLE_SECURITY_DATA_REQUEST:

            if (task->dObj) {
                HAGGLE_DBG("Handling SecurityDataRequest\n");
                handleSecurityDataRequest(task->dObj);
            }

            break;

        case SECURITY_TASK_HANDLE_SECURITY_DATA_RESPONSE:

            if (task->dObj) {
                HAGGLE_DBG("Handling SecurityDataResponse\n");
                handleSecurityDataResponse(task->dObj);
            }

            break;

        case SECURITY_TASK_VERIFY_AUTHORITY_CERTIFICATE:
            HAGGLE_DBG("Verifying authority certificate\n");

            if (task->cert) {
                if (!verifyAuthorityCertificate(task->cert)) {
                    HAGGLE_ERR("Couldn't verify authority certificate for node %s!\n", task->cert->getIssuer().c_str());
                } else {
                    if (task->cert->isVerified()) {

                        // Verify the self-signed certificate of the authority, if present and not previously verified
                        CertificateRef cert = getManager()->retrieveCertificate(task->cert->getSubject(), getManager()->certStore);
                        if (cert) {
                            if (!cert->isVerified()) {
                                cert->verifySignature(task->cert->getPubKey());
                                if (cert->isVerified()) {
                                    HAGGLE_DBG("Data objects from %s can now be verified\n", cert->getSubject().c_str());
                                } else {
                                    HAGGLE_DBG("Unable to verify certificate from %s\n", cert->getSubject().c_str());
                                }
                            }
                        }
                    }
                }
            }
            break;

        case SECURITY_TASK_DECRYPT_ATTRIBUTE_BUCKETS:
            HAGGLE_DBG("Decrypting attribute buckets with new role keys.\n");
            decryptAttributeBuckets();
        break;

            // CBMEN, HL, End

        // CBMEN, AG, Begin

        case SECURITY_TASK_UPDATE_WAITING_QUEUES:
                HAGGLE_DBG("Updating public attribute waiting queues\n");
                updateWaitingQueues(true);

                HAGGLE_DBG("Updating private attribute waiting queues\n");
                updateWaitingQueues(false);

            break;

        // CBMEN, AG, End
    }
    // Return result if the private event is valid
    if (Event::isPrivate(etype))
        addEvent(new Event(etype, task));
    else
        delete task;

    return;
}

bool SecurityHelper::run()
{
    HAGGLE_DBG("SecurityHelper running...\n");

    while (!shouldExit()) {
        QueueEvent_t qe;
        SecurityTask *task = NULL;

        qe = taskQ.retrieve(&task);

        switch (qe) {
        case QUEUE_ELEMENT:
            doTask(task);

            // Delete task here or return it with result in private event?
            //delete task;
            break;
        case QUEUE_WATCH_ABANDONED:
            HAGGLE_DBG("SecurityHelper instructed to exit...\n");
            return false;
        default:
            HAGGLE_ERR("Unknown security task queue return value\n");
        }
    }
    return false;
}

void SecurityHelper::cleanup()
{
    while (!taskQ.empty()) {
        SecurityTask *task = NULL;
        taskQ.retrieve(&task);
        delete task;
    }
}

const char *security_level_names[] = { "LOW", "MEDIUM", "HIGH", "VERYHIGH" }; // CBMEN, HL

SecurityManager::SecurityManager(HaggleKernel *_haggle,
                 const SecurityLevel_t slevel) :
    Manager("SecurityManager", _haggle), securityLevel(slevel),
    signNodeDescriptions(true), // MOS
    encryptFilePayload(false), // MOS
    securityConfigured(false), onRepositoryDataCalled(false), // CBMEN, HL
    etype(EVENT_TYPE_INVALID), helper(NULL),
    myCert(NULL), // CBMEN, AG
    pythonRunning(false), // CBMEN, HL
    privKey(NULL), pubKey(NULL), // CBMEN, HL, AG
    isAuthority(false), // CBMEN, HL
    certificateSigningRequestDelay(SIGNING_REQUEST_DELAY),  // CBMEN, HL, AG
    certificateSigningRequestRetries(SIGNING_REQUEST_RETRIES),  // MOS
    certificateSigningRequests(0),  // MOS
    updateWaitingQueueDelay(WAITING_QUEUE_DELAY), // CBMEN, AG
    outstandingSecurityDataRequests(0), // MOS
    maxOutstandingRequests(MAX_OUTSTANDING_REQUESTS), // CBMEN, AG
    securityDataRequestEType(EVENT_TYPE_INVALID), securityDataResponseEType(EVENT_TYPE_INVALID), // CBMEN, HL
    signatureChaining(false), // CBMEN, HL
    charmPersistenceData(CHARM_PERSISTENCE_DATA), tempFilePath(TEMP_SDR_FILE_PATH), // CBMEN, HL
    rsaKeyLength(RSA_KEY_LENGTH), securityDataRequestTimerRunning(false),
    enableCompositeSecurityDataRequests(false) // CBMEN, HL
{

}

SecurityManager::~SecurityManager()
{
    if (helper)
        delete helper;

    Event::unregisterType(etype);
    Event::unregisterType(securityDataRequestEType); // CBMEN, HL
    Event::unregisterType(securityDataResponseEType); // CBMEN, HL

    if (privKey)
        RSA_free(privKey);

    // CBMEN, HL
    if (pubKey)
        RSA_free(pubKey);

    if (onRepositoryDataCallback)
        delete onRepositoryDataCallback;

    // CBMEN, HL, Begin

    if (onWaitingQueueTimerCallback)
        delete onWaitingQueueTimerCallback;

    if (onSecurityDataRequestTimerCallback)
        delete onSecurityDataRequestTimerCallback;

    for (HashMap<string, Pair<string, unsigned char *> >::iterator it = policyCache.begin();
         it != policyCache.end(); it++) {

        if ((*it).second.second)
            free((*it).second.second);
    }

    for (HashMap<string, unsigned char *>::iterator it = capabilityCache.begin();
         it != capabilityCache.end(); it++) {

        if ((*it).second)
            free((*it).second);
    }

    for (HashMap<string, Pair<unsigned char *, unsigned char *> >::iterator it = derivedSharedSecretMap.begin();
         it != derivedSharedSecretMap.end(); it++) {

        if ((*it).second.first)
            free((*it).second.first);

        if ((*it).second.second)
            free((*it).second.second);
    }

    for (HashMap<DataObjectRef, NodeRefList*>::iterator it = waitingForPrivKey.begin();
         it != waitingForPrivKey.end(); it++) {

        if ((*it).second)
            delete (*it).second;
    }

    for (HashMap<DataObjectRef, NodeRefList*>::iterator it = waitingForPubKey.begin();
         it != waitingForPubKey.end(); it++) {

        if ((*it).second)
            delete (*it).second;
    }

    // CBMEN, HL, End
}

bool SecurityManager::init_derived()
{
#define __CLASS__ SecurityManager
    int ret;

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_RECEIVED, onReceivedDataObject);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND, onSendDataObject);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_INCOMING, onIncomingDataObject);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    // CBMEN, AG, Begin

    ret = setEventHandler(EVENT_TYPE_NODE_UPDATED, onNodeUpdated);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    // CBMEN, AG, End

    // CBMEN, HL, Begin

    ret = setEventHandler(EVENT_TYPE_SECURITY_CONFIGURE, onSecurityConfigure);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    // CBMEN, HL, End

    setEventHandler(EVENT_TYPE_DEBUG_CMD, onDebugCmdEvent);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

        /* This function must be called to load crypto algorithms used
         * for signing and verification of certificates. */
        OpenSSL_add_all_algorithms();
        OpenSSL_add_all_digests(); // CBMEN, HL

#if defined(DEBUG)
    /* Load ssl error strings. Needed by ERR_error_string() */
    ERR_load_crypto_strings();
#endif

    etype = registerEventType("SecurityTaskEvent", onSecurityTaskComplete);

    HAGGLE_DBG("Security level is set to %s\n", security_level_names[securityLevel]);

    helper = new SecurityHelper(this, etype);

    if (!helper || !helper->start()) {
        HAGGLE_ERR("Could not create or start security helper\n");
        return false;
    }

    onRepositoryDataCallback = newEventCallback(onRepositoryData); // CBMEN, AG: Moved here since callback depends on SecurityHelper

    // CBMEN, HL, Begin
    onSecurityDataRequestTimerCallback = newEventCallback(onSecurityDataRequestTimer);
    onWaitingQueueTimerCallback = newEventCallback(onWaitingQueueTimer); // MOS
    string filterString = string("SecurityDataResponse=") + kernel->getThisNode()->getIdStr();
    registerEventTypeForFilter(securityDataResponseEType, "SecurityDataResponse", onReceivedSecurityDataResponse,
        filterString.c_str());
    registerEventTypeForFilter(securityDataRequestEType, "SecurityDataRequest", onReceivedSecurityDataRequest, FILTER_SECURITYDATAREQUEST);
    // CBMEN, HL, End

        // BEGIN - MOS
        ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, onSendDataObjectResult);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }

    ret = setEventHandler(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, onSendDataObjectResult);

    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler\n");
        return false;
    }
        // END - MOS

    HAGGLE_DBG("Initialized security manager\n");

    return true;
}

void SecurityManager::onPrepareStartup()
{
    kernel->getDataStore()->readRepository(new RepositoryEntry(getName()), onRepositoryDataCallback);
}

// CBMEN, HL, Begin
void SecurityManager::onStartup() {
    if (isAuthority) {
        HAGGLE_DBG("Adding SecurityDataResponse and SecurityDataRequestAuthority attributes to this node.\n");
    } else {
        HAGGLE_DBG("Adding SecurityDataResponse attribute to this node.\n");
    }

    Attribute response("SecurityDataResponse", kernel->getThisNode()->getIdStr());
    if (kernel->getManager((char*)"ApplicationManager") &&
        ((ApplicationManager *)(kernel->getManager((char *)"ApplicationManager")))->addDefaultInterest(response)) {
        HAGGLE_DBG("Added SecurityDataResponse attribute to this node\n");
    } else {
        HAGGLE_ERR("Couldn't add attribute SecurityDataResponse to this node's attribute interests!\n");
    }

    // Register interests so that SecurityDataRequests/Responses are sent multi-hop
    if (isAuthority) {
        Attribute request("SecurityDataRequestAuthority", kernel->getThisNode()->getIdStr());
        if (kernel->getManager((char*)"ApplicationManager") &&
            ((ApplicationManager *)(kernel->getManager((char *)"ApplicationManager")))->addDefaultInterest(request)) {
            HAGGLE_DBG("Added SecurityDataRequestAuthority attribute to this node\n");
        } else {
            HAGGLE_ERR("Couldn't add attribute SecurityDataRequestAuthority to this node's attribute interests!\n");
        }
    }
}
// CBMEN, HL, End

void SecurityManager::onPrepareShutdown()
{
    Mutex::AutoLocker l(ccbMutex);
    Mutex::AutoLocker l2(certStoreMutex);
    Mutex::AutoLocker l3(dynamicConfigurationMutex);

    // Save our private key
    kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                                 "privKey",
                                                                 RSAKeyToString(privKey, KEY_TYPE_PRIVATE))); // CBMEN, HL
    HAGGLE_DBG("Storing node signing key\n"); // CBMEN, AG

    // CBMEN, HL, Begin
    for (CertificateStore_t::iterator it = certStore.begin();
         it != certStore.end(); it++) {
        kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                                     "cert|"+(*it).second->getSubject()+"|"+(*it).second->getIssuer(), // CBMEN, AG
                                                                     (*it).second->toPEM()));

        HAGGLE_DBG("Storing certificate for node: %s\n", (*it).second->getIssuer().c_str()); // CBMEN, AG
    }

    for (CertificateStore_t::iterator it = caCerts.begin();
         it != caCerts.end(); it++) {
        kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                                     "caCert|"+(*it).second->getSubject()+"|"+(*it).second->getIssuer(), // CBMEN, AG
                                                                     (*it).second->toPEM()));
        HAGGLE_DBG("Storing CA certificate of authority: %s\n", (*it).second->getIssuer().c_str()); // CBMEN, AG
    }

    for (CertificateStore_t::iterator it = myCerts.begin();
         it != myCerts.end(); it++) {
        kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                                     "myCert|"+(*it).second->getSubject()+"|"+(*it).second->getIssuer(), // CBMEN, AG
                                                                     (*it).second->toPEM()));
        HAGGLE_DBG("Storing certificate for this node (%s) issued by: %s\n",
                    (*it).second->getSubject().c_str(), (*it).second->getIssuer().c_str()); // CBMEN, AG
    }
    // CBMEN, HL, End

    // CBMEN, AG, Begin
    for (CertificateStore_t::iterator it = remoteSignedCerts.begin(); it != remoteSignedCerts.end(); it++) {
        kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                                     "remoteSignedCert|"+(*it).second->getSubject()+"|"+(*it).second->getIssuer(),
                                                                     (*it).second->toPEM()));

        HAGGLE_DBG("Storing certificate for node: %s issued by: %s\n",
                    (*it).second->getSubject().c_str(), (*it).second->getIssuer().c_str());
    }
    // CBMEN, AG, End

    // CBMEN, HL, Begin
    // Save our public key
    kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                                 "pubKey", // CBMEN, AG
                                                                 RSAKeyToString(pubKey, KEY_TYPE_PUBLIC)));
    HAGGLE_DBG2("Storing node public key\n"); // CBMEN, AG

    string serialized;
    for (HashMap<string, string>::iterator it = issuedPubKeys.begin();
         it != issuedPubKeys.end(); it++) {
        serialized += ((*it).first + ":" + (*it).second + "|");
        HAGGLE_DBG("Storing issued encryption attribute (by this node acting as authority): %s\n",
                    (*it).first.c_str()); // CBMEN, AG
    }
    kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                                 "issuedEncAttrs", // CBMEN, AG
                                                                 serialized));

    serialized = "";
    for (HashMap<string, string>::iterator it = pubKeys.begin();
         it != pubKeys.end(); it++) {
        serialized += ((*it).first + "|");
        HAGGLE_DBG("Storing encryption attribute: %s\n", (*it).first.c_str()); // CBMEN, AG
    }
    kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                                 "encAttrs", // CBMEN, AG
                                                                 serialized));

    serialized = "";
    for (HashMap<string, string>::iterator it = privKeys.begin();
         it != privKeys.end(); it++) {
        serialized += ((*it).first + "|");
        HAGGLE_DBG("Storing decryption attribute: %s\n", (*it).first.c_str()); // CBMEN, AG
    }
    kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                                 "decAttrs", // CBMEN, AG
                                                                 serialized));

    if (pythonRunning) {

        HAGGLE_DBG("About to shutdown ccb\n");
        charmPersistenceData = bridge::shutdown();
        HAGGLE_DBG("About to stop Python\n");
        if (bridge::stopPython()) {
            HAGGLE_ERR("Error stopping Python!\n");
        } else {
            HAGGLE_DBG("Stopped Python!\n");
        }
        kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                 "charmPersistenceData",
                                                 charmPersistenceData));
        HAGGLE_DBG("Storing charmPersistenceData\n");
    }

    // Save the encrypted public and private key buckets
    unsigned char *buf = NULL;
    size_t len;
    Metadata *m = NULL;

    m = new XMLMetadata("pubKeyBucket");
    if (m) {
        if (hashMapToMetadata(m, "Key", "name", pubKeyBucket)) {
            if (m->getRawAlloc(&buf, &len)) {
                HAGGLE_DBG("Storing encAttrsBucket\n");
                kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                         "encAttrsBucket",
                                                         string((char *)buf)));
                free(buf);
                delete m;
            } else {
                HAGGLE_ERR("Couldn't allocate memory to serialize metadata.\n");
                delete m;
            }
        } else {
            HAGGLE_ERR("Couldn't convert publicKeyBucket to metadata.\n");
            delete m;
        }
    } else {
        HAGGLE_ERR("Couldn't allocate metadata.\n");
    }

    m = new XMLMetadata("privKeyBucket");
    if (m) {
        if (hashMapToMetadata(m, "Key", "name", privKeyBucket)) {
            if (m->getRawAlloc(&buf, &len)) {
                HAGGLE_DBG("Storing decAttrsBucket\n");
                kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                         "decAttrsBucket",
                                                         string((char *)buf)));
                free(buf);
                delete m;
            } else {
                HAGGLE_ERR("Couldn't allocate memory to serialize metadata.\n");
                delete m;
            }
        } else {
            HAGGLE_ERR("Couldn't convert privKeyBucket to metadata.\n");
            delete m;
        }
    } else {
        HAGGLE_ERR("Couldn't allocate metadata.\n");
    }

    // Save the authorities list
    m = new XMLMetadata("authorityNameMap");
    if (m) {
        if (hashMapToMetadata(m, "Authority", "name", authorityNameMap)) {
            if (m->getRawAlloc(&buf, &len)) {
                HAGGLE_DBG("Storing authorityNameMap\n");
                kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                         "authorityNameMap",
                                                         string((char *)buf)));
                free(buf);
                delete m;
            } else {
                HAGGLE_ERR("Couldn't allocate memory to serialize metadata.\n");
                delete m;
            }
        } else {
            HAGGLE_ERR("Couldn't convert authorityNameMap to metadata.\n");
            delete m;
        }
    } else {
        HAGGLE_ERR("Couldn't allocate metadata.\n");
    }

    // Save the certificationACL
    m = new XMLMetadata("certificationACL");
    if (m) {
        if (hashMapToMetadata(m, "Node", "id", certificationACL)) {
            if (m->getRawAlloc(&buf, &len)) {
                HAGGLE_DBG("Storing certificationACL\n");
                kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                         "certificationACL",
                                                         string((char *)buf)));
                free(buf);
                delete m;
            } else {
                HAGGLE_ERR("Couldn't allocate memory to serialize metadata.\n");
                delete m;
            }
        } else {
            HAGGLE_ERR("Couldn't convert certificationACL to metadata.\n");
            delete m;
        }
    } else {
        HAGGLE_ERR("Couldn't allocate metadata.\n");
    }

    // Save the publicKeyACL
    m = new XMLMetadata("publicKeyACL");
    if (m) {
        if (hashMapToMetadata(m, "Attribute", "name", publicKeyACL)) {
            if (m->getRawAlloc(&buf, &len)) {
                HAGGLE_DBG("Storing publicKeyACL\n");
                kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                         "publicKeyACL",
                                                         string((char *)buf)));
                free(buf);
                delete m;
            } else {
                HAGGLE_ERR("Couldn't allocate memory to serialize metadata.\n");
                delete m;
            }
        } else {
            HAGGLE_ERR("Couldn't convert publicKeyACL to metadata.\n");
            delete m;
        }
    } else {
        HAGGLE_ERR("Couldn't allocate metadata.\n");
    }

    // Save the privateKeyACL
    m = new XMLMetadata("privateKeyACL");
    if (m) {
        if (hashMapToMetadata(m, "Attribute", "name", privateKeyACL)) {
            if (m->getRawAlloc(&buf, &len)) {
                HAGGLE_DBG("Storing privateKeyACL\n");
                kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                         "privateKeyACL",
                                                         string((char *)buf)));
                free(buf);
                delete m;
            } else {
                HAGGLE_ERR("Couldn't allocate memory to serialize metadata.\n");
                delete m;
            }
        } else {
            HAGGLE_ERR("Couldn't convert privateKeyACL to metadata.\n");
            delete m;
        }
    } else {
        HAGGLE_ERR("Couldn't allocate metadata.\n");
    }

    // Save the role shared secrets
    for (HashMap<string, string>::iterator it = rawRoleSharedSecretMap.begin(); it != rawRoleSharedSecretMap.end(); it++) {
        HAGGLE_DBG("Storing shared secret for role: %s\n", (*it).first.c_str());
    }

    m = new XMLMetadata("rawRoleSharedSecretMap");
    if (m) {
        if (hashMapToMetadata(m, "Role", "name", rawRoleSharedSecretMap)) {
            if (m->getRawAlloc(&buf, &len)) {
                HAGGLE_DBG("Storing rawRoleSharedSecretMap\n");
                kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                         "rawRoleSharedSecretMap",
                                                         string((char *)buf)));
                free(buf);
                delete m;
            } else {
                HAGGLE_ERR("Couldn't allocate memory to serialize metadata.\n");
                delete m;
            }
        } else {
            HAGGLE_ERR("Couldn't convert rawRoleSharedSecretMap to metadata.\n");
            delete m;
        }
    } else {
        HAGGLE_ERR("Couldn't allocate metadata.\n");
    }

    // Save the node shared secrets
    for (HashMap<string, string>::iterator it = rawNodeSharedSecretMap.begin(); it != rawNodeSharedSecretMap.end(); it++) {
        HAGGLE_DBG("Storing shared secret for node: %s\n", (*it).first.c_str());
    }

    m = new XMLMetadata("rawNodeSharedSecretMap");
    if (m) {
        if (hashMapToMetadata(m, "Node", "id", rawNodeSharedSecretMap)) {
            if (m->getRawAlloc(&buf, &len)) {
                HAGGLE_DBG("Storing rawNodeSharedSecretMap\n");
                kernel->getDataStore()->insertRepository(new RepositoryEntry(getName(),
                                                         "rawNodeSharedSecretMap",
                                                         string((char *)buf)));
                free(buf);
                delete m;
            } else {
                HAGGLE_ERR("Couldn't allocate memory to serialize metadata.\n");
                delete m;
            }
        } else {
            HAGGLE_ERR("Couldn't convert rawNodeSharedSecretMap to metadata.\n");
            delete m;
        }
    } else {
        HAGGLE_ERR("Couldn't allocate metadata.\n");
    }
    // CBMEN, HL, End

    signalIsReadyForShutdown();
}

void SecurityManager::onShutdown()
{
    if (helper) {
        HAGGLE_DBG("Stopping security helper...\n");
        helper->stop();
    }
    unregisterWithKernel();
}

void SecurityManager::onRepositoryData(Event *e)
{
    RepositoryEntryRef re;

    onRepositoryDataCalled = true;

    if (!e->getData()) {
        if (securityConfigured) {
            postConfiguration();
        }
        return;
    }

    HAGGLE_DBG("Got repository callback\n");

    DataStoreQueryResult *qr = static_cast < DataStoreQueryResult * >(e->getData());

    if (qr->countRepositoryEntries() == 0) {
        HAGGLE_DBG("No repository entries!\n");
    }

    while ((re = qr->detachFirstRepositoryEntry())) {
        if (strcmp(re->getKey(), "privKey") == 0) { // CBMEN, AG

            // Just to make sure
            if (privKey)
                RSA_free(privKey);

            privKey = stringToRSAKey(re->getValueStr(), KEY_TYPE_PRIVATE);

            HAGGLE_DBG("Read my own private key from repository\n");
        // CBMEN, HL, Begin
        } else if(strcmp(re->getKey(), "pubKey") == 0) { // CBMEN, AG

            if (pubKey)
                RSA_free(pubKey);

            pubKey = stringToRSAKey(re->getValueStr(), KEY_TYPE_PUBLIC);
            HAGGLE_DBG("Read my own public key from repository\n");
        } else if (strcmp(re->getKey(), "encAttrs") == 0) { // CBMEN, AG
            string serialized = re->getValueStr();
            size_t start, end;
            start = end = 0;
            while ((end = serialized.find_first_of("|", end+1)) != string::npos) {
                string key = serialized.substr(start, end-start);
                HAGGLE_DBG("Read encryption attribute: %s\n", key.c_str()); // CBMEN, AG
                pubKeys.insert(make_pair(key, ""));
                start = end+1;
            }
        } else if (strcmp(re->getKey(), "decAttrs") == 0) { // CBMEN, AG
            string serialized = re->getValueStr();
            size_t start, end;
            start = end = 0;
            while ((end = serialized.find_first_of("|", end+1)) != string::npos) {
                string key = serialized.substr(start, end-start);
                HAGGLE_DBG("Read decryption attribute: %s\n", key.c_str()); // CBMEN, AG
                privKeys.insert(make_pair(key, ""));
                start = end+1;
            }
        } else if (strcmp(re->getKey(), "issuedEncAttrs") == 0) { // CBMEN, AG
            string serialized = re->getValueStr();
            size_t start, end;
            start = end = 0;
            while ((end = serialized.find_first_of(":", end+1)) != string::npos) {
                string key = serialized.substr(start, end-start);
                start = end+1;
                end = serialized.find_first_of("|", end+1);
                string val = serialized.substr(start, end-start);
                HAGGLE_DBG("Read issued encryption attribute: %s\n Value: %s\n", key.c_str(), val.c_str()); // CBMEN, AG
                if (issuedPubKeys.find(key) == issuedPubKeys.end())
                    issuedPubKeys.insert(make_pair(key, val));
                start = end+1;
            }
        } else if (strcmp(re->getKey(), "encAttrsBucket") == 0) {
            string tmp = re->getValueStr();
            Metadata *m = new XMLMetadata("pubKeyBucket");

            if (m) {
                if (m->initFromRaw((unsigned char *)tmp.c_str(), tmp.length())) {
                    pubKeyBucket = metadataToHashMap(m, "Key", "name");
                    delete m;
                } else {
                    HAGGLE_ERR("Couldn't init metadata from raw repository data.\n");
                    delete m;
                }
            } else {
                HAGGLE_ERR("Couldn't allocate memory for metadata.\n");
            }

            for (HashMap<string, string>::iterator it = pubKeyBucket.begin(); it != pubKeyBucket.end(); it++) {
                HAGGLE_DBG("Read encrypted encryption attribute: %s\n", (*it).first.c_str());
            }
        } else if (strcmp(re->getKey(), "decAttrsBucket") == 0) {
            string tmp = re->getValueStr();
            Metadata *m = new XMLMetadata("privKeyBucket");

            if (m) {
                if (m->initFromRaw((unsigned char *)tmp.c_str(), tmp.length())) {
                    privKeyBucket = metadataToHashMap(m, "Key", "name");
                    delete m;
                } else {
                    HAGGLE_ERR("Couldn't init metadata from raw repository data.\n");
                    delete m;
                }
            } else {
                HAGGLE_ERR("Couldn't allocate memory for metadata.\n");
            }

            for (HashMap<string, string>::iterator it = privKeyBucket.begin(); it != privKeyBucket.end(); it++) {
                HAGGLE_DBG("Read encrypted decryption attribute: %s\n", (*it).first.c_str());
            }
        } else if (strcmp(re->getKey(), "charmPersistenceData") == 0) {
            charmPersistenceData = re->getValueStr();
            HAGGLE_DBG("Loaded charm persistence data from data store.\n");
            HAGGLE_DBG("Updating charmPersistenceData\n");
        } else if (strcmp(re->getKey(), "authorityNameMap") == 0) {
            tmpRepositoryData.insert(make_pair(re->getKey(), re->getValueStr()));
        } else if (strcmp(re->getKey(), "certificationACL") == 0) {
            tmpRepositoryData.insert(make_pair(re->getKey(), re->getValueStr()));
        } else if (strcmp(re->getKey(), "publicKeyACL") == 0) {
            tmpRepositoryData.insert(make_pair(re->getKey(), re->getValueStr()));
        } else if (strcmp(re->getKey(), "privateKeyACL") == 0) {
            tmpRepositoryData.insert(make_pair(re->getKey(), re->getValueStr()));
        } else if (strcmp(re->getKey(), "rawRoleSharedSecretMap") == 0) {
            tmpRepositoryData.insert(make_pair(re->getKey(), re->getValueStr()));
        } else if (strcmp(re->getKey(), "rawNodeSharedSecretMap") == 0) {
            tmpRepositoryData.insert(make_pair(re->getKey(), re->getValueStr()));

        // CBMEN, HL, End
        // CBMEN, AG, Begin
        } else if (strncmp(re->getKey(), "cert", strlen("cert")) == 0) {
            CertificateRef c = Certificate::fromPEM(re->getValueStr());
            if (c) {
                storeCertificate(c->getIssuer(), c, certStore, true);
                if (c->isVerified()) {
                    HAGGLE_DBG("Read verified certificate for subject %s issued by %s from repository\n",
                               c->getSubject().c_str(), c->getIssuer().c_str());
                } else {
                    HAGGLE_DBG("Read unverified certificate for subject %s issued by %s from repository\n",
                                c->getSubject().c_str(), c->getIssuer().c_str());
                }
                if ((c->getSubject() == c->getIssuer()) &&
                    (c->getIssuer() == kernel->getThisNode()->getIdStr())) {
                    myCert = c;
                    HAGGLE_DBG("Read this node's self-signed certificate\n");
                }
            } else {
                HAGGLE_ERR("Could not read certificate %s from repository\n", re->getKey());
            }
        } else if (strncmp(re->getKey(), "caCert", strlen("caCert")) == 0) {
            CertificateRef c = Certificate::fromPEM(re->getValueStr());
            if (c) {
                storeCertificate(c->getIssuer(), c, caCerts, true);
                HAGGLE_DBG("Read CA certificate for %s from repository\n", c->getIssuer().c_str());
            } else {
                HAGGLE_ERR("Could not read CA certificate %s from repository\n", re->getKey());
            }
        } else if (strncmp(re->getKey(), "myCert", strlen("myCert")) == 0) {
            CertificateRef c = Certificate::fromPEM(re->getValueStr());
            if (c) {
                storeCertificate(c->getIssuer(), c, myCerts, true);
                if (c->isVerified()) {
                    HAGGLE_DBG("Read this node's verified certificate signed by %s from repository\n", c->getIssuer().c_str());
                } else {
                    HAGGLE_DBG("Read this node's unverified certificate signed by %s from repository\n", c->getIssuer().c_str());
                }
            } else {
                HAGGLE_ERR("Could not read this node's signed certificate %s from repository\n", re->getKey());
            }
        } else if (strncmp(re->getKey(), "remoteSignedCert", strlen("remoteSignedCert")) == 0) {
            CertificateRef c = Certificate::fromPEM(re->getValueStr());
            if (c) {
                storeCertificate(c->getSubject()+"|"+c->getIssuer(), c, remoteSignedCerts, true);
                if (c->isVerified()) {
                    HAGGLE_DBG("Read verified certificate for %s issued by authority %s from repository\n",
                               c->getSubject().c_str(), c->getIssuer().c_str());
                } else {
                    HAGGLE_DBG("Read unverified certificate for %s issued by authority %s from repository\n",
                               c->getSubject().c_str(), c->getIssuer().c_str());
                }
            } else {
                HAGGLE_ERR("Could not read remote node's certificate %s from repository\n", re->getKey());
            }
        }
    }
    // CBMEN, AG, End

    if (securityConfigured) {
        postConfiguration();
    }

    delete qr;
}

void SecurityManager::postConfiguration() {

    if (!securityConfigured || !onRepositoryDataCalled) {
        HAGGLE_ERR("postConfiguration must be called after onConfig and onRepositoryData!\n");
        return;
    }

    HAGGLE_DBG("In postConfiguration!\n");

    if (!pubKey || !privKey) {
        HAGGLE_DBG("No repository entries and no keypair found, generating new certificate and keypair\n");
        helper->addTask(new SecurityTask(SECURITY_TASK_GENERATE_CERTIFICATE));
        // Delay signalling that we are ready for startup until we get the
        // task result indicating our certificate is ready.
        return;
    } else  {
        HAGGLE_DBG("No repository entries, keypair already present so not generating a new one!\n");
    }

    for (HashMap<string, string>::iterator pit = tmpRepositoryData.begin(); pit != tmpRepositoryData.end(); pit++) {
        if (strcmp((*pit).first.c_str(), "authorityNameMap") == 0) {
            HashMap<string, string> theMap;
            string tmp = (*pit).second;
            Metadata *m = new XMLMetadata("authorityNameMap");

            if (m) {
                if (m->initFromRaw((unsigned char *)tmp.c_str(), tmp.length())) {
                    theMap = metadataToHashMap(m, "Authority", "name");
                    delete m;
                } else {
                    HAGGLE_ERR("Couldn't init metadata from raw repository data.\n");
                    delete m;
                }
            } else {
                HAGGLE_ERR("Couldn't allocate memory for metadata.\n");
            }

            string name, id;
            HashMap<string, string>::iterator it;

            for (HashMap<string, string>::iterator mit = theMap.begin(); mit != theMap.end(); mit++) {

                name = (*mit).first;
                id = (*mit).second;

                it = authorityNameMap.find(name);
                if (it != authorityNameMap.end()) {
                    HAGGLE_DBG("Already have id [%s] for authority %s from configuration, going to replace with id [%s] from repository.\n", (*it).second.c_str(), name.c_str(), id.c_str());
                    authorityNameMap.erase(it);
                } else {
                    HAGGLE_DBG("Loaded authority %s [id=%s] from repository.\n", name.c_str(), id.c_str());
                }

                authorityNameMap.insert(make_pair(name, id));
            }
        } else if (strcmp((*pit).first.c_str(), "certificationACL") == 0) {
            HashMap<string, string> theMap;
            string tmp = (*pit).second;
            Metadata *m = new XMLMetadata("certificationACL");

            if (m) {
                if (m->initFromRaw((unsigned char *)tmp.c_str(), tmp.length())) {
                    theMap = metadataToHashMap(m, "Node", "id");
                    delete m;
                } else {
                    HAGGLE_ERR("Couldn't init metadata from raw repository data.\n");
                    delete m;
                }
            } else {
                HAGGLE_ERR("Couldn't allocate memory for metadata.\n");
            }

            string id;
            HashMap<string, string>::iterator it;

            for (HashMap<string, string>::iterator mit = theMap.begin(); mit != theMap.end(); mit++) {

                id = (*mit).first;

                it = certificationACL.find(id);
                if (it != certificationACL.end()) {
                    HAGGLE_DBG("Node %s was already authorized for certification based on configuration.\n", id.c_str());
                } else {
                    certificationACL.insert(make_pair(id, ""));
                    HAGGLE_DBG("Adding Node %s to the list of nodes which can be certified by this authority.\n", id.c_str());
                }

            }
        } else if (strcmp((*pit).first.c_str(), "publicKeyACL") == 0) {
            HashMap<string, string> theMap;
            string tmp = (*pit).second;
            Metadata *m = new XMLMetadata("publicKeyACL");

            if (m) {
                if (m->initFromRaw((unsigned char *)tmp.c_str(), tmp.length())) {
                    theMap = metadataToHashMap(m, "Attribute", "name");
                    delete m;
                } else {
                    HAGGLE_ERR("Couldn't init metadata from raw repository data.\n");
                    delete m;
                }
            } else {
                HAGGLE_ERR("Couldn't allocate memory for metadata.\n");
            }

            string entry;
            HashMap<string, string>::iterator it;

            for (HashMap<string, string>::iterator mit = theMap.begin(); mit != theMap.end(); mit++) {

                entry = (*mit).first;

                it = publicKeyACL.find(entry);
                if (it != publicKeyACL.end()) {
                    HAGGLE_DBG("publicKeyACL entry %s was already present based on on configuration.\n", entry.c_str());
                } else {
                    publicKeyACL.insert(make_pair(entry, ""));
                    HAGGLE_DBG("Adding publicKeyACL entry %s.\n", entry.c_str());
                }

            }
        } else if (strcmp((*pit).first.c_str(), "privateKeyACL") == 0) {
            HashMap<string, string> theMap;
            string tmp = (*pit).second;
            Metadata *m = new XMLMetadata("privateKeyACL");

            if (m) {
                if (m->initFromRaw((unsigned char *)tmp.c_str(), tmp.length())) {
                    theMap = metadataToHashMap(m, "Attribute", "name");
                    delete m;
                } else {
                    HAGGLE_ERR("Couldn't init metadata from raw repository data.\n");
                    delete m;
                }
            } else {
                HAGGLE_ERR("Couldn't allocate memory for metadata.\n");
            }

            string entry;
            HashMap<string, string>::iterator it;

            for (HashMap<string, string>::iterator mit = theMap.begin(); mit != theMap.end(); mit++) {

                entry = (*mit).first;

                it = privateKeyACL.find(entry);
                if (it != privateKeyACL.end()) {
                    HAGGLE_DBG("privateKeyACL entry %s was already present based on on configuration.\n", entry.c_str());
                } else {
                    privateKeyACL.insert(make_pair(entry, ""));
                    HAGGLE_DBG("Adding privateKeyACL entry %s.\n", entry.c_str());
                }

            }
        } else if (strcmp((*pit).first.c_str(), "rawRoleSharedSecretMap") == 0) {
            HashMap<string, string> theMap;
            string tmp = (*pit).second;
            Metadata *m = new XMLMetadata("rawRoleSharedSecretMap");

            if (m) {
                if (m->initFromRaw((unsigned char *)tmp.c_str(), tmp.length())) {
                    theMap = metadataToHashMap(m, "Role", "name");
                    delete m;
                } else {
                    HAGGLE_ERR("Couldn't init metadata from raw repository data.\n");
                    delete m;
                }
            } else {
                HAGGLE_ERR("Couldn't allocate memory for metadata.\n");
            }

            string role, shared_secret;
            HashMap<string, string>::iterator it;

            for (HashMap<string, string>::iterator mit = theMap.begin(); mit != theMap.end(); mit++) {

                role = (*mit).first;
                shared_secret = (*mit).second;

                HAGGLE_DBG("Loading shared secret for role %s from repository.\n", role.c_str());

                it = rawRoleSharedSecretMap.find(role);
                if (it != rawRoleSharedSecretMap.end()) {
                    HAGGLE_DBG("Already have shared secret for role %s from configuration, going to replace with shared secret from repository.\n", role.c_str());
                    rawRoleSharedSecretMap.erase(it);
                }

                rawRoleSharedSecretMap.insert(make_pair(role, shared_secret));
            }
        } else if (strcmp((*pit).first.c_str(), "rawNodeSharedSecretMap") == 0) {
            HashMap<string, string> theMap;
            string tmp = (*pit).second;
            Metadata *m = new XMLMetadata("rawNodeSharedSecretMap");

            if (m) {
                if (m->initFromRaw((unsigned char *)tmp.c_str(), tmp.length())) {
                    theMap = metadataToHashMap(m, "Node", "id");
                    delete m;
                } else {
                    HAGGLE_ERR("Couldn't init metadata from raw repository data.\n");
                    delete m;
                }
            } else {
                HAGGLE_ERR("Couldn't allocate memory for metadata.\n");
            }

            string id, shared_secret;
            HashMap<string, string>::iterator it;

            for (HashMap<string, string>::iterator mit = theMap.begin(); mit != theMap.end(); mit++) {

                id = (*mit).first;
                shared_secret = (*mit).second;

                // AESKey = (unsigned char *) malloc(16 * sizeof(unsigned char));
                HAGGLE_DBG("Loading shared secret for node %s from repository.\n", id.c_str());

                it = rawNodeSharedSecretMap.find(id);
                if (it != rawNodeSharedSecretMap.end()) {
                    HAGGLE_DBG("Already have shared secret for node %s from configuration, going to replace with shared secret from repository.\n", id.c_str());
                    rawNodeSharedSecretMap.erase(it);
                }

                rawNodeSharedSecretMap.insert(make_pair(id, shared_secret));
            }
        }
    }

    // CBMEN, HL, Begin
    // Make sure our self-signed certificate is in caCerts if we're an authority.
    // This is needed if we were previously not configured as an authority,
    // but have now been reconfigured to act as an authority.
    // Otherwise, we will not be able to verify certificates that we sign.
    if (isAuthority) {
        if (!retrieveCertificate(kernel->getThisNode()->getIdStr(), caCerts)) {
            HAGGLE_DBG("Our own self-signed certificate was not present in caCerts, adding.\n");
            storeCertificate(myCert->getIssuer(), myCert, caCerts, true);
        }
    }

    // Verify our own self-signed certificate, and mark it as verified.
    // This lets two applications on the same node send objects to each
    // other that can have their signature verified, even if this node
    // does not have any authorities configured. This would not be possible
    // if our own self-signed certificate was not verified.

    if (!myCert->isVerified())
        myCert->verifySignature(pubKey);

    // Verify all authority certificates
    {
        Mutex::AutoLocker l(certStoreMutex);
        for (CertificateStore_t::iterator it = caCerts.begin(); it != caCerts.end(); it++) {
            CertificateRef& cert = (*it).second;
            HAGGLE_DBG("Will try to verify CA certificate of %s\n", cert->getIssuer().c_str());
            helper->addTask(new SecurityTask(SECURITY_TASK_VERIFY_AUTHORITY_CERTIFICATE, NULL, cert));
        }
    }

    // TODO: Figure out appropriate place for this
    // // If we're an authority, do an eager request for all keys.
    // if (isAuthority) {
    //     if (helper->startPython()) {
    //         helper->requestAllKeys(authorityName, kernel->getThisNode()->getIdStr(), true);
    //         helper->requestAllKeys(authorityName, kernel->getThisNode()->getIdStr(), false);
    //     }
    // }
    // CBMEN, HL, End

    // cleanup
    tmpRepositoryData = HashMap<string, string>();

    signalIsReadyForStartup();
}

bool SecurityManager::storeCertificate(const string key, CertificateRef& cert, CertificateStore_t& store, bool replace) // CBMEN, HL
{
    Mutex::AutoLocker l(certStoreMutex);

    // CBMEN, HL - Remove as we want to store multiple times
    //if (cert->isStored())
        //return false;
    //cert->setStored();

    CertificateStore_t::iterator it = store.find(key); // CBMEN, HL

    if (it != store.end()) { // CBMEN, HL
        if (replace) {
            store.erase(it); // CBMEN, HL
        } else {
            return false;
        }
    }

    store.insert(make_pair(key, cert));

    return true;
}

CertificateRef SecurityManager::retrieveCertificate(const string key, CertificateStore_t& store) // CBMEN, HL
{
    Mutex::AutoLocker l(certStoreMutex);

    CertificateStore_t::iterator it = store.find(key); // CBMEN, HL

    if (it == store.end()) // CBMEN, HL
        return NULL;

    return (*it).second;
}

void SecurityManager::onDebugCmdEvent(Event *e)
{
    if (e->getDebugCmd()->getType() == DBG_CMD_PRINT_CERTIFICATES) {
        printCertificates();
    } else if (e->getDebugCmd()->getType() == DBG_CMD_OBSERVE_CERTIFICATES) {
        observeCertificatesAsMetadata(e->getDebugCmd()->getMsg());
    }
}

void SecurityManager::printCertificates()
{
    Mutex::AutoLocker l(certStoreMutex);
    int n = 0;

    HAGGLE_DBG("[Certificate Store]:\n");

    for (CertificateStore_t::iterator it = certStore.begin();
         it != certStore.end(); it++) {
        CertificateRef& cert = (*it).second;
        HAGGLE_DBG("# %d subject=%s issuer=%s\n",
               n++, cert->getSubject().c_str(), cert->getIssuer().c_str());
        HAGGLE_DBG("%s\n", cert->toString().c_str());
    }

}

void SecurityManager::observeCertificatesAsMetadata(string key) {
    Mutex::AutoLocker l(certStoreMutex);

    CertificateStore_t *certStores[] = {&myCerts, &caCerts, &remoteSignedCerts, &certStore};
    const char *names[] = {"myCerts", "caCerts", "remoteSignedCerts", "certStore"};

    Metadata *m = new XMLMetadata(key);

    if (!m) {
        return;
    }

    for (size_t i = 0; i < 4; i++) {
        Metadata *dm = m->addMetadata(names[i]);
        if (dm) {
            for (CertificateStore_t::iterator it = certStores[i]->begin(); it != certStores[i]->end(); it++) {
                Metadata *dmm = dm->addMetadata("Certificate");
                CertificateRef cert = (*it).second;
                if (dmm) {
                    dmm->setParameter("subject", cert->getSubject());
                    dmm->setParameter("issuer", cert->getIssuer());
                }
            }
        }
    }

    kernel->addEvent(new Event(EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, m));
}

/*
 This function is called after the SecurityHelper thread finished a task.
 The Security manager may act on any of the results if it wishes.

 */
void SecurityManager::onSecurityTaskComplete(Event *e)
{
    if (!e || !e->hasData())
        return;

        SecurityTask *task = static_cast<SecurityTask *>(e->getData());

        switch (task->type) {
            case SECURITY_TASK_GENERATE_CERTIFICATE:

                if (task->cert) {

                    HAGGLE_DBG("Signing our own certificate\n"); // CBMEN, HL, AG
                    if (!task->cert->sign(task->privKey)) { // CBMEN, HL
                        HAGGLE_ERR("Signing of certificate failed\n");
                    }
                    if (isAuthority) { // CBMEN, HL, AG
                        HAGGLE_DBG("Storing our CA certificate, as we're an authority\n"); // CBMEN, HL, AG
                        storeCertificate(task->cert->getSubject(), task->cert, caCerts); // CBMEN, HL, AG
                    }

                    storeCertificate(task->cert->getSubject(), task->cert, certStore); // CBMEN, HL, AG
                }

                if (task->cert->getSubject() == kernel->getThisNode()->getIdStr()) {
                    HAGGLE_DBG("Certificate generated for this node\n"); // CBMEN, AG

                    /* Save our private key and our certificate */
                    privKey = task->privKey;
                    myCert = task->cert;
                    pubKey = RSAPublicKey_dup(task->cert->getPubKey()); // CBMEN, HL
                    storeCertificate(task->cert->getIssuer(), task->cert, myCerts); // CBMEN, HL, AG
                    if (!task->cert->isVerified())
                        task->cert->verifySignature(pubKey);
                    // signalIsReadyForStartup(); CBMEN, HL - We want to do the things at the end of postConfiguration()
                    postConfiguration();
                }
                break;

            case SECURITY_TASK_VERIFY_CERTIFICATE:

                // CBMEN, AG, Begin
                if (task->cert) {
                    if (task->cert->isVerified()) {
                        HAGGLE_DBG("Certificate for %s issued by %s was verified\n",
                                   task->cert->getSubject().c_str(), task->cert->getIssuer().c_str());



                    } else {
                        HAGGLE_DBG("Invalid certificate for %s from authority %s\n",
                                   task->cert->getSubject().c_str(), task->cert->getIssuer().c_str());
                    }
                }
                // CBMEN, AG, End
                /*
                 if (task->cert && task->cert->isVerified()) {
                    printCertificates();
                 }
                */
                break;

            case SECURITY_TASK_VERIFY_DATAOBJECT:
                /*
                 NOTE:
                 Here is the possibility to generate a EVENT_TYPE_DATAOBJECT_VERIFIED
                 event even if the data object had a bad signature. In that case, the
                 Data manager will remove the data object from the bloomfilter so that
                 it can be received again (hopefully with a valid signature the next time).
                 However, this means that also the data object with the bad signature
                 can be received once more, in worst case in a never ending circle.

                 Perhaps the best solution is to hash both the data object ID and the
                 signature (if available) into a node's bloomfilter?
                 */
                if (task->dObj->hasValidSignature()) {
                    HAGGLE_DBG("DataObject %s has a valid signature!\n",
                               task->dObj->getIdStr());
                    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_VERIFIED,
                                               task->dObj));
                } else {
                    HAGGLE_ERR("DataObject %s has an unverifiable signature!\n",
                               task->dObj->getIdStr());
            kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_DELETED, task->dObj)); // MOS
                }
                break;

            case SECURITY_TASK_SIGN_DATAOBJECT:

                // CBMEN, AG, Begin

                /*
                 NOTE:
                 Here is the possibility to generate EVENT_TYPE_DATAOBJECT_SEND
                 event even if the data object signing had failed. In that case, the
                 Protocol Manager will send a data object with an invalid signature.
                 However, this means that the data object will be dropped during
                 signature verification at a remote node.

                 Perhaps the best solution is not to forward the data object if the
                 signing failed.
                 */

                if (task->dObj->hasValidSignature()) {
                    HAGGLE_DBG("DataObject %s has valid signature!\n", task->dObj->getIdStr());

                    NodeRefList targets = *(task->targets);
                    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND,
                                               task->dObj, targets));
                } else {
                    HAGGLE_DBG("DataObject %s has invalid signature!\n",
                                task->dObj->getIdStr());
                }

                // CBMEN, AG, End

                break;

            // CBMEN, HL, Begin

            case SECURITY_TASK_GENERATE_CAPABILITY:
            break;

            case SECURITY_TASK_USE_CAPABILITY:
            break;

            case SECURITY_TASK_ENCRYPT_DATAOBJECT:
                if (task->dObj->getABEStatus() >= DataObject::ABE_ENCRYPTION_DONE) {
            //HAGGLE_DBG("Successfully encrypted Data Object [%s].\n", task->dObj->getIdStr());
          HAGGLE_DBG("Successfully encrypted Data Object [%s] filename=%s\n", task->dObj->getIdStr(),task->dObj->getFileName().c_str());

                    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_VERIFIED, task->dObj));
                } else {
                    HAGGLE_ERR("Encryption failed for Data Object [%s].\n", task->dObj->getIdStr());
                }

            break;

            case SECURITY_TASK_DECRYPT_DATAOBJECT:
                if (task->dObj->getABEStatus() == DataObject::ABE_DECRYPTION_DONE) {
                    HAGGLE_DBG("Successfully decrypted Data Object [%s].\n", task->dObj->getIdStr());

            task->dObj->setPersistent(true); // MOS
            kernel->getDataStore()->deleteDataObject (task->dObj, true, false); // MOS - use shouldReport = false to keep files
            kernel->getDataStore()->insertDataObject (task->dObj, NULL); // MOS - insert updated data object and inherit files

                    NodeRefList targets = *(task->targets);
                    if(kernel->firstClassApplications()) kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_TO_APP, task->dObj, targets)); // MOS - only in case of fca

                    pendingDecryption.erase(task->dObj->getIdStr()); // CBMEN, AG
                } else {
                    HAGGLE_ERR("Decryption failed for Data Object [%s].\n", task->dObj->getIdStr());
                }
            break;

            case SECURITY_TASK_SEND_SECURITY_DATA_REQUEST:
            break;

            case SECURITY_TASK_HANDLE_SECURITY_DATA_REQUEST:
            break;

            case SECURITY_TASK_HANDLE_SECURITY_DATA_RESPONSE:
            break;

            case SECURITY_TASK_VERIFY_AUTHORITY_CERTIFICATE:
                if (task->cert) {
                    if (task->cert->isVerified()) {
                        HAGGLE_DBG("Authority certificate for node %s was verified!\n", task->cert->getIssuer().c_str());
                    } else {
                        HAGGLE_DBG("Invalid authority certificate for node %s\n", task->cert->getIssuer().c_str());
                    }
                }
            break;

            case SECURITY_TASK_DECRYPT_ATTRIBUTE_BUCKETS:
            break;
            // CBMEN, HL, End

            // CBMEN, AG, Begin
            case SECURITY_TASK_UPDATE_WAITING_QUEUES:
                break;
            // CBMEN, AG, End
        }
    delete task;
}

void SecurityManager::onReceivedDataObject(Event *e)
{
	if (!e || !e->hasData())
		return;
	
	DataObjectRef& dObj = e->getDataObject();
	
	if (!dObj)
		return;

	if (dObj->isDuplicate()) {
		HAGGLE_DBG("Data object [%s] is a duplicate, ignoring\n", dObj->getIdStr());		
		return;
	}

	InterfaceRef iface = dObj->getRemoteInterface();
	bool isApplication = iface && iface->isApplication();

	/* JJ source coding set  publisher */
	if(isApplication) {
	    dObj->setSignee(this->getKernel()->getThisNode()->getIdStr());
	}

	if ((isApplication && !dObj->getAttribute(HAGGLE_ATTR_CONTROL_NAME))) {
	  HAGGLE_DBG("Received data object %s, which was added by an application.\n", dObj->getIdStr());
	}
    
    // CBMEN, HL, Begin - Handle eager encryption
    const Attribute *policy = dObj->getAttribute(DATAOBJECT_ATTRIBUTE_ABE_POLICY);
    const Attribute *capability = dObj->getAttribute(DATAOBJECT_ATTRIBUTE_ABE);

    if (securityLevel >= SECURITY_LEVEL_VERY_HIGH || encryptFilePayload) { // MOS
        if (policy) {
            if (capability) {
                HAGGLE_DBG("Data object [%s] already has capability set, not generating a new one.\n", dObj->getIdStr()); // CBMEN, AG
            } else if (!dObj->hasData()) {
                HAGGLE_DBG("Data object [%s] has no data, not generating a capability.\n", dObj->getIdStr()); // CBMEN, AG
            } else if (dObj->getIsForLocalApp()) {
                HAGGLE_DBG("Data object [%s] is for local app, not generating a capability.\n", dObj->getIdStr()); // CBMEN, AG
            } else if (isApplication) {
                DataObjectRef newDObj = dObj->copy();
                dObj->setStored(true); // So that we don't delete the plaintext, which newDObj needs.
                HAGGLE_DBG("Enqueuing capability generation task for data object [%s].\n", dObj->getIdStr()); // CBMEN, AG
                newDObj->setABEStatus(DataObject::ABE_ENCRYPTION_NEEDED);
                helper->addTask(new SecurityTask(SECURITY_TASK_GENERATE_CAPABILITY, newDObj, NULL));
        return; // Return here so that we do verification on it later, not right now
            } else {
                HAGGLE_DBG("Received an object from a remote node that has a policy but no capability. This should never happen. Dropping data object [%s].\n", dObj->getIdStr()); // CBMEN, AG
                return;
            }
        } else {
            HAGGLE_DBG("Data object [%s] has no access policy defined. No encryption will be performed.\n", dObj->getIdStr()); // CBMEN, AG
        }
    }
    // CBMEN, HL, End

    // Check if the data object's signature should be verified. Otherwise, generate the
    // verified event immediately.
    // MOS - make sure to cover data objects without signature from peers

    // CBMEN, HL - Accept security data requests here.
    // We must do this because it's possible that the node only has an unsigned
    // certificate at this point.
    if (dObj->getAttribute("SecurityDataRequest") && !dObj->isNodeDescription()) {
        HAGGLE_DBG("Accepting Security Data Request object \n");
        dObj->setSignatureStatus(DataObject::SIGNATURE_VALID);
        kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_VERIFIED, dObj));
    } else if (dObj->getAttribute("SecurityDataResponse") && !dObj->isNodeDescription()) {
        // Delete data if we got a response that's not for us
        // if (dObj->getAttribute("SecurityDataResponse")->getValue() != kernel->getThisNode()->getIdStr()) {
        //     dObj->setStored(false);
        //     dObj->deleteData();
        //     kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_DELETED, dObj));
        // } else {
        // Just verify so we can forward
            dObj->setSignatureStatus(DataObject::SIGNATURE_VALID);
            kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_VERIFIED, dObj));
        // }
    } else if (!dObj->isSigned() && iface && !isApplication &&
        ((dObj->isNodeDescription() && !signNodeDescriptions) ||
         (securityLevel <= SECURITY_LEVEL_MEDIUM))) {
            HAGGLE_DBG("Accepting data object %s without signature!\n", dObj->getIdStr());
        dObj->setSignatureStatus(DataObject::SIGNATURE_VALID);
        kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_VERIFIED, dObj));
    } else if (!dObj->isSigned() && iface && !isApplication &&
        ((dObj->isNodeDescription() && signNodeDescriptions) ||
         (securityLevel >= SECURITY_LEVEL_HIGH))) {
            HAGGLE_DBG("DataObject %s has no signature!\n", dObj->getIdStr());
        kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_DELETED, dObj)); // MOS
    } else if (dObj->signatureIsUnverified() &&
        ((dObj->isNodeDescription() && signNodeDescriptions) ||
         (securityLevel >= SECURITY_LEVEL_HIGH))) {
        helper->addTask(new SecurityTask(SECURITY_TASK_VERIFY_DATAOBJECT, dObj));
    } else {
        kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_VERIFIED, dObj));
    }
}

/*
    Check incoming data objects for two reasons:
    1) whether they have an embedded certificate, in which case we verify
    it and add it to our store in case it is not already there.
    2) sign any data objects that were generated by local applications.
 */
void SecurityManager::onIncomingDataObject(Event *e)
{
    DataObjectRef dObj;

    if (!e || !e->hasData())
        return;

    dObj = e->getDataObject();

    // JJOY:
    if(!dObj) {
        HAGGLE_DBG("Data object is null\n");
        return;
    }

    if (dObj->isDuplicate())
        return;

#ifdef DEBUG
    dObj->print(NULL, true); // MOS - NULL means print to debug trace
#endif

    Metadata *m = dObj->getMetadata()->getMetadata("Security");

    // CBMEN, HL, Begin
    // Check if there are certificates embedded that we do not already have
    if (m) {
        HAGGLE_DBG("Retrieving embedded certificates from Data Object [%s]\n", dObj->getIdStr()); // CBMEN, AG

        Metadata *dm = m->getMetadata("Certificate");
        if (dm) {
            do {
                CertificateRef cert = Certificate::fromPEM(dm->getContent());
                CertificateRef oldCert = NULL;

                if (cert) { // CBMEN, AG, Begin
                        HAGGLE_DBG("Received certificate for %s issued by %s.\n",
                                   cert->getSubject().c_str(), cert->getIssuer().c_str());
                    if (cert->getSubject() == cert->getIssuer()) {

                        // Do we already have a verified copy of this certificate?
                        oldCert = retrieveCertificate(cert->getSubject(), certStore);
                        if (oldCert && oldCert->isVerified() && oldCert == cert) {
                            HAGGLE_DBG("Already have certificate, not inserting into certStore!\n");
                            continue;
                        }

                        storeCertificate(cert->getSubject(), cert, certStore, true);
                    } else {

                        // Do we already have a verified copy of this certificate?
                        oldCert = retrieveCertificate(cert->getSubject()+"|"+cert->getIssuer(), remoteSignedCerts);
                        if (oldCert && oldCert->isVerified() && oldCert == cert) {
                            HAGGLE_DBG("Already have certificate, not inserting into remoteSignedCerts!\n");
                            continue;
                        }

                        storeCertificate(cert->getSubject()+"|"+cert->getIssuer(), cert, remoteSignedCerts, true);
                    }

                    // CBMEN, HL - Try and verify this certificate
                    helper->addTask(new SecurityTask(SECURITY_TASK_VERIFY_CERTIFICATE, NULL, cert));
                }
            } while ((dm = m->getNextMetadata()));
        }
    }

    // CBMEN, HL, End

    // MOS - skip signing if not needed
    if(securityLevel <= SECURITY_LEVEL_LOW ||
       ((securityLevel == SECURITY_LEVEL_MEDIUM) && !dObj->isNodeDescription()) ||
       (!signNodeDescriptions && dObj->isNodeDescription())) {
        dObj->setSignatureStatus(DataObject::SIGNATURE_VALID);
        return;
    }

    InterfaceRef iface = dObj->getRemoteInterface();
    bool isApplication = iface && iface->isApplication();

    // Check if this data object came from an application, in that case we sign it.
    // In the future, the signing should potentially be handled by the application
    // itself. But this requires some major rethinking of how to manage certificates
    // and keys, etc.
    if (isApplication && dObj->shouldSign()) {

        if (securityLevel >= SECURITY_LEVEL_HIGH) { // MOS
      if(dObj->getAttribute(DATAOBJECT_ATTRIBUTE_ABE_POLICY)) {
        HAGGLE_DBG("Defer signing of data object [%s] with access policy from application\n", dObj->getIdStr());
        return;
      }
    }

    HAGGLE_DBG("Data object should be signed\n");

        // CBMEN, AG, Begin

        // Data objects should be signed in the SecurityHelper thread since
        // it is a potentially CPU intensive operation.

        NodeRefList targets = e->getNodeList();
        helper->addTask(new SecurityTask(SECURITY_TASK_SIGN_DATAOBJECT, dObj, NULL, &targets));

        // CBMEN, AG, End
    }
}

/*
    On send events, the security manager

 */
void SecurityManager::onSendDataObject(Event *e)
{
    if (!e || !e->hasData())
        return;

    DataObjectRef dObj = e->getDataObject();

    if (dObj->isControlMessage()) return; // MOS

    // MOS - skip adding certificate if not needed
    if(signNodeDescriptions) {
      if (dObj->isThisNodeDescription()) {
        // This is our node description. Piggy-back our certificates.

        // CBMEN, HL, Begin
        Metadata *m;

        m = dObj->getMetadata()->getMetadata("Security");

        if (m) {
            // CBMEN, HL: This is expected behaviour on send failures,
            // switch from ERR to DBG
            HAGGLE_DBG("Node description already has a Security tag!\n");
        } else {
                dObj.lock(); // MOS
                m = dObj->getMetadata()->addMetadata("Security");

            if (m) {
                Mutex::AutoLocker l(certStoreMutex);
                CertificateStore_t toSend;

                if (myCerts.size() > 0)
                    toSend = myCerts;
                else
                    toSend.insert(make_pair("", myCert));

                for (CertificateStore_t::iterator it = toSend.begin(); it != toSend.end(); it++) {
                    m->addMetadata((*it).second->toMetadata());
                }
            }
        dObj.unlock();
        }
        // CBMEN, HL, End
      }
    }

    // CBMEN, HL, Begin
    const Attribute *policy = dObj->getAttribute(DATAOBJECT_ATTRIBUTE_ABE_POLICY);
    const Attribute *capability = dObj->getAttribute(DATAOBJECT_ATTRIBUTE_ABE);

    if (dObj->getIsForLocalApp() && !dObj->getIsForMonitorApp() && // MOS
        (dObj->getABEStatus() >= DataObject::ABE_ENCRYPTION_DONE && dObj->getABEStatus() < DataObject::ABE_DECRYPTION_DONE) ) {

        if (policy && capability)
        {
            HAGGLE_DBG("Enqueuing Capability using task for Data Object [%s].\n", dObj->getIdStr());
            dObj->setABEStatus(DataObject::ABE_DECRYPTION_NEEDED);

        dObj.lock(); // MOS
        Metadata *m = dObj->getMetadata(); // MOS
        if (!m) {
          HAGGLE_ERR("Data Object [%s] has no metadata, can't remove app tag.\n", dObj->getIdStr());
          dObj.unlock();
          return;
        }
        m->removeMetadata(DATAOBJECT_METADATA_APPLICATION); // MOS
        dObj.unlock();

            NodeRefList targets = e->getNodeList();
            helper->addTask(new SecurityTask(SECURITY_TASK_USE_CAPABILITY, dObj, NULL, &targets));
        return; // We'll sign it later.
        } else {
            HAGGLE_DBG("Not trying to use capability for data Object [%s] since either policy [%p] or capability [%p] missing.\n", dObj->getIdStr(), policy, capability); // CBMEN, AG
        }
    } else if (dObj->getIsForLocalApp() && dObj->getIsForMonitorApp()) { // MOS
        HAGGLE_DBG("Not trying to use capability for data object [%s] - data object is for local monitoring application.\n", dObj->getIdStr()); // CBMEN, AG
    } else if (capability) {
        HAGGLE_DBG("Not trying to use capability for data object [%s] - data object has already been decrypted or is not for a local application.\n", dObj->getIdStr()); // CBMEN, AG
    }

    if (dObj->getIsForLocalApp() &&
        dObj->getABEStatus() == DataObject::ABE_DECRYPTION_DONE &&
        policy && capability) {
        // Remove capability, send out again, this restores the original data object ID

        DataObjectRef newDObj = dObj->copy();
        NodeRefList targets = e->getNodeList();
        newDObj->removeAttribute(*capability);
        newDObj->print(NULL);
        HAGGLE_DBG("Not sending [%s] as it has attribute. Re-sending [%s] to %d targets instead.\n", dObj->getIdStr(), newDObj->getIdStr(), targets.size());
        newDObj->setSignatureStatus(DataObject::SIGNATURE_VALID);

        for (NodeRefList::iterator it = targets.begin(); it != targets.end(); it++) { // CBMEN, AG
            if ((*it)->getType() != Node::TYPE_APPLICATION) {
                HAGGLE_ERR("isForLocalApp is true for DataObject [%s] but it is not going to local application!\n", dObj->getIdStr());
            }
            kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, dObj, *it)); // MOS - make sure it is not pending anymore
        }

        kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, newDObj, targets));

        return;

    }
    // CBMEN, HL, End

    // MOS - skip signing if not needed
    if(securityLevel <= SECURITY_LEVEL_LOW ||
       (securityLevel == SECURITY_LEVEL_MEDIUM && !dObj->isNodeDescription()) ||
       (!signNodeDescriptions && dObj->isNodeDescription())) {
      if(!dObj->isSigned()) {
        dObj->setSignatureStatus(DataObject::SIGNATURE_VALID);
        NodeRefList targets = e->getNodeList();
        HAGGLE_DBG("Resending data object %s after signature status validation\n", e->getDataObject()->getIdStr());
        kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND,
                       dObj, targets));
      }
      return;
    }

    // In most cases the data object is already signed here (e.g., if it is generated by a local
    // application, or was received from another node). The only reason to check if we should
    // sign the data object here, is if a data object was generated internally by Haggle -- in
    // which case the data object might not have a signature (e.g., the data object is a node
    // description).
    InterfaceRef iface = dObj->getRemoteInterface();

    if (dObj->shouldSign() && (!iface || iface->isApplication()))  { // MOS - never sign objects received from peers !

        // CBMEN, AG, Begin

        // Data objects should be signed in the SecurityHelper thread since
        // it is a potentially CPU intensive operation.

        NodeRefList targets = e->getNodeList();
        helper->addTask(new SecurityTask(SECURITY_TASK_SIGN_DATAOBJECT, dObj, NULL, &targets));

        // CBMEN, AG, End

        return; // CBMEN, HL
    }

    // CBMEN, HL, Begin

    if (signatureChaining) {
        if (dObj->getSignee() != kernel->getThisNode()->getIdStr()) {
            // Sign it, we'll update the signature chain next time around
            NodeRefList targets = e->getNodeList();
            helper->addTask(new SecurityTask(SECURITY_TASK_SIGN_DATAOBJECT, dObj, NULL, &targets));
        } else {
        }
    }

    // CBMEN, HL, End
}

// CBMEN, AG, HL, Begin

void SecurityManager::onNodeUpdated(Event *e)
{
    Mutex::AutoLocker l(dynamicConfigurationMutex);
    NodeRefList neighList;

    if (getState() > MANAGER_STATE_RUNNING) {
        return;
    }

    if (!e)
        return;

    NodeRef neigh = e->getNode();

    // CBMEN, HL - Only do this for authorities.
    for (HashMap<string, string>::iterator it = authorityNameMap.begin(); it != authorityNameMap.end(); it++) {
        if ((*it).second == neigh->getIdStr()) {
            HAGGLE_DBG("New contact with authority node %s [id=%s]. Updating encryption attribute request queues and checking whether we need certificates signed.\n", neigh->getName().c_str(), neigh->getIdStr());

            helper->addTask(new SecurityTask(SECURITY_TASK_UPDATE_WAITING_QUEUES));
            if (enableCompositeSecurityDataRequests) {
                generateCompositeRequest((*it).first, (*it).second);
            } else {
                generateCertificateSigningRequest((*it).first, (*it).second);
            }
        }
    }

    if (isAuthority) {
        if(neigh->getType() == Node::TYPE_PEER) {

            for (HashMap<string, DataObjectRef>::iterator it = securityDataResponseQueue.begin(); it != securityDataResponseQueue.end(); it++) {

                if ((*it).first == neigh->getIdStr()) {
                    HAGGLE_DBG("Sending queued SecurityDataResponse %s to node %s!\n", (*it).second->getIdStr(), neigh->getIdStr());
                    kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND, (*it).second, neigh));
                }

            }

            HashMap<string, DataObjectRef>::iterator it = securityDataResponseQueue.find(neigh->getIdStr());
            while (it != securityDataResponseQueue.end()) {
                securityDataResponseQueue.erase(it);
                it = securityDataResponseQueue.find(neigh->getIdStr());
            }

        }
    }

}
// CBMEN, AG, HL, End

// CBMEN, HL, Begin
bool SecurityManager::configureNodeSharedSecrets(Metadata *dm, bool fromConfig) {
    Mutex::AutoLocker l(dynamicConfigurationMutex);
    Metadata *node = NULL;
    bool added = false;
    string id, secret;

    if (!dm)
        return false;

    node = dm->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_METADATA);
    if (!node)
        return false;

    do {
        id = node->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_ID_PARAM);
        secret = node->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_SECRET_PARAM);

        HashMap<string, string>::iterator it = rawNodeSharedSecretMap.find(id);
        if (it != rawNodeSharedSecretMap.end()) {
            rawNodeSharedSecretMap.erase(it);
            HAGGLE_DBG("Replaced shared secret for node %s\n", id.c_str());
        } else if (fromConfig) {
            HAGGLE_DBG("Loaded shared secret for node %s\n", id.c_str());
        } else {
            HAGGLE_DBG("Added shared secret for node %s\n", id.c_str());
        }

        rawNodeSharedSecretMap.insert(make_pair(id, secret));
        added = true;

        // If we update an authority shared secret, send a new request
        if (!fromConfig) {
            for (HashMap<string, string>::iterator ait = authorityNameMap.begin(); ait != authorityNameMap.end(); ait++) {
                if ((*ait).second == id) {
                    HAGGLE_DBG("New shared secret for authority node [id=%s]. Updating encryption attribute request queues and checking whether we need certificates signed.\n", id.c_str());

                    helper->addTask(new SecurityTask(SECURITY_TASK_UPDATE_WAITING_QUEUES));
                    if (enableCompositeSecurityDataRequests) {
                        generateCompositeRequest((*ait).first, (*ait).second);
                    } else {
                        generateCertificateSigningRequest((*ait).first, (*ait).second);
                    }

                    if (!securityDataRequestTimerRunning) {
                        kernel->addEvent(new Event(onSecurityDataRequestTimerCallback, NULL, certificateSigningRequestDelay));
                        securityDataRequestTimerRunning = true;
                    }
                }
            }
        }
    } while ((node = dm->getNextMetadata()));

    return added;
}

bool SecurityManager::configureRoleSharedSecrets(Metadata *dm, bool fromConfig) {
    Mutex::AutoLocker l(dynamicConfigurationMutex);
    Metadata *role = NULL;
    bool added = false;
    const char *param = NULL;
    string secret, roleName;

    if (!dm)
        return false;

    role = dm->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_METADATA);
    if (!role)
        return false;

    do {
        param = role->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_NAME_PARAM);
        secret = role->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_SECRET_PARAM);
        roleName = param;
        bool valid = true;

        for (size_t i = 0; i < strlen(param); i++) {
            if (!(isalnum(param[i]) || param[i] == '_' || param[i] == '.')) {
                valid = false;
                break;
            } else {
                roleName[i] = toupper(param[i]);
            }
        }

        if (!valid)
            continue;

        Pair<string, string> thePair = getAuthorityAndAttribute(roleName);
        if (thePair.first == "") {
            HAGGLE_ERR("Improper scoping of role name: %s\n", roleName.c_str());
            continue;
        }

        HashMap<string, string>::iterator it = rawRoleSharedSecretMap.find(roleName);
        if (it != rawRoleSharedSecretMap.end()) {
            rawRoleSharedSecretMap.erase(it);
            HAGGLE_DBG("Replaced shared secret for role %s\n", roleName.c_str());
        } else if (fromConfig) {
            HAGGLE_DBG("Loaded shared secret for role %s\n", roleName.c_str());
        } else {
            HAGGLE_DBG("Added shared secret for role %s\n", roleName.c_str());
        }

        rawRoleSharedSecretMap.insert(make_pair(roleName, secret));
        added = true;
    } while ((role = dm->getNextMetadata()));

    return added;
}

bool SecurityManager::configureAuthorities(Metadata *dm, bool fromConfig) {
    Mutex::AutoLocker l(dynamicConfigurationMutex);
    bool added = false;
    Metadata *authority = NULL;
    string id, name;
    const char *param = NULL;

    if (!dm)
        return false;

    authority = dm->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_METADATA);
    if (!authority)
        return false;

    do {
        id = authority->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_ID_PARAM);
        param = authority->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_NAME_PARAM);

        name = param;
        bool valid = true;
        for (size_t i = 0; i < strlen(param); i++) {
            if (!(isalnum(param[i]) || param[i] == '_')) {
                valid = false;
                break;
            } else {
                name[i] = toupper(param[i]);
            }
        }

        if (!valid)
            continue;

        HashMap<string, string>::iterator it = authorityNameMap.find(name);
        if (it != authorityNameMap.end()) {
            authorityNameMap.erase(it);
            HAGGLE_DBG("Replaced id for authority %s [id=%s]\n", name.c_str(), id.c_str());
        } else if (fromConfig) {
            HAGGLE_DBG("Loaded id for authority %s [id=%s]\n", name.c_str(), id.c_str());
        } else {
            HAGGLE_DBG("Added id for authority %s [id=%s]\n", name.c_str(), id.c_str());
        }

        authorityNameMap.insert(make_pair(name, id));
        added = true;

        // If we have a shared secret, send a request
        if (!fromConfig) {
            if (rawNodeSharedSecretMap.find(id) != rawNodeSharedSecretMap.end()) {
                HAGGLE_DBG("New authority node [id=%s]. Updating encryption attribute request queues and checking whether we need certificates signed.\n", id.c_str());

                helper->addTask(new SecurityTask(SECURITY_TASK_UPDATE_WAITING_QUEUES));
                if (enableCompositeSecurityDataRequests) {
                    generateCompositeRequest(name, id);
                } else {
                    generateCertificateSigningRequest(name, id);
                }

                if (!securityDataRequestTimerRunning) {
                    kernel->addEvent(new Event(onSecurityDataRequestTimerCallback, NULL, certificateSigningRequestDelay));
                    securityDataRequestTimerRunning = true;
                }
            }
        }
    } while ((authority = dm->getNextMetadata()));

    return added;
}

bool SecurityManager::configureNodesForCertification(Metadata *dm, bool fromConfig) {
    Mutex::AutoLocker l(dynamicConfigurationMutex);
    bool added = false;
    Metadata *node = NULL;
    string id, certify;

    if (!dm)
        return false;

    node = dm->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_METADATA);
    if (!node)
        return false;

    do {
        id = node->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_ID_PARAM);
        certify = node->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_CERTIFY_PARAM);

        if (strcmp(certify.c_str(), DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODE_CERTIFY_TRUE_PARAM) == 0) {
            HashMap<string, string>::iterator it = certificationACL.find(id);
            if (it == certificationACL.end()) {
                certificationACL.insert(make_pair(id, ""));
            }
            added = true;
            HAGGLE_DBG("Adding Node %s to the list of nodes which can be certified by this authority.\n", id.c_str());
        } else {
            HAGGLE_DBG("Node %s will not be added to the list of nodes which can be certified by this authority.\n", id.c_str());
        }

    } while ((node = dm->getNextMetadata()));

    return added;
}

bool SecurityManager::configureRolesForAttributes(Metadata *dm, bool fromConfig) {
    Mutex::AutoLocker l(dynamicConfigurationMutex);
    bool added = false;
    Metadata *role = NULL;
    string roleName;
    const char *param = NULL;

    if (!dm)
        return false;

    role = dm->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_METADATA);
    if (!role)
        return false;

    do {
        param = role->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLE_NAME_PARAM);
        roleName = param;
        bool valid = true;

        for (size_t i = 0; i < strlen(param); i++) {
            if (!(isalnum(param[i]) || param[i] == '_' || param[i] == '.')) {
                valid = false;
                break;
            } else {
                roleName[i] = toupper(param[i]);  // CBMEN, AG
            }
        }

        if (!valid)
            continue;

        Pair<string, string> thePair = getAuthorityAndAttribute(roleName);
        if (thePair.first == "" || thePair.first != authorityName) {
            HAGGLE_ERR("Role %s should be scoped by this authority's name: %s\n", roleName.c_str(), authorityName.c_str());
            continue;
        }

        Metadata *attr = role->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_METADATA);
        if (!attr)
            continue;

        do {
            param = attr->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_NAME_PARAM);
            string pub = attr->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_ENCRYPTION_PARAM); // CBMEN, AG
            string priv = attr->getParameter(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_DECRYPTION_PARAM); // CBMEN, AG
            string attrName = param; // CBMEN, AG
            bool valid = true;

            for (size_t i = 0; i < strlen(param); i++) {
                if (!(isalnum(param[i]) || param[i] == '_')) {
                    valid = false;
                    break;
                } else {
                    attrName[i] = toupper(param[i]);  // CBMEN, AG
                }
            }

            if (!valid)
                continue;

            string entry = roleName + ":" + authorityName + "." + attrName;
            if (strcmp(pub.c_str(), DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_AUTHORIZED_PARAM) == 0) {
                HAGGLE_DBG("Adding publicKeyACL entry %s.\n", entry.c_str());

                HashMap<string, string>::iterator it = publicKeyACL.find(entry);
                if (it == publicKeyACL.end()) {
                    publicKeyACL.insert(make_pair(entry, ""));  // CBMEN, AG
                }
            }

            if (strcmp(priv.c_str(), DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ATTRIBUTE_AUTHORIZED_PARAM) == 0) {
                HAGGLE_DBG("Adding privateKeyACL entry %s.\n", entry.c_str());

                HashMap<string, string>::iterator it = privateKeyACL.find(entry);
                if (it == privateKeyACL.end()) {
                    privateKeyACL.insert(make_pair(entry, ""));  // CBMEN, AG
                }
            }
            added = true;
        } while ((attr = role->getNextMetadata()));
    } while ((role = dm->getNextMetadata()));

    return added;
}

// Only for certificates we trust - i.e. our own and authority certificates
// DO NOT USE for remoteSignedCerts
void SecurityManager::configureCertificates(Metadata *dm, CertificateStore_t& store) {
    Metadata *m = NULL;
    CertificateRef cert = NULL;

    if (!dm)
        return;

    m = dm->getMetadata("Certificate");
    if (!m)
        return;

    do {

        cert = Certificate::fromPEM(m->getContent());
        if (!cert)
            continue;

        HAGGLE_DBG("Loaded certificate for %s issued by %s.\n", cert->getSubject().c_str(), cert->getIssuer().c_str());

        if (&store == &myCerts && cert->getSubject() == cert->getIssuer() && cert->getIssuer() == kernel->getThisNode()->getIdStr()) {
            HAGGLE_DBG("Loaded myCert from config!\n");
            myCert = cert;
        }

        storeCertificate(cert->getSubject(), cert, certStore);
        storeCertificate(cert->getIssuer(), cert, store);

    } while ((m = dm->getNextMetadata()));
}

void SecurityManager::configureProvisionedAttributes(Metadata *dm, HashMap<string, string>& map) {
    Metadata *m = NULL;
    string name, value;

    if (!dm)
        return;

    m = dm->getMetadata("Attribute");
    if (!m)
        return;

    do {
        name = m->getParameter("name");
        value = m->getContent();

        HAGGLE_DBG("Loaded attribute %s from config with value %s.\n", name.c_str(), value.c_str());

        HashMap<string, string>::iterator it = map.find(name);
        if (it != map.end())
            map.erase(it);

        map.insert(make_pair(name, value));

    } while ((m = dm->getNextMetadata()));
}

void SecurityManager::configureAdditionalAttributes(Metadata *dm, List<Attribute>& list) {
    Metadata *m = NULL;
    string name, value;
    const char *param;
    unsigned long weight = 1;

    if (!dm)
        return;

    m = dm->getMetadata("Attribute");
    if (!m)
        return;

    do {
        name = m->getParameter("name");
        value = m->getParameter("value");

        param = m->getParameter("weight");
        if (param) {
            char *endptr = NULL;
            unsigned long tmp;
            tmp = strtol(param, &endptr, 10);
            if (endptr && endptr != param) {
                weight = tmp;
            }
        }

        Attribute attr(name, value, weight);
        HAGGLE_DBG("Configuring additional attribute %s\n", attr.getString().c_str());
        list.push_back(attr);
    } while ((m = dm->getNextMetadata()));
}

void SecurityManager::onSecurityConfigure(Event *e) {
    Mutex::AutoLocker l(dynamicConfigurationMutex);
    bool added = false;

    if (getState() > MANAGER_STATE_RUNNING) {
        return;
    }

    if (!e || !e->hasData())
        return;

    DataObjectRef dObj = e->getDataObject();

    if (!dObj->isControlMessage()) return;

    HAGGLE_DBG("Received control message to configure security.\n");

    Metadata *m = NULL, *dm = NULL;

    m = dObj->getMetadata();
    if (!m) {
        HAGGLE_ERR("Data object has no metadata!\n");
        return;
    }

    m = m->getMetadata(DATAOBJECT_METADATA_APPLICATION);
    if (!m) {
        HAGGLE_ERR("Data object has no application metadata!\n");
        return;
    }

    m = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL);
    if (!m) {
        HAGGLE_ERR("Data object has no control metadata!\n");
        return;
    }

    m = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION);
    if (!m) {
        HAGGLE_ERR("Data object has no security configuration metadata!\n");
        return;
    }

    dm = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_ROLES_METADATA);

    if (dm) {
        HAGGLE_DBG("Control message has new role shared secrets, loading!\n");
        if (configureRoleSharedSecrets(dm, false)) {
            added = true;
            HAGGLE_DBG("Enqueueing task to decrypt attribute buckets.\n");
            helper->addTask(new SecurityTask(SECURITY_TASK_DECRYPT_ATTRIBUTE_BUCKETS));
        } else {
            HAGGLE_ERR("Unable to load new role shared secrets.\n");
        }
    }

    dm = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_NODES_METADATA);

    if (dm) {
        HAGGLE_DBG("Control message has new node shared secrets, loading!\n");
        if (configureNodeSharedSecrets(dm, false)) {
            added = true;
        } else {
            HAGGLE_ERR("Unable to load new node shared secrets.\n");
        }
    }

    dm = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITIES_METADATA);

    if (dm) {
        HAGGLE_DBG("Control message has new authorities, loading!\n");
        if (configureAuthorities(dm, false)) {
            added = true;
        } else {
            HAGGLE_ERR("Unable to load new authorities.\n");
        }
    }

    dm = m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL_SECURITY_CONFIGURATION_AUTHORITY_METADATA);

    if (dm) {
        HAGGLE_DBG("Control message has new authority configuration, loading!\n");
        if (configureNodesForCertification(dm, false)) {
            added = true;
        } else {
            HAGGLE_DBG("Control message had no new nodes authorized for certification.\n");
        }
        if (configureRolesForAttributes(dm, false)) {
            added = true;
        } else {
            HAGGLE_DBG("Control message had no new roles authorized for attributes.\n");
        }
    }

    if (added) {
        HAGGLE_DBG("Successfully updated security configuration.\n");
    }

}

// CBMEN, HL, End

void SecurityManager::onConfig(Metadata *m)
{
    const char *param = m->getParameter("security_level");

    if (param) {
        char *level = new char[strlen(param) + 1];
        size_t i;

        // Convert string to uppercase
        for (i = 0; i < strlen(param); i++) {
            level[i] = toupper(param[i]);
        }

        level[i] = '\0';

        if (strcmp(level, security_level_names[SECURITY_LEVEL_HIGH]) == 0) {
            securityLevel = SECURITY_LEVEL_HIGH;
            HAGGLE_DBG("Security level set to %s\n", security_level_names[SECURITY_LEVEL_HIGH]);
        } else if (strcmp(level, security_level_names[SECURITY_LEVEL_MEDIUM]) == 0) {
            securityLevel = SECURITY_LEVEL_MEDIUM;
            HAGGLE_DBG("Security level set to %s\n", security_level_names[SECURITY_LEVEL_MEDIUM]);
        } else if (strcmp(level, security_level_names[SECURITY_LEVEL_LOW]) == 0) {
            securityLevel = SECURITY_LEVEL_LOW;
            HAGGLE_DBG("Security level set to %s\n", security_level_names[SECURITY_LEVEL_LOW]);
        } else if (strcmp(level, security_level_names[SECURITY_LEVEL_VERY_HIGH]) == 0) { // CBMEN, HL
            securityLevel = SECURITY_LEVEL_VERY_HIGH; // CBMEN, HL
            HAGGLE_DBG("Security level set to %s\n", security_level_names[SECURITY_LEVEL_VERY_HIGH]); // CBMEN, HL
        } else {
            HAGGLE_ERR("Unrecognized security level '%s'\n", level);
        }

        delete [] level;
    }

    param = m->getParameter("sign_node_descriptions");

    if (param) {
        if (securityLevel >= SECURITY_LEVEL_HIGH || strcmp(param, "true") == 0) { // CBMEN, AG, HL
            HAGGLE_DBG("Enabling signing of node descriptions\n");
            signNodeDescriptions = true;
        } else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling signing of node descriptions\n");
            signNodeDescriptions = false;
        }
    } else if (securityLevel >= SECURITY_LEVEL_HIGH) {
        HAGGLE_DBG("Enabling signing of node descriptions due to security level\n");
        signNodeDescriptions = true;
    }

    param = m->getParameter("encrypt_file_payload");

    if (param) {
        if (securityLevel == SECURITY_LEVEL_VERY_HIGH || strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling encryption of file payload\n");
            encryptFilePayload = true;
        } else if (strcmp(param, "false") == 0) {
            HAGGLE_DBG("Disabling encryption of file payload\n");
            encryptFilePayload = false;
        }
    } else if (securityLevel == SECURITY_LEVEL_VERY_HIGH) {
        HAGGLE_DBG("Enabling encryption of file pay load due to security level\n");
    }

    // CBMEN, AG, Begin

    param = m->getParameter("max_outstanding_requests");

    if (param) {
        char *endptr = NULL;
        unsigned int tmp;
        tmp = (unsigned int) strtoul(param, &endptr, 10);

        if (endptr && endptr != param) {
            HAGGLE_DBG("Setting the maximum number of outstanding security requests to %u.\n", tmp);
            maxOutstandingRequests = tmp;
        }
    }

    // CBMEN, AG, End

    // CBMEN, HL, Begin

    param = m->getParameter("charm_persistence_data");

    // Only update if onRepositoryData hasn't touched it
    if (param && charmPersistenceData == CHARM_PERSISTENCE_DATA) {
        HAGGLE_DBG("Updating charmPersistenceData\n");
        charmPersistenceData = param;
    }

    param = m->getParameter("temp_file_path");
    if (param)
        tempFilePath = param;

    param = m->getParameter("signature_chaining");
    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling signature chaining\n");
            signatureChaining = true;
        }
    }

    param = m->getParameter("rsa_key_length");

    if (param) {
        char *endptr = NULL;
        size_t tmp;
        tmp = strtoul(param, &endptr, 10);

        if (endptr && endptr != param) {
            HAGGLE_DBG("Setting RSA Key Length to %u.\n", tmp);
            rsaKeyLength = tmp;
        }
    }

    param = m->getParameter("composite_security_data_requests");
    if (param) {
        if (strcmp(param, "true") == 0) {
            HAGGLE_DBG("Enabling composite security data requests!\n");
            enableCompositeSecurityDataRequests = true;
        }
    }

    Metadata *dm = m->getMetadata("Authority");

    if (dm) {
        HAGGLE_DBG("Enabling authority mode\n");
        isAuthority = true;
        authorityName = "Haggle_CA";

        param = dm->getParameter("name");
        if (param) {
            bool valid = true;
            string tmp = param;

            for (size_t i = 0; i < strlen(param); i++) {
                if (!(isalnum(param[i]) || param[i] == '_')) {
                    valid = false;
                    break;
                } else {
                    tmp[i] = toupper(param[i]);
                }
            }

            if (valid) {
                authorityName = tmp;
                HAGGLE_DBG("Set authority name to: %s\n", authorityName.c_str());
            }
        }

        // Add ourselves as an authority
        authorityNameMap.insert(make_pair(authorityName, kernel->getThisNode()->getIdStr()));

        // Add a shared secret for ourselves
        unsigned char *AESKey = (unsigned char *) malloc(16 * sizeof(unsigned char));
        char *b64 = NULL;
        string secret;
        if (!AESKey) {
            HAGGLE_ERR("Couldn't allocate memory!\n");
        } else {
            RAND_bytes(AESKey, 16);
            if (base64_encode_alloc((char *) AESKey, 16, &b64) > 0) {
                secret = string(b64);
                free(AESKey);
                free(b64);
                rawNodeSharedSecretMap.insert(make_pair(kernel->getThisNode()->getIdStr(), secret));
            } else {
                HAGGLE_ERR("Error base64encoding for node shared secret!\n");
            }
        }

        configureNodesForCertification(dm);
        configureRolesForAttributes(dm);

    } else {
        HAGGLE_DBG("Disabling authority mode\n");
        isAuthority = false;
    }

    // CBMEN, HL, AG, Begin

    dm = m->getMetadata("SharedSecrets");
    if (dm) {
        configureNodeSharedSecrets(dm);
        configureRoleSharedSecrets(dm);
    }

    dm = m->getMetadata("Authorities");
    if (dm) {
        configureAuthorities(dm);
    }

    // CBMEN, HL, AG, End

    dm = m->getMetadata("Certificates");
    if (dm) {
        if (dm->getMetadata("myCerts"))
            configureCertificates(dm->getMetadata("myCerts"), myCerts);
        if (dm->getMetadata("caCerts"))
            configureCertificates(dm->getMetadata("caCerts"), caCerts);
    }

    dm = m->getMetadata("KeyPair");
    if (dm) {
        Metadata *priv = NULL, *pub = NULL;
        priv = dm->getMetadata("PrivateKey");
        pub = dm->getMetadata("PublicKey");

        if (priv && pub) {
            privKey = stringToRSAKey(priv->getContent().c_str(), KEY_TYPE_PRIVATE);
            if (privKey) {
                HAGGLE_DBG("Loaded private key from config.\n");
            } else {
                HAGGLE_ERR("Error loading private key from config.\n");
            }

            pubKey = stringToRSAKey(pub->getContent().c_str(), KEY_TYPE_PUBLIC);
            if (pubKey) {
                HAGGLE_DBG("Loaded public key from config.\n");
            } else {
                HAGGLE_ERR("Error loading public key from config.\n");
            }

            if (pubKey && privKey) {
                if (myCert) {
                    HAGGLE_DBG("Already loaded myCert, not generating!\n");
                } else {
                    string id = kernel->getThisNode()->getIdStr();
                    myCert = new Certificate(id.c_str(),
                        id, "forever",
                        kernel->getThisNode()->strIdToRaw(id.c_str()), // TODO: This call is not thread-safe
                        pubKey);

                    if (myCert) {
                        HAGGLE_DBG("Generated node's own certificate based on public and private key.\n");
                        storeCertificate(myCert->getIssuer(), myCert, myCerts);
                        if (myCert->sign(privKey)) {
                            HAGGLE_DBG("Signed node's own certificate.\n");
                            if (myCert->verifySignature(pubKey)) {
                                HAGGLE_DBG("Verified node's own certificate.\n");
                            } else {
                                HAGGLE_ERR("Error verifying node's own certificate!\n");
                            }
                        } else {

                            HAGGLE_ERR("Error signing node's own certificate!\n");
                        }
                    } else {
                        HAGGLE_ERR("Error generating node's own certificate!\n");
                    }
                }

            }

        } else {
            HAGGLE_ERR("Both PrivateKey and PublicKey must be set for KeyPair!\n");
        }

    }

    dm = m->getMetadata("ProvisionedAttributes");
    if (dm) {
        if (dm->getMetadata("IssuedEncryptionAttributes")) {
            HAGGLE_DBG("Loading IssuedEncryptionAttributes from config!\n");
            configureProvisionedAttributes(dm->getMetadata("IssuedEncryptionAttributes"), issuedPubKeysFromConfig);
        }
        if (dm->getMetadata("EncryptionAttributes")) {
            HAGGLE_DBG("Loading EncryptionAttributes from config!\n");
            configureProvisionedAttributes(dm->getMetadata("EncryptionAttributes"), pubKeysFromConfig);
        }
        if (dm->getMetadata("DecryptionAttributes")) {
            HAGGLE_DBG("Loading DecryptionAttributes from config!\n");
            configureProvisionedAttributes(dm->getMetadata("DecryptionAttributes"), privKeysFromConfig);
        }

        for (HashMap<string, string>::iterator it = issuedPubKeysFromConfig.begin(); it != issuedPubKeysFromConfig.end(); it++) {
            HashMap<string, string>::iterator mit = pubKeysFromConfig.find((*it).first);
            if (mit == pubKeysFromConfig.end()) {
                HAGGLE_DBG("Inserting IssuedEncryptionAttribute %s into pubKeysFromConfig!\n", (*it).first.c_str());
                pubKeysFromConfig.insert(make_pair((*it).first, (*it).second));
            }

            mit = issuedPubKeys.find((*it).first);
            if (mit == issuedPubKeys.end()) {
                HAGGLE_DBG("Inserting IssuedEncryptionAttribute %s into issuedPubKeys!\n", (*it).first.c_str());
                issuedPubKeys.insert(make_pair((*it).first, (*it).second));
            }
        }
    }

    dm = m->getMetadata("AdditionalSecurityDataRequestAttributes");
    if (dm) {
        HAGGLE_DBG("Configuring AdditionalSecurityDataRequestAttributes\n");
        configureAdditionalAttributes(dm, additionalSecurityDataRequestAttributes);
    }

    dm = m->getMetadata("AdditionalSecurityDataResponseAttributes");
    if (dm) {
        HAGGLE_DBG("Configuring AdditionalSecurityDataResponseAttributes\n");
        configureAdditionalAttributes(dm, additionalSecurityDataResponseAttributes);
    }

    int certificate_signing_first_request_delay = 0;

    param = m->getParameter("certificate_signing_request_delay");
    if (param) {
        char *endptr = NULL;
        double tmp;
        tmp = strtod(param, &endptr);

        if (endptr && endptr != param) {
            HAGGLE_DBG("Setting delay between certificate signing requests to %f\n", tmp); // CBMEN, AG
            certificateSigningRequestDelay = tmp;
            certificate_signing_first_request_delay = tmp;
        }
    }

    param = m->getParameter("certificate_signing_request_retries");
    if (param) {
        char *endptr = NULL;
        unsigned int tmp;
        tmp = strtol(param, &endptr, 10);

        if (endptr && endptr != param) {
            HAGGLE_DBG("Setting certificate signing retries to %u\n", tmp); // CBMEN, AG
            certificateSigningRequestRetries = tmp;
        }
    }

    param = m->getParameter("certificate_signing_first_request_delay");

    if (param) {
        char *endptr = NULL;
        double tmp;
        tmp = strtod(param, &endptr);

        if (endptr && endptr != param) {
            HAGGLE_DBG("Setting delay before first certificate signing request to %f\n", tmp); // CBMEN, AG
            certificate_signing_first_request_delay = tmp;
        }
    }

    param = m->getParameter("attribute_request_delay");

    if (param) {
        char *endptr = NULL;
        double tmp;
        tmp = strtod(param, &endptr);

        if (endptr && endptr != param) {
            HAGGLE_DBG("Setting delay to %f for requests for encryption attributes from authority.\n", tmp);
            updateWaitingQueueDelay = tmp;
        }
    }

    if (!kernel->isShuttingDown()) { // CBMEN, AG

        if (signNodeDescriptions ||
            (securityLevel >= SECURITY_LEVEL_HIGH)) { // CBMEN, AG // MOS - removed encryptFilePayload
            kernel->addEvent(new Event(onSecurityDataRequestTimerCallback, NULL, certificate_signing_first_request_delay));
            securityDataRequestTimerRunning = true;
        }

        if (securityLevel >= SECURITY_LEVEL_VERY_HIGH || encryptFilePayload)
            kernel->addEvent(new Event(onWaitingQueueTimerCallback, NULL, updateWaitingQueueDelay)); // MOS

    }

    // CBMEN, HL, End

    HAGGLE_DBG("Configured security manager.\n");
    securityConfigured = true;

    if (onRepositoryDataCalled) {
        postConfiguration();
    }
}

bool SecurityManager::tooManyOutstandingSecurityDataRequests() {

    Mutex::AutoLocker l(outstandingRequestsMutex); // CBMEN, AG

    if(outstandingSecurityDataRequests >= maxOutstandingRequests) { // CBMEN, AG
      HAGGLE_DBG("Not generating security data request - too many outstanding requests: %d\n", outstandingSecurityDataRequests);
      return true;
    }
    return false;
}

// Add a task to generate request a certificate from the authority if needed
bool SecurityManager::generateCertificateSigningRequest(string authName, string authID) {
    Mutex::AutoLocker l(dynamicConfigurationMutex);

    CertificateRef cert = retrieveCertificate(authID, myCerts);
    if (cert) {
        return false;
    } else {
        HAGGLE_DBG("Still need signed certificate from %s: %s\n", authName.c_str(), authID.c_str());
    }

    DataObjectRef dObj = DataObject::create();

    if (!dObj) {
        HAGGLE_ERR("Couldn't create data object\n");
        return false;
    }

    if(tooManyOutstandingSecurityDataRequests()) {
        HAGGLE_DBG("Too many outstanding security data requests, not issuing certificate signing request task!\n");
        return false; // MOS // CBMEN, AG, HL
    }

    Timeval now = Timeval::now();
    HashMap<string, Timeval>::iterator it = lastCertificateSignatureRequestTime.find(authID);
    if (it != lastCertificateSignatureRequestTime.end()) {
        Timeval then = (*it).second;

        if ((now - then).getTimeAsSecondsDouble() < certificateSigningRequestDelay) {
            HAGGLE_DBG("Trying to send SecurityDataRequest for certificate signature more often than %fs! ignoring.\n", certificateSigningRequestDelay);
            return true;
        }

        lastCertificateSignatureRequestTime.erase(it);
    }
    lastCertificateSignatureRequestTime.insert(make_pair(authID, now));

    HAGGLE_DBG2("Generating certificate signing request task\n");
    dObj->addAttribute("SecurityDataRequest", "certificate_signature");
    dObj->addAttribute("target", authID);
    dObj->setPersistent(false);
    helper->addTask(new SecurityTask(SECURITY_TASK_SEND_SECURITY_DATA_REQUEST, dObj));

    return true;
}

// Add a task to generate a composite request from the authority if needed
bool SecurityManager::generateCompositeRequest(string authName, string authID) {
    Mutex::AutoLocker l(dynamicConfigurationMutex);

    CertificateRef cert = retrieveCertificate(authID, myCerts);
    if (cert) {
        return false;
    } else {
        HAGGLE_DBG("Still need signed certificate from %s: %s\n", authName.c_str(), authID.c_str());
    }

    DataObjectRef dObj = DataObject::create();

    if (!dObj) {
        HAGGLE_ERR("Couldn't create data object\n");
        return true;
    }

    Timeval now = Timeval::now();
    HashMap<string, Timeval>::iterator it = lastCertificateSignatureRequestTime.find(authID);
    if (it != lastCertificateSignatureRequestTime.end()) {
        Timeval then = (*it).second;

        if ((now - then).getTimeAsSecondsDouble() < certificateSigningRequestDelay) {
            HAGGLE_DBG("Trying to send composite SecurityDataRequest more often than %fs! ignoring.\n", certificateSigningRequestDelay);
            return true;
        }

        lastCertificateSignatureRequestTime.erase(it);
    }
    lastCertificateSignatureRequestTime.insert(make_pair(authID, now));

    HAGGLE_DBG2("Generating composite request task\n");
    dObj->addAttribute("SecurityDataRequest", "composite");
    dObj->addAttribute("target", authID);
    dObj->setPersistent(false);

    Metadata *m;
    m = dObj->getMetadata()->addMetadata("PublicKeys");

    if (!(m && hashMapToMetadata(m, "Key", "name", pubKeys))) {
        HAGGLE_ERR("Couldn't create metadata.\n");
        return true;
    }
    m = dObj->getMetadata()->addMetadata("PrivateKeys");
    if (!(m && hashMapToMetadata(m, "Key", "name", privKeys))) {
        HAGGLE_ERR("Couldn't create metadata.\n");
        return true;
    }

    m = dObj->getMetadata()->addMetadata("roles");
    HashMap<string, string> roles;
    for (HashMap<string, string>::iterator rit = rawRoleSharedSecretMap.begin();
            rit != rawRoleSharedSecretMap.end(); rit++) {

        Pair<string, string> thePair = getAuthorityAndAttribute((*rit).first);

        if (thePair.first == "" || thePair.first != authName)
            continue;

        if (roles.find((*rit).first) == roles.end())
            roles.insert(make_pair((*rit).first, ""));
    }

    if (!(m && hashMapToMetadata(m, "Role", "name", roles))) {
        HAGGLE_ERR("Couldn't create metadata.\n");
        dObj = NULL;
        return true;
    }

    helper->addTask(new SecurityTask(SECURITY_TASK_SEND_SECURITY_DATA_REQUEST, dObj));

    return true;
}

void SecurityManager::onSecurityDataRequestTimer(Event *e) {
    Mutex::AutoLocker l(dynamicConfigurationMutex);
    Mutex::AutoLocker l2(outstandingRequestsMutex); // To prevent deadlock with sendSecurityDataRequest() running concurrently

    if (kernel->isShuttingDown()) return;

    // MOS
    if(certificateSigningRequestRetries >= 0  && certificateSigningRequests > certificateSigningRequestRetries) {
        // CBMEN, HL - Log this
        HAGGLE_DBG("Too many certificateSigningRequestRetries: %d > %d. Returning!\n", certificateSigningRequests, certificateSigningRequestRetries);
        return;
    }
    certificateSigningRequests++;

    bool request = false;
    for (HashMap<string, string>::iterator it = authorityNameMap.begin(); it != authorityNameMap.end(); it++) {
        if (enableCompositeSecurityDataRequests) {
            request |= generateCompositeRequest((*it).first, (*it).second);
        } else {
            request |= generateCertificateSigningRequest((*it).first, (*it).second);
        }
    }

    if (!kernel->isShuttingDown() && request) {
        HAGGLE_DBG("Re-issuing SecurityDataRequestTimerCallback as we need signed certificates\n");
        kernel->addEvent(new Event(onSecurityDataRequestTimerCallback, NULL, certificateSigningRequestDelay));
        securityDataRequestTimerRunning = true;
    } else {
        HAGGLE_DBG("Not issuing SecurityDataRequestTimerCallback as we've gotten all signed certificates\n");
        securityDataRequestTimerRunning = false;
    }

    // CBMEN, HL, AG, End
}

void SecurityManager::onReceivedSecurityDataResponse(Event *e) {

    if (!e || !e->hasData() || e->getDataObjectList().size() == 0)
        return;

    DataObjectRefList dObjs = e->getDataObjectList();
    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {

        DataObjectRef& dObj = *it;

        if (!(dObj->getAttribute("SecurityDataResponse") && dObj->getAttribute("SecurityDataResponse")->getValue() == kernel->getThisNode()->getIdStr()))
            continue;

        if (dObj->isNodeDescription())
            continue;

        helper->addTask(new SecurityTask(SECURITY_TASK_HANDLE_SECURITY_DATA_RESPONSE, dObj));

    }
}

void SecurityManager::onReceivedSecurityDataRequest(Event *e) {

    if (!e || !e->hasData() || e->getDataObjectList().size() == 0)
        return;

    DataObjectRefList dObjs = e->getDataObjectList();
    for (DataObjectRefList::iterator it = dObjs.begin(); it != dObjs.end(); it++) {

        DataObjectRef& dObj = *it;

        if (!dObj->getAttribute("SecurityDataRequest"))
            continue;

        if (dObj->isNodeDescription())
            continue;

        // We should forward instead of deleting
        if (!isAuthority) {
            // dObj->setStored(false);
            // dObj->deleteData();
            continue;
        }

        helper->addTask(new SecurityTask(SECURITY_TASK_HANDLE_SECURITY_DATA_REQUEST, dObj));
    }
}

// CBMEN, HL, End

// CBMEN, AG, Begin

void SecurityManager::onWaitingQueueTimer(Event *e) {

    if (kernel->isShuttingDown()) return;

    HAGGLE_DBG2("Waiting queue update task\n");

    helper->addTask(new SecurityTask(SECURITY_TASK_UPDATE_WAITING_QUEUES));

    kernel->addEvent(new Event(onWaitingQueueTimerCallback, NULL, updateWaitingQueueDelay));
}


// MOS
void SecurityManager::onSendDataObjectResult(Event *e)
{
    Mutex::AutoLocker l(outstandingRequestsMutex); // CBMEN, AG

    DataObjectRef& dObj = e->getDataObject();

    // CBMEN, HL - multi-hop SecurityDataRequests, don't update counts for forwarded objects
    if(dObj->getAttribute("SecurityDataRequestSubject") &&
       dObj->getAttribute("SecurityDataRequestSubject")->getValue() == kernel->getThisNode()->getIdStr() &&
       !dObj->isNodeDescription()) {
       if (e->getType() == EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL) {
         if (outstandingSecurityDataRequests > 0) // CBMEN, HL - Don't want underflow
             outstandingSecurityDataRequests--;
         HAGGLE_DBG("Updated outstanding security data requests after send success: %d\n", outstandingSecurityDataRequests);
       } else if (e->getType() == EVENT_TYPE_DATAOBJECT_SEND_FAILURE) {
         if (outstandingSecurityDataRequests > 0) // CBMEN, HL - Don't want underflow
             outstandingSecurityDataRequests--;
         HAGGLE_DBG("Updated outstanding security data requests after send failure: %d\n", outstandingSecurityDataRequests);
       }
    }
}


// CBMEN, AG, End

// SW: TODO: HACK: this is called after the thread has been killed, during the
// haggle shutdown sequence to avoid memory leaks detected by valgrind
void SecurityManager::shutdown_cleanup()
{
    // Unload OpenSSL algorithms
    // based on: apps_shutdown(); in openssl/apps.h
    // this is required to for valgrind to not report ssl memory leaks
    CONF_modules_unload(1);
    EVP_cleanup();
#if !defined(OS_ANDROID)
    ENGINE_cleanup();
#endif
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_state(0);
    ERR_free_strings();
}
