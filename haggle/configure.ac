AC_INIT([haggle], [0.4], [haggle-developer@googlegroups.com])
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_LIBTOOL
AC_PROG_INSTALL

# Basic compiler flag configuration
# Enforce no usage of runtime type identification (RTTI)
BASIC_CXXFLAGS="-Wall -fno-rtti" 
BASIC_CFLAGS="-Wall"

CXXFLAGS="${CXXFLAGS} ${BASIC_CXXFLAGS}"
CFLAGS="${CFLAGS} ${BASIC_CFLAGS}"

# Set SQLite version to use.
# When changing SQLite version here, also update 
# AC_CONFIG_FILES at the end of this file
SQLITE_VERSION='3.7.0.1'
SQLITE_SUBDIR=extlibs/sqlite-${SQLITE_VERSION}
AC_SUBST(SQLITE_VERSION)
AC_SUBST(SQLITE_SUBDIR)

extra_config_files=""

haggle_os_linux=false
PLATFORM=

AC_MSG_CHECKING([for Linux])
case $host in
  *-*-linux*)
	haggle_os_linux=true
	PLATFORM=linux
	;;
esac
AC_MSG_RESULT([$haggle_os_linux])

haggle_os_macosx=false

AC_MSG_CHECKING([for MacOS X])
case $host in
  *-*-darwin*)
	haggle_os_macosx=true
	PLATFORM=darwin
	;;
esac

AC_MSG_RESULT([$haggle_os_macosx])

haggle_os_iphoneos=false

AC_MSG_CHECKING([for iPhoneOS])
case $host in
  arm-*-darwin*)
	haggle_os_iphoneos=true
	PLATFORM=darwin
	;;
esac

AC_MSG_RESULT([$haggle_os_iphoneos])

# For Windows compatibility in the future (currently we build in Visual Studio only)
haggle_os_windows_xp=false
haggle_os_windows_mobile=false

AM_CONDITIONAL(OS_LINUX, [test x$haggle_os_linux = xtrue])
AM_CONDITIONAL(OS_MACOSX, [test x$haggle_os_macosx = xtrue])
AM_CONDITIONAL(OS_IPHONEOS, [test x$haggle_os_iphoneos = xtrue])
AM_CONDITIONAL(OS_WINDOWS, [test x$haggle_os_windows_mobile = xtrue -o x$haggle_os_windows_xp = xtrue])
AM_CONDITIONAL(OS_WINDOWS_XP, [test x$haggle_os_windows_xp = xtrue])
AM_CONDITIONAL(OS_WINDOWS_MOBILE, [test x$haggle_os_windows_mobile = xtrue])

if test x$haggle_os_linux = xtrue
then
	CPPFLAGS="${CPPFLAGS} -DOS_LINUX"
fi
if test x$haggle_os_macosx = xtrue
then
	CPPFLAGS="${CPPFLAGS} -DOS_MACOSX"
fi

if test x$haggle_os_iphoneos = xtrue
then
	CPPFLAGS="${CPPFLAGS} -DOS_IPHONEOS"
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h stdlib.h string.h sys/socket.h sys/time.h unistd.h pthread.h])

# Do not use STL by default, but make it an option to choose
AC_ARG_ENABLE([stl], 
AS_HELP_STRING([--enable-stl],[Use STL instead of bundled String and container classes.]),
[case "${enableval}" in
  yes) stl=yes ;;
  no)  stl=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-stl]) ;;
esac],[stl=no])

AM_CONDITIONAL([ENABLE_STL], [test x$stl = xyes])

if test x$stl = xyes
then
	AC_CXX_HAVE_STD
	AC_CXX_HAVE_STL

	if test x$ac_cv_cxx_have_stl = xno
	then
		AC_MSG_ERROR(You need STL headers and libraries to enable STL)
	fi
     	CPPFLAGS="${CPPFLAGS} -DENABLE_STL"
fi

AM_PATH_XML2(2.4.0)

compile_openssl=no

