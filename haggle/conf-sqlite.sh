# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Mark-Oliver Stehr (MOS)

sed "s/-DSQLITE_THREADSAFE=1/-DSQLITE_THREADSAFE=0/g" android/extlibs/external/sqlite/dist/Android.mk > /tmp/Android.mk
mv /tmp/Android.mk android/extlibs/external/sqlite/dist/Android.mk
