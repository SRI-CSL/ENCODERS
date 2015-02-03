/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Ashish Gehani (AG)
 *   Mark-Oliver Stehr (MOS)
 *   Hasnain Lakhani (HL)
 *   Hasanat Kazmi (HK)
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
#ifndef _SECURITYMANAGER_H
#define _SECURITYMANAGER_H

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/

#include <Python.h>

#include <libcpphaggle/String.h>
#include <libcpphaggle/List.h>
#include <libcpphaggle/HashMap.h>
#include <libcpphaggle/Mutex.h>
#include <libcpphaggle/GenericQueue.h>
#include <libcpphaggle/Pair.h>

using namespace haggle;

#include <openssl/rsa.h>
#include <stdio.h>
#include <stdlib.h>

class SecurityManager;

#include "Manager.h"
#include "Certificate.h"
#include "ManagerModule.h"
#include "DataObject.h"
#include "Event.h"

// CBMEN, HL, Begin
// For control message metadata parameters
#include "ApplicationManager.h"
#define FILTER_SECURITYDATAREQUEST "SecurityDataRequest" "=" ATTR_WILDCARD
#ifdef OS_ANDROID
#define TEMP_SDR_FILE_PATH "/data/data/org.haggle.kernel/files/haggletmpsecdata.XXXXXX"
#define PYTHON_UNZIP_COMPLETE_FILE "/data/data/org.haggle.kernel/files/python_unzip_complete"
#define PYTHON_UNZIP_IN_PROGRESS_FILE "/data/data/org.haggle.kernel/files/python_unzip_in_progress"
#define PYTHON_UNZIP_WAIT_TIME 5
#else
#define TEMP_SDR_FILE_PATH "/tmp/haggletmpsecdata.XXXXXX"
#endif
#define CHARM_PERSISTENCE_DATA "eJyrVkosLcnIL8osqYwvzlayUqiu1VFQKi1OLULiIpQUwMXSC0AspXQgqWRolWucka2dXRluEBhl6OsXVKJvnGeWExpRFmpqFOZcHOGmnWaSkh/pmZFi6lPkX55RZmga5lVhbFLsGOCcG2WhbeCXVJalb14QGVYVGVAekVJSGeGd4ufoaKtUWwsA7yI01Q=="
#define PBKDF2_ITERATIONS 1000 // may need to increase
#define RSA_KEY_LENGTH 512
// CBMEN, HL, End

// CBMEN, AG, Begin
#define SIGNING_REQUEST_DELAY 60
#define SIGNING_REQUEST_RETRIES -1
#define WAITING_QUEUE_DELAY 60
#define MAX_OUTSTANDING_REQUESTS 40 // MOS - needs to be large enough to allow cert requests and specific key requests to go through
// CBMEN, AG, End

typedef enum {
	SECURITY_LEVEL_LOW = 0, // No security enabled.
	SECURITY_LEVEL_MEDIUM = 1, // Only require valid signatures for node descriptions, but not for other data objects.
	SECURITY_LEVEL_HIGH = 2, // Require valid signatures for all data objects. 
    SECURITY_LEVEL_VERY_HIGH = 3, // Require encryption for data with ABE policies.
} SecurityLevel_t;

typedef enum {
        SECURITY_TASK_GENERATE_CERTIFICATE,
	SECURITY_TASK_VERIFY_CERTIFICATE,
	SECURITY_TASK_VERIFY_DATAOBJECT,
	SECURITY_TASK_SIGN_DATAOBJECT,
    // CBMEN, HL, Begin
    SECURITY_TASK_GENERATE_CAPABILITY,
    SECURITY_TASK_USE_CAPABILITY,
    SECURITY_TASK_ENCRYPT_DATAOBJECT,
    SECURITY_TASK_DECRYPT_DATAOBJECT,
    SECURITY_TASK_SEND_SECURITY_DATA_REQUEST,
    SECURITY_TASK_HANDLE_SECURITY_DATA_REQUEST,
    SECURITY_TASK_HANDLE_SECURITY_DATA_RESPONSE,
    SECURITY_TASK_VERIFY_AUTHORITY_CERTIFICATE,
    SECURITY_TASK_DECRYPT_ATTRIBUTE_BUCKETS,
    // CBMEN, HL, End
    SECURITY_TASK_UPDATE_WAITING_QUEUES, // CBMEN, AG
    // IRD, HK, Begin
    SECURITY_TASK_GENERATE_INTERESTS_CAPABILITY,
    SECURITY_TASK_USE_INTERESTS_CAPABILITY,
    SECURITY_TASK_ENCRYPT_INTERESTS,
    SECURITY_TASK_DECRYPT_INTERESTS,
    // IRD, HK, End
} SecurityTaskType_t;