AC_ARG_VAR([OPENSSL_DIR], [Location of OpenSSL source code])
dnl Allow the user to set the openssl dir
AC_ARG_WITH(openssl,
	    AS_HELP_STRING([--with-openssl=DIR],[DIR to look for OpenSSL]),
	    [OPENSSL_DIR=$withval;compile_openssl=yes],)

if test "x$compile_openssl" = 'xyes';
then
	compile_openssl=yes
	OPENSSL_INCLUDE_DIR="${OPENSSL_DIR}/include"
	OPENSSL_LIB_DIR="${OPENSSL_DIR}"
	CPPFLAGS="${CPPFLAGS} -I${OPENSSL_INCLUDE_DIR}"
	CFLAGS="${CFLAGS} -I${OPENSSL_INCLUDE_DIR}"
     	AC_SUBST(OPENSSL_INCLUDE_DIR)
     	AC_SUBST(OPENSSL_LIB_DIR)
     	AC_SUBST(OPENSSL_DIR)
else
# Check for OpenSSL in standard paths
  	AC_CHECK_LIB(crypto,RSA_sign,
		[AC_CHECK_HEADERS([openssl/rsa.h],,AC_MSG_ERROR(You have openssl but not the development headers))],AC_MSG_ERROR(openssl library is not found...),)

fi


AC_ARG_ENABLE([bundled_sqlite],
AS_HELP_STRING([--enable-bundled-sqlite],[Use bundled sqlite (in extlibs)]),
[case "${enableval}" in
  yes) bundled_sqlite=yes ;;
  no)  bundled_sqlite=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-bundled-sqlite]) ;;
esac],[bundled_sqlite=no])

AM_CONDITIONAL([BUNDLED_SQLITE], [test x$bundled_sqlite = xyes])

if test x$bundled_sqlite = xyes
then 
	SQLITE_INCLUDE_DIR="${PWD}/extlibs/sqlite-${SQLITE_VERSION}"
     	SQLITE_LIB_DIR="${PWD}/extlibs/sqlite-${SQLITE_VERSION}"
     	AC_SUBST(SQLITE_INCLUDE_DIR)
     	AC_SUBST(SQLITE_LIB_DIR)
else	
	AC_CHECK_LIB(sqlite3,sqlite3_open,
		[AC_CHECK_HEADERS([sqlite3.h],,AC_MSG_ERROR(You have libsqlite3 but not the development headers))],AC_MSG_ERROR(sqlite library not found...),)

fi

AC_ARG_ENABLE([media],
AS_HELP_STRING([--enable-media],[Turn on USB media support]),
[case "${enableval}" in
  yes) media=yes ;;
  no)  media=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-media]) ;;
esac],[media=no])

AC_ARG_ENABLE([ethernet],
AS_HELP_STRING([--disable-ethernet],[Turn off ethernet support]),
[case "${enableval}" in
  yes) ethernet=yes ;;
  no)  ethernet=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --disable-ethernet]) ;;
esac],[ethernet=yes])

AC_ARG_ENABLE([ipv6],
AS_HELP_STRING([--enable-ipv6],[Turn on IPv6 support]),
[case "${enableval}" in
  yes) ipv6=yes ;;
  no)  ipv6=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --disable-ethernet]) ;;
esac],[ipv6=no])

AC_ARG_ENABLE([bluetooth],
AS_HELP_STRING([--disable-bluetooth],[Turn off bluetooth support]),
[case "${enableval}" in
  yes) bluetooth=yes ;;
  no)  bluetooth=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --disable-bluetooth]) ;;
esac],[bluetooth=yes])

dnl Exception support
AC_ARG_ENABLE([exceptions],
AS_HELP_STRING([--disable-exceptions],[Turn off C++ exceptions]),
[case "${enableval}" in
  yes) exceptions=yes ;;
  no)  exceptions=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --disable-exceptions]) ;;
esac],[exceptions=yes])

