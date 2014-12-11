# Copyright (c) 2014 SRI International
# Developed under DARPA contract N66001-11-C-4022.
# Authors:
#   Joshua Joy (JJ, jjoy)

import testcallback


from ctypes import c_void_p, CFUNCTYPE, c_int

testcallback.myprint()

def on_callback(arg1):
    print "callback method arg=",arg1
    return 0

ONCALLBACKFUNC = CFUNCTYPE(c_int,c_int)
on_callback_func = ONCALLBACKFUNC(on_callback)
print on_callback_func

testcallback.runcallback(on_callback_func)