class SecurityTask {
public:
	SecurityTaskType_t type;
	bool completed;
	DataObjectRef dObj;
        RSA *privKey;
        CertificateRef cert;
    NodeRefList *targets; // CBMEN, AG
    unsigned char *AESKey; // CBMEN, HL
	SecurityTask(const SecurityTaskType_t _type, DataObjectRef _dObj = NULL, CertificateRef _cert = NULL,
                 NodeRefList *_targets = NULL, unsigned char *AESKey = NULL); // CBMEN, HL
        ~SecurityTask();
};

class SecurityHelper : public ManagerModule<SecurityManager> {
	friend class SecurityManager;
	GenericQueue<SecurityTask *> taskQ;
	const EventType etype;
    // CBMEN, HL, Begin - Helper methods
    bool deriveSecretKeys(string secret);
    bool encryptString(unsigned char *plaintext, size_t ptlen, unsigned char **ciphertext, size_t *ctlen, string secret);
    bool encryptStringBase64(string plaintext, string secret, string &ciphertext);
    bool decryptString(unsigned char *ciphertext, size_t ctlen, unsigned char **plaintext, size_t *ptlen, string secret);
    bool decryptStringBase64(string ciphertext, string secret, string &plaintext);
    // CBMEN, HL, End
	bool signDataObject(DataObjectRef& dObj, RSA *key);
	bool verifyDataObject(DataObjectRef& dObj, CertificateRef& cert) const;
    bool startPython(); // CBMEN, AG
    // CBMEN, HL, Begin
    bool generateCapability(DataObjectRef& dObj);
    bool useCapability(DataObjectRef& dObj, NodeRefList *targets);
    bool encryptDataObject(DataObjectRef& dObj, unsigned char *AESKey);
    bool decryptDataObject(DataObjectRef &dObj, unsigned char *AESKey);
    void addAdditionalAttributes(DataObjectRef& dObj, List<Attribute>& attributes);
    void sendSecurityDataRequest(DataObjectRef& dObj);
    void handleSecurityDataRequest(DataObjectRef& dObj);
    void handleSecurityDataResponse(DataObjectRef& dObj);
    void requestSpecificKeys(List<string>& keys, DataObjectRef& dObj, NodeRefList *targets, bool publicKeys = true, bool interests = false); // IRD, HK
    void requestAllKeys(string authName, string authID, bool publicKeys = true);
    bool signCertificate(string subject, Metadata *m, Metadata *response);
    bool sendSpecificPublicKeys(string subject, Metadata *m, Metadata *response);
    bool sendSpecificPrivateKeys(string subject, Metadata *m, Metadata *response);
    bool sendAllPublicKeys(string subject, Metadata *m, Metadata *response);
    bool sendAllPrivateKeys(string subject, Metadata *m, Metadata *response);
    bool decryptAttributeKey(string ciphertext, string& roleName, string& plaintext);
    void decryptAttributeBuckets();
    bool saveReceivedKeys(Metadata *m, bool publicKeys = true);
    bool issuePublicKeys(List<string>& names);
    bool issuePrivateKey(string fullName, string subject, string &privkey, bool self = false);
    bool updateWaitingQueues(bool publicKeys = true, bool interests = false); // IRD, HK
    bool verifyAuthorityCertificate(CertificateRef caCert);
    typedef HashMap<string,CertificateRef> CertificateStore_t;
    // CBMEN, HL, End
    // IRD, HK, Begin
    bool generateInterestsCapability(DataObjectRef& dObj, NodeRefList *targets);
    bool useInterestsCapability(DataObjectRef& dObj, NodeRefList *targets);
    bool encryptDataObjectInterests(DataObjectRef& dObj, NodeRefList *targets); 
    bool decryptDataObjectInterests(DataObjectRef &dObj, NodeRefList *targets); 
    // IRD, HK, End
    bool updateSignatureChain(DataObjectRef& dObj, RSA *key); // CBMEN, HL, AG
	void doTask(SecurityTask *task);
	bool run();
        void cleanup();
public:
	SecurityHelper(SecurityManager *m, const EventType _etype);
	~SecurityHelper();

