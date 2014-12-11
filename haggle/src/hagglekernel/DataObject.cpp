/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2012 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
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
#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Timeval.h>

#include <libxml/tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <haggleutils.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/stat.h>
#endif

#include "XMLMetadata.h"
#include "DataObject.h"
#include "Trace.h"
#include "fragmentation/FragmentationConstants.h"
#include "networkcoding/NetworkCodingConstants.h"

// This include is for the HAGGLE_ATTR_CONTROL_NAME attribute name. 
// TODO: remove dependency on libhaggle header
#include "../libhaggle/include/libhaggle/ipc.h"

unsigned int DataObject::totNum = 0;

// This is used in putData():
typedef struct pDd_s {
	// The metadata header
	unsigned char *header;
	// The current length of the metadata header (above):
	size_t header_len;
	// The current length of the metadata header pointer (above):
	size_t header_alloc_len;
	// File pointer to the file where the data object's data is stored:
	FILE *fp;
	// The amount of data left to write to the data file:
	size_t bytes_left;
} *pDd;

// Creates and initializes a pDd data structure.
static pDd create_pDd(void)
{
	pDd retval;
	
	retval = (pDd) malloc(sizeof(struct pDd_s));
	
	if (!retval)
		return NULL;

	retval->header = NULL;
	retval->header_len = 0;
	retval->header_alloc_len = 0;
	retval->fp = NULL;
	retval->bytes_left = 0;
	
	return retval;
}

// Releases the header part of a pDd structure.
static void free_pDd_header(pDd data)
{
	if (!data)
		return;

	data->header_len = 0;
	data->header_alloc_len = 0;
	free(data->header);
	// Let's not have any lingering pointers to dead data:
	data->header = NULL;
}

// Releases an entire pDd structure.
void DataObject::free_pDd(void)
{
	pDd data = (pDd) putData_data;
	if (!data)
		return;
	
	if (data->header != NULL)
		free(data->header);
	if (data->fp != NULL)
	        fclose(data->fp);
	free(data);
	// Let's not have any lingering pointers to dead data:
	putData_data = NULL;
}

DataObject::DataObject(InterfaceRef _localIface, InterfaceRef _remoteIface, const string _storagepath) : 
#ifdef DEBUG_LEAKS
                LeakMonitor(LEAK_TYPE_DATAOBJECT),
#endif
                signatureStatus(DataObject::SIGNATURE_MISSING),
                signee(""), signature(NULL), signature_len(0), num(totNum++), 
                metadata(NULL), filename(""), filepath(""), isForLocalApp(false), isForMonitorApp(false), 
		storagepath(_storagepath), dataLen(0), origDataLen(0), createTime(-1), updateTime(-1), receiveTime(-1), 
                localIface(_localIface), remoteIface(_remoteIface), rxTime(0), 
                persistent(true), duplicate(false), stored(false), obsolete(false), isNodeDesc(false), 
		isThisNodeDesc(false), controlMessage(false), putData_data(NULL), 
		dataState(DATA_STATE_UNKNOWN),
		fatalError(false),
		encryptedFilePath(""), encryptedFileLength(0), ABEStatus(ABE_NOT_NEEDED) // CBMEN, HL
{
	memset(id, 0, sizeof(DataObjectId_t));
	memset(idStr, 0, MAX_DATAOBJECT_ID_STR_LEN);
	memset(dataHash, 0, sizeof(DataHash_t));
	memset(encryptedFileHash, 0, sizeof(DataHash_t));
}

// Copy constructor
DataObject::DataObject(const DataObject& dObj) : 
#ifdef DEBUG_LEAKS
                LeakMonitor(LEAK_TYPE_DATAOBJECT),
#endif
                signatureStatus(dObj.signatureStatus),
                signee(dObj.signee), signature(NULL), signature_len(dObj.signature_len), 
		num(totNum++), metadata(dObj.metadata ? dObj.metadata->copy() : NULL), 
                attrs(dObj.attrs), filename(dObj.filename), filepath(dObj.filepath),
                isForLocalApp(dObj.isForLocalApp), isForMonitorApp(dObj.isForMonitorApp), storagepath(dObj.storagepath),
                dataLen(dObj.dataLen), origDataLen(dObj.origDataLen), createTime(dObj.createTime), updateTime(dObj.updateTime), 
		receiveTime(dObj.receiveTime), localIface(dObj.localIface), 
		remoteIface(dObj.remoteIface), rxTime(dObj.rxTime), 
		persistent(dObj.persistent), duplicate(false), 
		stored(dObj.stored), obsolete(dObj.obsolete), isNodeDesc(dObj.isNodeDesc), 
		isThisNodeDesc(dObj.isThisNodeDesc),
                controlMessage(dObj.controlMessage), putData_data(NULL), dataState(dObj.dataState),
		fatalError(dObj.fatalError),
		encryptedFilePath(dObj.encryptedFilePath), // CBMEN, HL
		encryptedFileLength(dObj.encryptedFileLength), ABEStatus(dObj.ABEStatus) // CBMEN, HL
{
	memcpy(id, dObj.id, DATAOBJECT_ID_LEN);
	memcpy(idStr, dObj.idStr, MAX_DATAOBJECT_ID_STR_LEN);
	memcpy(dataHash, dObj.dataHash, sizeof(DataHash_t));
    memcpy(encryptedFileHash, dObj.encryptedFileHash, sizeof(DataHash_t)); // CBMEN, HL
	
	if (dObj.signature && signature_len) {
		signature = (unsigned char *)malloc(signature_len);
		
		if (signature) 
			memcpy(signature, dObj.signature, signature_len);
	}
}

DataObject *DataObject::create_for_putting(InterfaceRef sourceIface, InterfaceRef remoteIface, const string storagepath)
{
	DataObject *dObj = new DataObject(sourceIface, remoteIface, storagepath);

	if (!dObj)
		return NULL;

	dObj->putData_data = create_pDd();

	if (!dObj->putData_data) {
		HAGGLE_ERR("Could not initialize data object to put data\n");
		goto out_failure;
	}

	return dObj;

out_failure:
	delete dObj;

	return NULL;
}

DataObject *DataObject::create(const string filepath, const string filename)
{
	if (filepath.length() == 0) {
		HAGGLE_ERR("Invalid file path\n");
		return NULL;
	}

	DataObject *dObj = new DataObject(NULL, NULL, HAGGLE_DEFAULT_STORAGE_PATH);

	if (!dObj)
		return NULL;

	if (!dObj->initMetadata()) {
		HAGGLE_ERR("Could not init metadata\n");
		delete dObj;
		return NULL;
	}

	dObj->filepath = filepath;

	if (filename.length() == 0) {
#ifdef OS_WINDOWS
                  dObj->filename = filepath.substr(filepath.find_last_of("\\") + 1); // CBMEN, HL - String so it gets null terminated
#else 
                  dObj->filename = filepath.substr(filepath.find_last_of("/") + 1); // CBMEN, HL - String so it gets null terminated
#endif
	} else {
		dObj->filename = filename;
	}

	// Set the file hash
	char data[4096];
	size_t read_bytes;
        SHA_CTX ctx;

        // Open the file:
        FILE *fp = fopen(filepath.c_str(), "rb");
	if(fp) HAGGLE_DBG("Opening file %s with file descriptor %d\n", filepath.c_str(), fileno(fp));

	if (fp == NULL) {
	  if(errno == ENFILE) dObj->setFatalError();
	  HAGGLE_ERR("Could not open file %s - error: %s (%d)\n", filepath.c_str(), strerror(errno), errno);
		delete dObj;
		return NULL;
	}
	// Initialize the SHA1 hash context
	SHA1_Init(&ctx);

	// Go through the file until there is no more, or there was
	// an error:
	while (!(feof(fp) || ferror(fp))) {
		// Read up to 4096 bytes more:
		read_bytes = fread(data, 1, 4096, fp);
		// Insert them into the hash:
		SHA1_Update(&ctx, data, read_bytes);
		dObj->dataLen += read_bytes;
		dObj->origDataLen = dObj->dataLen;
	}
	
	// Was there an error?
	if (!ferror(fp)) {
		// No? Finalize the hash:
		SHA1_Final(dObj->dataHash, &ctx);
		dObj->dataState = DATA_STATE_VERIFIED_OK;
	} else {
	        fclose(fp);
		HAGGLE_ERR("Could not create data hash for file %s\n", filepath.c_str());
		delete dObj;
		return NULL;
	}
	// Close the file
	fclose(fp);

	dObj->setCreateTime();

	if (dObj->calcId() < 0) {
		HAGGLE_ERR("Could not calculate data object identifier\n");
		delete dObj;
		return NULL;
	}
	return dObj;
}

