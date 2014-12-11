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
#include <libcpphaggle/Thread.h>
#include "utils.h"
#include <haggleutils.h>
#include "XMLMetadata.h"
#include "Interface.h"
#include "Node.h"

using namespace haggle;

#include <libxml/tree.h>
/*
  This program tests the Metadata (XML) class
*/

#if defined(OS_WINDOWS)
int haggle_test_metadata(void)
#else
int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Metadata test: ");
#if 0
	try {
		bool success = true;
		bool tmp_succ;
	
		try {
		
			Metadata *md = new XMLMetadata();

			print_over_test_str(1, "Add three attributes to metadata: ");
		
			Attribute attr("foo", "bar");
			Attribute attr2("foo", "baz");
			Attribute attr3("biz", "buz");
		
			tmp_succ = (md->addAttribute(attr) == 1 && md->numAttributes() == 1);
			tmp_succ &= (md->addAttribute(attr2) == 1 && md->numAttributes() == 2);
			tmp_succ &= (md->addAttribute(attr3) == 1 && md->numAttributes() == 3);
			
			success &= tmp_succ;
			print_pass(tmp_succ);

			print_over_test_str(1, "Get exact attribute: ");
		
			const Attribute *attr_ret = md->getAttribute("foo", "baz");
		
			tmp_succ = (attr_ret && attr_ret->getName() == attr2.getName() && 
				    attr_ret->getValue() == attr2.getValue());

			success &= tmp_succ;
			print_pass(tmp_succ);

			print_over_test_str(1, "Get n:th attribute (wildcard): ");
		
			attr_ret = md->getAttribute("foo", "*", 1);
	
			tmp_succ = (attr_ret && attr_ret->getName() == attr2.getName() && 
				    attr_ret->getValue() == attr2.getValue());

			success &= tmp_succ;
			print_pass(tmp_succ);

			print_over_test_str(1, "Remove one attribute: ");
		
			tmp_succ = (md->removeAttribute(attr3) == 1);
			tmp_succ &= (md->numAttributes() == 2);

			success &= tmp_succ;
			print_pass(tmp_succ);
		
			print_over_test_str(1, "Remove two attributes (wildcard): ");
		
			tmp_succ = (md->removeAttribute("foo", "*") == 2);
			success &= (md->numAttributes() == 0);

			success &= tmp_succ;
			print_pass(tmp_succ);

			print_over_test_str(1, "Add node to metadata: ");
		
			NodeRef myNode = NodeRef(new Node(NODE_TYPE_PEER, "TestNode"));
		
			// Add a couple of interfaces
			for (int i = 0; i < 2; i++) {
				char mac[6] = { (char) RANDOM_INT(255), 
						(char) RANDOM_INT(255), 
						(char) RANDOM_INT(255), 
						(char) RANDOM_INT(255), 
						(char) RANDOM_INT(255), 
						(char) RANDOM_INT(255) };
				Address addr(AddressType_BTMAC, mac);
				InterfaceRef myIface = InterfaceRef(new Interface(IFTYPE_BLUETOOTH, mac, &addr, "TestInterface"));
				myNode->addInterface(myIface);
			}

			tmp_succ = (md->addNode(myNode) > 0);
		
			success &= tmp_succ;
			print_pass(tmp_succ);
		
			print_over_test_str(1, "Create node from metadata: ");

			Node *cn = md->createNode();
		
			tmp_succ = (cn != NULL);

			success &= tmp_succ;
			print_pass(tmp_succ);
		
			if (cn)
				delete cn;

			print_over_test_str(1, "Remove node from metadata: ");
			char *raw;
			size_t rawlen;

			md->getRawAlloc(&raw, &rawlen);

			if (raw) {
				//fprintf(stdout, "before remove\n%s\n",  raw);
				free(raw);
			}
		
			tmp_succ = (md->removeNode(myNode) == 1);

			success &= tmp_succ;
			print_pass(tmp_succ);

			md->getRawAlloc(&raw, &rawlen);

			if (raw) {
				//fprintf(stdout, "after remove\n%s\n",  raw);
				free(raw);
			}
		
			delete md;

			print_over_test_str(1, "Total: ");
		} catch(Exception &) {
			return 2;
		}
		return success ? 0 : 1;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
#else
	return 0;
#endif
}
