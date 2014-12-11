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
#ifndef _STRINGH_H_
#define _STRINGH_H_

#include <stdlib.h>

#if defined(ENABLE_STL)
#include <string>

using namespace std;
#define String string

// Dummy namespace
namespace haggle {}

#else

namespace haggle {

class String {
private:
	char *s;
	size_t alloc_len;  // the length of the malloc'd buffer
	size_t slen;  // the length of the string (i.e., strlen(s))
        static char nullchar;
        char *alloc(size_t len);
	String(const char *_s, size_t n);
public:
	static const size_t npos = -1; // the largest possible position
	String(const char *_s = NULL);
	String(const char c);
	String(const String& str);
        ~String();
	const char* c_str () const;
        size_t size() const;
        size_t length() const;
        void clear();
        bool empty () const;

        const char& at(size_t pos ) const;
        char& at (size_t pos );
        
        String substr(size_t pos = 0, size_t n = npos) const;

        String& append(const String& str);
        String& append(const String& str, size_t pos, size_t n);
        String& append(const char* s, size_t n);
        String& append(const char* s);
        String& append(size_t n, char c);

        int compare(const String& str) const;
        int compare(const char* s) const;
        int compare(size_t pos1, size_t n1, const String& str) const;
        int compare(size_t pos1, size_t n1, const char* s) const;
        int compare(size_t pos1, size_t n1, const String& str, size_t pos2, size_t n2) const;
        int compare(size_t pos1, size_t n1, const char* s, size_t n2) const;

        size_t find(const String& str, size_t pos = 0) const;
        size_t find(const char* s, size_t pos, size_t n) const;
        size_t find(const char* s, size_t pos = 0) const;
        size_t find(char c, size_t pos = 0) const;

        size_t find_first_of(const String& str, size_t pos = 0) const;
        size_t find_first_of(const char* s, size_t pos, size_t n) const;
        size_t find_first_of(const char* s, size_t pos = 0) const;
        size_t find_first_of(char c, size_t pos = 0) const;

        size_t find_last_of(const String& str, size_t pos = npos) const;
        size_t find_last_of(const char* s, size_t pos, size_t n) const;
        size_t find_last_of(const char* s, size_t pos = npos) const;
        size_t find_last_of(char c, size_t pos = npos) const;
	int count_delimiter(char c) const;

	String& erase(size_t pos = 0, size_t n = npos);

        // operators
        String& operator=(const String& str);
        String& operator=(const char* s);
        String& operator=(char c);

        const char& operator[](size_t pos) const;
        char& operator[](size_t pos);

        String& operator+=(const String& str);
        String& operator+=(const char* s);
        String& operator+=(char c);

};

// Global operators
extern "C++" String operator+(const String& lhs, const String& rhs);
extern "C++" String operator+(const char* lhs, const String& rhs);
extern "C++" String operator+(char lhs, const String& rhs);
extern "C++" String operator+(const String& lhs, const char* rhs);
extern "C++" String operator+(const String& lhs, char rhs);

extern "C++" bool operator==(const String& lhs, const String& rhs);
extern "C++" bool operator==(const char* lhs, const String& rhs);
extern "C++" bool operator==(const String& lhs, const char* rhs);

extern "C++" bool operator!=(const String& lhs, const String& rhs);
extern "C++" bool operator!=(const char* lhs, const String& rhs);
extern "C++" bool operator!=(const String& lhs, const char* rhs);

extern "C++" bool operator<(const String& lhs, const String& rhs);
extern "C++" bool operator<(const char* lhs, const String& rhs);
extern "C++" bool operator<(const String& lhs, const char* rhs);

extern "C++" bool operator>(const String& lhs, const String& rhs);
extern "C++" bool operator>(const char* lhs, const String& rhs);
extern "C++" bool operator>(const String& lhs, const char* rhs);

extern "C++" bool operator<=(const String& lhs, const String& rhs);
extern "C++" bool operator<=(const char* lhs, const String& rhs);
extern "C++" bool operator<=(const String& lhs, const char* rhs);

extern "C++" bool operator>=(const String& lhs, const String& rhs);
extern "C++" bool operator>=(const char* lhs, const String& rhs);
extern "C++" bool operator>=(const String& lhs, const char* rhs);

}; // namespace haggle

typedef haggle::String string;

#endif /* ENABLE_STL */

/**
	This works the same way as sprintf, except that it does its work on a 
	string.
*/
int stringprintf(string &str, const char *format, ...);

#endif /* _STRINGH_H_ */