// MOS - added encryption fields
DataObject *DataObject::create(const unsigned char *raw, size_t len, InterfaceRef sourceIface, InterfaceRef remoteIface, 
			       bool stored, const string filepath, const string encfilepath, const string filename, SignatureStatus_t sig_status, 
			       const string signee, const unsigned char *signature, unsigned long siglen, const Timeval create_time, 
			       const Timeval receive_time, unsigned long rxtime, size_t datalen, size_t encdatalen, DataState_t datastate, 
			       const unsigned char *datahash, const unsigned char *encdatahash, const string storagepath,
			       bool from_network)
{
	DataObject *dObj = new DataObject(sourceIface, remoteIface, storagepath);

	if (!dObj)
		return NULL;

	/*
	  Note, that many of these values set here will probably be overwritten in 
	  case the metadata is valid. The values will be parsed from the metadata
	  and set according to the values there, which in most cases will anyhow
	  be the same.

	  In the future, we might want to figure out a way to either create a 
	  data object entirely from the raw metadata, or only from the values
	  we pass into the function here.
	*/
	dObj->filepath = filepath;
	dObj->encryptedFilePath = encfilepath;
	dObj->filename = filename;
	dObj->stored = stored;

	if (filepath.length()) {
	        if (!dObj->setFilePath(filepath, datalen,from_network)) {
			goto out_failure;
		}
	}

	if (encfilepath.length()) {
		if (!dObj->setEncryptedFilePath(encfilepath, encdatalen)) {
			goto out_failure;
		}
	}

	dObj->signatureStatus = sig_status;

	if (signature && siglen) {
		unsigned char *signature_copy = (unsigned char *)malloc(siglen);
		if (!signature_copy) {
			HAGGLE_ERR("Could not set signature\n");
			goto out_failure;
		}
		memcpy(signature_copy, signature, siglen);
		dObj->setSignature(signee, signature_copy, siglen);
	}
	dObj->createTime = create_time;
	dObj->receiveTime = receive_time;
	dObj->rxTime = rxtime;
	dObj->dataLen = datalen;
	dObj->origDataLen = datalen;
	dObj->dataState = datastate;

	if (datahash && datastate > DATA_STATE_NO_DATA) {
               memcpy(dObj->dataHash, datahash, sizeof(DataHash_t));
	}

	if (!raw) {
                if (!dObj->initMetadata()) {
                        HAGGLE_ERR("Could not init metadata\n");
			goto out_failure;
                }

		dObj->setCreateTime();

		if (dObj->calcId() < 0) {
			HAGGLE_ERR("Could not calculate node identifier\n");
			goto out_failure;
		}
	} else {
                dObj->metadata = new XMLMetadata();
                
		if (!dObj->metadata) {
			HAGGLE_ERR("Could not allocate new metadata\n");
			goto out_failure;
		}

		if (!dObj->metadata->initFromRaw(raw, len) || dObj->metadata->getName() != "Haggle") {
			HAGGLE_ERR("Could not create metadata\n");
			delete dObj->metadata;
			dObj->metadata = NULL;
			goto out_failure;
		}

		if (dObj->parseMetadata() < 0) {
			HAGGLE_ERR("Metadata parsing failed\n");
			delete dObj->metadata;
			dObj->metadata = NULL;
			goto out_failure;
		}
	}

	// MOS - for encrypted objects we directly use fields from db
	if (dObj->hasABEAttribute()) {

	  dObj->dataLen = datalen;
	  dObj->encryptedFileLength = encdatalen;
	  dObj->origDataLen = encdatalen;
	  dObj->dataState = datastate;
	  
	  if (datahash && datastate > DATA_STATE_NO_DATA) {
	    memcpy(dObj->dataHash, datahash, sizeof(DataHash_t));
	  }
	  
	  if (encdatahash && datastate > DATA_STATE_NO_DATA) {
	    memcpy(dObj->encryptedFileHash, encdatahash, sizeof(DataHash_t));
	  }

	  dObj->calcId();

	  if (dObj->filepath.length() > 0) {
	    dObj->setABEStatus(ABE_DECRYPTION_DONE);
	  }
	  else if (dObj->encryptedFilePath.length() > 0) {
	    dObj->setABEStatus(ABE_ENCRYPTION_DONE);
	  }
	}

	return dObj;

out_failure:
	delete dObj;

	return NULL;
}

string DataObject::idString(const DataObjectRef& dObj)
{
  string id(dObj->getIdStr());
  return id;
}

string DataObject::idString(const DataObjectId_t o_id)
{
  char idStrTmp[MAX_DATAOBJECT_ID_STR_LEN];
  int len = 0;

  // Generate a readable string of the Id
  for (int i = 0; i < DATAOBJECT_ID_LEN; i++) {
    len += sprintf(idStrTmp + len, "%02x", o_id[i] & 0xff);
  }

  string id(idStrTmp);
  return id;
}


DataObject::~DataObject()
{
	if (putData_data) {
		free_pDd();
	}
	if (metadata) {
                delete metadata;
	}
	if (signature)
		free(signature);

	if (!stored) {
		deleteData();
	}
 
}

void DataObject::deleteData()
{
	if (filepath.size() > 0) {

		HAGGLE_DBG("Deleting file \'%s\' associated with data object [%s]\n", 
			filepath.c_str(), idStr);
#if defined(OS_WINDOWS_MOBILE)
		wchar_t *wpath = strtowstr_alloc(filepath.c_str());

		if (wpath) {
			DeleteFile(wpath);
			free(wpath);
		}
#else
		remove(filepath.c_str());
#endif 
	}
	filepath = "";
	dataLen = 0;
	dataState = DATA_STATE_NO_DATA;
    
    // CBMEN, HL, Begin
    if (encryptedFilePath.size() > 0) {
        HAGGLE_DBG("Deleting encrypted file \'%s\' associated with data object [%s]\n", 
            encryptedFilePath.c_str(), idStr);
#if defined(OS_WINDOWS_MOBILE)
        wchar_t *wpath = strtowstr_alloc(encryptedFilePath.c_str());

        if (wpath) {
            DeleteFile(wpath);
            free(wpath);
        }
#else
        remove(encryptedFilePath.c_str());
#endif 
    }
    
    encryptedFilePath = "";
    encryptedFileLength = 0;
    // CBMEN, HL, End
}

DataObject *DataObject::copy() const 
{
	return new DataObject(*this);
}

bool DataObject::initMetadata()
{
        if (metadata)
                return false;

        metadata = new XMLMetadata("Haggle");
        
        if (!metadata)
                return false;
        
        return true;
}

const unsigned char *DataObject::getSignature() const
{
	return signature;
}

void DataObject::setSignature(const string signee, unsigned char *sig, size_t siglen)
{
	if (signature)
		free(signature);
	
	this->signee = signee;
	signature = sig;
	signature_len = siglen;
	signatureStatus = DataObject::SIGNATURE_UNVERIFIED;
	
	//HAGGLE_DBG("Set signature on data object, siglen=%lu\n", siglen);
}

// MOS
void DataObject::removeSignature()
{
        if(isSigned()) {
	  if (signature)
		free(signature);
	
	  this->signee = "";
	  signature = NULL;
	  signature_len = 0;
	  signatureStatus = DataObject::SIGNATURE_MISSING;	
	}
}

