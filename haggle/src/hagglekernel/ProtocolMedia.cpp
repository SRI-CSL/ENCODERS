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

/* experimental code to support memory sticks
 * todo: testing
 * todo: ageing
 * todo: extend support for several .haggle folders (which to read from?)
 */

#if defined(ENABLE_MEDIA) && defined(OS_MACOSX)

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <copyfile.h>

#include <cstring>

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

#include <haggleutils.h>
#include <libcpphaggle/Thread.h>
#include "ProtocolMedia.h"
#include "ProtocolManager.h"

using namespace std;

int metadataFile = 0;

ProtocolMedia::ProtocolMedia(const InterfaceRef& _localIface, const InterfaceRef& _peerIface, ProtocolManager * m) : 
	Protocol(Protocol::TYPE_MEDIA, "ProtocolMedia", _localIface, _peerIface, 
		 PROT_FLAG_CLIENT, m)
{
	HAGGLE_DBG("ProtocolMediaClient\n");
	remotePath = iface->getName();
}

ProtocolMedia::~ProtocolMedia()
{
}

bool ProtocolMedia::sendDataObject(DataObjectRef& dObj, InterfaceRef& to)
{
	int ret;

	/* Add to send queue */
	Protocol::sendDataObject(dObj, to);

	/* Call send function */
	ret = sendDataObjectNow();
	
	// Send success/fail event with this DO:
	if (ret == PROT_EVENT_SUCCESS)
		getHaggle()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL, dObj, getHaggle()->getNodeStore()->retrieve(to)));
	else
		getHaggle()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_SEND_FAILURE, dObj, getHaggle()->getNodeStore()->retrieve(to)));
	//ret = startTxRx();
	return true;
}

