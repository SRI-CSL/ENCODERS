/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2012 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
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
#ifndef _DATAOBJECT_H
#define _DATAOBJECT_H

/*
  Forward declarations of all data types declared in this file. This is to
  avoid circular dependencies. If/when a data type is added to this file,
  remember to add it here.
*/
class DataObject;
class DataObjectDataRetriever;

#include <libcpphaggle/Reference.h>
#include <libcpphaggle/String.h>
#include "Trace.h"
#include <openssl/sha.h>
#include <stdio.h>

using namespace haggle;

typedef Reference<DataObject> DataObjectRef;

#define DATAOBJECT_ID_LEN SHA_DIGEST_LENGTH
typedef unsigned char DataObjectId_t[DATAOBJECT_ID_LEN];
typedef unsigned char DataHash_t[SHA_DIGEST_LENGTH];

#include "Node.h"
#include "Metadata.h"
#include "Debug.h"
#include "Utility.h"

#define DATAOBJECT_ATTRIBUTE_NAME "Attr"
#define DATAOBJECT_ATTRIBUTE_NAME_PARAM "name"
#define DATAOBJECT_ATTRIBUTE_WEIGHT_PARAM "weight"

#define DATAOBJECT_METADATA_DATA "Data"
#define DATAOBJECT_CREATE_TIME_PARAM "create_time"
#define DATAOBJECT_UPDATE_TIME_PARAM "update_time" // MOS
#define DATAOBJECT_PERSISTENT_PARAM "persistent"

#define DATAOBJECT_METADATA_DATA_DATALEN_PARAM "data_len"

#define DATAOBJECT_METADATA_DATA_FILEPATH "FilePath"
#define DATAOBJECT_METADATA_DATA_FILENAME "FileName"
#define DATAOBJECT_METADATA_DATA_FILEHASH "FileHash"

// CBMEN, HL, Begin
#define DATAOBJECT_METADATA_ABE "ABE"
#define DATAOBJECT_METADATA_ABE_CAPABILITY "Capability"
#define DATAOBJECT_ATTRIBUTE_ABE "_ABE_"
#define DATAOBJECT_ATTRIBUTE_ABE_POLICY "Access"
// CBMEN, HL, Begin

#define DATAOBJECT_METADATA_SIGNATURE "Signature"
#define DATAOBJECT_METADATA_SIGNATURE_SIGNEE_PARAM "signee"

#define MAX_DATAOBJECT_ID_STR_LEN (2*DATAOBJECT_ID_LEN+1) // +1 for null termination

 /* DATAOBJECT_METADATA_PENDING is based on the POSIX value
  * _POSIX_SSIZE_MAX. We should probably figure out a better way to
  * set this max value. */
#define DATAOBJECT_METADATA_PENDING (2147483647) // (32767)

/*
	The maximum size of a metadata header that we allow.
	
	This value may be used to reject certain data objects due to size 
	constraints.
	
	The data object class will not in any way enforce this limit, it is up to
	the managers to enforce it.
*/
#define DATAOBJECT_MAX_METADATA_SIZE (65536)
/*
	The maximum size of a data object that we allow.
	
	This value may be used to reject certain data objects due to size 
	constraints.
	
	The data object class will not in any way enforce this limit, it is up to
	the managers to enforce it.
*/
#define DATAOBJECT_MAX_DATA_SIZE (1LL<<32)

/*
	This macro is meant to be used by managers to determine if a data object
	that contains configuration data can be trusted.
	
	There are three criteria a data object must fulfill to be valid:
	1) It may not be persistent
	2) It must be signed correctly (currently disabled)
	3) It must be signed by the local node (currently disabled)
	
	This is a macro and not a member of the data object class because it needs
	access to getKernel().
	
	FIXME: currently, the code "|| false " is inserted because no signature 
	status or signee information is inserted into any data objects at any time.
*/
#define isValidConfigDataObject(dObj) \
	(!(dObj)->isPersistent() || (false &&                  \
	 ((dObj)->getSignatureStatus() == DataObject::SIGNATURE_VALID) && \
         ((dObj)->getSignee() == getKernel()->getThisNode()->getIdStr())))