bool DataObject::setFilePath(const string _filepath, 
			     size_t data_len, 
			     bool from_network) 
{
	filepath = _filepath;
	dataLen = data_len;
	origDataLen = data_len;

	if (from_network)
		return true;

	/*
	The fopen() gets the file size of the file that is given in the
	metadata. This really only applies to locally generated data
	objects that are receieved from applications. In this case, the
	payload will not arrive over the socket from the application, but
	rather the local file is being pointed to by the file attribute in
	the metadata. The file path and file size is in this case read
	here with the fopen() call.

	If the data object arrives from another node (over, e.g., the
	network) the payload will arrive as part of a byte stream, back-
	to-back with the metadata header. In that case, this call will
	fail when the metadata attributes are checked here. This is
	perfectly fine, since the file does not exists yet and is
	currently being put.
	*/
	FILE *fp = fopen(filepath.c_str(), "rb");
	if(fp) HAGGLE_DBG2("Opening file %s with file descriptor %d\n", filepath.c_str(), fileno(fp));

	if (fp) {
		// Find file size:
		fseek(fp, 0L, SEEK_END);
		ssize_t size = ftell(fp);
		int lasterrno = errno;
		fclose(fp);

		if(size < 0) { // MOS - added missing error check
		  HAGGLE_ERR("Error when checking file size for file %s, error: %s (%d)\n", 
			     filepath.c_str(), strerror(lasterrno), lasterrno);
		  return false;
		} else if (dataLen == 0) {
			dataLen = size;
			origDataLen = size;
		} else if (size != dataLen) {
			HAGGLE_ERR("File size %lu does not match data length %lu\n", 
				   size, dataLen);
			return false;
		}

		//FIXME comment out for now called way too many times
		//HAGGLE_DBG("\'%s\' %lu bytes\n", filepath.c_str(), dataLen);
	} else {
		HAGGLE_DBG("File \'%s\' does not exist\n", 
			   filepath.c_str());
		return false;
	}

	return true;
}

// MOS - similar to previous function but for encrypted payload
bool DataObject::setEncryptedFilePath(const string _enc_filepath, 
			     size_t enc_data_len, 
			     bool from_network) 
{
	encryptedFilePath = _enc_filepath;
	encryptedFileLength = enc_data_len;

	if (from_network)
		return true;

	/*
	The fopen() gets the file size of the file that is given in the
	metadata. This really only applies to locally generated data
	objects that are receieved from applications. In this case, the
	payload will not arrive over the socket from the application, but
	rather the local file is being pointed to by the file attribute in
	the metadata. The file path and file size is in this case read
	here with the fopen() call.

	If the data object arrives from another node (over, e.g., the
	network) the payload will arrive as part of a byte stream, back-
	to-back with the metadata header. In that case, this call will
	fail when the metadata attributes are checked here. This is
	perfectly fine, since the file does not exists yet and is
	currently being put.
	*/
	FILE *fp = fopen(encryptedFilePath.c_str(), "rb");

	if (fp) {
		// Find file size:
		fseek(fp, 0L, SEEK_END);
		ssize_t size = ftell(fp);
		int lasterrno = errno;
		fclose(fp);

		if(size < 0) { // MOS - added missing error check
		  HAGGLE_ERR("Error when checking encrypted file size for file %s, error: %s (%d)\n", 
			     encryptedFilePath.c_str(), strerror(lasterrno), lasterrno);
		  return false;
		} else if (encryptedFileLength == 0) {
			encryptedFileLength = size;
		} else if (size != encryptedFileLength) {
			HAGGLE_ERR("Encrypted file size %lu does not match data length %lu\n", 
				   size, encryptedFileLength);
			return false;
		}

		//FIXME comment out for now called way too many times
		//HAGGLE_DBG("\'%s\' %lu bytes\n", filepath.c_str(), dataLen);
	} else {
		HAGGLE_DBG("Encrypted file \'%s\' does not exist\n", 
			   encryptedFilePath.c_str());
		return false;
	}

	return true;
}

bool DataObject::shouldSign() const
{
	return (!isSigned() && !getAttribute(HAGGLE_ATTR_CONTROL_NAME));
}

bool DataObject::createFilePath()
{
#define PATH_LENGTH 1024 // MOS - be generous
        char path[PATH_LENGTH];
        long i;
	bool duplicate = false;
        FILE *fp;

	const char *format = "%s%s%s-%s";
	if(encryptedFilePath.length() == 0 && getAttribute(DATAOBJECT_ATTRIBUTE_ABE)) format = "%s%s%s-%s.enc";

	Timeval ct = Timeval::now(); // MOS - use high resolution time to avoid name collision

        // Try to just use the sha1 hash value for the filename
	// MOS - not a good idea due to concurrent receptions of same data object
        //snprintf(path, PATH_LENGTH, "%s%s%s-%s", 
        //         storagepath.c_str(), PLATFORM_PATH_DELIMITER, 
        //         filename.c_str(),
	//	 ct.getAsString().c_str()); // MOS - this is important to avoid multi-thread name collision

	// MOS - use a different order to preserve filename extensions (currently required for drexel .ttl files)
        snprintf(path, PATH_LENGTH, format, 
                 storagepath.c_str(), PLATFORM_PATH_DELIMITER, 
		 ct.getAsString().c_str(), // MOS - this is important to avoid multi-thread name collision
                 filename.c_str());

/*
	// MOS the following shouldn't be needed anymore
        do {
                // Is there already a file there?
                fp = fopen(path, "rb");
                
                if (fp != NULL) {
                        // Yep.
			duplicate = true;
			
                        // Close that file:
                        fclose(fp);
			
                        // Generate a new filename:
                        i++;
                        sprintf(path, "%s%s%ld-%s-%s", 
                                storagepath.c_str(), 
                                PLATFORM_PATH_DELIMITER,
                                i, 
                                filename.c_str(),
				ct.getAsString().c_str()); // MOS - see above
                }
        } while (fp != NULL);
*/

        // Make sure the file path is the same as the file path
        // written to:
        filepath = path;

	return duplicate;
}

// CBMEN, HL, Begin
// MOS - simplified
string DataObject::createEncryptedFilePath()
{
        char path[PATH_LENGTH];
        Timeval ct = Timeval::now();

        // MOS - use a different order to preserve filename extensions (currently required for drexel .ttl files)
        snprintf(path, PATH_LENGTH, "%s%s%s-%s.enc", 
                 storagepath.c_str(), PLATFORM_PATH_DELIMITER, 
                 ct.getAsString().c_str(), // MOS - this is important to avoid multi-thread name collision
                 filename.c_str());

        // Make sure the file path is the same as the file path
        // written to:
        encryptedFilePath = path;

       return encryptedFilePath;
}
// CBMEN, HL, End

Metadata *DataObject::getOrCreateDataMetadata()
{
        if (!metadata)
                return NULL;

        Metadata *md = metadata->getMetadata(DATAOBJECT_METADATA_DATA);

        if (!md) {
                return metadata->addMetadata(DATAOBJECT_METADATA_DATA);
        }
        return md;
}

void DataObject::setThumbnail(char *data, long len)
{
	Metadata *md = getOrCreateDataMetadata();
	char *b64 = NULL;
	
	if (!md)
		return;
	
	if (base64_encode_alloc(data, len, &b64) == 0)
		return;
	
	string str = "";
	
	str += b64;
	free(b64);
	
	// FIXME: does not caring about the return value leak memory?
	md->addMetadata("Thumbnail", str);
}


void DataObject::setIsForLocalApp(const bool val)
{
        isForLocalApp = val;
	toMetadata(); // MOS - metadata may depend on this
}

void DataObject::setIsForMonitorApp(const bool val)
{
        isForMonitorApp = val;
}