AM_CONDITIONAL([HAVE_EXCEPTION], [test x$exceptions = xyes])

# enable filedrop by default
enable_filedrop=yes

dnl Linux specific stuff
if test x$haggle_os_linux = xtrue
then
	have_bluetooth=no
	if test x$bluetooth = xyes
	then
		AC_CHECK_LIB(bluetooth,hci_open_dev,
			[AC_CHECK_HEADERS([bluetooth/bluetooth.h],have_bluetooth=yes,AC_MSG_RESULT(You have libbluetooth but not the development headers))],AC_MSG_RESULT(bluetooth library not found...),)
		AC_MSG_CHECKING([for Bluetooth])
		AC_MSG_RESULT([$have_bluetooth])
	fi

	have_ethernet=no
	if test x$ethernet = xyes
	then
		# Should probably do some checking or something...
		have_ethernet=yes
	fi

	dnl no media support for Linux
	have_media=no

	dnl Check for D-Bus
	have_dbus=no

	AC_ARG_ENABLE([dbus],
	AS_HELP_STRING([--disable-dbus],[Disable D-Bus support]),
	[case "${enableval}" in
	  yes) dbus=yes ;;
	  no)  dbus=no ;;
	 *) AC_MSG_ERROR([bad value ${enableval} for --disable-dbus]) ;;
	esac],[dbus=yes])


	if test x$dbus = xyes
	then
		# Check for D-Bus
		DBUS_INCLUDES="`pkg-config --cflags dbus-1`"
		DBUS_LIBS="`pkg-config --libs dbus-1`"
		save_CPPFLAGS=${CPPFLAGS}
		CPPFLAGS="${CPPFLAGS} ${DBUS_INCLUDES}"
		AC_CHECK_LIB(dbus-1,dbus_bus_get,
			[AC_CHECK_HEADERS([dbus-1.0/dbus/dbus.h],have_dbus=yes,AC_MSG_RESULT(You have libdbus-1 but not the development headers))],AC_MSG_RESULT(D-Bus not found...),)
		AC_MSG_CHECKING([for D-Bus])	
     		AC_MSG_RESULT([$have_dbus])

		if test x$have_dbus = xyes; 
		then
			AC_DEFINE([HAVE_DBUS], 1, [D-Bus support])
			AC_CHECK_LIB(dbus-1,dbus_watch_get_unix_fd, AC_DEFINE([HAVE_DBUS_WATCH_GET_UNIX_FD], 1, [Have dbus_watch_get_unix_fd function]), AC_DEFINE([HAVE_DBUS_WATCH_GET_UNIX_FD], 0, [Does not have dbus_watch_get_unix_fd function]),,)
			AC_SUBST(DBUS_INCLUDES)
			AC_SUBST(DBUS_LIBS)	
		fi
		CPPFLAGS=${save_CPPFLAGS}
	fi

	dnl Check for inotify (needed by filedrop)
	AC_CHECK_HEADERS([linux/inotify.h],have_inotify=yes,have_inotify=no)

	if test x$have_inotify = xno;
	then
		enable_filedrop=no
	fi
fi

AM_CONDITIONAL(HAVE_DBUS, [test x$have_dbus = xyes])
AM_CONDITIONAL(HAVE_INOTIFY, [test x$have_inotify = xyes])

dnl extractor checking
	
AC_ARG_ENABLE(extractor, AS_HELP_STRING([--disable-extractor],[Build without extractor support]))
have_extractor=no
if test "x$enable_extractor" != "xno"; then
   	AC_CHECK_HEADERS([iconv.h],,AC_MSG_ERROR(You do not have the iconv development headers, required for extractor))

   	AC_CHECK_LIB(extractor,EXTRACTOR_binaryEncode,
		[AC_CHECK_HEADERS([extractor.h],have_extractor=yes,AC_MSG_RESULT(You have libextractor but not the development headers))],AC_MSG_RESULT(extractor library not found...),)
	
	AC_MSG_CHECKING([for Extractor])
	AC_MSG_RESULT([$have_extractor])

	if test x$have_extractor = xyes
	then
		AC_DEFINE(HAVE_EXTRACTOR, 1, [Define to enable extractor support])
        	AC_SUBST(EXTRACTOR_CFLAGS)
		AC_SUBST(EXTRACTOR_LIBS)
	fi
