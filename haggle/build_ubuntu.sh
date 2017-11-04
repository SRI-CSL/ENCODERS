#!/bin/bash -e

./autogen.sh
export CPPFLAGS="-DU_HAVE_STDINT_H=1 -DU_EXPORT= -DU_EXPORT2= -DU_IMPORT= -DU_CALLCONV="
./configure --enable-gcov
make clean
make
make test_thread
sudo make install

pushd src/libhaggle
make
sudo make install
popd

pushd src/libhaggle/jni
#include is looking at /Headers, not sure where to pass as variable so linking for now
sudo ln -s /usr/lib/jvm/java-8-openjdk-amd64/include /Headers
make
sudo make install
./dependencies.sh
export PATH=${HOME}/installs/apache-maven-3.5.2/bin:$PATH
mvn clean install
popd

#arphelper
pushd src/arphelper
make clean
make
sudo make install
popd