ssize_t DataObject::putData(void *_data, size_t len, size_t *remaining, bool putHeaderOnly)
{
        pDd info = (pDd) putData_data;
        unsigned char *data = (unsigned char *)_data;
        ssize_t putLen = 0;
	
        if (!remaining)
                return -1;
        
        if (len == 0)
                return 0;
        
        *remaining = DATAOBJECT_METADATA_PENDING;
	
        // Is this data object being built?
        if (putData_data == NULL) {
                // Put on a finished data object should return 0 put bytes.
                *remaining = 0;
                return 0;
        }
	
        // Has the metadata been filled in yet?
        if (metadata == NULL) {
                
                // No. Insert the bytes given into the header buffer first:
                /*
                  FIXME: this function searches for the XML end tag </haggle> (or
                  <haggle ... />) to determine where the metadata ends. This REQUIRES
                  the use of XML as metadata, or this function will never stop adding
                  to the header.
                */

		if (info->header_len + len >= DATAOBJECT_MAX_METADATA_SIZE) {
			HAGGLE_ERR("Header length %lu exceeds maximum length %lu\n", 
				info->header_len + len, DATAOBJECT_MAX_METADATA_SIZE);
			return -1;
		} 
		if (info->header_len + len > info->header_alloc_len) {
			unsigned char *tmp;
			/* We allocate a larger chunk of memory to put the header data into 
			and then hope the header fits. If the chunk proves to be too small, 
			we increase the size in a future put call.
			*/
			tmp = (unsigned char *)realloc(info->header, info->header_len + len + 1024);
			
                        if (tmp == NULL)
                                return -1;

                        info->header = tmp;
                        info->header_alloc_len = info->header_len + len + 1024;
		}
		
                // Add the data, byte for byte:
                while (len > 0 && !metadata) {
                        // Insert another byte into the header:
                        info->header[info->header_len] = data[0];
                        info->header_len++;
                        data++;
                        putLen++;
                        len--;

			// MOS
                        if (info->header_len == 7) {
                                if (info->header[0] == '<' && info->header[1] == '?'
                                    && (info->header[2] == 'X' || info->header[2] == 'x')
                                    && (info->header[3] == 'M' || info->header[3] == 'm')
                                    && (info->header[4] == 'L' || info->header[4] == 'l')) {
				}
				else if (info->header[0] == '<'
					 && (info->header[1] == 'H' || info->header[1] == 'h')
					 && (info->header[2] == 'A' || info->header[2] == 'a')
					 && (info->header[3] == 'G' || info->header[3] == 'g')
					 && (info->header[4] == 'G' || info->header[4] == 'g')
					 && (info->header[5] == 'L' || info->header[5] == 'l')
					 && (info->header[6] == 'E' || info->header[6] == 'e')) {
				} else {
				  free_pDd_header(info);
				  HAGGLE_ERR("Cannot recognize header - <?XML..> or <HAGGLE..> tag missing\n");
				  return -1;
				}
			}
		
			
                        // Is this the end of the header?
                        // FIXME: find a way to search for the <haggle ... /> style header.
                        if (info->header_len >= 9) {
                                if (info->header[info->header_len - 9] == '<' && info->header[info->header_len - 8] == '/'
                                    && (info->header[info->header_len - 7] == 'H' || info->header[info->header_len - 7] == 'h')
                                    && (info->header[info->header_len - 6] == 'A' || info->header[info->header_len - 6] == 'a')
                                    && (info->header[info->header_len - 5] == 'G' || info->header[info->header_len - 5] == 'g')
                                    && (info->header[info->header_len - 4] == 'G' || info->header[info->header_len - 4] == 'g')
                                    && (info->header[info->header_len - 3] == 'L' || info->header[info->header_len - 3] == 'l')
                                    && (info->header[info->header_len - 2] == 'E' || info->header[info->header_len - 2] == 'e')
                                    && info->header[info->header_len - 1] == '>') {
                                        // Yes. Yay!

                                        metadata = new XMLMetadata();

                                        if (!metadata) {
						free_pDd_header(info);
                                                HAGGLE_ERR("Could not create metadata\n");
                                                return -1;
                                        }

					if (!metadata->initFromRaw(info->header, info->header_len)) {
						free_pDd_header(info);
                                                HAGGLE_ERR("data object header not could not be parsed\n");
                                                delete metadata;
						metadata = NULL;
                                                return -1;
					}

                                        if (metadata->getName() != "Haggle") {
						free_pDd_header(info);
                                                HAGGLE_ERR("Metadata not recognized\n");
                                                delete metadata;
						metadata = NULL;
                                                return -1;
                                        }

                                        if (parseMetadata(true) < 0) {
						free_pDd_header(info);
                                                HAGGLE_ERR("Parse metadata on new data object failed\n");
						delete metadata;
						metadata = NULL;
                                                return -1;
                                        }
					free_pDd_header(info);
                                }
                        }
                }
        }	
        // Is the file to write into already open?
        if (info->fp == NULL && metadata) {

                // Figure out how many bytes should be put into the file:
                info->bytes_left = ABEStatus == DataObject::ABE_ENCRYPTION_DONE ? getEncryptedFileLength() : getDataLen(); // CBMEN, HL

                *remaining = info->bytes_left;

                // Any bytes at all?
                if (info->bytes_left == 0) {
                        // Nope.

                        // We're done with the info:
                        free_pDd();
                        *remaining = 0;
                        return putLen;
                }

		// MOS
		if(putHeaderOnly) {
		  return putLen;
		}

                // Create the path and file were we store the data.
		createFilePath();

                HAGGLE_DBG("Going to put %lu bytes into file %s\n", 
                           info->bytes_left, filepath.c_str());

                // Open the file for writing:
                info->fp = fopen(filepath.c_str(), "wb");
		if(info->fp) HAGGLE_DBG2("Opening file %s for writing with file descriptor %d\n", filepath.c_str(), fileno(info->fp));
		
                if (info->fp == NULL) {
		        if(errno == ENFILE) setFatalError();
			free_pDd();
                        HAGGLE_ERR("Could not open %s for writing data object data - error: %s (%d))\n", 
                                   filepath.c_str(),  strerror(errno), errno);
                        return -1;
                }
        }
        // If we just finished putting the metadata header, then len will be
        // zero and we should return the amount put.
        if (len == 0)
                return putLen;

        // Is this less data than what is left?
        if (info->bytes_left > len) {
                // Write all the data to the file:
                size_t nitems = fwrite(data, len, 1, info->fp);
		
                if (nitems != 1) {
                        // TODO: check error values with, e.g., ferror()?
                        HAGGLE_ERR("Error-1 on writing %lu bytes to file %s, nitems=%lu - error: %s (%d)\n", 
                                   len, getFilePath().c_str(), nitems, strerror(errno), errno);
			free_pDd();
                        return -1;
                }

                putLen = len;

                // Decrease the amount of data left:
                info->bytes_left -= putLen;
                // Return the number of bytes left to write:
                *remaining = info->bytes_left;
        } else if (info->bytes_left > 0) {
		
                // Write the last bytes of the actual data object:
                size_t nitems = fwrite(data, info->bytes_left, 1, info->fp);
                // We're done with the file now:
				
                if (nitems != 1) {
                        // TODO: check error values with, e.g., ferror()?
                        HAGGLE_ERR("Error-2 on writing %lu bytes to file %s, nitems=%lu - error: %s (%d)\n", 
                                   info->bytes_left, getFilePath().c_str(), nitems, strerror(errno), errno);
			free_pDd();
                        return -1;
                }
                fclose(info->fp);
                info->fp = NULL;

		restoreEncryptedDataObject(); // MOS
		
                putLen = info->bytes_left;

                info->bytes_left -= putLen;
                *remaining = info->bytes_left;

		free_pDd();
        }
	
        return putLen;
}

class DataObjectDataRetrieverImplementation : public DataObjectDataRetriever {
    public:
        /**
           The data object. We keep a reference to it to make sure it isn't 
           deleted. Just in case it would be in charge of deleting its file or 
           something.
        */
        DataObjectRef dObj; // MOS - no const
        /// The metadata header
        unsigned char *header;
        /// The length of the metadata header (above):
        size_t header_len;
        /// File pointer to the file where the data object's data is stored:
        FILE *fp;
        /// The amount of header data left to read:
        size_t header_bytes_left;
        /// The amount of data left to read from the data file:
        size_t bytes_left;
	
        DataObjectDataRetrieverImplementation(DataObjectRef _dObj); // MOS - no const
        ~DataObjectDataRetrieverImplementation();
	
