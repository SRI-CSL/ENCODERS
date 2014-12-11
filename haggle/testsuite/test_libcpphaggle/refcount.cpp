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
#include "Interface.h"

using namespace haggle;
/*
  This program tests refcounting
*/

static long pass_cppreference(const InterfaceRef &iface)
{
	return iface.refcount();
}

#if defined(OS_WINDOWS)
int haggle_test_refcount(void)
#else
	int main(int argc, char *argv[])
#endif
{	
	// Disable tracing
	trace_disable(true);

	print_over_test_str_nl(0, "Refcount test: ");

	try {
		bool success = true;
		bool tmp_succ;
		
		unsigned char mac[6] = { (unsigned char) RANDOM_INT(255), 
				(unsigned char) RANDOM_INT(255), 
				(unsigned char) RANDOM_INT(255), 
				(unsigned char) RANDOM_INT(255), 
				(unsigned char) RANDOM_INT(255), 
				(unsigned char) RANDOM_INT(255) };

		Interface *iface = new EthernetInterface(mac);
		
		try {
			{
				print_over_test_str(1, "Create and check refcount==0: ");
				InterfaceRef nullRef;
				
				tmp_succ = (!nullRef && nullRef.refcount() == 0);
				success &= tmp_succ;
				
				print_pass(tmp_succ);
				
				print_over_test_str(1, "Create and check refcount==1: ");
				InterfaceRef ifaceRef(iface);
				
				tmp_succ = (ifaceRef && ifaceRef.refcount() == 1);
				success &= tmp_succ;
				
				print_pass(tmp_succ);
				
				print_over_test_str(1, "Increase refcount with operator \'=\': ");
				{
					InterfaceRef ifaceRef2 = ifaceRef;
					
					tmp_succ = (ifaceRef2 && 
						    ifaceRef2.refcount() == 2 && 
						    ifaceRef.refcount() == 2);
					success &= tmp_succ;
					
					print_pass(tmp_succ);
				}
				
				print_over_test_str(1, "No refcount increase on passing C++ reference: ");
				long count = ifaceRef.refcount();

				tmp_succ = (pass_cppreference(ifaceRef) == count);
				success &= tmp_succ;

				print_pass(tmp_succ);

				print_over_test_str(1, "Decrease refcount due to context: ");
				
				tmp_succ = (ifaceRef.refcount() == 1);
				success &= tmp_succ;
				
				print_pass(tmp_succ);
			}

			InterfaceRef ifaceRef = new EthernetInterface(mac);
		
			print_over_test_str(1, "Refcount==0 when NULL: ");
		
			tmp_succ &= (ifaceRef && ifaceRef.refcount() == 1);
			success &= tmp_succ;

			ifaceRef = NULL;
			
			tmp_succ &= (!ifaceRef && ifaceRef.refcount() == 0);
			success &= tmp_succ;
			
			print_pass(tmp_succ);

			print_over_test_str(1, "No exception when refcounting object twice: ");
			iface = new EthernetInterface(mac);
			Interface *iface2 = iface;
			ifaceRef = InterfaceRef(iface);
			
			try {
				InterfaceRef ifaceRef = iface2;
				tmp_succ = true;
			} catch (Exception &) {
				tmp_succ = false;
			}
			success &= tmp_succ;
			print_pass(tmp_succ);

			print_over_test_str(1, "Decrease/increase refcount on reassignment: ");
			InterfaceRef ifaceRef3 = new EthernetInterface(mac);
			InterfaceRef ifaceRef2 = ifaceRef;
			
			tmp_succ = (ifaceRef.refcount() == 2 && 
				    ifaceRef2.refcount() == 2 && 
				    ifaceRef3.refcount() == 1);

 			ifaceRef2 = ifaceRef3;
			
			tmp_succ &= (ifaceRef.refcount() == 1 &&
				    ifaceRef2.refcount() == 2 && 
				    ifaceRef3.refcount() == 2);

			success &= tmp_succ;
 			print_pass(tmp_succ);

			print_over_test_str(1, "Refcount on reassignment with derived reference: ");
			EthernetInterfaceRef ifaceRef4 = new EthernetInterface(mac);
			InterfaceRef ifaceRef5 = ifaceRef4;
			WiFiInterfaceRef ifaceRef6 = ifaceRef5;

			tmp_succ &= (ifaceRef4.refcount() == 3 &&
				     ifaceRef5.refcount() == 3 &&
				     ifaceRef6.refcount() == 3);

			ifaceRef6 = NULL;

			tmp_succ &= (ifaceRef4.refcount() == 2 &&
				     ifaceRef5.refcount() == 2);

			success &= tmp_succ;
 			print_pass(tmp_succ);
			
			print_over_test_str(1, "Total: ");
					
		} catch(Exception &) {
			return 2;
		}
		return success ? 0 : 1;
	} catch(Exception &) {
		printf("**CRASH** ");
		return 1;
	}
}