fi

AM_CONDITIONAL(HAVE_EXTRACTOR, [test x$have_extractor = xyes])

# MacOSX specific stuff
if test x$haggle_os_macosx = xtrue
then
	have_bluetooth=no
	if test x$bluetooth = xyes
	then
		#AC_CHECK_LIB(IOBluetooth,hci_open_dev,
			#[AC_CHECK_HEADERS([IOBluetooth/Bluetooth.h],have_bluetooth=yes,AC_MSG_RESULT(You have libbluetooth but not the development headers))],AC_MSG_RESULT(bluetooth library not found...),)
		AC_CHECK_HEADERS([IOBluetooth/Bluetooth.h],have_bluetooth=yes,AC_MSG_RESULT(You do not have the Bluetooth development headers))
		AC_MSG_CHECKING([for Bluetooth])
		AC_MSG_RESULT([$have_bluetooth])
	fi
	
	have_ethernet=no
	if test x$ethernet = xyes
	then
		# Should probably do some checking or something...
		have_ethernet=yes
	fi
	
	have_media=no
	if test x$media = xyes
	then
		# Should probably do some checking or something...
		have_media=yes
	fi
fi

# Check for bluetooth stuff:
AM_CONDITIONAL(ENABLE_BLUETOOTH, [test x$have_bluetooth = xyes])

if test x$have_bluetooth = xyes
then
	AC_DEFINE([ENABLE_BLUETOOTH], 1, [Bluetooth support])
	CPPFLAGS="${CPPFLAGS} -DENABLE_BLUETOOTH"
fi

# Check for ethernet stuff:
AM_CONDITIONAL(ENABLE_ETHERNET, [test x$have_ethernet = xyes])

if test x$have_ethernet = xyes
then
	AC_DEFINE([ENABLE_ETHERNET], 1, [Ethernet support])
    	CPPFLAGS="${CPPFLAGS} -DENABLE_ETHERNET"
fi

# Check for IPv6 stuff:
AM_CONDITIONAL(ENABLE_IPv6, [test x$ipv6 = xyes])

if test x$ipv6 = xyes
then
	AC_DEFINE([ENABLE_IPv6], 1, [IPv6 support])
    	CPPFLAGS="${CPPFLAGS} -DENABLE_IPv6"
fi

# Check for media stuff:
AM_CONDITIONAL(ENABLE_MEDIA, [test x$have_media = xyes])

if test x$have_media = xyes
then
	AC_DEFINE([ENABLE_MEDIA], 1, [USB media support])
   	CPPFLAGS="${CPPFLAGS} -DENABLE_MEDIA"
fi

# Check for filedrop stuff:
AM_CONDITIONAL(ENABLE_FILEDROP, [test x$enable_filedrop = xyes])

if test x$enable_filedrop = xyes
then
	AC_DEFINE([ENABLE_FILEDROP], 1, [Filedrop compilation])
fi

# Enable debug by default 
AC_ARG_ENABLE([debug],
AS_HELP_STRING([--disable-debug],[Turn off debugging]),
[case "${enableval}" in
  yes) debug=yes ;;
  no)  debug=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --disable-debug]) ;;
esac],[debug=yes])

AM_CONDITIONAL([DEBUG], [test x$debug = xyes])

# It seems autoconf has -g enabled by default. Therefore, 
# turn off -g when $debug is disabled
if test x$debug = xno
then 
     CFLAGS="${BASIC_CFLAGS} -O2"
     CXXFLAGS="${BASIC_CXXFLAGS} -O2"
else
     CPPFLAGS="${CPPFLAGS} -DDEBUG"
fi