        ssize_t retrieve(void *data, size_t len, bool getHeaderOnly);
	bool isValid() const;
};

DataObjectDataRetrieverImplementation::DataObjectDataRetrieverImplementation(DataObjectRef _dObj) :
                dObj(_dObj), header(NULL), header_len(0), fp(NULL), header_bytes_left(0), bytes_left(0)
{ 
    // CBMEN, HL, Begin
    if (dObj->getABEStatus() >= DataObject::ABE_ENCRYPTION_DONE) {

        if (dObj->getEncryptedFileLength() > 0 && !dObj->isForLocalApp) {

            fp = fopen(dObj->getEncryptedFilePath().c_str(), "rb");
            
            if (fp == NULL) {
	        if(errno == ENFILE) dObj->setFatalError();
                HAGGLE_ERR("ERROR: Unable to open file \"%s\". dataLen = %lu\n",
                        dObj->getEncryptedFilePath().c_str(), dObj->getEncryptedFileLength());
                goto fail_open;
            }
            
            bytes_left = dObj->getEncryptedFileLength();

            HAGGLE_DBG("Opened file %s for reading, %lu bytes, data object [%s]\n",
                    dObj->getEncryptedFilePath().c_str(), bytes_left, dObj->getIdStr());

        } else {

            if (dObj->isForLocalApp) { 
                HAGGLE_DBG("Data Object [%s] is for local app, not retrieving data.\n", dObj->getIdStr());
            } else {
                if (dObj->getEncryptedFilePath().length() > 0) {
                    HAGGLE_ERR("Data length is 0, even though encrypted file path \'%s\' is valid.\n",
                            dObj->getEncryptedFilePath().c_str());
                } else {
                    HAGGLE_DBG("No file associated with Data Object [%s].\n", dObj->getIdStr());
                }
            }
            fp = NULL;
            bytes_left = 0;
        }
        
    } else if (dObj->getABEStatus() == DataObject::ABE_NOT_NEEDED) {

    // CBMEN, HL, End
	if (dObj->getDataLen() > 0 && !dObj->isForLocalApp) {
                
                fp = fopen(dObj->getFilePath().c_str(), "rb");
		if(fp) HAGGLE_DBG2("Opening file %s with file descriptor %d\n", dObj->getFilePath().c_str(), fileno(fp));

                if (fp == NULL) {
		        if(errno == ENFILE) dObj->setFatalError();
                        HAGGLE_ERR("Unable to open file \"%s\" - error: %s (%d) - dataLen=%lu\n", 
                                   dObj->getFilePath().c_str(),  strerror(errno), errno, dObj->getDataLen());
                        // Unable to open file - fail:
                        goto fail_open;
                }

		bytes_left = dObj->getDataLen();

		HAGGLE_DBG("Opened file %s for reading, %lu bytes, data object [%s]\n", 
			dObj->getFilePath().c_str(), bytes_left, dObj->getIdStr());
	} else {
		if (dObj->isForLocalApp) {
			HAGGLE_DBG("Data object [%s] is scheduled for local application, not retrieving data\n", dObj->getIdStr());
		} else {
			if (dObj->getFilePath().length() > 0) {
				HAGGLE_ERR("Data length is 0, although filepath \'%s\' is valid\n", 
					   dObj->getFilePath().c_str());
			} else {
			        //HAGGLE_DBG("No file associated with data object [%s]\n", dObj->getIdStr());
			}
		}

		fp = NULL;
		bytes_left = 0;
        }
    } // CBMEN, HL

        // Find header size:
        if (!dObj->getRawMetadataAlloc(&header, &header_len)) {
                HAGGLE_ERR("Unable to retrieve header.\n");
                goto fail_header;
        }
	
        // Remove trailing characters up to the end of the metadata:
        while (header_len && (char)header[header_len-1] != '>') {
                header_len--;
        }
	
	if (header_len <= 0)
		goto fail_header;

        // The entire header is left to read:
        header_bytes_left = header_len;

        // Succeeded!
        return;

fail_header:
        // Close the file:
        if (fp != NULL)
	        fclose(fp);
fail_open:
        // Failed!
        HAGGLE_ERR("Unable to start getting data!\n");
        return;
}

DataObjectDataRetrieverImplementation::~DataObjectDataRetrieverImplementation()
{
        // Close the file:
        if (fp != NULL)
	        fclose(fp);

        // We must free the metadata header
        if (header)
                free(header);
}

bool DataObjectDataRetrieverImplementation::isValid() const
{
	return (header != NULL && header_len > 0);
}

DataObjectDataRetrieverRef DataObject::getDataObjectDataRetriever(void) const
{
       DataObjectDataRetrieverImplementation *retriever = new DataObjectDataRetrieverImplementation(this);

       if (!retriever)
	       return NULL;

       if (!retriever->isValid()) {
	       delete retriever; // MOS - was missing
	       return NULL;
       }

       return retriever;
}

ssize_t DataObjectDataRetrieverImplementation::retrieve(void *data, size_t len, bool getHeaderOnly)
{
        size_t readLen = 0;
	
        // Can we fill in that buffer?
        if (len == 0)
                // NO: can't do the job:
                return 0;
	
        // Is there anything left to read in the header?
        if (header_bytes_left) {
                // Yep.
		
                // Is there enough header left to fill the buffer, or more?
                if (header_bytes_left >= len) {
                        // Yep. Fill the buffer:
                        memcpy(data, &header[header_len - header_bytes_left], len);
                        header_bytes_left -= len;
                        return len;
                } else {
                        // Nope. Fill the buffer with the rest of the header:
                        readLen = header_bytes_left;
			
                        memcpy(data, &header[header_len - header_bytes_left], header_bytes_left);
                        data = (char *)data + header_bytes_left;
                        len -= header_bytes_left;
                        header_bytes_left = 0;
                }
        }
		if (getHeaderOnly)
			return readLen;
        if (!fp) {
                return readLen;
        }
	
        // Make sure we don't try to read too much data:
        if (len > bytes_left)
                len = bytes_left;
	
        // Read data.
        ssize_t fstart = ftell(fp);
	if(fstart < 0) { // MOS - added missing error check
	  HAGGLE_ERR("Error-1 when checking file position, error: %s (%d)\n", 
		     strerror(errno), errno);
	  return -1;
	}

        size_t nitems = fread(data, len, 1, fp);

        ssize_t fstop = ftell(fp);
	if(fstop < 0) { // MOS - added missing error check
	  HAGGLE_ERR("Error-2 when checking file position, error: %s (%d)\n", 
		     strerror(errno), errno);
	  return -1;
	}
	
        readLen += (fstop - fstart);
        // Count down the amount of data left to read:
        bytes_left -= (fstop - fstart);
	
        // Are we done?
        if (bytes_left == 0) {
                HAGGLE_DBG("EOF reached, readlen=%u, nitems=%lu\n", ftell(fp) - fstart, nitems);
                // Yep. Close the file...
                fclose(fp);
                // ...and set the file pointer to NULL
                fp = NULL;
                return readLen;
        } else {
                //HAGGLE_DBG("Read %lu bytes data from file\n", fstop - fstart);
		
                // Was there an error?
                if (nitems != 1) {
                        // Check what kind:
                        if (ferror(fp)) {
                                HAGGLE_ERR("Tried to fill the buffer but failed, nitems=%lu\n", nitems);
                                return -1;
                        } else if (feof(fp)) {
                                HAGGLE_DBG("End of file reached, readlen=%u, bytes left=%ld\n", fstop - fstart, bytes_left);
				
                                // Close file stream here and set to NULL.
                                // Next time this function is called readLen will be 0
                                // and returned, indicating end of file
                                fclose(fp);
                                fp = NULL;
                                return readLen;
                        }
                        HAGGLE_ERR("Undefined read error\n");
                        return -1;
                }
        }
	
        // Return how many bytes were read:
        return readLen;
}
void DataObject::setCreateTime(Timeval t)
{
        if (!metadata)
                return;
        
        createTime = t;

	if (createTime.isValid())
		metadata->setParameter(DATAOBJECT_CREATE_TIME_PARAM, createTime.getAsString());

	removeSignature(); // MOS
        calcId();
}

