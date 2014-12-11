'''Wrapper for attribute.h

Generated with:
ctypesgen.py -L/usr/local/lib -lhaggle /usr/local/include/libhaggle/attribute.h /usr/local/include/libhaggle/dataobject.h /usr/local/include/libhaggle/debug.h /usr/local/include/libhaggle/error.h /usr/local/include/libhaggle/exports.h /usr/local/include/libhaggle/haggle.h /usr/local/include/libhaggle/interface.h /usr/local/include/libhaggle/ipc.h /usr/local/include/libhaggle/list.h /usr/local/include/libhaggle/node.h /usr/local/include/libhaggle/platform.h -o haggle.py

Do not modify this file.
'''

__docformat__ =  'restructuredtext'

# Begin preamble

import ctypes, os, sys
from ctypes import *

_int_types = (c_int16, c_int32)
if hasattr(ctypes, 'c_int64'):
    # Some builds of ctypes apparently do not have c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (c_int64,)
for t in _int_types:
    if sizeof(t) == sizeof(c_size_t):
        c_ptrdiff_t = t
del t
del _int_types

class c_void(Structure):
    # c_void_p is a buggy return type, converting to int, so
    # POINTER(None) == c_void_p is actually written as
    # POINTER(c_void), so it can be treated as a real pointer.
    _fields_ = [('dummy', c_int)]

def POINTER(obj):
    p = ctypes.POINTER(obj)

    # Convert None to a real NULL pointer to work around bugs
    # in how ctypes handles None on 64-bit platforms
    if not isinstance(p.from_param, classmethod):
        def from_param(cls, x):
            if x is None:
                return cls()
            else:
                return x
        p.from_param = classmethod(from_param)

    return p

class UserString:
    def __init__(self, seq):
        if isinstance(seq, basestring):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq)
    def __str__(self): return str(self.data)
    def __repr__(self): return repr(self.data)
    def __int__(self): return int(self.data)
    def __long__(self): return long(self.data)
    def __float__(self): return float(self.data)
    def __complex__(self): return complex(self.data)
    def __hash__(self): return hash(self.data)

    def __cmp__(self, string):
        if isinstance(string, UserString):
            return cmp(self.data, string.data)
        else:
            return cmp(self.data, string)
    def __contains__(self, char):
        return char in self.data

    def __len__(self): return len(self.data)
    def __getitem__(self, index): return self.__class__(self.data[index])
    def __getslice__(self, start, end):
        start = max(start, 0); end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, basestring):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other))
    def __radd__(self, other):
        if isinstance(other, basestring):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other) + self.data)
    def __mul__(self, n):
        return self.__class__(self.data*n)
    __rmul__ = __mul__
    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self): return self.__class__(self.data.capitalize())
    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))
    def count(self, sub, start=0, end=sys.maxint):
        return self.data.count(sub, start, end)
    def decode(self, encoding=None, errors=None): # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())
    def encode(self, encoding=None, errors=None): # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())
    def endswith(self, suffix, start=0, end=sys.maxint):
        return self.data.endswith(suffix, start, end)
    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))
    def find(self, sub, start=0, end=sys.maxint):
        return self.data.find(sub, start, end)
    def index(self, sub, start=0, end=sys.maxint):
        return self.data.index(sub, start, end)
    def isalpha(self): return self.data.isalpha()
    def isalnum(self): return self.data.isalnum()
    def isdecimal(self): return self.data.isdecimal()
    def isdigit(self): return self.data.isdigit()
    def islower(self): return self.data.islower()
    def isnumeric(self): return self.data.isnumeric()
    def isspace(self): return self.data.isspace()
    def istitle(self): return self.data.istitle()
    def isupper(self): return self.data.isupper()
    def join(self, seq): return self.data.join(seq)
    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))
    def lower(self): return self.__class__(self.data.lower())
    def lstrip(self, chars=None): return self.__class__(self.data.lstrip(chars))
    def partition(self, sep):
        return self.data.partition(sep)
    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))
    def rfind(self, sub, start=0, end=sys.maxint):
        return self.data.rfind(sub, start, end)
    def rindex(self, sub, start=0, end=sys.maxint):
        return self.data.rindex(sub, start, end)
    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))
    def rpartition(self, sep):
        return self.data.rpartition(sep)
    def rstrip(self, chars=None): return self.__class__(self.data.rstrip(chars))
    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)
    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)
    def splitlines(self, keepends=0): return self.data.splitlines(keepends)
    def startswith(self, prefix, start=0, end=sys.maxint):
        return self.data.startswith(prefix, start, end)
    def strip(self, chars=None): return self.__class__(self.data.strip(chars))
    def swapcase(self): return self.__class__(self.data.swapcase())
    def title(self): return self.__class__(self.data.title())
    def translate(self, *args):
        return self.__class__(self.data.translate(*args))
    def upper(self): return self.__class__(self.data.upper())
    def zfill(self, width): return self.__class__(self.data.zfill(width))

class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""
    def __init__(self, string=""):
        self.data = string
    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")
    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data): raise IndexError
        self.data = self.data[:index] + sub + self.data[index+1:]
    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data): raise IndexError
        self.data = self.data[:index] + self.data[index+1:]
    def __setslice__(self, start, end, sub):
        start = max(start, 0); end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start]+sub.data+self.data[end:]
        elif isinstance(sub, basestring):
            self.data = self.data[:start]+sub+self.data[end:]
        else:
            self.data =  self.data[:start]+str(sub)+self.data[end:]
    def __delslice__(self, start, end):
        start = max(start, 0); end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]
    def immutable(self):
        return UserString(self.data)
    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, basestring):
            self.data += other
        else:
            self.data += str(other)
        return self
    def __imul__(self, n):
        self.data *= n
        return self

class String(MutableString, Union):

    _fields_ = [('raw', POINTER(c_char)),
                ('data', c_char_p)]

    def __init__(self, obj=""):
        if isinstance(obj, (str, unicode, UserString)):
            self.data = str(obj)
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(POINTER(c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj)

        # Convert from c_char_p
        elif isinstance(obj, c_char_p):
            return obj

        # Convert from POINTER(c_char)
        elif isinstance(obj, POINTER(c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(cast(obj, POINTER(c_char)))

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)
    from_param = classmethod(from_param)

def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)

# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to c_void_p.
def UNCHECKED(type):
    if (hasattr(type, "_type_") and isinstance(type._type_, str)
        and type._type_ != "P"):
        return type
    else:
        return c_void_p

# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self,func,restype,argtypes):
        self.func=func
        self.func.restype=restype
        self.argtypes=argtypes
    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func
    def __call__(self,*args):
        fixed_args=[]
        i=0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i+=1
        return self.func(*fixed_args+list(args[i:]))

# End preamble

_libs = {}
_libdirs = ['/usr/local/lib']

# Begin loader

# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

import os.path, re, sys, glob
import ctypes
import ctypes.util

def _environ_path(name):
    if name in os.environ:
        return os.environ[name].split(":")
    else:
        return []

class LibraryLoader(object):
    def __init__(self):
        self.other_dirs=[]

    def load_library(self,libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            if os.path.exists(path):
                return self.load(path)

        raise ImportError("%s not found." % libname)

    def load(self,path):
        """Given a path to a library, load it."""
        try:
            # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
            # of the default RTLD_LOCAL.  Without this, you end up with
            # libraries not being loadable, resulting in "Symbol not found"
            # errors
            if sys.platform == 'darwin':
                return ctypes.CDLL(path, ctypes.RTLD_GLOBAL)
            else:
                return ctypes.cdll.LoadLibrary(path)
        except OSError,e:
            raise ImportError(e)

    def getpaths(self,libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # FIXME / TODO return '.' and os.path.dirname(__file__)
            for path in self.getplatformpaths(libname):
                yield path

            path = ctypes.util.find_library(libname)
            if path: yield path

    def getplatformpaths(self, libname):
        return []

# Darwin (Mac OS X)

class DarwinLibraryLoader(LibraryLoader):
    name_formats = ["lib%s.dylib", "lib%s.so", "lib%s.bundle", "%s.dylib",
                "%s.so", "%s.bundle", "%s"]

    def getplatformpaths(self,libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [format % libname for format in self.name_formats]

        for dir in self.getdirs(libname):
            for name in names:
                yield os.path.join(dir,name)

    def getdirs(self,libname):
        '''Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        '''

        dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [os.path.expanduser('~/lib'),
                                          '/usr/local/lib', '/usr/lib']

        dirs = []

        if '/' in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))

        dirs.extend(self.other_dirs)
        dirs.append(".")
        dirs.append(os.path.dirname(__file__))

        if hasattr(sys, 'frozen') and sys.frozen == 'macosx_app':
            dirs.append(os.path.join(
                os.environ['RESOURCEPATH'],
                '..',
                'Frameworks'))

        dirs.extend(dyld_fallback_library_path)

        return dirs

# Posix

class PosixLibraryLoader(LibraryLoader):
    _ld_so_cache = None

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = []
        for name in ("LD_LIBRARY_PATH",
                     "SHLIB_PATH", # HPUX
                     "LIBPATH", # OS/2, AIX
                     "LIBRARY_PATH", # BE/OS
                    ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))
        directories.extend(self.other_dirs)
        directories.append(".")
        directories.append(os.path.dirname(__file__))

        try: directories.extend([dir.strip() for dir in open('/etc/ld.so.conf')])
        except IOError: pass

        directories.extend(['/lib', '/usr/lib', '/lib64', '/usr/lib64'])

        cache = {}
        lib_re = re.compile(r'lib(.*)\.s[ol]')
        ext_re = re.compile(r'\.s[ol]$')
        for dir in directories:
            try:
                for path in glob.glob("%s/*.s[ol]*" % dir):
                    file = os.path.basename(path)

                    # Index by filename
                    if file not in cache:
                        cache[file] = path

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        if library not in cache:
                            cache[library] = path
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname)
        if result: yield result

        path = ctypes.util.find_library(libname)
        if path: yield os.path.join("/lib",path)

