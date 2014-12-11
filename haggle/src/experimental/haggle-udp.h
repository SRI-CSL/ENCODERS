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
#ifndef _HAGGLE_UDP_H
#define _HAGGLE_UDP_H
#ifdef __cplusplus
extern "C" {
#endif
	
#define HAGGLE_UDP_TYPE_DISCOVER		1
#define HAGGLE_UDP_TYPE_DATA			2

#define HAGGLE_UDP_SOCKET_PORT 12345

	struct haggle_udp {
		unsigned short type;
		unsigned long len;
		char data[0];
	};

	struct haggle_udp_fake_event {
		unsigned short type;
		unsigned long len;
		unsigned long delay;
	};

	int haggle_udp_create_socket(void);
	void haggle_udp_close_socket(int sock);
	
	int haggle_udp_read(int sock);
	int haggle_udp_sendDiscovery(int sock);
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
#endif /* _HAGGLE_UDP_H */
