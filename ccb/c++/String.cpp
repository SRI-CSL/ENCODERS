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
#include <libcpphaggle/String.h>
#if !defined(ENABLE_STL)
#include <string.h>
#include <stdlib.h>

namespace haggle {

char String::nullchar = '\0';

char *String::alloc(size_t len)
{
        char *tmp;

        if (len <= alloc_len || len == 0)
                return s;

        // If alloc_len is 0, then s points to the static nullchar.
        // Therefore, we must reset the s pointer to NULL first,
        // so that realloc does not try to free it.
        if (alloc_len == 0)
                s = NULL;

        tmp = (char *)realloc(s, len);
        
        if (tmp) {
                s = tmp;
                alloc_len = len;

                // Initialize allocated memory to zero, but do not
                // overwrite any previous string
                memset(s + slen, 0, alloc_len - slen);
        }

        return tmp;
}


String::String(const char *_s, size_t n) : s(&nullchar), alloc_len(0), slen(0)
{
	if (!_s)
		return;

	size_t len = strlen(_s);

	if (n > len)
		n = len;

	if (alloc(n + 1)) {
                strncpy(s, _s, n);
                slen = n;
        }
}

String::String(const char *_s) : s(&nullchar), alloc_len(0), slen(0)
{
	if (_s && alloc(strlen(_s) + 1)) {
                strcpy(s, _s);
                slen = strlen(s);
        }
}

String::String(const String& str) : s(&nullchar), alloc_len(0), slen(0)
{
	if (alloc(str.slen + 1)) {
		strcpy(s, str.s);
                slen = str.slen;
        }
}

String::String(const char c) : s(&nullchar), alloc_len(0), slen(0)
{
	if (c != '\0' && alloc(2)) {
                s[0] = c;
                s[1] = '\0';
                slen = 1;
	} else if (alloc(1)) {
		s[0] = '\0';
		slen = 0;
	}
}

String::~String() 
{
        if (alloc_len) 
                free(s);
}

const char* String::c_str () const
{
        return s;
}

size_t String::length() const
{
        return slen;
}

size_t String::size() const
{
        return slen;
}

void String::clear()
{
        if (slen) {
                s[0] = '\0';
                slen = 0;
        }
}

bool String::empty () const
{
        return (slen == 0);
}

/*
  "at" should return the character at position pos, and throw
  an out of range exception if pos is too large.
  However, we avoid exceptions here and instead return 
  the null character in case we are out of range.
 */
const char& String::at(size_t pos) const
{
        if (slen && slen <= pos)
                return s[pos];
        else
                return nullchar; 
}

char& String::at(size_t pos)
{
         if (s && slen <= pos)
                 return s[pos];

         return nullchar; // not really ideal to return a non const
}

String String::substr(size_t pos, size_t n) const
{
	if (pos >= slen)
                return (char *)NULL;

        if (n == npos || n > (slen - pos))
                n = (slen - pos);

        if (pos + n <= slen)
                return String(s + pos, n);

        return (char *)NULL;
}

String& String::append(const String& str)
{
        if (str.slen && alloc(slen + str.slen + 1)) {
                strcpy(s + slen, str.s);
                slen += str.slen;
        }

        return *this;
}

String& String::append(const String& str, size_t pos, size_t n)
{
        if (n == 0 || str.slen == 0 || str.slen <= (pos + n))
                return *this;

        if (alloc(slen + n + 1)) {
                strncpy(s + slen, str.s + pos, n);
                slen += n;
        }

        return *this;
}

String& String::append(const char* _s, size_t n)
{
        if (n == 0 || !_s || n > strlen(_s))
                return *this;

        if (alloc(slen + n + 1)) {
                strncpy(s + slen, _s, n);
                slen += n;
        }

        return *this;
}

String& String::append(const char* _s)
{
        if (!_s || strlen(_s) == 0)
                return *this;

        if (alloc(slen + strlen(_s) + 1)) {
                strcpy(s + slen, _s);
                slen += strlen(_s);
        }

        return *this;
}

String& String::append(size_t n, char c)
{
        if (alloc(slen + n + 1)) {
                while (n) {
                        s[slen++] = c;
                        n--;
                }
                s[slen] = '\0';
        }
        return *this;
}

static int _compare(const char *str1, size_t str1_len, size_t pos1, size_t n1, const char *str2, size_t str2_len, size_t pos2, size_t n2)
{
	if (!str1)
		return -1;
	else if (!str2)
		return 1;
	else if (str1_len == 0 && str2_len == 0)
                return 0;
        else if (str1_len == 0) 
                return -1;
        else if (str1_len == 0)
                return 1;
        else if (n1 == 0 || pos1 > (str1_len - 1) || pos2 > (str2_len - 1))
                return -1;
        
	return strncmp(str1 + pos1, str2 + pos2, n1 > n2 ? n1 : n2);
}

int String::compare(const String& str) const
{            
        return _compare(s, slen, 0, slen, str.s, str.slen, 0, str.slen);
}

int String::compare(const char* _s) const
{
	return _compare(s, slen, 0, slen, _s, strlen(_s), 0, strlen(_s));
}

int String::compare(size_t pos1, size_t n1, const String& str) const
{
        return _compare(s, slen, pos1, n1, str.s, str.slen, 0, str.slen);
}

int String::compare(size_t pos1, size_t n1, const char* _s) const
{  
	return _compare(s, slen, pos1, n1, _s, strlen(_s), 0, strlen(_s));
}

int String::compare(size_t pos1, size_t n1, const String& str, size_t pos2, size_t n2) const
{
        return _compare(s, slen, pos1, n1, str.s, str.slen, pos2, n2);
}

int String::compare(size_t pos1, size_t n1, const char* _s, size_t n2) const
{ 
	return _compare(s, slen, pos1, n1, _s, strlen(_s), 0, n2);
}

size_t String::find(const String& str, size_t pos) const
{
        return find(str.s, pos, str.slen);
}

size_t String::find(const char* _s, size_t pos, size_t n) const
{
        if (!_s || strlen(_s) < n || n > slen || pos >= (slen - n))
                return npos;
        
        while (pos <= (slen - n)) {
                if (strncmp(s + pos, _s, n) == 0) {
                        return pos;
                }
                pos++;
        }
        return npos;
}

size_t String::find(const char* _s, size_t pos) const
{
        return find(_s, pos, strlen(_s));
}

size_t String::find(char c, size_t pos) const
{
        if (pos >= slen)
                return npos;

        while (pos < slen) {
                if (s[pos] == c)
                        return pos;
                pos++;
        }
        return npos;
}

size_t String::find_first_of(const String& str, size_t pos) const
{
        return find_first_of(str.s, pos, str.slen);
}

size_t String::find_first_of(const char* _s, size_t pos, size_t n) const
{
        if (!_s || pos >= slen || strlen(_s) < n)
                return npos;
        
        while (pos < slen) {
                for (size_t i = 0; i < n; i++) {
                        if (s[pos] == _s[i])
                                return pos;
                }
                pos++;
        }
        return npos;
}

size_t String::find_first_of(const char* _s, size_t pos) const
{
        if (!_s)
                return npos;

        return find_first_of(_s, pos, strlen(_s));
}

size_t String::find_first_of(char c, size_t pos) const
{
        return find_first_of(&c, pos, 1);
}

size_t String::find_last_of(const String& str, size_t pos) const
{
        return find_last_of(str.s, pos, str.slen);
}

size_t String::find_last_of(const char* _s, size_t pos, size_t n) const
{
        if (!_s)
                return npos;

        if (pos == npos)
                pos = slen - 1;

        if (pos > slen || strlen(_s) < n)
                return npos;
        
        while (pos != npos) {
                for (size_t i = 0; i < n; i++) {
                        if (s[pos] == _s[i])
                                return pos;
                }
                pos--;
        }
        return npos;
}

size_t String::find_last_of(const char* _s, size_t pos) const
{
        if (!_s)
                return npos;

        return find_last_of(_s, pos, strlen(_s));
}

size_t String::find_last_of(char c, size_t pos) const
{
        return find_last_of(&c, pos, 1);
}

String& String::erase(size_t pos, size_t n)
{
	if (pos == 0 && n == npos) {
		clear();
		return *this;
	} 
	if (pos >= slen)
		return *this;

	if (n == npos || pos + n > slen) {
                s[pos] = '\0';
                slen -= (slen - pos);
            
                return *this;
        }
	long cplen = slen - pos - n;

	if (cplen > 0)
		strncpy(s + pos, s + pos + n, cplen);

	slen -= n;
        s[slen] = '\0';

	return *this;
}

// operators
String& String::operator=(const string& str)
{
        if (alloc(str.slen + 1)) {
                strcpy(s, str.s);
                slen = str.slen;
        }

        return *this;
}

String& String::operator=(const char* _s)
{
        if (_s && alloc(strlen(_s) + 1)) {
                strcpy(s, _s);
                slen = strlen(s);
        }
        return *this;
}

String& String::operator=(char c)
{
        if (c != '\0' && alloc(2)) {
                s[0] = c;
                s[1] = '\0';
                slen = 1;
	} else if (alloc(1)) {
		s[0] = '\0';
		slen = 0;
	}
        return *this;
}

const char& String::operator[](size_t pos) const
{
        return s[pos];
}

char& String::operator[](size_t pos)
{
        return s[pos];
}

String& String::operator+=(const String& str)
{
        return append(str);
}

String& String::operator+=(const char* s)
{
        return append(s);
}

String& String::operator+=(char c)
{
        return append(1, c);
}

String operator+(const String& lhs, const String& rhs)
{
        String ret(lhs);

        ret += rhs;

        return ret;
}

String operator+(const char* lhs, const String& rhs)
{
        String ret(lhs);

        ret += rhs;

        return ret;
}

String operator+(char lhs, const String& rhs)
{
        String ret;

        ret += lhs;
        ret += rhs;

        return ret;
}

String operator+(const String& lhs, const char* rhs)
{
        String ret(lhs);

        ret += rhs;

        return ret;
}

String operator+(const String& lhs, char rhs)
{
        return String(lhs).append(1, rhs);
}

bool operator==(const String& lhs, const String& rhs)
{
        return lhs.compare(rhs) == 0;
}

bool operator==(const char* lhs, const String& rhs)
{
        return strcmp(lhs, rhs.c_str()) == 0;
}

bool operator==(const String& lhs, const char* rhs)
{
        return lhs.compare(rhs) == 0;
}

bool operator!=(const String& lhs, const String& rhs)
{
        return lhs.compare(rhs) != 0;
}

bool operator!=(const char* lhs, const String& rhs)
{
        return strcmp(lhs, rhs.c_str()) != 0;
}

bool operator!=(const String& lhs, const char* rhs)
{
        return lhs.compare(rhs) != 0;
}

bool operator<(const String& lhs, const String& rhs)
{
        return lhs.compare(rhs) < 0;
}

bool operator<(const char* lhs, const String& rhs)
{
        return strcmp(lhs, rhs.c_str()) < 0;
}

bool operator<(const String& lhs, const char* rhs)
{
        return lhs.compare(rhs) < 0;
}

bool operator>(const String& lhs, const String& rhs)
{
        return lhs.compare(rhs) > 0;
}

bool operator>(const char* lhs, const String& rhs)
{
        return strcmp(lhs, rhs.c_str()) > 0;
}

bool operator>(const String& lhs, const char* rhs)
{
        return lhs.compare(rhs) > 0;
}

bool operator<=(const String& lhs, const String& rhs)
{
        return lhs.compare(rhs) <= 0;
}

bool operator<=(const char* lhs, const String& rhs)
{
        return strcmp(lhs, rhs.c_str()) <= 0;
}

bool operator<=(const String& lhs, const char* rhs)
{
        return lhs.compare(rhs) <= 0;
}

bool operator>=(const String& lhs, const String& rhs)
{
        return lhs.compare(rhs) >= 0;
}

bool operator>=(const char* lhs, const String& rhs)
{
        return strcmp(lhs, rhs.c_str()) >= 0;
}

bool operator>=(const String& lhs, const char* rhs)
{
        return lhs.compare(rhs) >= 0;
}

}; // namespace haggle

