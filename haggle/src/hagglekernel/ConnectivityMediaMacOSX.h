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
#ifndef _CONNECTIVITYMEDIAMACOSX_H
#define _CONNECTIVITYMEDIAMACOSX_H

/**
 Media connectivity manager module
 
 This module listens on FSEvents notifications for mounted/unmounted volumes.
 The volumes are treated the same way as interfaces.
 
 Reports to the connectivity manager when it finds new volumes.
 */
class ConnectivityMedia : public Connectivity
{
	void findRemoteInterfaces(char *path);
	void hookStopOrCancel();
	void hookCleanup();
	void run();
public:
	ConnectivityMedia(ConnectivityManager *m, const InterfaceRef& iface);
	~ConnectivityMedia();
};


#endif /* _CONNECTIVITYMEDIAMACOSX_H */