// MOS
void DataObject::setUpdateTime(Timeval t)
{
        if (!metadata)
                return;
        
        updateTime = t;

	if (updateTime.isValid())
		metadata->setParameter(DATAOBJECT_UPDATE_TIME_PARAM, updateTime.getAsString());

	removeSignature(); // MOS
        calcId();
}

bool DataObject::addAttribute(const Attribute& a)
{
	if (hasAttribute(a))
		return false;
	
        bool ret = attrs.add(a);
       
	removeSignature(); // MOS
        calcId();

        return ret;
}

bool DataObject::addAttribute(const string name, const string value, const unsigned long weight)
{
        Attribute attr(name, value, weight);
        
        return addAttribute(attr);
}

size_t DataObject::removeAttribute(const Attribute& a)
{
        size_t n = attrs.erase(a);

        if (n > 0) {
	        removeSignature(); // MOS
                calcId();
	}
        return n;
}

size_t DataObject::removeAttribute(const string name, const string value)
{
        size_t n;

        if (value == "*")
                n = attrs.erase(name);
        else {
                Attribute attr(name, value);
                n = attrs.erase(attr);
        }
        
        if (n > 0) {
	        removeSignature(); // MOS
                calcId();
	}

        return n;
}

const Attribute *DataObject::getAttribute(const string name, const string value, const unsigned int n) const 
{
        Attribute a(name, value, n);
        Attributes::const_iterator it = attrs.find(a);

        if (it == attrs.end())
                return NULL;

        return &(*it).second;
} 

const Attributes *DataObject::getAttributes() const 
{
        return &attrs;
} 

bool DataObject::hasAttribute(const Attribute &a) const
{
	return getAttribute(a.getName(), a.getValue(), a.getWeight()) != NULL;
}

DataObject::DataState_t DataObject::verifyData()
{
        SHA_CTX ctx;
        FILE *fp;
        unsigned char *data;
        DataHash_t digest;
        bool success = false;
        size_t read_bytes;
        string filename; // CBMEN, HL
        unsigned char *compareHash; // CBMEN, HL
                        
        // CBMEN, HL, Begin
        if (encryptedFilePath.length()) { // MOS
            filename = encryptedFilePath;
            compareHash = encryptedFileHash;
            if (encryptedFileLength == 0) return dataState;
        } else {
            filename = filepath;
            compareHash = dataHash;
	    if (dataLen == 0) return dataState;
        }
        // CBMEN, HL, End
			
        // Is there a data hash
        if (!dataIsVerifiable())
		return dataState;

	if (dataState == DATA_STATE_VERIFIED_OK || dataState == DATA_STATE_VERIFIED_BAD)
		return dataState;

	dataState = DATA_STATE_VERIFIED_BAD;

        // Allocate a data buffer:
        data = (unsigned char *) malloc(4096);

	if (data == NULL)
                return dataState;

        // Open the file:
        fp = fopen(filename.c_str(), "rb"); // CBMEN, HL
	if(fp) HAGGLE_DBG2("Opening file %s with file descriptor %d\n", filename.c_str(), fileno(fp));

	if (fp == NULL) {
		if(errno == ENFILE) setFatalError();
	        HAGGLE_ERR("Could not open file %s - error: %s (%d)\n", filename.c_str(), strerror(errno), errno);
                return dataState;
	}
        // Initialize the SHA1 hash context
        SHA1_Init(&ctx);
					
        // Go through the file until there is no more, or there was
        // an error:
        while (!(feof(fp) || ferror(fp))) {
                // Read up to 4096 bytes more:
                read_bytes = fread(data, 1, 4096, fp);
                // Insert them into the hash:
                SHA1_Update(&ctx, data, read_bytes);
        }
        // Was there an error?
        if (!ferror(fp)) {
                // No? Finalize the hash:
                SHA1_Final(digest, &ctx);
					
                // Flag success:
                success = true;
        }
        // Close the file
        fclose(fp);
        // Free the data buffer
        free(data);

        // Did we succeed above?
	if (!success)
		return dataState;
                
        // Yes? Check the hash:
        if (memcmp(compareHash, digest, sizeof(DataHash_t)) != 0) { // CBMEN, HL
                // Hashes not equal. Fail.
		HAGGLE_ERR("Verification failed: The data hash is not the same as the one in the data object\n");
                return dataState;
        }

	dataState = DATA_STATE_VERIFIED_OK;

        return dataState;
}

int DataObject::parseMetadata(bool from_network)
{
        const char *pval;

        if (!metadata)
                return -1;

        // Check persistency
        pval = metadata->getParameter(DATAOBJECT_PERSISTENT_PARAM);
        
        if (pval) {
		if (strcmp(pval, "no") == 0 || strcmp(pval, "false") == 0)
			persistent = false;
		else if (strcmp(pval, "yes") == 0 || strcmp(pval, "true") == 0)
			persistent = true;
        }

        // Check create time
	pval = metadata->getParameter(DATAOBJECT_CREATE_TIME_PARAM);

	if (pval) {
		Timeval ct(pval);
		createTime = ct;
	}
	
        // Check update time
	pval = metadata->getParameter(DATAOBJECT_UPDATE_TIME_PARAM);

	if (pval) {
		Timeval ut(pval);
		updateTime = ut;
	}
	
	// Check if this is a node description. 
	Metadata *m = metadata->getMetadata(NODE_METADATA);

	if (m)
		isNodeDesc = true;

	// Check if this is a control message from an application
	m = metadata->getMetadata(DATAOBJECT_METADATA_APPLICATION);

	if (m) {
		if (m->getMetadata(DATAOBJECT_METADATA_APPLICATION_CONTROL)) {
			controlMessage = true;
		}
	}
    
    // CBMEN, HL - Move this up so that we can use attribute below
    // Parse attributes
        Metadata *mattr = metadata->getMetadata(DATAOBJECT_ATTRIBUTE_NAME);
        
        while (mattr) {
                const char *attrName = mattr->getParameter(DATAOBJECT_ATTRIBUTE_NAME_PARAM);
                const char *weightStr = mattr->getParameter(DATAOBJECT_ATTRIBUTE_WEIGHT_PARAM);
                // unsigned long weight = weightStr ? strtoul(weightStr, NULL, 10) : 1;
                unsigned long weight = weightStr ? strtoul(weightStr, NULL, 10) : 0; // MOS - use 0 as default to reduce header size

                Attribute a(attrName, mattr->getContent(), weight);

                if (a.getName() == NODE_DESC_ATTR)
                        isNodeDesc = true;
                        
                if (!hasAttribute(a))
            attrs.add(a);
        
                mattr = metadata->getNextMetadata();
        }
    
        // Parse the "Data" tag if it exists
        Metadata *dm = metadata->getMetadata(DATAOBJECT_METADATA_DATA);

        if (dm) {
                // Check the data length
                pval = dm->getParameter(DATAOBJECT_METADATA_DATA_DATALEN_PARAM);
                
                if (pval) {
                        dataLen = strtoul(pval, NULL, 10);
			origDataLen = dataLen;
		}

                // Check optional file metadata
                Metadata *m = dm->getMetadata(DATAOBJECT_METADATA_DATA_FILENAME);

		if (m) {
			filename = m->getContent();
		}

		m = dm->getMetadata(DATAOBJECT_METADATA_DATA_FILEPATH);
                
                if (m) {
                        filepath = m->getContent();

                        HAGGLE_DBG("Data object has file path: %s\n", filepath.c_str());
		
                        /*
                          The fopen() gets the file size of the file that is given in the
                          metadata. This really only applies to locally generated data
                          objects that are receieved from applications. In this case, the
                          payload will not arrive over the socket from the application, but
                          rather the local file is being pointed to by the file attribute in
                          the metadata. The file path and file size is in this case read
                          here with the fopen() call.

                          If the data object arrives from another node (over, e.g., the
                          network) the payload will arrive as part of a byte stream, back-
                          to-back with the metadata header. In that case, this call will
                          fail when the metadata attributes are checked here. This is
                          perfectly fine, since the file does not exists yet and is
                          currently being put.
                        */
			if (!setFilePath(filepath, dataLen, from_network)) {
				return false;
			}

			if (filename.length() == 0) {
				// Hmm, in Windows we might need to look for a backslash instead
#ifdef OS_WINDOWS
				filename = filepath.substr(filepath.find_last_of('\\') + 1);
#else 
				filename = filepath.substr(filepath.find_last_of('/') + 1);
#endif
				HAGGLE_DBG("Filename from filepath is %s\n", filename.c_str());

				// Add the missing filename metadata
				dm->addMetadata(DATAOBJECT_METADATA_DATA_FILENAME, filename);
			}
                        // Remove filepath from the metadata since it is only valid locally
                        if (!dm->removeMetadata(DATAOBJECT_METADATA_DATA_FILEPATH)) {
                                HAGGLE_ERR("Could not remove filepath metadata\n");
                        }                                
                }
		
		if (dataState == DataObject::DATA_STATE_UNKNOWN) {
			// Check if there is a hash
			HAGGLE_DBG("Data object data state is UNKNOWN, checking for data hash\n");

			m = dm->getMetadata(DATAOBJECT_METADATA_DATA_FILEHASH);

			if (m) {
				base64_decode_context ctx;
				size_t len = sizeof(DataHash_t);
				base64_decode_ctx_init(&ctx);

				if (base64_decode(&ctx, m->getContent().c_str(), m->getContent().length(), (char *)dataHash, &len)) {
					HAGGLE_DBG("Data object has data hash=[%s]\n", m->getContent().c_str());
					dataState = DATA_STATE_NOT_VERIFIED;
				}
			}
		}
        }

        
	// MOS - now happening at the end - because previous updates can remove signature

	if (signatureStatus == DataObject::SIGNATURE_MISSING) {
		Metadata *sm = metadata->getMetadata(DATAOBJECT_METADATA_SIGNATURE);

		if (sm) {
			base64_decode_context ctx;

			base64_decode_ctx_init(&ctx);

			if (signature) {
				free(signature);
				signature = NULL;
				signature_len = 0;
			}
			signee = sm->getParameter(DATAOBJECT_METADATA_SIGNATURE_SIGNEE_PARAM);

			if (!base64_decode_alloc(&ctx, sm->getContent().c_str(), sm->getContent().length(), (char **)&signature, &signature_len)) {
				HAGGLE_ERR("Could not create signature from metadata\n");
				return -1;
			}

			signatureStatus = DataObject::SIGNATURE_UNVERIFIED;
		}
	}

        return calcId();
}