ProtocolEvent ProtocolMedia::sendDataObjectNow()
{
	size_t len = 0;
	ProtocolEvent pEvent;
	int ret = 0;
	bool hasFile = false;
	const char *fileName = NULL;
	const char *localFilepath = NULL;
	const char *remotePath = iface->getName();
	char *remoteFilepath = NULL;
	const char *metadataFilename = NULL;
	char *metadataFilepath = NULL;

	HAGGLE_DBG("BEGIN -----------------------------------------\n");
	
	QueueElement *qe;
	Queue *q = getQueue();

	if (!q)
		return PROT_EVENT_ERROR;

	q->retrieve(&qe);
	DataObjectRef dObj = qe->getDataObject();
	delete qe;
	
	if (!dObj) {
		error = PROT_ERROR_BAD_HANDLE;
		return PROT_EVENT_ERROR;
	}
	// copy file
	hasFile = (dObj->getDataLen() > 0);
	if (hasFile) {
		fileName = dObj->getFileName().c_str();
		remoteFilepath = (char *) malloc(strlen(remotePath) + strlen(fileName) + 2);
		sprintf(remoteFilepath, "%s/%s", remotePath, fileName);

/* chek if file already exists
		int file = open(remoteFilepath, );
		if (file) {
			
			close(file);
		}
*/

		localFilepath = dObj->getFilePath().c_str();

		ret = copyfile(localFilepath, remoteFilepath, NULL, COPYFILE_ALL | COPYFILE_EXCL);

		if (ret < 0) {
			HAGGLE_DBG("error copying file %s to media\n", localFilepath);
		}
	}
	// get Metadata -> buffer
        char *raw;
        size_t rawLen;

	if (!dObj->getRawMetadataAlloc(&raw, &rawLen)) {
		HAGGLE_DBG("%s:%lu Get raw metadata failed\n", getName(), getId());
		error = PROT_ERROR_UNKNOWN;
		return PROT_EVENT_ERROR;
	}
	// open file based on dataobject id
	dObj->calcId();
	dObj->calcIdStr();
	metadataFilename = dObj->getIdStr();
	metadataFilepath = (char *) malloc(strlen(remotePath) + strlen(metadataFilename) + 2);
	sprintf(metadataFilepath, "%s/%s.xml", remotePath, metadataFilename);

	/*
	   // if getIdStr() does not return a value we use a timestamp
	   Timeval n = Timeval::now();
	   metadataFilepath = (char*)malloc(strlen(remotePath) + 18 + 2);
	   sprintf(metadataFilepath, "%s/%ld%ld.xml", remotePath, n.getSeconds(), n.getMicroSeconds());
	 */

	// write to file
	HAGGLE_DBG("Writing %d bytes metadata to [%s]\n", len, metadataFilepath);

	metadataFile = open(metadataFilepath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
	pEvent = sendData(buffer, len, 0, &len);
	close(metadataFile);
	metadataFile = 0;
        
        free(raw);

	if (pEvent == PROT_EVENT_ERROR) {
		HAGGLE_DBG("problem with writing\n", metadataFilepath);
		return pEvent;
	}

	if (metadataFilepath)
		delete metadataFilepath;
	
	HAGGLE_DBG("END -----------------------------------------\n");

	return PROT_EVENT_SUCCESS;
}

ProtocolEvent ProtocolMedia::sendData(const void *buf, size_t len, const int flags, size_t *bytes)
{
	ssize_t writeLen;
	
	if (!metadataFile)
		return PROT_EVENT_ERROR;

	*bytes = 0;
	
	writeLen = write(metadataFile, buf, len);

	close(metadataFile);

	if (writeLen < 0)
		return PROT_EVENT_ERROR;
	
	*bytes = writeLen;
	
	return PROT_EVENT_SUCCESS;
}

void ProtocolMedia::run() {
	receiveDataObject();
}

ProtocolEvent ProtocolMedia::receiveDataObject()
{
	// read (own) haggle folder
	struct dirent **files;
	size_t len = 0;
	ProtocolEvent pEvent;
	bool hasFile = false;
	const char *fileName = NULL;
	char *localFilepath = NULL;
	const char *remotePath = iface->getName();
	char *remoteFilepath = NULL;
	
	// check if real local directory/interface
	//if (!iface->getMacAddr()) 
//		return PROT_EVENT_ERROR;
	
	
	HAGGLE_DBG("BEGIN -----------------------------------------\n");
	HAGGLE_DBG("%s\n", remotePath);
	int numFiles = scandir(remotePath, &files, NULL, NULL);

	for (int i = 0; i < numFiles; i++) {
		if (files[i]->d_type == DT_REG) {
			stringstream filename;
			filename << remotePath << "/" << files[i]->d_name;

			ifstream FileStream(filename.str().c_str(), ios::binary);
			FileStream.seekg(0, ios::end);
			int filesize = FileStream.tellg();
			FileStream.close();

			char *buf = (char *) malloc(filesize);
			metadataFile = open(filename.str().c_str(), O_RDONLY);
			
			pEvent = receiveData(buf, filesize, 0, &len);
			close(metadataFile);
			metadataFile = NULL;
			
			if (pEvent == PROT_EVENT_ERROR) {
				HAGGLE_ERR("%s:%d Error on receive data\n", getName(), getId());
				return pEvent;
			}
			HAGGLE_DBG("read %s (%d of %d bytes)\n", filename.str().c_str(), len, filesize);

			// create dataobject
			DataObject *dObj = DataObject::create(buf, len);

			if (!dObj) {
				return PROT_EVENT_ERROR;
			}
			
			// set remoteFilepath
			hasFile = (dObj->getDataLen() > 0);
			if (hasFile) {
				fileName = dObj->getFileName().c_str();
				remoteFilepath = (char *) malloc(strlen(remotePath) + 4 + strlen(fileName) + 1);
				sprintf(remoteFilepath, "%s/../%s", remotePath, fileName);
				dObj->createFilePath();
				localFilepath = (char *) dObj->getFilePath().c_str();

				// todo: check if file already exists. In that case, change filename
				// now: write over existing file...

				HAGGLE_DBG("copy file %s to %s\n", remoteFilepath, localFilepath);
				int ret = 0;
				ret = copyfile(remoteFilepath, localFilepath, NULL, COPYFILE_ALL);  //  COPYFILE_EXCL

				if (ret < 0) {
					HAGGLE_DBG("error copying file %s\n", remoteFilepath);
				}
				
				delete localFilepath;
				delete remoteFilepath;
			}
			// add event
			getHaggle()->addEvent(new Event(EVENT_TYPE_DATAOBJECT_RECEIVED, dObj));

			delete buf;
		}
	}

	HAGGLE_DBG("END -----------------------------------------\n");

	return PROT_EVENT_SUCCESS;
}

ProtocolEvent ProtocolMedia::receiveData(void *buf, size_t buflen, const int flags, size_t *bytes)
{
	ssize_t len;
	*bytes = 0;
	
	len = read(metadataFile, buf, buflen);
	
	if (len < 0)
		return PROT_EVENT_ERROR;
		
	*bytes = len;
	
	return PROT_EVENT_SUCCESS;
}

#endif
