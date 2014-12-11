# The libxml2 library bundled with Android is very stripped down.
# For example, it doesn't have the xpath or tree modules we use,
# and therefore we statically link in the object files from our
# own libxml2.

LOCAL_PATH := $(my-dir)
include $(CLEAR_VARS)

# Do not build for simulator
ifneq ($(TARGET_SIMULATOR),true)

LIBXML_SOURCE_FILES := \
	SAX.c \
	entities.c \
	encoding.c \
	error.c \
	parserInternals.c \
	parser.c \
	tree.c \
	hash.c \
	list.c \
	xmlIO.c \
	xmlmemory.c \
	uri.c \
	valid.c \
	xlink.c \
	HTMLparser.c \
	HTMLtree.c \
	debugXML.c \
	xpath.c \
	xpointer.c \
	xinclude.c \
	nanohttp.c \
	nanoftp.c \
	DOCBparser.c \
	catalog.c \
	globals.c \
	threads.c \
	c14n.c \
	xmlstring.c \
	xmlregexp.c \
	xmlschemas.c \
	xmlschemastypes.c \
	xmlunicode.c \
	xmlreader.c \
	relaxng.c \
	dict.c \
	SAX2.c \
	legacy.c \
	chvalid.c \
	pattern.c \
	xmlsave.c \
	xmlmodule.c \
	xmlwriter.c \
	schematron.c

#CONFIG_H := $(LOCAL_PATH)/config.h

#$(CONFIG_H):
#	rm -f $(LOCAL_PATH)/config.h
#	ln -s $(LOCAL_PATH)/config-android.h $(LOCAL_PATH)/config.h

LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

#LOCAL_GENERATED_SOURCES := $(CONFIG_H)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := $(LIBXML_SOURCE_FILES)
LOCAL_MODULE := libhaggle-xml2

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)

endif 
