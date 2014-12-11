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

#include <stdlib.h>
#include <base64.h>
#include "databuf.h"
#include "mini_base64.h"

data_buffer mini_base64_encode(char *data, size_t len)
{
	data_buffer retval;
	
	retval.data = NULL;
	
	retval.len = base64_encode_alloc(data, len, &(retval.data));
	
	return retval;
}

data_buffer mini_base64_decode(char *data, size_t len)
{
	data_buffer						retval;
	struct base64_decode_context	ctx;
	size_t							new_len;
	
	base64_decode_ctx_init(&ctx);

	if (!base64_decode_alloc(&ctx, data, len, &(retval.data), &new_len))
	{
		retval.data = NULL;
		retval.len = 0;
	}
	retval.len = new_len;
	
	return retval;
}