/** 
    This class is used to retrieve the serialized data object.
*/
class DataObjectDataRetriever {
    public:
	DataObjectDataRetriever() {}
	virtual ~DataObjectDataRetriever() {}
	/**
           This function retrieves more (starting at the beginning) of the 
           serialized data object (up to len bytes of it, to be precise).
		
           The idea here is that someone that wants to retrieve the data from a
           data object does (essentially) the following:
		
           myRetriever = dObj->getData_begin();
           while(not_done)
           process_data(myRetriever->retrieve(...));
		
           The advantage of using this function is that the one retreiving the data
           does not need to know/assume anything about how a data object is
           formatted, how large the data object is or where any files are stored.
           All that needs to be done is to call retrieve until it returns a
           non-positive integer.
		
           The data pointer is not accessed beyond the limit given by the length
           parameter.
		
           Returns: 
           Positive value: placed this many bytes into the buffer.
           Zero: no more bytes left to be read.
           Negative value: error.
	*/
	virtual ssize_t retrieve(void *data, size_t len, bool getHeaderOnly) = 0;
	virtual bool isValid() const = 0;
};

/**
   This makes allocation/deallocation of data object data retrievers much 
   simpler.
*/
typedef Reference<DataObjectDataRetriever> DataObjectDataRetrieverRef;
typedef ReferenceList<DataObject> DataObjectRefList;

