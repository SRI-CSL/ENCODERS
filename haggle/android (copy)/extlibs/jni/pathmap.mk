#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# A central place to define mappings to paths, to avoid hard-coding
# them in Android.mk files.
#
# TODO: Allow each project to define stuff like this before the per-module
#       Android.mk files are included, so we don't need to have a big central
#       list.
#

#
# A mapping from shorthand names to include directories.
#
pathmap_INCL := \
    bluez:external/bluetooth/bluez \
    glib:external/bluetooth/glib \
    sqlite:external/sqlite \
    dbus:external/dbus \
    system-core:external/core/include

#
# Returns the path to the requested module's include directory,
# relative to the root of the source tree.  Does not handle external
# modules.
#
# $(1): a list of modules (or other named entities) to find the includes for
#
define include-path-for
$(foreach n,$(1),$(patsubst $(n):%,%,$(filter $(n):%,$(pathmap_INCL))))
endef

#
# Many modules expect to be able to say "#include <jni.h>",
# so make it easy for them to find the correct path.
#
JNI_H_INCLUDE := $(call include-path-for,libnativehelper)/nativehelper

#
# A list of all source roots under frameworks/base, which will be
# built into the android.jar.
#
# Note - "common" is included here, even though it is also built
# into a static library (android-common) for unbundled use.  This
# is so common and the other framework libraries can have mutual
# interdependencies.
#
FRAMEWORKS_BASE_SUBDIRS := \
	$(addsuffix /java, \
	    core \
	    graphics \
	    location \
	    media \
	    opengl \
	    sax \
	    telephony \
	    wifi \
	    vpn \
	    keystore \
	 )

#
# A version of FRAMEWORKS_BASE_SUBDIRS that is expanded to full paths from
# the root of the tree.  This currently needs to be here so that other libraries
# and apps can find the .aidl files in the framework, though we should really
# figure out a better way to do this.
#
FRAMEWORKS_BASE_JAVA_SRC_DIRS := \
	$(addprefix frameworks/base/,$(FRAMEWORKS_BASE_SUBDIRS))
