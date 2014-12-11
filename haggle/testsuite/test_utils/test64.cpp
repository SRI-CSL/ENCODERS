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

#include "testhlp.h"

#include "base64.h"

#include <string.h>
#include <stdio.h>

#if defined(OS_WINDOWS)
int haggle_test_test64(void)
#else
int main(int argc, char *argv[])
#endif
{
	// This is the text we encode/decode. Has to be something, right? :P
	
	char	*license =
"/* Copyright 2008 Uppsala University\n"
" *\n"
" * Licensed under the Apache License, Version 2.0 (the \"License\"); \n"
" * you may not use this file except in compliance with the License. \n"
" * You may obtain a copy of the License at \n"
" *     \n"
" *     http://www.apache.org/licenses/LICENSE-2.0 \n"
" *\n"
" * Unless required by applicable law or agreed to in writing, software \n"
" * distributed under the License is distributed on an \"AS IS\" BASIS, \n"
" * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. \n"
" * See the License for the specific language governing permissions and \n"
" * limitations under the License.\n"
" */ \n",
	// This is what the text should encode to:
			*encoded_license =
				"LyogQ29weXJpZ2h0IDIwMDggVXBwc2Fs"
				"YSBVbml2ZXJzaXR5CiAqCiAqIExpY2Vu"
				"c2VkIHVuZGVyIHRoZSBBcGFjaGUgTGlj"
				"ZW5zZSwgVmVyc2lvbiAyLjAgKHRoZSAi"
				"TGljZW5zZSIpOyAKICogeW91IG1heSBu"
				"b3QgdXNlIHRoaXMgZmlsZSBleGNlcHQg"
				"aW4gY29tcGxpYW5jZSB3aXRoIHRoZSBM"
				"aWNlbnNlLiAKICogWW91IG1heSBvYnRh"
				"aW4gYSBjb3B5IG9mIHRoZSBMaWNlbnNl"
				"IGF0IAogKiAgICAgCiAqICAgICBodHRw"
				"Oi8vd3d3LmFwYWNoZS5vcmcvbGljZW5z"
				"ZXMvTElDRU5TRS0yLjAgCiAqCiAqIFVu"
				"bGVzcyByZXF1aXJlZCBieSBhcHBsaWNh"
				"YmxlIGxhdyBvciBhZ3JlZWQgdG8gaW4g"
				"d3JpdGluZywgc29mdHdhcmUgCiAqIGRp"
				"c3RyaWJ1dGVkIHVuZGVyIHRoZSBMaWNl"
				"bnNlIGlzIGRpc3RyaWJ1dGVkIG9uIGFu"
				"ICJBUyBJUyIgQkFTSVMsIAogKiBXSVRI"
				"T1VUIFdBUlJBTlRJRVMgT1IgQ09ORElU"
				"SU9OUyBPRiBBTlkgS0lORCwgZWl0aGVy"
				"IGV4cHJlc3Mgb3IgaW1wbGllZC4gCiAq"
				"IFNlZSB0aGUgTGljZW5zZSBmb3IgdGhl"
				"IHNwZWNpZmljIGxhbmd1YWdlIGdvdmVy"
				"bmluZyBwZXJtaXNzaW9ucyBhbmQgCiAq"
				"IGxpbWl0YXRpb25zIHVuZGVyIHRoZSBM"
				"aWNlbnNlLgogKi8gCgA=",
			*encoded = NULL, *decoded = NULL;
	size_t	license_len = strlen(license)+1,
			encoded_len = 0, decoded_len = 0;
	
	struct base64_decode_context	ctx;
	
	print_over_test_str_nl(0, "Base64 test: ");
	
	// Encode data:
	encoded_len = base64_encode_alloc(license, license_len, &encoded);
	
	print_over_test_str(1, "Base64 encode: ");
	// Just checking...
	if(encoded_len == 0 || encoded == NULL)
		return 1;
	
	// Checking encoding length:
	if(encoded_len != strlen(encoded_license))
		return 1;
	
	// Checking encoding:
	if(strncmp(encoded_license, encoded, encoded_len) != 0)
		return 1;
	
	print_passed();
	print_over_test_str(1, "Base64 decode: ");
	// Decode data:
	base64_decode_ctx_init(&ctx);
	
	if(!base64_decode_alloc(
		&ctx, 
		encoded, 
		encoded_len, 
		&decoded, 
		&decoded_len))
		return 1;
	
	// Just checking...
	if(decoded_len == 0 || decoded == NULL)
		return 1;
	
	// Compare:
	
	// Lengths:
	if(decoded_len != license_len)
		return 1;
	
	// Data:
	if(memcmp(decoded, license, license_len) != 0)
		return 1;
	
	print_passed();
	print_over_test_str(1, "Total: ");
	// Success:
	return 0;
}
