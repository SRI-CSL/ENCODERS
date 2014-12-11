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
#include <libcpphaggle/Map.h>
#include <haggleutils.h>
#include <libcpphaggle/Exception.h>

using namespace haggle;
/*
  This program tests the map implementation
*/

class TestObj {
        static long num;       
    public:
        long i, val; 
        TestObj(long _val) : i(num++), val(_val) {}
        ~TestObj() { /* printf("obj num %ld, val=%ld destroyed\n", i, val); */ }

        friend bool operator==(const TestObj& o1, const TestObj& o2) {
                return o1.val == o2.val;
        }
        friend bool operator!=(const TestObj& o1, const TestObj& o2) {
                return o1.val != o2.val;
        }
        friend bool operator==(const TestObj& o, const long l) {
                return o.val == l;
        }
        friend bool operator!=(const TestObj& o, const long l) {
                return o.val != l;
        }
        friend bool operator==(const long l, const TestObj& o) {
                return l == o.val;
        }
        friend bool operator!=(const long l, const TestObj& o) {
                return l != o.val;
        }
};

long TestObj::num = 0;

#if defined(OS_WINDOWS) 
int haggle_test_map(void)
#else 
int main(int argc, char *argv[])
#endif

{	
// Disable tracing
trace_disable(true);

print_over_test_str_nl(0, "Map test: ");

try {
        bool success = true;
        bool tmp_succ;
		
        try {
                print_over_test_str(1, "Create: ");
                tmp_succ = true;
                try {
                        Map<long, TestObj *> test1_map;
                } catch (...) { 
                        tmp_succ = false;
                }
                success &= tmp_succ;
                print_pass(tmp_succ);
			
                {
			Map<long, TestObj *> test2_map;
			print_over_test_str(1, "Access with [] operator: ");
			tmp_succ = true;
			try {
				test2_map[23] = new TestObj(5);
				test2_map[5] = new TestObj(23);
				tmp_succ = (*test2_map[test2_map[5]->val] == 5);
			} catch (...) {
				tmp_succ = false;
			};
			success &= tmp_succ;
			print_pass(tmp_succ);
                }
			
                {
			Map<long, TestObj *> test3_map;
			print_over_test_str(1, "Iteration: ");
			test3_map[25] = new TestObj(3);
			test3_map[12] = new TestObj(3);
			test3_map[4711] = new TestObj(3);
			test3_map[42] = new TestObj(3);
			test3_map[1337] = new TestObj(3);
			
			tmp_succ = true;
			for(Map<long, TestObj *>::iterator it = test3_map.begin();
                            it != test3_map.end();
                            it++)
			{
				// These should both work, but it's good to check that both can
				// be compiled.
				if(*it->second != 3 || *(*it).second != 3)
					tmp_succ = false;
			}
			
			success &= tmp_succ;
			print_pass(tmp_succ);

			print_over_test_str(1, "Erase key 12: ");
			tmp_succ = true;

                        Map<long, TestObj *>::iterator it = test3_map.find(12);

                        if (it != test3_map.end()) {
                                TestObj *o = (*it).second;
                                test3_map.erase(it);
                                delete o;
                        }
                        
                        if (test3_map.find(12) != test3_map.end())
                                tmp_succ = false;
                        
			success &= tmp_succ;
			print_pass(tmp_succ);
                        
                        /*
                          This will test that an element can be added
                          again without allocation, now when we have a free
                          entry in the map.
                         */
			print_over_test_str(1, "Add key 12 again: ");
			tmp_succ = true;

			test3_map[12] = new TestObj(3);
                        
                        if (test3_map.find(12) == test3_map.end())
                                tmp_succ = false;

			success &= tmp_succ;
			print_pass(tmp_succ);

			print_over_test_str(1, "Copying: ");
			tmp_succ = true;

                        for  (int i = 0; i < 2; i++) {
                                Map<long, TestObj * > test4_map;
                                
                                //test4_map[4] = 3;
                                test4_map = test3_map;

                                tmp_succ = true;
                                Map<long, TestObj *>::iterator it1 = test3_map.begin();
                                Map<long, TestObj *>::iterator it2 = test4_map.begin();
                                
                                for(; it2 != test4_map.end();
                                    it1++, it2++)
                                {
                                        // These should both work, but it's good to check that both can
                                        // be compiled.
                                        if((*it1).first != (*it2).first || (*it1).second->val != (*it2).second->val)
                                                tmp_succ = false;
                                }
                        }
			
			success &= tmp_succ;
			print_pass(tmp_succ);
                        
			print_over_test_str(1, "Clearing: ");

                        // This will leak the TestObjs, but it doesn't
                        // really matter for the test.
			test3_map.clear();
			
			tmp_succ = true;
			for(Map<long, TestObj *>::iterator it = test3_map.begin();
                            it != test3_map.end();
                            it++)
			{
				// If we get here, there is something left in the map:
				tmp_succ = false;
			}
			success &= tmp_succ;
			print_pass(tmp_succ);
			
			print_over_test_str(1, "No more values: ");
			if(test3_map.find(25) != test3_map.end())
				tmp_succ = false;
			if(test3_map.find(12) != test3_map.end())
				tmp_succ = false;
			if(test3_map.find(4711) != test3_map.end())
				tmp_succ = false;
			if(test3_map.find(42) != test3_map.end())
				tmp_succ = false;
			if(test3_map.find(1337) != test3_map.end())
				tmp_succ = false;
			success &= tmp_succ;
			print_pass(tmp_succ);
                }
			
                print_over_test_str(1, "Total: ");
					
        } catch(...) {
                return 2;
        }
        return success ? 0 : 1;
} catch(...) {
        printf("**CRASH** ");
        return 1;
}
}
