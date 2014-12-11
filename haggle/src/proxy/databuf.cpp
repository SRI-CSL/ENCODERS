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

#include "databuf.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

data_buffer data_buffer_cons(data_buffer buf1, data_buffer buf2)
{
	data_buffer	retval;
	unsigned long	i;
	
	retval.len = buf1.len + buf2.len;
	retval.data = (char *) malloc(retval.len);
	if(retval.data == NULL)
	{
		retval.len = 0;
		return retval;
	}
	
	i = 0;
#define insert(b) \
	if((b).len != 0) \
	{ \
		memcpy(&(retval.data[i]), (b).data, (b).len); \
		i += (b).len; \
	}
	insert(buf1);
	insert(buf2);
	
	return retval;
}

data_buffer data_buffer_cons_free(data_buffer buf1, data_buffer buf2)
{
	data_buffer	retval;
	
	retval = data_buffer_cons(buf1, buf2);
	if(buf1.data != NULL)
		free(buf1.data);
	if(buf2.data != NULL)
		free(buf2.data);
	
	return retval;
}

data_buffer load_file(char *filename)
{
	FILE			*fp;
	data_buffer		retval;
	
	retval.len = 0;
	retval.data = NULL;
	
	fp = fopen(filename, "r");
	if(fp == NULL)
		goto fail_open;
	
	fseek(fp, 0, SEEK_END);
	retval.len = ftell(fp);
	if(retval.len == 0)
		goto fail_len;
	fseek(fp, 0, SEEK_SET);
	retval.data = (char *) malloc(retval.len);
	if(retval.data == NULL)
		goto fail_malloc;
	if(fread(retval.data, retval.len, 1, fp) != 1)
		goto fail_read;
	
	fclose(fp);
	
	return retval;
	
	if(0)
fail_read:
		fprintf(stderr, "Unable to read file!\n");
	free(retval.data);
	retval.data = NULL;
	if(0)
fail_malloc:
		fprintf(stderr, "Unable to malloc data!\n");
	retval.len = 0;
	if(0)
fail_len:
		fprintf(stderr, "Unable to get file length!\n");
	fclose(fp);
	if(0)
fail_open:
		fprintf(stderr, "Unable to open file!\n");
	return retval;
}
