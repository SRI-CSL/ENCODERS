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
#ifndef _PROTOCOLMEDIA_H
#define _PROTOCOLMEDIA_H

#include <libcpphaggle/Platform.h>

#if defined(ENABLE_MEDIA)
/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/

class ProtocolMedia;

#include "Protocol.h"


#define LOCAL_CONTENT_PATH "."		// without terminating /
// TODO: What is a good buffer size here?
#define BUFSIZE (METADATA_MAX_RAWSIZE+1) // +1 for '\0'-termination

/** */
class ProtocolMedia : public Protocol
{
        const char *remotePath;
        ProtocolEvent receiveDataObject();
        ProtocolEvent receiveData(void *buf, size_t buflen, const int flags, size_t *bytes);
        ProtocolEvent sendData(const void *buf, size_t buflen, const int flags, size_t *bytes);
	void run();
public:
        ProtocolMedia(const InterfaceRef& _localIface, const InterfaceRef& _peerIface, ProtocolManager *m);
        ~ProtocolMedia();

        ProtocolEvent sendDataObjectNow();
        bool sendDataObject(DataObjectRef& dObj, InterfaceRef& to);
};

#endif

#endif /* _PROTOCOLMEDIA_H */
