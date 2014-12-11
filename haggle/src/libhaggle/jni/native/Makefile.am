lib_LTLIBRARIES = libhaggle_jni.la
libhaggle_jni_la_SOURCES = javaclass.c org_haggle_Handle.c org_haggle_DataObject.c org_haggle_Attribute.c org_haggle_Node.c org_haggle_Interface.c common.c

EXTRA_DIST = \
	common.h \
	javaclass.h \
	org_haggle_Attribute.h \
	org_haggle_DataObject.h \
	org_haggle_DataObject_DataObjectException.h \
	org_haggle_EventHandler.h \
	org_haggle_Handle.h \
	org_haggle_Handle_RegistrationFailedException.h \
	org_haggle_Interface.h \
	org_haggle_LaunchCallback.h \
	org_haggle_Node.h \
	org_haggle_Node_NodeException.h \
	Android.mk

libhaggle_jni_la_CFLAGS =
libhaggle_jni_la_LDFLAGS = -lhaggle -L$(top_builddir)/src/libhaggle/

# TODO: Should somehow detect the java include path
libhaggle_jni_la_CPPFLAGS =-I$(top_srcdir)/src/libhaggle/include $(JAVA_INCLUDE)

if OS_IPHONEOS
libhaggle_jni_la_LDFLAGS +=-Wl,-dylib_install_name libhaggle_jni.dylib
endif

clean-local:
	rm -f *~