# Windows

class _WindowsLibrary(object):
    def __init__(self, path):
        self.cdll = ctypes.cdll.LoadLibrary(path)
        self.windll = ctypes.windll.LoadLibrary(path)

    def __getattr__(self, name):
        try: return getattr(self.cdll,name)
        except AttributeError:
            try: return getattr(self.windll,name)
            except AttributeError:
                raise

class WindowsLibraryLoader(LibraryLoader):
    name_formats = ["%s.dll", "lib%s.dll", "%slib.dll"]

    def load_library(self, libname):
        try:
            result = LibraryLoader.load_library(self, libname)
        except ImportError:
            result = None
            if os.path.sep not in libname:
                for name in self.name_formats:
                    try:
                        result = getattr(ctypes.cdll, name % libname)
                        if result:
                            break
                    except WindowsError:
                        result = None
            if result is None:
                try:
                    result = getattr(ctypes.cdll, libname)
                except WindowsError:
                    result = None
            if result is None:
                raise ImportError("%s not found." % libname)
        return result

    def load(self, path):
        return _WindowsLibrary(path)

    def getplatformpaths(self, libname):
        if os.path.sep not in libname:
            for name in self.name_formats:
                dll_in_current_dir = os.path.abspath(name % libname)
                if os.path.exists(dll_in_current_dir):
                    yield dll_in_current_dir
                path = ctypes.util.find_library(name % libname)
                if path:
                    yield path

# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin":   DarwinLibraryLoader,
    "cygwin":   WindowsLibraryLoader,
    "win32":    WindowsLibraryLoader
}

loader = loaderclass.get(sys.platform, PosixLibraryLoader)()

def add_library_search_dirs(other_dirs):
    loader.other_dirs = other_dirs

load_library = loader.load_library

del loaderclass

# End loader

add_library_search_dirs(['/usr/local/lib'])

# Begin libraries

_libs["haggle"] = load_library("haggle")

# 1 libraries
# End libraries

# No modules

__time_t = c_long # /usr/include/x86_64-linux-gnu/bits/types.h: 149

__suseconds_t = c_long # /usr/include/x86_64-linux-gnu/bits/types.h: 151

# /usr/include/x86_64-linux-gnu/bits/time.h: 75
class struct_timeval(Structure):
    pass

struct_timeval.__slots__ = [
    'tv_sec',
    'tv_usec',
]
struct_timeval._fields_ = [
    ('tv_sec', __time_t),
    ('tv_usec', __suseconds_t),
]

# /usr/local/include/libhaggle/list.h: 30
class struct_list(Structure):
    pass

struct_list.__slots__ = [
    'prev',
    'next',
]
struct_list._fields_ = [
    ('prev', POINTER(struct_list)),
    ('next', POINTER(struct_list)),
]

list_t = struct_list # /usr/local/include/libhaggle/list.h: 32

# /usr/local/include/libhaggle/attribute.h: 47
class struct_attribute(Structure):
    pass

struct_attribute.__slots__ = [
    'l',
    'weight',
    'value',
    'name',
]
struct_attribute._fields_ = [
    ('l', list_t),
    ('weight', c_ulong),
    ('value', String),
    ('name', String),
]

haggle_attr_t = struct_attribute # /usr/local/include/libhaggle/attribute.h: 47