	bool addTask(SecurityTask *task) { return taskQ.insert(task); }
};

class SecurityManager : public Manager {
        friend class SecurityHelper;
private:
	SecurityLevel_t securityLevel;
	bool signNodeDescriptions; // MOS
	bool encryptFilePayload; // MOS
    bool securityConfigured; // CBMEN, AG
    bool onRepositoryDataCalled;
	EventType etype;
	SecurityHelper *helper;
	EventCallback<EventHandler> *onRepositoryDataCallback;
	typedef HashMap<string,CertificateRef> CertificateStore_t;
	CertificateStore_t certStore;
	RecursiveMutex certStoreMutex;
	CertificateRef myCert;
    HashMap<string, string> pendingDecryption; // CBMEN, AG
    HashMap<string, string > policyInterests; // IRD, HK 
    HashMap<string, string > hashCache; // IRD, HK
    // CBMEN, HL, Begin
    HashMap<string, Pair<string, unsigned char *> > policyCache;
    HashMap<string, unsigned char *> capabilityCache;
    HashMap<string, string> dataObjectCapabilityCache;
    CertificateStore_t myCerts, remoteSignedCerts; // CBMEN, AG
    CertificateStore_t caCertCache;
    CertificateStore_t caCerts;
    bool pythonRunning;
    RSA *privKey, *pubKey; // CBMEN, AG
    bool isAuthority; // CBMEN, AG, HL
    double certificateSigningRequestDelay;
    int certificateSigningRequestRetries;
    int certificateSigningRequests;
    RecursiveMutex outstandingRequestsMutex; // CBMEN, AG, HL
    double updateWaitingQueueDelay; // CBMEN, AG
    unsigned int outstandingSecurityDataRequests; // MOS, CBMEN, HL
    unsigned int maxOutstandingRequests; // CBMEN, AG
    bool tooManyOutstandingSecurityDataRequests(); // MOS
    string authorityName;
    HashMap<string, string> authorityNameMap;
    EventCallback<EventHandler> *onSecurityDataRequestTimerCallback;
    EventCallback<EventHandler> *onWaitingQueueTimerCallback; // CBMEN, AG    
    void onSendDataObjectResult(Event *e); // MOS
    EventType securityDataRequestEType, securityDataResponseEType;
    bool signatureChaining;
    bool storeCertificate(const string key, CertificateRef& cert, CertificateStore_t& store, bool replace = false);
    CertificateRef retrieveCertificate(const string key, CertificateStore_t& store);
    bool generateCertificateSigningRequest(string authName, string authID);
    bool generateCompositeRequest(string authName, string authID);
    void onSecurityDataRequestTimer(Event *e);
    void onWaitingQueueTimer(Event *e); // CBMEN, AG
    void onReceivedSecurityDataResponse(Event *e);
    void onReceivedSecurityDataRequest(Event *e);
    HashMap<string, string> issuedPubKeys; // Ones which we've issued, as an authority
    HashMap<string, string> pubKeyBucket; // Encrypted, unusable keys
    HashMap<string, string> privKeyBucket; // Encrypted, unusable keys
    HashMap<string, string> pubKeys; // Decrypted, usable keys
    HashMap<string, string> privKeys; // Decrypted, usable keys
    HashMap<string, string> pubKeysNeeded; 
    HashMap<string, string> privKeysNeeded;
    HashMap<string, string> issuedPubKeysFromConfig; // Ones that we've read from config but not necessarily sent to charm
    HashMap<string, string> pubKeysFromConfig; // Ones that we've read from config but not necessarily sent to charm
    HashMap<string, string> privKeysFromConfig; // Ones that we've read from config but not necessarily sent to charm
    HashMap<DataObjectRef, NodeRefList*> waitingForPubKey;
    HashMap<DataObjectRef, NodeRefList*> waitingForPrivKey;
    // IRD, HK, Begin
    HashMap<DataObjectRef, NodeRefList*> waitingForPubKeyInt;
    HashMap<DataObjectRef, NodeRefList*> waitingForPrivKeyInt;
    // IRD, HK, End
    string charmPersistenceData;
    string tempFilePath;
    size_t rsaKeyLength;
    HashMap<string, string> rawNodeSharedSecretMap;
    HashMap<string, string> rawRoleSharedSecretMap;
    HashMap<string, Pair<unsigned char *, unsigned char *> > derivedSharedSecretMap;
    HashMap<string, string> publicKeyACL;
    HashMap<string, string> privateKeyACL;
    HashMap<string, string> certificationACL;
    HashMap<string, string> tmpRepositoryData;
    HashMap<string, DataObjectRef> securityDataResponseQueue;
    HashMap<string, Timeval> lastCertificateSignatureRequestTime;
    bool securityDataRequestTimerRunning;
    bool enableCompositeSecurityDataRequests;
    List<Attribute> additionalSecurityDataRequestAttributes;
    List<Attribute> additionalSecurityDataResponseAttributes;
    RecursiveMutex ccbMutex;
    // CBMEN, HL, End
	void onRepositoryData(Event *e);
	void onSecurityTaskComplete(Event *e);
	void onIncomingDataObject(Event *e);
	void onReceivedDataObject(Event *e);
	void onSendDataObject(Event *e);
    void onNewNodeContact(Event *e); // CBMEN, AG
    void onNodeUpdated(Event *e); // CBMEN, AG, HL

