#!/bin/bash -e

./autogen.sh
export CPPFLAGS="-DU_HAVE_STDINT_H=1 -DU_EXPORT= -DU_EXPORT2= -DU_IMPORT= -DU_CALLCONV="
./configure --enable-gcov
make
make test_thread