AC_ARG_ENABLE([debug-manager],
AS_HELP_STRING([--enable-debug-manager],[Enable debug manager]),
[case "${enableval}" in
  yes) debugmanager=yes ;;
  no)  debugmanager=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],[debugmanager=$debug])

if test x$debugmanager = xyes
then
	if test x$debug = xno    
	then
		CPPFLAGS="${CPPFLAGS} -DENABLE_DEBUG_MANAGER"
	fi
else
	if test x$debug = xyes
    	then
		CPPFLAGS="${CPPFLAGS} -DDISABLE_DEBUG_MANAGER"
	fi
fi

AM_CONDITIONAL([DEBUG_MANAGER], [test x$debugmanager = xyes])

# Leak reporting
AC_ARG_ENABLE([debug_leaks],
AS_HELP_STRING([--enable-debug-leaks],[Turn on leak reporting]),
[case "${enableval}" in
  yes) debug_leaks=yes ;;
  no)  debug_leaks=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug-leaks]) ;;
esac],[debug_leaks=no])

AM_CONDITIONAL([DEBUG_LEAKS], [test x$debug_leaks = xyes])

if test x$debug_leaks = xyes
then 
     CPPFLAGS="${CPPFLAGS} -DDEBUG_LEAKS"
fi


AC_ARG_ENABLE([benchmark],
AS_HELP_STRING([--enable-benchmark],[Turn on benchmarking and profiling support.]),
[case "${enableval}" in
  yes) benchmark=yes ;;
  no)  benchmark=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-benchmark]) ;;
esac],[benchmark=no])

AM_CONDITIONAL([BENCHMARK], [test x$benchmark = xyes])

if test x$benchmark = xyes
then 
     CPPFLAGS="${CPPFLAGS} -DBENCHMARK"
fi


# Use a UNIX domain socket for IPC between application and daemon
AC_ARG_ENABLE([unix_socket],
AS_HELP_STRING([--enable-unix-socket],[Use a UNIX domain socket for IPC instead of UDP socket]),
[case "${enableval}" in
  yes) unix_socket=yes ;;
  no)  unix_socket=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-unix-socket]) ;;
esac],[unix_socket=no])

AM_CONDITIONAL([USE_UNIX_APPLICATION_SOCKET], [test x$unix_socket = xyes])

if test x$unix_socket = xyes
then 
     CPPFLAGS="${CPPFLAGS} -DUSE_UNIX_APPLICATION_SOCKET"
fi

# Omnet++ 
AC_ARG_ENABLE([omnetpp],
AS_HELP_STRING([--enable-omnetpp],[Enable OMNeT++ simulation support]),
[case "${enableval}" in
  *) omnetpp=yes; OMNETPP_PATH="${enableval}";;
esac],[omnetpp=no])

if test x$omnetpp = xyes
then
	CXXFLAGS="${CXXFLAGS} -I${OMNETPP_PATH}/include"
	#AC_MSG_NOTICE([Enabling OMNeT++ support.])
	# For some reason this check fails although the path is correct
	#AC_CHECK_HEADER([omnetpp.h],,AC_MSG_ERROR(Cannot find OMNeT++ headers. Check your OMNeT++ path set with --enable-omnetpp=PATH. Currently set to: $OMNETPP_PATH))
	AC_DEFINE([OMNETPP], 1, [OMNeT++ support])
else
	#AC_MSG_NOTICE([Omitting OMNeT++ support.])
	AC_DEFINE([OMNETPP], 0, [OMNeT++ support])
fi

AM_CONDITIONAL([OMNETPP], [test x$omnetpp = xyes])

# Check whether we should generate documentation with Doxygen
AC_ARG_ENABLE([docs], 
AS_HELP_STRING([--enable-docs],[Generate documentation with Doxygen]),
[case "${enableval}" in
  yes) docs=yes ;;
  no)  docs=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-docs]) ;;
esac],[docs=no])

AM_CONDITIONAL([DOCS], [test x$docs = xyes])