# /usr/local/include/libhaggle/attribute.h: 64
if hasattr(_libs['haggle'], 'haggle_attribute_get_name'):
    haggle_attribute_get_name = _libs['haggle'].haggle_attribute_get_name
    haggle_attribute_get_name.argtypes = [POINTER(struct_attribute)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_attribute_get_name.restype = ReturnString
    else:
        haggle_attribute_get_name.restype = String
        haggle_attribute_get_name.errcheck = ReturnString

# /usr/local/include/libhaggle/attribute.h: 75
if hasattr(_libs['haggle'], 'haggle_attribute_get_value'):
    haggle_attribute_get_value = _libs['haggle'].haggle_attribute_get_value
    haggle_attribute_get_value.argtypes = [POINTER(struct_attribute)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_attribute_get_value.restype = ReturnString
    else:
        haggle_attribute_get_value.restype = String
        haggle_attribute_get_value.errcheck = ReturnString

# /usr/local/include/libhaggle/attribute.h: 87
if hasattr(_libs['haggle'], 'haggle_attribute_set_name'):
    haggle_attribute_set_name = _libs['haggle'].haggle_attribute_set_name
    haggle_attribute_set_name.argtypes = [POINTER(struct_attribute), String]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_attribute_set_name.restype = ReturnString
    else:
        haggle_attribute_set_name.restype = String
        haggle_attribute_set_name.errcheck = ReturnString

# /usr/local/include/libhaggle/attribute.h: 101
if hasattr(_libs['haggle'], 'haggle_attribute_set_value'):
    haggle_attribute_set_value = _libs['haggle'].haggle_attribute_set_value
    haggle_attribute_set_value.argtypes = [POINTER(struct_attribute), String]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_attribute_set_value.restype = ReturnString
    else:
        haggle_attribute_set_value.restype = String
        haggle_attribute_set_value.errcheck = ReturnString

# /usr/local/include/libhaggle/attribute.h: 108
if hasattr(_libs['haggle'], 'haggle_attribute_get_weight'):
    haggle_attribute_get_weight = _libs['haggle'].haggle_attribute_get_weight
    haggle_attribute_get_weight.argtypes = [POINTER(struct_attribute)]
    haggle_attribute_get_weight.restype = c_ulong

# /usr/local/include/libhaggle/attribute.h: 115
if hasattr(_libs['haggle'], 'haggle_attribute_set_weight'):
    haggle_attribute_set_weight = _libs['haggle'].haggle_attribute_set_weight
    haggle_attribute_set_weight.argtypes = [POINTER(struct_attribute), c_ulong]
    haggle_attribute_set_weight.restype = c_ulong

# /usr/local/include/libhaggle/attribute.h: 124
if hasattr(_libs['haggle'], 'haggle_attribute_new'):
    haggle_attribute_new = _libs['haggle'].haggle_attribute_new
    haggle_attribute_new.argtypes = [String, String]
    haggle_attribute_new.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/attribute.h: 133
if hasattr(_libs['haggle'], 'haggle_attribute_copy'):
    haggle_attribute_copy = _libs['haggle'].haggle_attribute_copy
    haggle_attribute_copy.argtypes = [POINTER(struct_attribute)]
    haggle_attribute_copy.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/attribute.h: 145
if hasattr(_libs['haggle'], 'haggle_attribute_new_weighted'):
    haggle_attribute_new_weighted = _libs['haggle'].haggle_attribute_new_weighted
    haggle_attribute_new_weighted.argtypes = [String, String, c_ulong]
    haggle_attribute_new_weighted.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/attribute.h: 157
if hasattr(_libs['haggle'], 'haggle_attribute_free'):
    haggle_attribute_free = _libs['haggle'].haggle_attribute_free
    haggle_attribute_free.argtypes = [POINTER(struct_attribute)]
    haggle_attribute_free.restype = None

# /usr/local/include/libhaggle/attribute.h: 170
class struct_attributelist(Structure):
    pass

struct_attributelist.__slots__ = [
    'attributes',
    'num_attributes',
]
struct_attributelist._fields_ = [
    ('attributes', list_t),
    ('num_attributes', c_ulong),
]

haggle_attrlist_t = struct_attributelist # /usr/local/include/libhaggle/attribute.h: 170

# /usr/local/include/libhaggle/attribute.h: 182
if hasattr(_libs['haggle'], 'haggle_attributelist_new'):
    haggle_attributelist_new = _libs['haggle'].haggle_attributelist_new
    haggle_attributelist_new.argtypes = []
    haggle_attributelist_new.restype = POINTER(struct_attributelist)

# /usr/local/include/libhaggle/attribute.h: 194
if hasattr(_libs['haggle'], 'haggle_attributelist_new_from_attribute'):
    haggle_attributelist_new_from_attribute = _libs['haggle'].haggle_attributelist_new_from_attribute
    haggle_attributelist_new_from_attribute.argtypes = [POINTER(struct_attribute)]
    haggle_attributelist_new_from_attribute.restype = POINTER(struct_attributelist)

# /usr/local/include/libhaggle/attribute.h: 208
if hasattr(_libs['haggle'], 'haggle_attributelist_new_from_string'):
    haggle_attributelist_new_from_string = _libs['haggle'].haggle_attributelist_new_from_string
    haggle_attributelist_new_from_string.argtypes = [String]
    haggle_attributelist_new_from_string.restype = POINTER(struct_attributelist)

# /usr/local/include/libhaggle/attribute.h: 220
if hasattr(_libs['haggle'], 'haggle_attributelist_copy'):
    haggle_attributelist_copy = _libs['haggle'].haggle_attributelist_copy
    haggle_attributelist_copy.argtypes = [POINTER(struct_attributelist)]
    haggle_attributelist_copy.restype = POINTER(struct_attributelist)

# /usr/local/include/libhaggle/attribute.h: 226
if hasattr(_libs['haggle'], 'haggle_attributelist_free'):
    haggle_attributelist_free = _libs['haggle'].haggle_attributelist_free
    haggle_attributelist_free.argtypes = [POINTER(struct_attributelist)]
    haggle_attributelist_free.restype = None

# /usr/local/include/libhaggle/attribute.h: 238
if hasattr(_libs['haggle'], 'haggle_attributelist_remove_attribute'):
    haggle_attributelist_remove_attribute = _libs['haggle'].haggle_attributelist_remove_attribute
    haggle_attributelist_remove_attribute.argtypes = [POINTER(struct_attributelist), String, String]
    haggle_attributelist_remove_attribute.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/attribute.h: 248
if hasattr(_libs['haggle'], 'haggle_attributelist_add_attribute'):
    haggle_attributelist_add_attribute = _libs['haggle'].haggle_attributelist_add_attribute
    haggle_attributelist_add_attribute.argtypes = [POINTER(struct_attributelist), POINTER(struct_attribute)]
    haggle_attributelist_add_attribute.restype = c_ulong

# /usr/local/include/libhaggle/attribute.h: 260
if hasattr(_libs['haggle'], 'haggle_attributelist_get_attribute_n'):
    haggle_attributelist_get_attribute_n = _libs['haggle'].haggle_attributelist_get_attribute_n
    haggle_attributelist_get_attribute_n.argtypes = [POINTER(struct_attributelist), c_ulong]
    haggle_attributelist_get_attribute_n.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/attribute.h: 272
if hasattr(_libs['haggle'], 'haggle_attributelist_get_attribute_by_name'):
    haggle_attributelist_get_attribute_by_name = _libs['haggle'].haggle_attributelist_get_attribute_by_name
    haggle_attributelist_get_attribute_by_name.argtypes = [POINTER(struct_attributelist), String]
    haggle_attributelist_get_attribute_by_name.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/attribute.h: 283
if hasattr(_libs['haggle'], 'haggle_attributelist_get_attribute_by_name_value'):
    haggle_attributelist_get_attribute_by_name_value = _libs['haggle'].haggle_attributelist_get_attribute_by_name_value
    haggle_attributelist_get_attribute_by_name_value.argtypes = [POINTER(struct_attributelist), String, String]
    haggle_attributelist_get_attribute_by_name_value.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/attribute.h: 294
if hasattr(_libs['haggle'], 'haggle_attributelist_get_attribute_by_name_n'):
    haggle_attributelist_get_attribute_by_name_n = _libs['haggle'].haggle_attributelist_get_attribute_by_name_n
    haggle_attributelist_get_attribute_by_name_n.argtypes = [POINTER(struct_attributelist), String, c_int]
    haggle_attributelist_get_attribute_by_name_n.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/attribute.h: 304
if hasattr(_libs['haggle'], 'haggle_attributelist_detach_attribute'):
    haggle_attributelist_detach_attribute = _libs['haggle'].haggle_attributelist_detach_attribute
    haggle_attributelist_detach_attribute.argtypes = [POINTER(struct_attributelist), POINTER(struct_attribute)]
    haggle_attributelist_detach_attribute.restype = c_int

# /usr/local/include/libhaggle/attribute.h: 313
if hasattr(_libs['haggle'], 'haggle_attributelist_size'):
    haggle_attributelist_size = _libs['haggle'].haggle_attributelist_size
    haggle_attributelist_size.argtypes = [POINTER(struct_attributelist)]
    haggle_attributelist_size.restype = c_ulong

# /usr/local/include/libhaggle/attribute.h: 323
if hasattr(_libs['haggle'], 'haggle_attributelist_pop'):
    haggle_attributelist_pop = _libs['haggle'].haggle_attributelist_pop
    haggle_attributelist_pop.argtypes = [POINTER(struct_attributelist)]
    haggle_attributelist_pop.restype = POINTER(struct_attribute)

enum_path_type = c_int # /usr/local/include/libhaggle/platform.h: 219

PLATFORM_PATH_HAGGLE_EXE = 0 # /usr/local/include/libhaggle/platform.h: 219

PLATFORM_PATH_HAGGLE_PRIVATE = (PLATFORM_PATH_HAGGLE_EXE + 1) # /usr/local/include/libhaggle/platform.h: 219

PLATFORM_PATH_HAGGLE_DATA = (PLATFORM_PATH_HAGGLE_PRIVATE + 1) # /usr/local/include/libhaggle/platform.h: 219

PLATFORM_PATH_HAGGLE_TEMP = (PLATFORM_PATH_HAGGLE_DATA + 1) # /usr/local/include/libhaggle/platform.h: 219

PLATFORM_PATH_APP_DATA = (PLATFORM_PATH_HAGGLE_TEMP + 1) # /usr/local/include/libhaggle/platform.h: 219

path_type_t = enum_path_type # /usr/local/include/libhaggle/platform.h: 219

# /usr/local/include/libhaggle/platform.h: 224
if hasattr(_libs['haggle'], 'libhaggle_platform_set_path'):
    libhaggle_platform_set_path = _libs['haggle'].libhaggle_platform_set_path
    libhaggle_platform_set_path.argtypes = [path_type_t, String]
    libhaggle_platform_set_path.restype = c_int

# /usr/local/include/libhaggle/platform.h: 225
if hasattr(_libs['haggle'], 'libhaggle_platform_get_path'):
    libhaggle_platform_get_path = _libs['haggle'].libhaggle_platform_get_path
    libhaggle_platform_get_path.argtypes = [path_type_t, String]
    if sizeof(c_int) == sizeof(c_void_p):
        libhaggle_platform_get_path.restype = ReturnString
    else:
        libhaggle_platform_get_path.restype = String
        libhaggle_platform_get_path.errcheck = ReturnString

# /usr/local/include/libhaggle/platform.h: 236
if hasattr(_libs['haggle'], 'prng_init'):
    prng_init = _libs['haggle'].prng_init
    prng_init.argtypes = []
    prng_init.restype = None

# /usr/local/include/libhaggle/platform.h: 241
if hasattr(_libs['haggle'], 'prng_uint8'):
    prng_uint8 = _libs['haggle'].prng_uint8
    prng_uint8.argtypes = []
    prng_uint8.restype = c_ubyte

# /usr/local/include/libhaggle/platform.h: 246
if hasattr(_libs['haggle'], 'prng_uint32'):
    prng_uint32 = _libs['haggle'].prng_uint32
    prng_uint32.argtypes = []
    prng_uint32.restype = c_ulong

dataobject_id_t = c_ubyte * 20 # /usr/local/include/libhaggle/dataobject.h: 45

# /usr/local/include/libhaggle/dataobject.h: 48
class struct_dataobject(Structure):
    pass

haggle_dobj_t = struct_dataobject # /usr/local/include/libhaggle/dataobject.h: 48

# /usr/local/include/libhaggle/dataobject.h: 79
try:
    haggle_directory = (String).in_dll(_libs['haggle'], 'haggle_directory')
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 90
if hasattr(_libs['haggle'], 'haggle_dataobject_new'):
    haggle_dataobject_new = _libs['haggle'].haggle_dataobject_new
    haggle_dataobject_new.argtypes = []
    haggle_dataobject_new.restype = POINTER(struct_dataobject)

# /usr/local/include/libhaggle/dataobject.h: 100
if hasattr(_libs['haggle'], 'haggle_dataobject_new_from_file'):
    haggle_dataobject_new_from_file = _libs['haggle'].haggle_dataobject_new_from_file
    haggle_dataobject_new_from_file.argtypes = [String]
    haggle_dataobject_new_from_file.restype = POINTER(struct_dataobject)

# /usr/local/include/libhaggle/dataobject.h: 115
if hasattr(_libs['haggle'], 'haggle_dataobject_new_from_buffer'):
    haggle_dataobject_new_from_buffer = _libs['haggle'].haggle_dataobject_new_from_buffer
    haggle_dataobject_new_from_buffer.argtypes = [POINTER(c_ubyte), c_size_t]
    haggle_dataobject_new_from_buffer.restype = POINTER(struct_dataobject)

# /usr/local/include/libhaggle/dataobject.h: 125
if hasattr(_libs['haggle'], 'haggle_dataobject_new_from_raw'):
    haggle_dataobject_new_from_raw = _libs['haggle'].haggle_dataobject_new_from_raw
    haggle_dataobject_new_from_raw.argtypes = [POINTER(c_ubyte), c_size_t]
    haggle_dataobject_new_from_raw.restype = POINTER(struct_dataobject)

# /usr/local/include/libhaggle/dataobject.h: 133
if hasattr(_libs['haggle'], 'haggle_dataobject_free'):
    haggle_dataobject_free = _libs['haggle'].haggle_dataobject_free
    haggle_dataobject_free.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_free.restype = None

# /usr/local/include/libhaggle/dataobject.h: 144
if hasattr(_libs['haggle'], 'haggle_dataobject_calculate_id'):
    haggle_dataobject_calculate_id = _libs['haggle'].haggle_dataobject_calculate_id
    haggle_dataobject_calculate_id.argtypes = [POINTER(struct_dataobject), POINTER(dataobject_id_t)]
    haggle_dataobject_calculate_id.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 154
if hasattr(_libs['haggle'], 'haggle_dataobject_set_flags'):
    haggle_dataobject_set_flags = _libs['haggle'].haggle_dataobject_set_flags
    haggle_dataobject_set_flags.argtypes = [POINTER(struct_dataobject), c_ushort]
    haggle_dataobject_set_flags.restype = c_ushort

# /usr/local/include/libhaggle/dataobject.h: 163
if hasattr(_libs['haggle'], 'haggle_dataobject_unset_flags'):
    haggle_dataobject_unset_flags = _libs['haggle'].haggle_dataobject_unset_flags
    haggle_dataobject_unset_flags.argtypes = [POINTER(struct_dataobject), c_ushort]
    haggle_dataobject_unset_flags.restype = c_ushort

# /usr/local/include/libhaggle/dataobject.h: 172
if hasattr(_libs['haggle'], 'haggle_dataobject_set_createtime'):
    haggle_dataobject_set_createtime = _libs['haggle'].haggle_dataobject_set_createtime
    haggle_dataobject_set_createtime.argtypes = [POINTER(struct_dataobject), POINTER(struct_timeval)]
    haggle_dataobject_set_createtime.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 180
if hasattr(_libs['haggle'], 'haggle_dataobject_get_data_size'):
    haggle_dataobject_get_data_size = _libs['haggle'].haggle_dataobject_get_data_size
    haggle_dataobject_get_data_size.argtypes = [POINTER(struct_dataobject), POINTER(c_size_t)]
    haggle_dataobject_get_data_size.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 192
if hasattr(_libs['haggle'], 'haggle_dataobject_read_data_start'):
    haggle_dataobject_read_data_start = _libs['haggle'].haggle_dataobject_read_data_start
    haggle_dataobject_read_data_start.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_read_data_start.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 202
if hasattr(_libs['haggle'], 'haggle_dataobject_read_data_stop'):
    haggle_dataobject_read_data_stop = _libs['haggle'].haggle_dataobject_read_data_stop
    haggle_dataobject_read_data_stop.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_read_data_stop.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 211
if hasattr(_libs['haggle'], 'haggle_dataobject_read_data'):
    haggle_dataobject_read_data = _libs['haggle'].haggle_dataobject_read_data
    haggle_dataobject_read_data.argtypes = [POINTER(struct_dataobject), POINTER(None), c_size_t]
    haggle_dataobject_read_data.restype = c_ptrdiff_t

# /usr/local/include/libhaggle/dataobject.h: 230
if hasattr(_libs['haggle'], 'haggle_dataobject_get_data_all'):
    haggle_dataobject_get_data_all = _libs['haggle'].haggle_dataobject_get_data_all
    haggle_dataobject_get_data_all.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_get_data_all.restype = POINTER(None)

# /usr/local/include/libhaggle/dataobject.h: 240
if hasattr(_libs['haggle'], 'haggle_dataobject_get_raw'):
    haggle_dataobject_get_raw = _libs['haggle'].haggle_dataobject_get_raw
    haggle_dataobject_get_raw.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_get_raw.restype = POINTER(c_ubyte)

# /usr/local/include/libhaggle/dataobject.h: 241
if hasattr(_libs['haggle'], 'haggle_dataobject_get_raw_length'):
    haggle_dataobject_get_raw_length = _libs['haggle'].haggle_dataobject_get_raw_length
    haggle_dataobject_get_raw_length.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_get_raw_length.restype = c_size_t

# /usr/local/include/libhaggle/dataobject.h: 254
if hasattr(_libs['haggle'], 'haggle_dataobject_get_raw_alloc'):
    haggle_dataobject_get_raw_alloc = _libs['haggle'].haggle_dataobject_get_raw_alloc
    haggle_dataobject_get_raw_alloc.argtypes = [POINTER(struct_dataobject), POINTER(POINTER(c_ubyte)), POINTER(c_size_t)]
    haggle_dataobject_get_raw_alloc.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 255
class struct_metadata(Structure):
    pass

# /usr/local/include/libhaggle/dataobject.h: 255
if hasattr(_libs['haggle'], 'haggle_dataobject_to_metadata'):
    haggle_dataobject_to_metadata = _libs['haggle'].haggle_dataobject_to_metadata
    haggle_dataobject_to_metadata.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_to_metadata.restype = POINTER(struct_metadata)

# /usr/local/include/libhaggle/dataobject.h: 270
if hasattr(_libs['haggle'], 'haggle_dataobject_get_filename'):
    haggle_dataobject_get_filename = _libs['haggle'].haggle_dataobject_get_filename
    haggle_dataobject_get_filename.argtypes = [POINTER(struct_dataobject)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_dataobject_get_filename.restype = ReturnString
    else:
        haggle_dataobject_get_filename.restype = String
        haggle_dataobject_get_filename.errcheck = ReturnString

# /usr/local/include/libhaggle/dataobject.h: 271
if hasattr(_libs['haggle'], 'haggle_dataobject_set_filename'):
    haggle_dataobject_set_filename = _libs['haggle'].haggle_dataobject_set_filename
    haggle_dataobject_set_filename.argtypes = [POINTER(struct_dataobject), String]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_dataobject_set_filename.restype = ReturnString
    else:
        haggle_dataobject_set_filename.restype = String
        haggle_dataobject_set_filename.errcheck = ReturnString

# /usr/local/include/libhaggle/dataobject.h: 281
if hasattr(_libs['haggle'], 'haggle_dataobject_get_filepath'):
    haggle_dataobject_get_filepath = _libs['haggle'].haggle_dataobject_get_filepath
    haggle_dataobject_get_filepath.argtypes = [POINTER(struct_dataobject)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_dataobject_get_filepath.restype = ReturnString
    else:
        haggle_dataobject_get_filepath.restype = String
        haggle_dataobject_get_filepath.errcheck = ReturnString

# /usr/local/include/libhaggle/dataobject.h: 282
if hasattr(_libs['haggle'], 'haggle_dataobject_set_filepath'):
    haggle_dataobject_set_filepath = _libs['haggle'].haggle_dataobject_set_filepath
    haggle_dataobject_set_filepath.argtypes = [POINTER(struct_dataobject), String]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_dataobject_set_filepath.restype = ReturnString
    else:
        haggle_dataobject_set_filepath.restype = String
        haggle_dataobject_set_filepath.errcheck = ReturnString

# /usr/local/include/libhaggle/dataobject.h: 297
if hasattr(_libs['haggle'], 'haggle_dataobject_add_hash'):
    haggle_dataobject_add_hash = _libs['haggle'].haggle_dataobject_add_hash
    haggle_dataobject_add_hash.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_add_hash.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 309
if hasattr(_libs['haggle'], 'haggle_dataobject_set_thumbnail'):
    haggle_dataobject_set_thumbnail = _libs['haggle'].haggle_dataobject_set_thumbnail
    haggle_dataobject_set_thumbnail.argtypes = [POINTER(struct_dataobject), String, c_size_t]
    haggle_dataobject_set_thumbnail.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 318
if hasattr(_libs['haggle'], 'haggle_dataobject_read_thumbnail'):
    haggle_dataobject_read_thumbnail = _libs['haggle'].haggle_dataobject_read_thumbnail
    haggle_dataobject_read_thumbnail.argtypes = [POINTER(struct_dataobject), String, c_size_t]
    haggle_dataobject_read_thumbnail.restype = c_ptrdiff_t

# /usr/local/include/libhaggle/dataobject.h: 331
if hasattr(_libs['haggle'], 'haggle_dataobject_get_thumbnail_size'):
    haggle_dataobject_get_thumbnail_size = _libs['haggle'].haggle_dataobject_get_thumbnail_size
    haggle_dataobject_get_thumbnail_size.argtypes = [POINTER(struct_dataobject), POINTER(c_size_t)]
    haggle_dataobject_get_thumbnail_size.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 341
if hasattr(_libs['haggle'], 'haggle_dataobject_add_attribute'):
    haggle_dataobject_add_attribute = _libs['haggle'].haggle_dataobject_add_attribute
    haggle_dataobject_add_attribute.argtypes = [POINTER(struct_dataobject), String, String]
    haggle_dataobject_add_attribute.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 350
if hasattr(_libs['haggle'], 'haggle_dataobject_add_attribute_weighted'):
    haggle_dataobject_add_attribute_weighted = _libs['haggle'].haggle_dataobject_add_attribute_weighted
    haggle_dataobject_add_attribute_weighted.argtypes = [POINTER(struct_dataobject), String, String, c_ulong]
    haggle_dataobject_add_attribute_weighted.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 357
if hasattr(_libs['haggle'], 'haggle_dataobject_get_num_attributes'):
    haggle_dataobject_get_num_attributes = _libs['haggle'].haggle_dataobject_get_num_attributes
    haggle_dataobject_get_num_attributes.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_get_num_attributes.restype = c_ulong

# /usr/local/include/libhaggle/dataobject.h: 367
if hasattr(_libs['haggle'], 'haggle_dataobject_get_attribute_n'):
    haggle_dataobject_get_attribute_n = _libs['haggle'].haggle_dataobject_get_attribute_n
    haggle_dataobject_get_attribute_n.argtypes = [POINTER(struct_dataobject), c_ulong]
    haggle_dataobject_get_attribute_n.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/dataobject.h: 377
if hasattr(_libs['haggle'], 'haggle_dataobject_get_attribute_by_name'):
    haggle_dataobject_get_attribute_by_name = _libs['haggle'].haggle_dataobject_get_attribute_by_name
    haggle_dataobject_get_attribute_by_name.argtypes = [POINTER(struct_dataobject), String]
    haggle_dataobject_get_attribute_by_name.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/dataobject.h: 387
if hasattr(_libs['haggle'], 'haggle_dataobject_get_attribute_by_name_n'):
    haggle_dataobject_get_attribute_by_name_n = _libs['haggle'].haggle_dataobject_get_attribute_by_name_n
    haggle_dataobject_get_attribute_by_name_n.argtypes = [POINTER(struct_dataobject), String, c_ulong]
    haggle_dataobject_get_attribute_by_name_n.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/dataobject.h: 397
if hasattr(_libs['haggle'], 'haggle_dataobject_get_attribute_by_name_value'):
    haggle_dataobject_get_attribute_by_name_value = _libs['haggle'].haggle_dataobject_get_attribute_by_name_value
    haggle_dataobject_get_attribute_by_name_value.argtypes = [POINTER(struct_dataobject), String, String]
    haggle_dataobject_get_attribute_by_name_value.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/dataobject.h: 407
if hasattr(_libs['haggle'], 'haggle_dataobject_get_attributelist'):
    haggle_dataobject_get_attributelist = _libs['haggle'].haggle_dataobject_get_attributelist
    haggle_dataobject_get_attributelist.argtypes = [POINTER(struct_dataobject)]
    haggle_dataobject_get_attributelist.restype = POINTER(struct_attributelist)

# /usr/local/include/libhaggle/dataobject.h: 420
if hasattr(_libs['haggle'], 'haggle_dataobject_remove_attribute'):
    haggle_dataobject_remove_attribute = _libs['haggle'].haggle_dataobject_remove_attribute
    haggle_dataobject_remove_attribute.argtypes = [POINTER(struct_dataobject), POINTER(struct_attribute)]
    haggle_dataobject_remove_attribute.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 432
if hasattr(_libs['haggle'], 'haggle_dataobject_remove_attribute_by_name_value'):
    haggle_dataobject_remove_attribute_by_name_value = _libs['haggle'].haggle_dataobject_remove_attribute_by_name_value
    haggle_dataobject_remove_attribute_by_name_value.argtypes = [POINTER(struct_dataobject), String, String]
    haggle_dataobject_remove_attribute_by_name_value.restype = c_int

# /usr/local/include/libhaggle/dataobject.h: 444
if hasattr(_libs['haggle'], 'haggle_dataobject_get_metadata'):
    haggle_dataobject_get_metadata = _libs['haggle'].haggle_dataobject_get_metadata
    haggle_dataobject_get_metadata.argtypes = [POINTER(struct_dataobject), String]
    haggle_dataobject_get_metadata.restype = POINTER(struct_metadata)

# /usr/local/include/libhaggle/dataobject.h: 454
if hasattr(_libs['haggle'], 'haggle_dataobject_add_metadata'):
    haggle_dataobject_add_metadata = _libs['haggle'].haggle_dataobject_add_metadata
    haggle_dataobject_add_metadata.argtypes = [POINTER(struct_dataobject), POINTER(struct_metadata)]
    haggle_dataobject_add_metadata.restype = c_int

# /usr/local/include/libhaggle/debug.h: 36
if hasattr(_libs['haggle'], 'set_trace_level'):
    set_trace_level = _libs['haggle'].set_trace_level
    set_trace_level.argtypes = [c_int]
    set_trace_level.restype = None

# /usr/local/include/libhaggle/debug.h: 38
if hasattr(_libs['haggle'], 'libhaggle_debug_init'):
    libhaggle_debug_init = _libs['haggle'].libhaggle_debug_init
    libhaggle_debug_init.argtypes = []
    libhaggle_debug_init.restype = None

# /usr/local/include/libhaggle/debug.h: 39
if hasattr(_libs['haggle'], 'libhaggle_debug_fini'):
    libhaggle_debug_fini = _libs['haggle'].libhaggle_debug_fini
    libhaggle_debug_fini.argtypes = []
    libhaggle_debug_fini.restype = None

# /usr/local/include/libhaggle/debug.h: 40
if hasattr(_libs['haggle'], 'libhaggle_trace'):
    _func = _libs['haggle'].libhaggle_trace
    _restype = c_int
    _argtypes = [c_int, String, String]
    libhaggle_trace = _variadic_function(_func,_restype,_argtypes)

enum_anon_24 = c_int # /usr/local/include/libhaggle/error.h: 25

HAGGLE_ERROR = (-100) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_ALLOC_ERROR = (HAGGLE_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_SOCKET_ERROR = (HAGGLE_ALLOC_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_REGISTRATION_ERROR = (HAGGLE_SOCKET_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_BUSY_ERROR = (HAGGLE_REGISTRATION_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_PARAM_ERROR = (HAGGLE_BUSY_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_INTERNAL_ERROR = (HAGGLE_PARAM_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_EVENT_LOOP_ERROR = (HAGGLE_INTERNAL_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_EVENT_HANDLER_ERROR = (HAGGLE_EVENT_LOOP_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_FILE_ERROR = (HAGGLE_EVENT_HANDLER_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_DATAOBJECT_ERROR = (HAGGLE_FILE_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_WSA_ERROR = (HAGGLE_DATAOBJECT_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_HANDLE_ERROR = (HAGGLE_WSA_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_METADATA_ERROR = (HAGGLE_HANDLE_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_TIMEOUT_ERROR = (HAGGLE_METADATA_ERROR + 1) # /usr/local/include/libhaggle/error.h: 25

HAGGLE_NO_ERROR = 0 # /usr/local/include/libhaggle/error.h: 25

# /usr/local/include/libhaggle/error.h: 64
try:
    libhaggle_errno = (c_int).in_dll(_libs['haggle'], 'libhaggle_errno')
except:
    pass

# /usr/local/include/libhaggle/error.h: 72
if hasattr(_libs['haggle'], 'haggle_get_error'):
    haggle_get_error = _libs['haggle'].haggle_get_error
    haggle_get_error.argtypes = []
    haggle_get_error.restype = c_int

enum_haggle_interface_type = c_int # /usr/local/include/libhaggle/interface.h: 50

IF_TYPE_UNDEFINED = 0 # /usr/local/include/libhaggle/interface.h: 50

IF_TYPE_APPLICATION_PORT = (IF_TYPE_UNDEFINED + 1) # /usr/local/include/libhaggle/interface.h: 50

IF_TYPE_APPLICATION_LOCAL = (IF_TYPE_APPLICATION_PORT + 1) # /usr/local/include/libhaggle/interface.h: 50

IF_TYPE_ETHERNET = (IF_TYPE_APPLICATION_LOCAL + 1) # /usr/local/include/libhaggle/interface.h: 50

IF_TYPE_WIFI = (IF_TYPE_ETHERNET + 1) # /usr/local/include/libhaggle/interface.h: 50

IF_TYPE_BLUETOOTH = (IF_TYPE_WIFI + 1) # /usr/local/include/libhaggle/interface.h: 50

IF_TYPE_MEDIA = (IF_TYPE_BLUETOOTH + 1) # /usr/local/include/libhaggle/interface.h: 50

_IF_TYPE_MAX = (IF_TYPE_MEDIA + 1) # /usr/local/include/libhaggle/interface.h: 50

haggle_interface_type_t = enum_haggle_interface_type # /usr/local/include/libhaggle/interface.h: 50

enum_haggle_interface_status = c_int # /usr/local/include/libhaggle/interface.h: 58

IF_STATUS_UNDEFINED = 0 # /usr/local/include/libhaggle/interface.h: 58

IF_STATUS_UP = (IF_STATUS_UNDEFINED + 1) # /usr/local/include/libhaggle/interface.h: 58

IF_STATUS_DOWN = (IF_STATUS_UP + 1) # /usr/local/include/libhaggle/interface.h: 58

_IF_STATUS_MAX = (IF_STATUS_DOWN + 1) # /usr/local/include/libhaggle/interface.h: 58

haggle_interface_status_t = enum_haggle_interface_status # /usr/local/include/libhaggle/interface.h: 58

# /usr/local/include/libhaggle/interface.h: 69
class struct_haggle_interface(Structure):
    pass

struct_haggle_interface.__slots__ = [
    'l',
    'type',
    'status',
    'name',
    'identifier_str',
    'identifier_len',
    'identifier',
]
struct_haggle_interface._fields_ = [
    ('l', list_t),
    ('type', haggle_interface_type_t),
    ('status', haggle_interface_status_t),
    ('name', String),
    ('identifier_str', String),
    ('identifier_len', c_size_t),
    ('identifier', c_char * 0),
]

haggle_interface_t = struct_haggle_interface # /usr/local/include/libhaggle/interface.h: 69

# /usr/local/include/libhaggle/interface.h: 79
if hasattr(_libs['haggle'], 'haggle_interface_str_to_type'):
    haggle_interface_str_to_type = _libs['haggle'].haggle_interface_str_to_type
    haggle_interface_str_to_type.argtypes = [String]
    haggle_interface_str_to_type.restype = haggle_interface_type_t

# /usr/local/include/libhaggle/interface.h: 80
if hasattr(_libs['haggle'], 'haggle_interface_new'):
    haggle_interface_new = _libs['haggle'].haggle_interface_new
    haggle_interface_new.argtypes = [haggle_interface_type_t, String, String, c_size_t]
    haggle_interface_new.restype = POINTER(haggle_interface_t)

# /usr/local/include/libhaggle/interface.h: 81
if hasattr(_libs['haggle'], 'haggle_interface_copy'):
    haggle_interface_copy = _libs['haggle'].haggle_interface_copy
    haggle_interface_copy.argtypes = [POINTER(haggle_interface_t)]
    haggle_interface_copy.restype = POINTER(haggle_interface_t)

# /usr/local/include/libhaggle/interface.h: 82
if hasattr(_libs['haggle'], 'haggle_interface_get_type'):
    haggle_interface_get_type = _libs['haggle'].haggle_interface_get_type
    haggle_interface_get_type.argtypes = [POINTER(haggle_interface_t)]
    haggle_interface_get_type.restype = haggle_interface_type_t

# /usr/local/include/libhaggle/interface.h: 83
if hasattr(_libs['haggle'], 'haggle_interface_get_type_name'):
    haggle_interface_get_type_name = _libs['haggle'].haggle_interface_get_type_name
    haggle_interface_get_type_name.argtypes = [POINTER(haggle_interface_t)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_interface_get_type_name.restype = ReturnString
    else:
        haggle_interface_get_type_name.restype = String
        haggle_interface_get_type_name.errcheck = ReturnString

# /usr/local/include/libhaggle/interface.h: 84
if hasattr(_libs['haggle'], 'haggle_interface_get_status'):
    haggle_interface_get_status = _libs['haggle'].haggle_interface_get_status
    haggle_interface_get_status.argtypes = [POINTER(haggle_interface_t)]
    haggle_interface_get_status.restype = haggle_interface_status_t

# /usr/local/include/libhaggle/interface.h: 85
if hasattr(_libs['haggle'], 'haggle_interface_get_status_name'):
    haggle_interface_get_status_name = _libs['haggle'].haggle_interface_get_status_name
    haggle_interface_get_status_name.argtypes = [POINTER(haggle_interface_t)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_interface_get_status_name.restype = ReturnString
    else:
        haggle_interface_get_status_name.restype = String
        haggle_interface_get_status_name.errcheck = ReturnString

# /usr/local/include/libhaggle/interface.h: 86
if hasattr(_libs['haggle'], 'haggle_interface_get_name'):
    haggle_interface_get_name = _libs['haggle'].haggle_interface_get_name
    haggle_interface_get_name.argtypes = [POINTER(haggle_interface_t)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_interface_get_name.restype = ReturnString
    else:
        haggle_interface_get_name.restype = String
        haggle_interface_get_name.errcheck = ReturnString

# /usr/local/include/libhaggle/interface.h: 87
if hasattr(_libs['haggle'], 'haggle_interface_get_identifier'):
    haggle_interface_get_identifier = _libs['haggle'].haggle_interface_get_identifier
    haggle_interface_get_identifier.argtypes = [POINTER(haggle_interface_t)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_interface_get_identifier.restype = ReturnString
    else:
        haggle_interface_get_identifier.restype = String
        haggle_interface_get_identifier.errcheck = ReturnString

# /usr/local/include/libhaggle/interface.h: 88
if hasattr(_libs['haggle'], 'haggle_interface_get_identifier_length'):
    haggle_interface_get_identifier_length = _libs['haggle'].haggle_interface_get_identifier_length
    haggle_interface_get_identifier_length.argtypes = [POINTER(haggle_interface_t)]
    haggle_interface_get_identifier_length.restype = c_size_t

# /usr/local/include/libhaggle/interface.h: 89
if hasattr(_libs['haggle'], 'haggle_interface_get_identifier_str'):
    haggle_interface_get_identifier_str = _libs['haggle'].haggle_interface_get_identifier_str
    haggle_interface_get_identifier_str.argtypes = [POINTER(haggle_interface_t)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_interface_get_identifier_str.restype = ReturnString
    else:
        haggle_interface_get_identifier_str.restype = String
        haggle_interface_get_identifier_str.errcheck = ReturnString

# /usr/local/include/libhaggle/interface.h: 90
if hasattr(_libs['haggle'], 'haggle_interface_free'):
    haggle_interface_free = _libs['haggle'].haggle_interface_free
    haggle_interface_free.argtypes = [POINTER(haggle_interface_t)]
    haggle_interface_free.restype = None

# /usr/local/include/libhaggle/interface.h: 91
if hasattr(_libs['haggle'], 'haggle_interface_new_from_metadata'):
    haggle_interface_new_from_metadata = _libs['haggle'].haggle_interface_new_from_metadata
    haggle_interface_new_from_metadata.argtypes = [POINTER(struct_metadata)]
    haggle_interface_new_from_metadata.restype = POINTER(haggle_interface_t)

# /usr/local/include/libhaggle/node.h: 42
class struct_node(Structure):
    pass

struct_node.__slots__ = [
    'l',
    'id',
    'name',
    'num_attr',
    'num_interfaces',
    'attributes',
    'interfaces',
]
struct_node._fields_ = [
    ('l', list_t),
    ('id', c_char * 20),
    ('name', String),
    ('num_attr', c_int),
    ('num_interfaces', c_int),
    ('attributes', list_t),
    ('interfaces', list_t),
]

haggle_node_t = struct_node # /usr/local/include/libhaggle/node.h: 42

# /usr/local/include/libhaggle/node.h: 50
if hasattr(_libs['haggle'], 'haggle_node_new_from_metadata'):
    haggle_node_new_from_metadata = _libs['haggle'].haggle_node_new_from_metadata
    haggle_node_new_from_metadata.argtypes = [POINTER(struct_metadata)]
    haggle_node_new_from_metadata.restype = POINTER(struct_node)

# /usr/local/include/libhaggle/node.h: 56
if hasattr(_libs['haggle'], 'haggle_node_free'):
    haggle_node_free = _libs['haggle'].haggle_node_free
    haggle_node_free.argtypes = [POINTER(struct_node)]
    haggle_node_free.restype = None

# /usr/local/include/libhaggle/node.h: 58
if hasattr(_libs['haggle'], 'haggle_node_get_name'):
    haggle_node_get_name = _libs['haggle'].haggle_node_get_name
    haggle_node_get_name.argtypes = [POINTER(struct_node)]
    if sizeof(c_int) == sizeof(c_void_p):
        haggle_node_get_name.restype = ReturnString
    else:
        haggle_node_get_name.restype = String
        haggle_node_get_name.errcheck = ReturnString

# /usr/local/include/libhaggle/node.h: 60
if hasattr(_libs['haggle'], 'haggle_node_get_num_interfaces'):
    haggle_node_get_num_interfaces = _libs['haggle'].haggle_node_get_num_interfaces
    haggle_node_get_num_interfaces.argtypes = [POINTER(struct_node)]
    haggle_node_get_num_interfaces.restype = c_int

# /usr/local/include/libhaggle/node.h: 63
if hasattr(_libs['haggle'], 'haggle_node_get_interface_n'):
    haggle_node_get_interface_n = _libs['haggle'].haggle_node_get_interface_n
    haggle_node_get_interface_n.argtypes = [POINTER(struct_node), c_int]
    haggle_node_get_interface_n.restype = POINTER(haggle_interface_t)

# /usr/local/include/libhaggle/node.h: 73
if hasattr(_libs['haggle'], 'haggle_node_add_attribute'):
    haggle_node_add_attribute = _libs['haggle'].haggle_node_add_attribute
    haggle_node_add_attribute.argtypes = [POINTER(struct_node), String, String]
    haggle_node_add_attribute.restype = c_int

# /usr/local/include/libhaggle/node.h: 82
if hasattr(_libs['haggle'], 'haggle_node_add_attribute_weighted'):
    haggle_node_add_attribute_weighted = _libs['haggle'].haggle_node_add_attribute_weighted
    haggle_node_add_attribute_weighted.argtypes = [POINTER(struct_node), String, String, c_ulong]
    haggle_node_add_attribute_weighted.restype = c_int

# /usr/local/include/libhaggle/node.h: 90
if hasattr(_libs['haggle'], 'haggle_node_get_num_attributes'):
    haggle_node_get_num_attributes = _libs['haggle'].haggle_node_get_num_attributes
    haggle_node_get_num_attributes.argtypes = [POINTER(struct_node)]
    haggle_node_get_num_attributes.restype = c_int

# /usr/local/include/libhaggle/node.h: 100
if hasattr(_libs['haggle'], 'haggle_node_get_attribute_n'):
    haggle_node_get_attribute_n = _libs['haggle'].haggle_node_get_attribute_n
    haggle_node_get_attribute_n.argtypes = [POINTER(struct_node), c_int]
    haggle_node_get_attribute_n.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/node.h: 110
if hasattr(_libs['haggle'], 'haggle_node_get_attribute_by_name'):
    haggle_node_get_attribute_by_name = _libs['haggle'].haggle_node_get_attribute_by_name
    haggle_node_get_attribute_by_name.argtypes = [POINTER(struct_node), String]
    haggle_node_get_attribute_by_name.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/node.h: 120
if hasattr(_libs['haggle'], 'haggle_node_get_attribute_by_name_n'):
    haggle_node_get_attribute_by_name_n = _libs['haggle'].haggle_node_get_attribute_by_name_n
    haggle_node_get_attribute_by_name_n.argtypes = [POINTER(struct_node), String, c_int]
    haggle_node_get_attribute_by_name_n.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/node.h: 130
if hasattr(_libs['haggle'], 'haggle_node_get_attribute_by_name_value'):
    haggle_node_get_attribute_by_name_value = _libs['haggle'].haggle_node_get_attribute_by_name_value
    haggle_node_get_attribute_by_name_value.argtypes = [POINTER(struct_node), String, String]
    haggle_node_get_attribute_by_name_value.restype = POINTER(struct_attribute)

# /usr/local/include/libhaggle/node.h: 155
class struct_nodelist(Structure):
    pass

struct_nodelist.__slots__ = [
    'nodes',
    'num_nodes',
]
struct_nodelist._fields_ = [
    ('nodes', list_t),
    ('num_nodes', c_int),
]

haggle_nodelist_t = struct_nodelist # /usr/local/include/libhaggle/node.h: 155

# /usr/local/include/libhaggle/node.h: 157
if hasattr(_libs['haggle'], 'haggle_nodelist_new_from_metadata'):
    haggle_nodelist_new_from_metadata = _libs['haggle'].haggle_nodelist_new_from_metadata
    haggle_nodelist_new_from_metadata.argtypes = [POINTER(struct_metadata)]
    haggle_nodelist_new_from_metadata.restype = POINTER(haggle_nodelist_t)

# /usr/local/include/libhaggle/node.h: 158
if hasattr(_libs['haggle'], 'haggle_nodelist_get_node_n'):
    haggle_nodelist_get_node_n = _libs['haggle'].haggle_nodelist_get_node_n
    haggle_nodelist_get_node_n.argtypes = [POINTER(haggle_nodelist_t), c_int]
    haggle_nodelist_get_node_n.restype = POINTER(struct_node)

# /usr/local/include/libhaggle/node.h: 160
if hasattr(_libs['haggle'], 'haggle_nodelist_pop'):
    haggle_nodelist_pop = _libs['haggle'].haggle_nodelist_pop
    haggle_nodelist_pop.argtypes = [POINTER(haggle_nodelist_t)]
    haggle_nodelist_pop.restype = POINTER(struct_node)

# /usr/local/include/libhaggle/node.h: 162
if hasattr(_libs['haggle'], 'haggle_nodelist_size'):
    haggle_nodelist_size = _libs['haggle'].haggle_nodelist_size
    haggle_nodelist_size.argtypes = [POINTER(haggle_nodelist_t)]
    haggle_nodelist_size.restype = c_int

# /usr/local/include/libhaggle/node.h: 164
if hasattr(_libs['haggle'], 'haggle_nodelist_free'):
    haggle_nodelist_free = _libs['haggle'].haggle_nodelist_free
    haggle_nodelist_free.argtypes = [POINTER(haggle_nodelist_t)]
    haggle_nodelist_free.restype = None

enum_event_type = c_int # /usr/local/include/libhaggle/ipc.h: 40

LIBHAGGLE_EVENT_SHUTDOWN = 0 # /usr/local/include/libhaggle/ipc.h: 40

LIBHAGGLE_EVENT_NEIGHBOR_UPDATE = (LIBHAGGLE_EVENT_SHUTDOWN + 1) # /usr/local/include/libhaggle/ipc.h: 40

LIBHAGGLE_EVENT_NEW_DATAOBJECT = (LIBHAGGLE_EVENT_NEIGHBOR_UPDATE + 1) # /usr/local/include/libhaggle/ipc.h: 40

LIBHAGGLE_EVENT_INTEREST_LIST = (LIBHAGGLE_EVENT_NEW_DATAOBJECT + 1) # /usr/local/include/libhaggle/ipc.h: 40

_LIBHAGGLE_NUM_EVENTS = (LIBHAGGLE_EVENT_INTEREST_LIST + 1) # /usr/local/include/libhaggle/ipc.h: 40

haggle_event_type_t = enum_event_type # /usr/local/include/libhaggle/ipc.h: 40

# /usr/local/include/libhaggle/ipc.h: 44
class union_anon_25(Union):
    pass

union_anon_25.__slots__ = [
    'dobj',
    'interests',
    'neighbors',
    'shutdown_reason',
]
union_anon_25._fields_ = [
    ('dobj', POINTER(struct_dataobject)),
    ('interests', POINTER(struct_attributelist)),
    ('neighbors', POINTER(struct_nodelist)),
    ('shutdown_reason', c_uint),
]

# /usr/local/include/libhaggle/ipc.h: 50
class struct_event(Structure):
    pass

struct_event.__slots__ = [
    'type',
    'unnamed_1',
]
struct_event._anonymous_ = [
    'unnamed_1',
]
struct_event._fields_ = [
    ('type', haggle_event_type_t),
    ('unnamed_1', union_anon_25),
]

haggle_event_t = struct_event # /usr/local/include/libhaggle/ipc.h: 50

enum_daemon_status = c_int # /usr/local/include/libhaggle/ipc.h: 52

HAGGLE_DAEMON_ERROR = HAGGLE_ERROR # /usr/local/include/libhaggle/ipc.h: 52

HAGGLE_DAEMON_NOT_RUNNING = HAGGLE_NO_ERROR # /usr/local/include/libhaggle/ipc.h: 52

HAGGLE_DAEMON_RUNNING = 1 # /usr/local/include/libhaggle/ipc.h: 52

HAGGLE_DAEMON_CRASHED = 2 # /usr/local/include/libhaggle/ipc.h: 52

# /usr/local/include/libhaggle/ipc.h: 59
class struct_haggle_handle(Structure):
    pass

haggle_handle_t = POINTER(struct_haggle_handle) # /usr/local/include/libhaggle/ipc.h: 59

haggle_event_handler_t = CFUNCTYPE(UNCHECKED(c_int), POINTER(haggle_event_t), POINTER(None)) # /usr/local/include/libhaggle/ipc.h: 76

haggle_event_loop_start_t = CFUNCTYPE(UNCHECKED(None), POINTER(None)) # /usr/local/include/libhaggle/ipc.h: 80

haggle_event_loop_stop_t = CFUNCTYPE(UNCHECKED(None), POINTER(None)) # /usr/local/include/libhaggle/ipc.h: 81

daemon_spawn_callback_t = CFUNCTYPE(UNCHECKED(c_int), c_uint) # /usr/local/include/libhaggle/ipc.h: 91

enum_control_type = c_int # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_INVALID = (-1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_REGISTRATION_REQUEST = 0 # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_REGISTRATION_REPLY = (CTRL_TYPE_REGISTRATION_REQUEST + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_DEREGISTRATION_NOTICE = (CTRL_TYPE_REGISTRATION_REPLY + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_REGISTER_INTEREST = (CTRL_TYPE_DEREGISTRATION_NOTICE + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_REMOVE_INTEREST = (CTRL_TYPE_REGISTER_INTEREST + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_GET_INTERESTS = (CTRL_TYPE_REMOVE_INTEREST + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_REGISTER_EVENT_INTEREST = (CTRL_TYPE_GET_INTERESTS + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_MATCHING_DATAOBJECT = (CTRL_TYPE_REGISTER_EVENT_INTEREST + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_DELETE_DATAOBJECT = (CTRL_TYPE_MATCHING_DATAOBJECT + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_GET_DATAOBJECTS = (CTRL_TYPE_DELETE_DATAOBJECT + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_SEND_NODE_DESCRIPTION = (CTRL_TYPE_GET_DATAOBJECTS + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_SHUTDOWN = (CTRL_TYPE_SEND_NODE_DESCRIPTION + 1) # /usr/local/include/libhaggle/ipc.h: 112

CTRL_TYPE_EVENT = (CTRL_TYPE_SHUTDOWN + 1) # /usr/local/include/libhaggle/ipc.h: 112

control_type_t = enum_control_type # /usr/local/include/libhaggle/ipc.h: 112

# /usr/local/include/libhaggle/ipc.h: 176
if hasattr(_libs['haggle'], 'haggle_handle_get'):
    haggle_handle_get = _libs['haggle'].haggle_handle_get
    haggle_handle_get.argtypes = [String, POINTER(haggle_handle_t)]
    haggle_handle_get.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 183
if hasattr(_libs['haggle'], 'haggle_handle_free'):
    haggle_handle_free = _libs['haggle'].haggle_handle_free
    haggle_handle_free.argtypes = [haggle_handle_t]
    haggle_handle_free.restype = None

# /usr/local/include/libhaggle/ipc.h: 195
if hasattr(_libs['haggle'], 'haggle_daemon_pid'):
    haggle_daemon_pid = _libs['haggle'].haggle_daemon_pid
    haggle_daemon_pid.argtypes = [POINTER(c_ulong)]
    haggle_daemon_pid.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 207
if hasattr(_libs['haggle'], 'haggle_daemon_spawn'):
    haggle_daemon_spawn = _libs['haggle'].haggle_daemon_spawn
    haggle_daemon_spawn.argtypes = [String]
    haggle_daemon_spawn.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 208
if hasattr(_libs['haggle'], 'haggle_daemon_spawn_with_callback'):
    haggle_daemon_spawn_with_callback = _libs['haggle'].haggle_daemon_spawn_with_callback
    haggle_daemon_spawn_with_callback.argtypes = [String, daemon_spawn_callback_t]
    haggle_daemon_spawn_with_callback.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 219
if hasattr(_libs['haggle'], 'haggle_unregister'):
    haggle_unregister = _libs['haggle'].haggle_unregister
    haggle_unregister.argtypes = [String]
    haggle_unregister.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 226
if hasattr(_libs['haggle'], 'haggle_handle_get_session_id'):
    haggle_handle_get_session_id = _libs['haggle'].haggle_handle_get_session_id
    haggle_handle_get_session_id.argtypes = [haggle_handle_t]
    haggle_handle_get_session_id.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 235
if hasattr(_libs['haggle'], 'haggle_ipc_publish_dataobject'):
    haggle_ipc_publish_dataobject = _libs['haggle'].haggle_ipc_publish_dataobject
    haggle_ipc_publish_dataobject.argtypes = [haggle_handle_t, POINTER(struct_dataobject)]
    haggle_ipc_publish_dataobject.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 246
if hasattr(_libs['haggle'], 'haggle_ipc_register_event_interest'):
    haggle_ipc_register_event_interest = _libs['haggle'].haggle_ipc_register_event_interest
    haggle_ipc_register_event_interest.argtypes = [haggle_handle_t, c_int, haggle_event_handler_t]
    haggle_ipc_register_event_interest.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 247
if hasattr(_libs['haggle'], 'haggle_ipc_register_event_interest_with_arg'):
    haggle_ipc_register_event_interest_with_arg = _libs['haggle'].haggle_ipc_register_event_interest_with_arg
    haggle_ipc_register_event_interest_with_arg.argtypes = [haggle_handle_t, c_int, haggle_event_handler_t, POINTER(None)]
    haggle_ipc_register_event_interest_with_arg.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 258
if hasattr(_libs['haggle'], 'haggle_ipc_add_application_interest'):
    haggle_ipc_add_application_interest = _libs['haggle'].haggle_ipc_add_application_interest
    haggle_ipc_add_application_interest.argtypes = [haggle_handle_t, String, String]
    haggle_ipc_add_application_interest.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 269
if hasattr(_libs['haggle'], 'haggle_ipc_add_application_interest_weighted'):
    haggle_ipc_add_application_interest_weighted = _libs['haggle'].haggle_ipc_add_application_interest_weighted
    haggle_ipc_add_application_interest_weighted.argtypes = [haggle_handle_t, String, String, c_ulong]
    haggle_ipc_add_application_interest_weighted.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 280
if hasattr(_libs['haggle'], 'haggle_ipc_add_application_interests'):
    haggle_ipc_add_application_interests = _libs['haggle'].haggle_ipc_add_application_interests
    haggle_ipc_add_application_interests.argtypes = [haggle_handle_t, POINTER(struct_attributelist)]
    haggle_ipc_add_application_interests.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 288
if hasattr(_libs['haggle'], 'haggle_ipc_remove_application_interest'):
    haggle_ipc_remove_application_interest = _libs['haggle'].haggle_ipc_remove_application_interest
    haggle_ipc_remove_application_interest.argtypes = [haggle_handle_t, String, String]
    haggle_ipc_remove_application_interest.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 298
if hasattr(_libs['haggle'], 'haggle_ipc_remove_application_interests'):
    haggle_ipc_remove_application_interests = _libs['haggle'].haggle_ipc_remove_application_interests
    haggle_ipc_remove_application_interests.argtypes = [haggle_handle_t, POINTER(struct_attributelist)]
    haggle_ipc_remove_application_interests.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 308
if hasattr(_libs['haggle'], 'haggle_ipc_get_application_interests_async'):
    haggle_ipc_get_application_interests_async = _libs['haggle'].haggle_ipc_get_application_interests_async
    haggle_ipc_get_application_interests_async.argtypes = [haggle_handle_t]
    haggle_ipc_get_application_interests_async.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 317
if hasattr(_libs['haggle'], 'haggle_ipc_get_data_objects_async'):
    haggle_ipc_get_data_objects_async = _libs['haggle'].haggle_ipc_get_data_objects_async
    haggle_ipc_get_data_objects_async.argtypes = [haggle_handle_t]
    haggle_ipc_get_data_objects_async.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 333
if hasattr(_libs['haggle'], 'haggle_ipc_delete_data_object_by_id_bloomfilter'):
    haggle_ipc_delete_data_object_by_id_bloomfilter = _libs['haggle'].haggle_ipc_delete_data_object_by_id_bloomfilter
    haggle_ipc_delete_data_object_by_id_bloomfilter.argtypes = [haggle_handle_t, c_ubyte * 20, c_int]
    haggle_ipc_delete_data_object_by_id_bloomfilter.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 334
if hasattr(_libs['haggle'], 'haggle_ipc_delete_data_object_bloomfilter'):
    haggle_ipc_delete_data_object_bloomfilter = _libs['haggle'].haggle_ipc_delete_data_object_bloomfilter
    haggle_ipc_delete_data_object_bloomfilter.argtypes = [haggle_handle_t, POINTER(struct_dataobject), c_int]
    haggle_ipc_delete_data_object_bloomfilter.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 335
if hasattr(_libs['haggle'], 'haggle_ipc_delete_data_object_by_id'):
    haggle_ipc_delete_data_object_by_id = _libs['haggle'].haggle_ipc_delete_data_object_by_id
    haggle_ipc_delete_data_object_by_id.argtypes = [haggle_handle_t, c_ubyte * 20]
    haggle_ipc_delete_data_object_by_id.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 336
if hasattr(_libs['haggle'], 'haggle_ipc_delete_data_object'):
    haggle_ipc_delete_data_object = _libs['haggle'].haggle_ipc_delete_data_object
    haggle_ipc_delete_data_object.argtypes = [haggle_handle_t, POINTER(struct_dataobject)]
    haggle_ipc_delete_data_object.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 345
if hasattr(_libs['haggle'], 'haggle_ipc_send_node_description'):
    haggle_ipc_send_node_description = _libs['haggle'].haggle_ipc_send_node_description
    haggle_ipc_send_node_description.argtypes = [haggle_handle_t]
    haggle_ipc_send_node_description.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 352
if hasattr(_libs['haggle'], 'haggle_ipc_shutdown'):
    haggle_ipc_shutdown = _libs['haggle'].haggle_ipc_shutdown
    haggle_ipc_shutdown.argtypes = [haggle_handle_t]
    haggle_ipc_shutdown.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 360
if hasattr(_libs['haggle'], 'haggle_event_loop_is_running'):
    haggle_event_loop_is_running = _libs['haggle'].haggle_event_loop_is_running
    haggle_event_loop_is_running.argtypes = [haggle_handle_t]
    haggle_event_loop_is_running.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 370
if hasattr(_libs['haggle'], 'haggle_event_loop_run'):
    haggle_event_loop_run = _libs['haggle'].haggle_event_loop_run
    haggle_event_loop_run.argtypes = [haggle_handle_t]
    haggle_event_loop_run.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 379
if hasattr(_libs['haggle'], 'haggle_event_loop_run_async'):
    haggle_event_loop_run_async = _libs['haggle'].haggle_event_loop_run_async
    haggle_event_loop_run_async.argtypes = [haggle_handle_t]
    haggle_event_loop_run_async.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 385
if hasattr(_libs['haggle'], 'haggle_event_loop_stop'):
    haggle_event_loop_stop = _libs['haggle'].haggle_event_loop_stop
    haggle_event_loop_stop.argtypes = [haggle_handle_t]
    haggle_event_loop_stop.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 387
if hasattr(_libs['haggle'], 'haggle_event_loop_run_async'):
    haggle_event_loop_run_async = _libs['haggle'].haggle_event_loop_run_async
    haggle_event_loop_run_async.argtypes = [haggle_handle_t]
    haggle_event_loop_run_async.restype = c_int

# /usr/local/include/libhaggle/ipc.h: 402
if hasattr(_libs['haggle'], 'haggle_event_loop_register_callbacks'):
    haggle_event_loop_register_callbacks = _libs['haggle'].haggle_event_loop_register_callbacks
    haggle_event_loop_register_callbacks.argtypes = [haggle_handle_t, haggle_event_loop_start_t, haggle_event_loop_stop_t, POINTER(None)]
    haggle_event_loop_register_callbacks.restype = c_int

# /usr/local/include/libhaggle/list.h: 34
try:
    LIST_NULL = (-1)
except:
    pass

# /usr/local/include/libhaggle/list.h: 35
try:
    LIST_SUCCESS = 1
except:
    pass

# /usr/local/include/libhaggle/list.h: 111
def list_empty(head):
    return (head == ((head.contents.next).value))

# /usr/local/include/libhaggle/list.h: 113
def list_first(head):
    return (head.contents.next)

# /usr/local/include/libhaggle/list.h: 115
def list_unattached(le):
    return ((((le.contents.next).value) == le) and (((le.contents.prev).value) == le))

# /usr/local/include/libhaggle/dataobject.h: 43
try:
    HASH_LENGTH = 20
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 51
try:
    DATAOBJECT_FLAG_NONE = 0
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 52
try:
    DATAOBJECT_FLAG_PERSISTENT = 1
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 53
try:
    DATAOBJECT_FLAG_ALL = (~0)
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 56
try:
    DATAOBJECT_CREATE_TIME_PARAM = 'create_time'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 57
try:
    DATAOBJECT_PERSISTENT_PARAM = 'persistent'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 60
try:
    DATAOBJECT_METADATA_DATA = 'Data'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 61
try:
    DATAOBJECT_METADATA_DATA_DATALEN_PARAM = 'data_len'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 62
try:
    DATAOBJECT_METADATA_DATA_FILEPATH = 'FilePath'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 63
try:
    DATAOBJECT_METADATA_DATA_FILENAME = 'FileName'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 64
try:
    DATAOBJECT_METADATA_DATA_FILEHASH = 'FileHash'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 65
try:
    DATAOBJECT_METADATA_DATA_THUMBNAIL = 'Thumbnail'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 68
try:
    DATAOBJECT_CONTROL_ATTR = 'Control'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 70
try:
    DATAOBJECT_METADATA_ATTRIBUTE = 'Attr'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 71
try:
    DATAOBJECT_METADATA_ATTRIBUTE_NAME_PARAM = 'name'
except:
    pass

# /usr/local/include/libhaggle/dataobject.h: 72
try:
    DATAOBJECT_METADATA_ATTRIBUTE_WEIGHT_PARAM = 'weight'
except:
    pass

# /usr/local/include/libhaggle/node.h: 32
try:
    NODE_ID_LEN = 20
except:
    pass

# /usr/local/include/libhaggle/node.h: 44
try:
    HAGGLE_XML_NODE_NAME = 'Node'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 94
try:
    LIBHAGGLE_ERR_BAD_HANDLE = 1
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 95
try:
    LIBHAGGLE_ERR_NOT_CONNECTED = 2
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 114
try:
    DATAOBJECT_METADATA_APPLICATION = 'Application'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 115
try:
    DATAOBJECT_METADATA_APPLICATION_NAME_PARAM = 'name'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 116
try:
    DATAOBJECT_METADATA_APPLICATION_ID_PARAM = 'id'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 117
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL = 'Control'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 118
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_TYPE_PARAM = 'type'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 119
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_MESSAGE = 'Message'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 120
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_DIRECTORY = 'Directory'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 121
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_SESSION = 'Session'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 122
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT = 'Event'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 123
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_TYPE_PARAM = 'type'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 124
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST = 'Interest'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 125
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_NAME_PARAM = 'name'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 126
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_INTEREST_WEIGHT_PARAM = 'weight'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 127
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST = 'Interest'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 128
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST_NAME_PARAM = 'name'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 129
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_EVENT_INTEREST_WEIGHT_PARAM = 'weight'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 130
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT = 'DataObject'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 131
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT_ID_PARAM = 'id'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 132
try:
    DATAOBJECT_METADATA_APPLICATION_CONTROL_DATAOBJECT_BLOOMFILTER_PARAM = 'keep_in_bloomfilter'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 135
try:
    HAGGLE_ATTR_CONTROL_NAME = 'HaggleIPC'
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 138
try:
    IO_NO_REPLY = (-2)
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 139
try:
    IO_REPLY_BLOCK = (-1)
except:
    pass

# /usr/local/include/libhaggle/ipc.h: 140
try:
    IO_REPLY_NON_BLOCK = 0
except:
    pass

list = struct_list # /usr/local/include/libhaggle/list.h: 30

attribute = struct_attribute # /usr/local/include/libhaggle/attribute.h: 47

attributelist = struct_attributelist # /usr/local/include/libhaggle/attribute.h: 170

dataobject = struct_dataobject # /usr/local/include/libhaggle/dataobject.h: 48

metadata = struct_metadata # /usr/local/include/libhaggle/dataobject.h: 255

haggle_interface = struct_haggle_interface # /usr/local/include/libhaggle/interface.h: 69

node = struct_node # /usr/local/include/libhaggle/node.h: 42

nodelist = struct_nodelist # /usr/local/include/libhaggle/node.h: 155

event = struct_event # /usr/local/include/libhaggle/ipc.h: 50

haggle_handle = struct_haggle_handle # /usr/local/include/libhaggle/ipc.h: 59

# No inserted files