#if defined(ENABLE_STRING_TEST)
#include <stdio.h>
#include <unistd.h>
using namespace haggle;

void SplitFilename (const String& str)
{
  size_t found;
  printf("Splitting: %s\n", str.c_str());
  found=str.find_first_of("/\\", 1);
  printf("folder: %s\n", str.substr(0,found).c_str());
  printf("file: %s\n", str.substr(found+1).c_str());
}

int main(int argc, char **argv)
{
  String str1 ("/usr/bin/man");
  String str2 ("c:\\windows\\winhelp.exe");

  SplitFilename (str1);
  SplitFilename (str2);

  string str3 ("green apple");
  string str4 ("red apple");
  if (str3.compare(str4) != 0)
          printf("%s is not %s\n", str3.c_str(), str4.c_str());
  
  if (str3.compare(6,5,"apple") == 0)
          printf("still, %s is an apple\n", str3.c_str());
  
  if (str4.compare(str4.size()-5,5,"apple") == 0)
          printf("and %s is also an apple\n", str4.c_str());
  
  if (str3.compare(6,5,str4,4,5) == 0)
          printf("therefore, both are apples\n");


  String f("foo=bar bin=baz john=doe erik=nordstrom");
  String filepath = "/var/mobile/Applications/1C1A24D8-7222-4D22-A1BA-4038BB95667C/Documents/foo.jpg";

  string namevalue;
  size_t start = 0, end = 0;
        
  size_t pos = filepath.find_last_of('/');

  String fileName = filepath.substr(pos + 1);

  printf("Find last of \'/\' : %lu %s\n", pos, fileName.c_str());

  printf("Splitting %s\n", f.c_str());

  while (end != string::npos) {
          end = f.find(" ", start);
          printf("start=%lu end=%lu ", start, end);
          namevalue = f.substr(start, end - start);
          printf("substr=%s\n", namevalue.c_str());
          start = end + 1;
  }

  f = "/this/is/a/path/and/filename.txt";

  printf("str: %s\n", f.c_str());

  f.erase(f.find_last_of('/'), 4);
  
  printf("str.erase: %s\n", f.c_str());

  f.append("///filename-2.txt");

  printf("str.append: %s\n", f.c_str());

  return 0;
}

