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

#ifndef _databuf_h_
#define _databuf_h_

typedef struct {
	char			*data;
	unsigned long	len;
} data_buffer;

static inline data_buffer data_buffer_create(char *data, unsigned long len)
{
	data_buffer	retval;
	
	retval.len = len;
	retval.data = data;
	
	return retval;
}

#define data_buffer_copy(buf) \
	(data_buffer_cons((buf),data_buffer_create(NULL,0)))
#define data_buffer_create_copy(data, len) \
	(data_buffer_cons( \
		data_buffer_create((data),(len)), \
		data_buffer_create(NULL,0)))
data_buffer data_buffer_cons(data_buffer buf1, data_buffer buf2);
data_buffer data_buffer_cons_free(data_buffer buf1, data_buffer buf2);
data_buffer load_file(char *filename);

#endif