/*
  We base the unique ID of a data object on its attributes and create
  time. This means we can add other metadata to the header (e.g., for
  piggy-backing), without making the data object "different" in terms
  of what it represents to applications. Ideally, only the attributes
  and perhaps the hash of the payload (e.g., the associated data file)
  should define the object, so that two identically pieces of content
  that are published by different sources are actually seen as the
  same data objects -- in a true data-centric fashion.
 */
int DataObject::calcId()
{
        SHA_CTX ctxt;
        SHA1_Init(&ctxt);

        for (Attributes::iterator it = attrs.begin(); it != attrs.end(); it++) {
		// Insert the name of the attribute into the hash
                SHA1_Update(&ctxt, (*it).second.getName().c_str(), 
                            (*it).second.getName().length());
		
                // Insert the value of the attribute into the hash
                SHA1_Update(&ctxt, (*it).second.getValue().c_str(), 
                            (*it).second.getValue().length());
                
                // Insert the weight of the attribute into the hash.
		// We must use a specified length integer to make
		// sure the hash is the same on all architectures independent
		// of word/integer size.
                u_int32_t w = htonl((*it).second.getWeight());
                SHA1_Update(&ctxt, (unsigned char *) &w, sizeof(w));
        }

	// If this data object has a create time:
	if (createTime.isValid()) {
		// Add the create time to make sure the id is unique:
		SHA1_Update(&ctxt, (unsigned char *)createTime.getAsString().c_str(), createTime.getAsString().length());
	}

	/*
	 If the data object has associated data, we add the data's file hash.
	 If the data is a file, but there is no hash, then we instead use
	 the filename and data length.
	*/ 
	if (dataIsVerifiable()) {
	  if(encryptedFilePath.length() == 0) SHA1_Update(&ctxt, dataHash, sizeof(DataHash_t)); // MOS
	  else SHA1_Update(&ctxt, encryptedFileHash, sizeof(DataHash_t));
	} else if (filename.length() > 0 && dataLen > 0) {
		SHA1_Update(&ctxt, filename.c_str(), filename.length());
		SHA1_Update(&ctxt, &dataLen, sizeof(dataLen));

	}
	// Create the final hash value:
        SHA1_Final(this->id, &ctxt);

        // Also save as a string:
        calcIdStr();

	return 0;
}

// SW: START GET DOBJ FROM ID:
void
DataObject::idStrToId(string idStr, DataObjectId_t o_id) 
{
    if (!o_id) {
        return;
    }

	memset(o_id, 0, sizeof(DataObjectId_t));

    const char *str = idStr.c_str();

    if (NULL == str) {
        return;
    }

    int j = 0;
    for (unsigned int i = 0; i < strlen(str) && j < DATAOBJECT_ID_LEN; i+=2) {
        unsigned int c;
        sscanf(&str[i], "%02x", &c);
        o_id[j++] = (c & 0xff);
    }
}
// SW: END GET DOBJ FROM ID.

void DataObject::calcIdStr()
{
        int len = 0;

        // Generate a readable string of the Id
        for (int i = 0; i < DATAOBJECT_ID_LEN; i++) {
                len += sprintf(idStr + len, "%02x", id[i] & 0xff);
        }
}

bool operator==(const DataObject&a, const DataObject&b)
{
        return memcmp(a.id, b.id, sizeof(DataObjectId_t)) == 0;
}


bool operator!=(const DataObject&a, const DataObject&b)
{
        return memcmp(a.id, b.id, sizeof(DataObjectId_t)) != 0;
}

/*
  NOTE: Currently, the metadata is recreated/updated (at least
  partly), every time we do a getMetadata(). This is obviously not
  very efficient, but something we have to live with if we want to
  have some metadata that the data object understands and at the same
  time allow others to add their own metadata to the data object (for
  example, the "Node" tag added by the NodeManager).

  Ideally, the data object would only have one internal
  representation, and then that state is converted once to metadata at
  the time the data object goes onto the wire.
 */
Metadata *DataObject::getMetadata()
{
        return toMetadata();
}

const Metadata *DataObject::getMetadata() const
{
        return toMetadata();
}


const Metadata *DataObject::toMetadata() const
{
	return const_cast<DataObject *>(this)->toMetadata();
}