#endif /* ENABLE_STRING_TEST */

#endif /* !ENABLE_STL */

#include <stdio.h>
#include <stdarg.h>
#include <libcpphaggle/PlatformDetect.h>

#if defined(OS_WINDOWS) || defined(OS_WINDOWS_MOBILE)
/*
	The reason this function is here is because windows doesn't have a 
	vasprintf function.

	The reason it's written the way it is is because the vsnprintf function 
	doesn't report the number of bytes it would have written as it does on 
	other systems.
*/
static int vasprintf(char **ret, const char *format, va_list ap)
{
	size_t to_allocate = 512;
	int retval;

	*ret = NULL;

	do {
		// Why not realloc? Because we don't need to copy the bytes.
		if (*ret != NULL)
			free(*ret);

		*ret = (char *) malloc(to_allocate);

		if (*ret == NULL)
			return -1;

		retval = _vsnprintf(*ret, to_allocate, format, ap);
		to_allocate += to_allocate;

	} while (retval == -1);

	return retval;
}
#endif

int stringprintf(string &str, const char *format, ...)
{
	va_list	args;
	char *temp = NULL;
	int retval;

	memset(&args, 0, sizeof(args));
	va_start(args, format);
	retval = vasprintf(&temp, format, args);
	va_end(args);
	if (temp == NULL || retval == -1) {
		return retval;
	}
	str = temp;
	free(temp);
	return retval;
}
