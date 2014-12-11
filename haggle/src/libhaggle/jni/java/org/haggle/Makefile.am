# Do not install compiled class files. We instead generate and install
# a jar file with the class files in a subdirectory
noinst_JAVA = Handle.java EventHandler.java Node.java Attribute.java DataObject.java Interface.java LaunchCallback.java

EXTRA_DIST=

if DEBUG
noinst_JAVA += TestApp.java
else
EXTRA_DIST+=TestApp.java
endif

EXTRA_DIST+= $(noinst_JAVA)

JAVAROOT=$(top_builddir)/src/libhaggle/jni/java

AM_JAVACFLAGS=

clean-local:
	rm -f *~
	rm -f *.class
	rm -f *.stamp