/** */
#if OMNETPP
#include <omnetpp.h>
class DataObject : public cObject
{
#else
#ifdef DEBUG_LEAKS
class DataObject : public LeakMonitor
#else
class DataObject
#endif
{
#endif /* OMNETPP */
    public:
	typedef enum {
		DATA_STATE_UNKNOWN,
		DATA_STATE_NO_DATA,
		DATA_STATE_NOT_VERIFIED,
		DATA_STATE_VERIFIED_OK,
		DATA_STATE_VERIFIED_BAD
	} DataState_t;
	typedef enum {
		SIGNATURE_MISSING,
		SIGNATURE_UNVERIFIED,
		SIGNATURE_VALID,
		SIGNATURE_INVALID
	} SignatureStatus_t;
    // CBMEN, HL, Begin
    typedef enum {
        ABE_NOT_NEEDED,
        ABE_ENCRYPTION_NEEDED,
        ABE_ENCRYPTION_DONE,
        ABE_DECRYPTION_NEEDED,
        ABE_DECRYPTION_DONE,
    } ABEStatus_t;
    // CBMEN, HL, End
    private:
	friend class DataObjectDataRetrieverImplementation;
	SignatureStatus_t signatureStatus;
	string signee;
	unsigned char *signature;
	size_t signature_len;
        static unsigned int totNum;
        unsigned int num;
        Metadata* metadata; // The metadata part of the data object
        Attributes attrs; // The attributes of this data object
        string filename;
        string filepath;
	bool isForLocalApp; // Indicates whether this data object is
                            // for a local application, in which case
                            // the filepath should be added to the
                            // metadata.
	bool isForMonitorApp; // MOS - data object is for local monitor app
        /*
          This is set to true iff the data object is responsible for deleting
          the file when it is destroyed.
	*/
	string storagepath;
	size_t dataLen;
	// SW: this data len should never be altered
	size_t origDataLen;
	/* A timestamp indicating when this data object was created
           at the source (in the source's local time) */
	Timeval createTime;
	/* A timestamp indicating when this data object was updated
           at the source (in the source's local time) */
	Timeval updateTime; // MOS
	
	/* A timestamp indicating when this data object was first received. */
	Timeval receiveTime; 

	InterfaceRef localIface;  // The local interface that received this data object
        InterfaceRef remoteIface; // The remote interface from which this data object was sent
        unsigned long rxTime;  // Time taken to transfer/receive the object in milliseconds
        bool persistent; // Determines whether data object should be stored persistently
        bool duplicate; // Set if the data object was received, but already existed in the data store
	bool stored; // Set if the data object is stored in the data store
	bool obsolete; // MOS - obsolete - no need to send
	bool isNodeDesc; // True if this is a node description
	bool isThisNodeDesc; // True iff this is the node description for the local node.
	bool controlMessage; // True if this is a control message from an application
        /*
          this is for putData().
        */
	void *putData_data;
	/*
	For internal use by putData().
	*/
	void free_pDd(void);

        int parseMetadata(bool from_network = false);

        // Retrieves the 'Data' section of the metadata, or creates it
        // if it doesn't exist.
        Metadata *getOrCreateDataMetadata();
        /*
          Suggestion for improvement:
        	
          Perhaps it could be a good idea to only calculate these when they are
          requested the first time after they've been outdated. It would mean
          rewriting the selector for retreiving these values to calculate the
          value when outdated, and maintaining a boolean that shows wether or not
          the hash is outdated. (Setting the boolean to false in the constructor
          and whenever the attributes are changed, and setting it to true when
          the id is generated.)
        */

        // Creates the basic metadata needed in all data objects
        bool initMetadata();
	DataObjectId_t id;
	char idStr[MAX_DATAOBJECT_ID_STR_LEN];
	DataHash_t dataHash;
	DataState_t dataState;

	bool fatalError; // MOS - fatal error condition associated with this data object

    // CBMEN, HL, Begin
    string encryptedFilePath;
    size_t encryptedFileLength;
    DataHash_t encryptedFileHash;
    ABEStatus_t ABEStatus;
    // CBMEN, HL, End

	bool setFilePath(const string _filepath, size_t data_len = 0, bool from_network = false);
	bool setEncryptedFilePath(const string _enc_filepath, size_t enc_data_len = 0, bool from_network = false);
	
	/*
	   Constructors are private. Use create functions which can return a failure value.
	*/

	
	/**
           This constructor is for use in combination with putData(). The data
           object created with this function is unfinished. It is only truly
	   created when putData() says it is.
	   */
	DataObject(InterfaceRef _sourceIface, InterfaceRef _remoteIface, const string storagepath);
	DataObject(const DataObject& dObj);
public:
	// Create from raw metadata
	static DataObject *create(const unsigned char *raw = NULL, size_t len = 0, InterfaceRef sourceIface = NULL, InterfaceRef remoteIface = NULL, 
		bool stored = false, const string filepath = "", const string encfilepath = "", const string filename = "", 
                SignatureStatus_t sig_status = DataObject::SIGNATURE_MISSING, 
		const string signee = "", const unsigned char *signature = NULL, unsigned long siglen = 0, const Timeval create_time = -1, const Timeval receive_time = -1,
		unsigned long rxtime = 0, size_t datalen = 0, size_t encdatalen = 0, DataState_t datastate = DATA_STATE_UNKNOWN, 
		const unsigned char *datahash = NULL, const unsigned char *encdatahash = NULL, const string storagepath  = HAGGLE_DEFAULT_STORAGE_PATH,
		bool from_network = false);

	// Create from file
	static DataObject *create(const string filepath, const string filename = "");
	// Create from network
	static DataObject *create_for_putting(InterfaceRef _sourceIface = NULL, InterfaceRef _remoteIface = NULL, const string storagepath = HAGGLE_DEFAULT_STORAGE_PATH);

	static string idString(const DataObjectRef& dObj); // MOS
	static string idString(const DataObjectId_t o_id); // MOS

// SW: START GET DOBJ FROM ID:
    static void idStrToId(string idStr, DataObjectId_t o_id);
// SW: END GET DOBJ FROM ID.

	~DataObject();
	DataObject *copy() const;

	/**
           Creates a file path for this object. Returns true if there was a conflict between 
	   the first file path that was attempted and an already existing file, otherwise false.	
	*/
	bool createFilePath();

	int calcId();
	void calcIdStr();
	
	/**
           This function checks if there is a file with this data object, and if 
           there is a hash attribute in the data object. If so, it checks the hash
           to see if it is correct.
		
           Returns: The data state after verification.
	*/
	DataState_t verifyData();
	void deleteData();
	bool hasData() const { return (dataLen && filepath.length()); }

	DataState_t getDataState() const { return dataState; }
        bool dataIsVerifiable() const { return dataState > DATA_STATE_NO_DATA; }
	const unsigned char *getDataHash() const { return dataHash; }

	SignatureStatus_t getSignatureStatus() const { return signatureStatus; }
	void setSignatureStatus(const SignatureStatus_t s) { signatureStatus = s; }
	const string &getSignee() const { return signee; }
	void setSignee(const string s) { signee = s; }
	bool hasValidSignature() const { return signatureStatus == SIGNATURE_VALID; }
	bool signatureIsUnverified() const { return signatureStatus == SIGNATURE_UNVERIFIED; }
	bool isSigned() const { return signatureStatus != SIGNATURE_MISSING; }
	const unsigned char *getSignature() const;
	size_t getSignatureLength() const { return signature_len; }
	void setSignature(const string signee, unsigned char *sig, size_t siglen);
	void removeSignature(); // MOS
	bool shouldSign() const;
	
	const DataObjectId_t &getId() const {
		return id;
	}
	const char *getIdStr() const {
		return idStr;
	}
	int getShortId() const {
	        return (id[1] << 8) + id[0];
	}	
	unsigned int getNum() const {
		return num;
	}
	bool hasCreateTime() const { return createTime.isValid(); }
	Timeval getCreateTime() const { return createTime; }
	void setCreateTime(Timeval t = Timeval::now());
	bool hasUpdateTime() const { return updateTime.isValid(); } // MOS
	Timeval getUpdateTime() const { return updateTime; } // MOS
	void setUpdateTime(Timeval t = Timeval::now()); // MOS
	Timeval getReceiveTime() const { return receiveTime; }
	void setReceiveTime(Timeval t) { receiveTime = t; }
	Timeval getReceiveOrCreateTime() const { if(receiveTime.isValid()) return receiveTime; else return createTime; } // MOS
	bool isNodeDescription() const { return isNodeDesc; }
	void setIsThisNodeDescription(bool yes) { isThisNodeDesc = yes; }
	bool isThisNodeDescription() const { return isThisNodeDesc; }
	bool isControlMessage() const { return controlMessage; }
	void setControlMessage() { controlMessage = true; } // MOS
	void setStored(bool _stored = true) { stored = _stored; }
	bool isStored() const { return stored; }
	void setObsolete() { obsolete = true; } // MOS
	bool isObsolete() const { return obsolete; } // MOS
	void setFatalError() { fatalError = true; } // MOS
	bool hasFatalError() const { return fatalError; } // MOS

	// Metadata functions
	const Metadata *toMetadata() const;
	Metadata *toMetadata();
        Metadata *getMetadata();
        const Metadata *getMetadata() const;
        ssize_t getRawMetadata(unsigned char *raw, size_t len) const;
	bool getRawMetadataAlloc(unsigned char **raw, size_t *len) const;
        
		// Thumbnail functions
        /**
           Function to set the thumbnail of a data object. The given data
           is assumed to be binary, and of length "len".
           
           The function does not take ownership of the data pointer, it will
           remain the property of the owner.
        */
        void setThumbnail(char *data, long len);
        
        void setDuplicate(bool duplicate = true) {
                this->duplicate = duplicate;
        }
        void setNodeDesc(bool isNodeDesc = true) { // MOS
                this->isNodeDesc = isNodeDesc;
        }
        bool isDuplicate() const {
                return duplicate;
        }
        // Data/File functions
        const string &getFilePath() const {
                return filepath;
        }
        const string &getFileName() const {
                return filename;
        }
        const string &getFilePathDynamic() const {
	  if(encryptedFilePath.length()) return encryptedFilePath; else return filepath;
        }

        // CBMEN, HL, Begin
        ABEStatus_t getABEStatus() const { return ABEStatus; }
        void setABEStatus(const ABEStatus_t a) { ABEStatus = a; }
        
        const string &getEncryptedFilePath() const {
            return encryptedFilePath;
        }

        const size_t getEncryptedFileLength() const {
            return encryptedFileLength;
        }

        void setEncryptedFileLength(size_t newLength) {
            encryptedFileLength = newLength;
        }

        unsigned char *getEncryptedFileHash() { return encryptedFileHash; }

        string createEncryptedFilePath();
        
	void setData(const unsigned char *datahash, size_t datalen); // MOS
	void setEncryptedData(const unsigned char *encdatahash, size_t encdatalen); // MOS

	bool hasABEAttribute(); // MOS
	void restoreEncryptedDataObject(); // MOS

        // CBMEN, HL, End

        void setIsForLocalApp(bool val = true);
        bool getIsForLocalApp() { return isForLocalApp; } // CBMEN, HL
        void setIsForMonitorApp(bool val = true); // MOS
        bool getIsForMonitorApp() { return isForMonitorApp; } // MOS

        size_t getDataLen() const {
                return dataLen;
        }

        size_t getOrigDataLen() const {
                return origDataLen;
        }

        void setRxTime(unsigned long time) {
                rxTime = time;
        }
        unsigned long getRxTime() const {
                return rxTime;
        }
        bool isPersistent() const {
                return persistent;
        };
	void setPersistent(bool _persistent = true)
	{
		persistent = _persistent;
	}
	/**
           This function inserts data into a data object. The data object must
           have been created using the constructor only taking an interface. It
           returns how many bytes were successfully put into the data object or 
           -1 on error.

           The caller MUST also use the "remaining" parameter to see how many bytes
           of data remain to put until the data object is complete. This is necessary
           as several data objects can come back-to-back on a byte stream.
           If the data object has auxiliary data, the metadata header will have a data 
           length attribute which is automatically read once the metadata has been 
           completely put. Before that happens, "remaining" will be set to 1.

           The advantage of using this function is that the one creating the data
           object does not need to know/assume anything about how a data object is
           formatted, how large the data object is or where to save the data. All
           that needs to be done is to create an empty data object, and then start
           filling it up with data until it's complete.

           The data pointer is not modified, merely accessed. The data pointer
           is not accessed beyond the limit given by the length parameter.

           Return values:
           positive integer: this many bytes were put into the data object.
           zero: The data object is complete.
           negative integer: An error occurred.
	*/
	
	// MOS - added putHeaderOnly option
	ssize_t putData(void *data, size_t len, size_t *remaining, bool putHeaderOnly = false);

	/**
           This function is for starting retreival of the data that makes up a data
           object.

           See more about how to use this function in the description of the
           DataObjectDataRetriever class.

           Return values:
           non-NULL: this function was successful, meaning that the returned object
           can be used to retrieve data.
           NULL: this function was not successful, meaning that the returned boject
           can (of course) not be used to retrieve data.
	*/
	DataObjectDataRetrieverRef getDataObjectDataRetriever(void) const;

	
        // Attribute functions
        bool addAttribute(const Attribute &a);
        bool addAttribute(const string name, const string value = "", unsigned long weight = 1);
	size_t removeAttribute(const Attribute &a);
	size_t removeAttribute(const string name, const string value = "*");
	const Attribute *getAttribute(const string name, const string value = "*", unsigned int n = 0) const;
	const Attributes *getAttributes() const;
	/**
		Returns true iff the data object has the given attribute.
	*/
	bool hasAttribute(const Attribute &a) const;

        InterfaceRef getRemoteInterface() const {
                return remoteIface;
        }
        void setRemoteInterface(InterfaceRef remote) {
                remoteIface = remote;
        }
        InterfaceRef getLocalInterface() const {
                return localIface;
        }
	void print(FILE *fp = stdout, bool truncate = false) const;

        friend bool operator==(const DataObject&a, const DataObject&b);
        friend bool operator!=(const DataObject&a, const DataObject&b);
};

#endif /* _DATAOBJECT_H */