    // CBMEN, HL, Begin
    // Must be Recursive or there will be deadlocks since handleSecurityDataResponse() calls requestSpecificKeys() and both need to hold this mutex.
    RecursiveMutex dynamicConfigurationMutex; 
    bool configureInterestsPolicies(Metadata *dm, bool fromConfig = true); // IRD, HK
    bool configureNodeSharedSecrets(Metadata *dm, bool fromConfig = true);
    bool configureRoleSharedSecrets(Metadata *dm, bool fromConfig = true);
    bool configureAuthorities(Metadata *dm, bool fromConfig = true);
    bool configureNodesForCertification(Metadata *dm, bool fromConfig = true);
    bool configureRolesForAttributes(Metadata *dm, bool fromConfig = true);
    void configureCertificates(Metadata *dm, CertificateStore_t& store);
    void configureProvisionedAttributes(Metadata *dm, HashMap<string, string>& map);
    void configureAdditionalAttributes(Metadata *dm, List<Attribute>& list);
    void onSecurityConfigure(Event *e);
    // CBMEN, HL, End
	
    void onInterestsPoliciesRequested(Event *e); // IRD, HK

	void onPrepareStartup();
    void onStartup(); // CBMEN, HL
	void onPrepareShutdown();
	void onShutdown();
	bool init_derived();
	void onConfig(Metadata *m);
    void postConfiguration(); // CBMEN, AG, HL
public:
	void printCertificates();
	void onDebugCmdEvent(Event *e);
    void observeCertificatesAsMetadata(string key); // CBMEN, HL
	void setSecurityLevel(const SecurityLevel_t slevel) { securityLevel = slevel; }
	SecurityLevel_t getSecurityLevel() const { return securityLevel; }
	bool getEncryptFilePayload() const { return encryptFilePayload; }

	SecurityManager(HaggleKernel *_haggle = haggleKernel, SecurityLevel_t slevel = SECURITY_LEVEL_MEDIUM);
	~SecurityManager();

    // SW: TODO: HACK: needed to cleanup openssl library data structures
    // and not show memory leak in valgrind
    static void shutdown_cleanup();
};

#endif