if test x$docs = xyes
then 
     AC_CHECK_PROG([have_doxygen], [doxygen], yes,no,,)
 
     if test x$have_doxygen = xno
     then
	AC_MSG_ERROR([You need Doxygen installed to generate documentation...])
     fi
fi

# Configuration for Java bindings
java_bindings=no;
have_jni=no;

AC_ARG_VAR([JAVA_HOME], [Java Runtime Environment (JRE) location])

dnl Allow the user to set the JAVA_HOME dir?
AC_ARG_WITH(java-home,
	    AS_HELP_STRING([--with-java-home=DIR],[DIR to look for Java headers]),
	    [JAVA_HOME=$withval],)

dnl Default to some qualified path guessing...
java_dirs="/usr /usr/local /usr/lib/jvm/java-5-sun /usr/lib/jvm/java-6-sun /usr/lib/jvm/java-6-openjdk /usr/lib/jvm/java /System/Library/Frameworks/JavaVM.framework/Versions/Current /System/Library/Frameworks/JavaVM.framework /opt /mingw"
java_inc_dirs="include include/libgcj Headers"

AC_CHECK_PROG(JAVA, javac, javac)
AC_MSG_CHECKING([for jni.h])

dnl Try to locate jni.h
for java_dir in $JAVA_HOME $java_dirs;
do
	for java_inc_dir in $java_inc_dirs
	do
		#AC_MSG_RESULT("${java_dir}/${java_inc_dir}/jni.h")	
		if test -r "${java_dir}/${java_inc_dir}/jni.h"; 
		then
			have_jni=yes
			java_bindings=yes
			JAVA_PATH=${java_dir}
  			break
       		fi
	done
	if test x$have_jni = xyes;
	then
		break
	fi
done

AC_MSG_RESULT([$have_jni])

JAVA_INCLUDE="-I${JAVA_PATH}/${java_inc_dir} -I${JAVA_PATH}/${java_inc_dir}/${PLATFORM}"

AC_SUBST(JAVA_INCLUDE)

dnl Allow the user to force Java bindings
AC_ARG_ENABLE([java_bindings], 
AS_HELP_STRING([--disable-java-bindings], [Disable libhaggle Java bindings]),
[case "${enableval}" in
  yes) java_bindings=yes ;;
  no)  java_bindings=no ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --disable-java-bindings]) ;;
esac],)

dnl Java bindings were detected or was forced
if test x$java_bindings = xyes; 
then
   	if test x$have_jni = xno; 
	then
   	   	AC_MSG_ERROR([Java bindings require jni.h. Please check your JAVA_HOME paths(or set --with-java-home).])
	else
		if test "x$JAVA" == 'x'; 
		then
			AC_MSG_ERROR([Java bindings require java and javac. Check your PATH or disable with --disable-java-bindings])
		fi
	fi
fi

AM_CONDITIONAL([JAVA_BINDINGS], [test x$java_bindings = xyes])

# Check if we should build Android applications
have_android_sdk=yes

# If we cannot build the the bindings, do not care about apps
if test x$java_bindings = xyes;
then
	# Check for Android SDK and NDK
	AC_CHECK_PROG(have_android_sdk, apkbuilder, yes, no)
	AC_PATH_PROG(sdk_path, apkbuilder)
	# Strip app name and tools directory
	sdk_path=`AS_DIRNAME(["$sdk_path"])`
	sdk_path=`AS_DIRNAME(["$sdk_path"])`
	AC_SUBST(ANDROID_SDK_PATH, [$sdk_path])	

   	if test x$have_android_sdk = xno
     	then	   
		AC_MSG_RESULT(Install Android SDK to build Android applications)
	fi

	AC_CHECK_PROG(have_ant, ant, yes, no)

	if test x$have_ant = xno
	then
		AC_MSG_RESULT(Install Ant to build Android applications)
		have_android_sdk=no
	fi
else
	have_android_sdk=no
fi

