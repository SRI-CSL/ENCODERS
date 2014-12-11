# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Joshua Joy (JJ, jjoy)

rm *.so
gcc -shared -Wl,-soname,libtestcallback -o libtestcallback.so -fPIC testcallback.c