Metadata *DataObject::toMetadata()
{
	if (!metadata)
		return NULL;
	
	metadata->setParameter(DATAOBJECT_PERSISTENT_PARAM, persistent ? "yes" : "no");

	bool hasABEAttribute = getAttribute(DATAOBJECT_ATTRIBUTE_ABE) != NULL;
	
        // Create/update "Data" part of data object
        if ((dataLen && filename.length() != 0) || (encryptedFileLength && encryptedFilePath.length() != 0)) {
                char dataLenStr[20];
                Metadata *md = getOrCreateDataMetadata();

                if (!md)
                        return NULL;
                
                if ((isForLocalApp && filepath.length() != 0 && !hasABEAttribute) || encryptedFilePath.length() == 0) { // MOS
		  snprintf(dataLenStr, 20, "%lu", (unsigned long)dataLen);
		  md->setParameter(DATAOBJECT_METADATA_DATA_DATALEN_PARAM, dataLenStr);
		} else { // MOS
		  snprintf(dataLenStr, 20, "%lu", (unsigned long)encryptedFileLength);
		  md->setParameter(DATAOBJECT_METADATA_DATA_DATALEN_PARAM, dataLenStr);
		}

                /* Only add file path for data objects going to local applications. */
                if (isForLocalApp) {
                        Metadata *fpm = md->getMetadata(DATAOBJECT_METADATA_DATA_FILEPATH);

			if(filepath.length() != 0 && !hasABEAttribute) {
			  if (fpm) {
			    fpm->setContent(filepath);
			  } else {
			    md->addMetadata(DATAOBJECT_METADATA_DATA_FILEPATH, filepath);
			  }
			} else if(encryptedFilePath.length() != 0) {
			  if (fpm) {
			    fpm->setContent(encryptedFilePath);
			  } else {
			    md->addMetadata(DATAOBJECT_METADATA_DATA_FILEPATH, encryptedFilePath);
			  }
			}
                }
                Metadata *fnm = md->getMetadata(DATAOBJECT_METADATA_DATA_FILENAME);
                
                if (fnm) {
                        fnm->setContent(filename);
                } else {
                        md->addMetadata(DATAOBJECT_METADATA_DATA_FILENAME, filename);
                }       
        }

	if (dataIsVerifiable()) {
		char base64_hash[BASE64_LENGTH(sizeof(DataHash_t)) + 1];
		memset(base64_hash, '\0', sizeof(base64_hash));

                Metadata *md = getOrCreateDataMetadata();

		if (!md)
			return NULL;

                Metadata *fhm = md->getMetadata(DATAOBJECT_METADATA_DATA_FILEHASH);

                if ((isForLocalApp && filepath.length() != 0 && !hasABEAttribute) || encryptedFilePath.length() == 0) { // MOS
		  base64_encode((char *)dataHash, sizeof(DataHash_t), base64_hash, sizeof(base64_hash));
		} else {
		  base64_encode((char *)encryptedFileHash, sizeof(DataHash_t), base64_hash, sizeof(base64_hash));
		}

                if (fhm) {
                        fhm->setContent(base64_hash);
                } else {
                        md->addMetadata(DATAOBJECT_METADATA_DATA_FILEHASH, base64_hash);
                }
	}
	
	if (signature && signature_len) {
		Metadata *ms;
		char *base64_signature = NULL;

		if (base64_encode_alloc((char *)signature, signature_len, &base64_signature) > 0) {

			ms = metadata->getMetadata(DATAOBJECT_METADATA_SIGNATURE);

			if (ms) {
				ms->setContent(base64_signature);
			} else {
				ms = metadata->addMetadata(DATAOBJECT_METADATA_SIGNATURE, base64_signature);
			}

			if (ms) {
				ms->setParameter(DATAOBJECT_METADATA_SIGNATURE_SIGNEE_PARAM, signee.c_str());
			}
			free(base64_signature);
		}
	}
        // Sync attributes with metadata by first deleting the existing ones in the
        // metadata object and then adding the ones in our attributes container
        metadata->removeMetadata(DATAOBJECT_ATTRIBUTE_NAME);
        
        // Add attributes:
        for (Attributes::const_iterator it = attrs.begin(); it != attrs.end(); it++) {
                Metadata *mattr = metadata->addMetadata(DATAOBJECT_ATTRIBUTE_NAME, (*it).second.getValue());
              
                if (mattr) {
                        mattr->setParameter(DATAOBJECT_ATTRIBUTE_NAME_PARAM, (*it).second.getName());  
                        // if ((*it).second.getWeight() != 1) {
                        if ((*it).second.getWeight() != 0) { // MOS - use 0 as default to reduce header size
				char weight[20];
				snprintf(weight, 20, "%lu", (*it).second.getWeight());
                                mattr->setParameter(DATAOBJECT_ATTRIBUTE_WEIGHT_PARAM, weight);
			}
                }      
        }
        return metadata;
}

ssize_t DataObject::getRawMetadata(unsigned char *raw, size_t len) const
{
        if (!toMetadata())
                return -1;

        return metadata->getRaw(raw, len);
} 

bool DataObject::getRawMetadataAlloc(unsigned char **raw, size_t *len) const
{
        if (!toMetadata())
                return false;

        return metadata->getRawAlloc(raw, len);
} 

void DataObject::print(FILE *fp, bool truncate) const
{
	unsigned char *raw;
	size_t len;

	/* MOS - NULL means print to debug trace
	if (!fp) {
		HAGGLE_ERR("Invalid FILE pointer\n");
		return;
	}
	*/

	if (getRawMetadataAlloc(&raw, &len)) {

	  // MOS - added the following processing to 
	  // reduce output and avoid loss of nl
	  // (it would be cleaner to add a mechanism 
	  // for selective serialization to the metadata class)

	        size_t maxlen = 4000;
	        unsigned char buf[maxlen+1+3+3];
		size_t j = 0;
		size_t i = 0;

		while(i < len) {
		  if(j >= maxlen) { 
		    buf[j++] = '.'; buf[j++] = '.'; buf[j++] = '.';
		    break;
		  }
		  else if(truncate && i >= 12 && strncmp((const char *)&raw[i-12],"<Bloomfilter",12) == 0) {
		    buf[j++] = '.'; buf[j++] = '.'; buf[j++] = '.';
		    while(i < len && strncmp((const char *)&raw[i],"/Bloomfilter>",13) != 0) i++;
		  }
		  else if(truncate && i >= 12 && strncmp((const char *)&raw[i-12],"<Certificate",12) == 0) {
		    buf[j++] = '.'; buf[j++] = '.'; buf[j++] = '.';
		    while(i < len && strncmp((const char *)&raw[i],"/Certificate>",13) != 0) i++;
		  }
		  else {
		    buf[j++] = raw[i++];
		  }
		}

		buf[j] = 0;

		// MOS - printing buf instead of raw
	        if(fp) fprintf(fp, "DataObject id=%s:\n\n%s\n", getIdStr(), buf);
		else HAGGLE_DBG("DataObject id=%s:\n\n%s\n", getIdStr(), buf); // MOS

		free(raw);
	} else {
		HAGGLE_ERR("Could not allocate raw metadata\n");
	}

}

void DataObject::setData(const unsigned char *datahash, size_t datalen) {
    memcpy(dataHash, datahash, sizeof(DataHash_t));
    dataLen = datalen;
}

void DataObject::setEncryptedData(const unsigned char *encdatahash, size_t encdatalen) {
    memcpy(encryptedFileHash, encdatahash, sizeof(DataHash_t));
    encryptedFileLength = encdatalen;
    dataState = DataObject::DATA_STATE_NOT_VERIFIED;
}

// MOS 

bool DataObject::hasABEAttribute() {
  const Attribute *attr = getAttribute(DATAOBJECT_ATTRIBUTE_ABE);
  if (!attr) return false;

  const Attribute* attributeIsNetworkCoding = new Attribute(HAGGLE_ATTR_NETWORKCODING_NAME, "TRUE");
  bool hasAttributeIsNetworkCoding = hasAttribute(*attributeIsNetworkCoding);
  delete attributeIsNetworkCoding;
  if(hasAttributeIsNetworkCoding) return false;

  const Attribute* attributeIsFragmentation = new Attribute(HAGGLE_ATTR_FRAGMENTATION_NAME, "TRUE");
  bool hasAttributeIsFragmentation = hasAttribute(*attributeIsFragmentation);
  delete attributeIsFragmentation;
  if(hasAttributeIsFragmentation) return false;

  return true;
}

// MOS - restore encrypted state if recognized as encrypted data object

void DataObject::restoreEncryptedDataObject() {
  if (hasABEAttribute()) {
    if(encryptedFilePath.length() == 0 && filepath.length() > 0) {
      encryptedFilePath = filepath;
      memcpy(encryptedFileHash, dataHash, sizeof(DataHash_t));
      encryptedFileLength = dataLen;
      
      filepath = "";
      dataLen = 0;
      memset(dataHash, 0, sizeof(DataHash_t));

      calcId();

      setABEStatus(ABE_DECRYPTION_NEEDED);
      HAGGLE_DBG("Restored encrypted status for data object %s\n", DataObject::idString(this).c_str());
    } else {
      HAGGLE_ERR("unexpected condition: detected ABE attribute but file path is empty\n");
    }
  }
}