AM_CONDITIONAL(HAVE_ANDROID_SDK, [test x$have_android_sdk = xyes])
AC_DEFINE(HAVE_ANDROID_SDK, [test x$have_android_sdk = xyes], [Android SDK found])

AC_CHECK_PROG(PYTHONDEV, python2.7-config, yes, no)
AC_CHECK_PROG(PYCFLAGS, python2.7-config, `python2.7-config --cflags`)
AC_CHECK_PROG(PYLDFLAGS, python2.7-config, `python2.7-config --ldflags`)

# If we don't have python-dev, we can't build haggle
if test x$PYTHONDEV != xyes;
then
    AC_MSG_RESULT(Install python-dev to build haggle)
fi

AC_CONFIG_FILES([Makefile
		 src/libhaggle/Doxyfile
		 src/libcpphaggle/Doxyfile
		 src/hagglekernel/Doxyfile
		 extlibs/sqlite-3.7.0.1/Makefile
                 doc/Makefile
                 src/Makefile
                 src/utils/Makefile
                 src/filedrop/Makefile
                 src/libhaggle/Makefile
                 src/libhaggle/jni/Makefile
                 src/libhaggle/jni/native/Makefile
                 src/libhaggle/jni/java/org/haggle/Makefile
                 src/libcpphaggle/Makefile
                 src/experimental/Makefile
                 src/bbs/Makefile
                 src/proxy/Makefile
                 src/clitool/Makefile
                 src/haggletest/Makefile
                 src/haggleobserver/Makefile
                 src/arphelper/Makefile
                 src/repltest/Makefile
                 src/luckyMe/Makefile
                 src/tests/test01/Makefile
                 src/webserver/Makefile
                 src/hagglekernel/Makefile
                 src/apps/filegenerate/Makefile
		 testsuite/Makefile 
		 testsuite/test_thread/Makefile 
		 testsuite/test_mutex/Makefile 
		 testsuite/test_condition/Makefile 
		 testsuite/test_metadata/Makefile
		 testsuite/test_libcpphaggle/Makefile
		 testsuite/test_Queue/Makefile 
		 testsuite/test_utils/Makefile 
		 testsuite/test_dObj/Makefile
		 testsuite/test_kernel/Makefile
		 testsuite/test_security/Makefile
		 testsuite/test_nc/Makefile
		 testsuite/test_fragment/Makefile
		 android/Makefile
		 android/PhotoShare/Makefile
 		 android/PhotoShare/local.properties
		 android/LuckyMe/Makefile
 		 android/LuckyMe/local.properties
		 android/Haggle/Makefile
		 android/Haggle/local.properties
                 $extra_config_files])
AC_OUTPUT

echo
echo $PACKAGE v$VERSION
echo

echo Prefix..................... : $prefix
echo Use C++ exceptions......... : $exceptions
echo Use C++ STL................ : $stl
echo Java bindings.............. : $java_bindings
echo UNIX domain socket for IPC. : $unix_socket
if test x$haggle_os_linux = xtrue
then
	echo D-Bus support.............. : $have_dbus
fi
echo Extractor support.......... : $have_extractor
# echo OMNeT++ support............ : $omnetpp
echo Use bundled Sqlite......... : $bundled_sqlite
echo Generate documentation..... : $docs
echo Benchmarking support....... : $benchmark
echo Debug support.............. : $debug
echo Debug manager.............. : $debugmanager
echo Leak detection............. : $debug_leaks
echo Bluetooth support.......... : $have_bluetooth
echo Ethernet support........... : $have_ethernet
echo IPv6 support............... : $ipv6
echo USB media support.......... : $have_media
echo Filedrop support........... : $enable_filedrop
echo Android applications....... : $have_android_sdk
echo
echo "Type make to compile"
echo "Then su to root and type: make install" if you want to.
echo
if test x$debug = xyes
then
	echo "Note that debugging is enabled. To turn off, pass --disable-debug to"
	echo "the configure script."
	echo
fi
