from ctypes import byref, c_int, CFUNCTYPE, POINTER, c_voidp, c_void_p
import sys
import time
import shutil
import logging

sys.path.append("/usr/local/sbin")
import haggle

logger = logging.getLogger('haggleapp')
logger.setLevel(logging.DEBUG)

def on_dataobject(event,nix):
    logger.info("on_dataobject\n")
    return 0
ONDATAOBJECTFUNC = CFUNCTYPE(c_int, POINTER(haggle.struct_event), c_void_p)
on_dataobject_func = ONDATAOBJECTFUNC(on_dataobject)

def on_neighbor_update(event,nix):
    logger.info("on neighbor update\n")
    return 0
ONNEIGHBORUPDATEFUNC = CFUNCTYPE(c_int, POINTER(haggle.struct_event), c_voidp)
on_neighbor_update_func = ONNEIGHBORUPDATEFUNC(on_neighbor_update)

def isHaggleRunning():
    pid = c_int()
    retcode = haggle.haggle_daemon_pid(byref(pid));
    return haggle.HAGGLE_ERROR != retcode

def registerEvents(hagglehandle):
    logger.info("registering events\n")
    retcode = haggle.haggle_ipc_register_event_interest(hagglehandle,
            haggle.LIBHAGGLE_EVENT_NEW_DATAOBJECT, on_dataobject_func);
    print "registered ondataobject retcode=",retcode
    if haggle.HAGGLE_NO_ERROR != retcode:
        print "error registering ondataobject\n"
        sys.exit(-1)
    retcode = haggle.haggle_ipc_register_event_interest(hagglehandle,
            haggle.LIBHAGGLE_EVENT_NEIGHBOR_UPDATE, on_neighbor_update_func);
    print "registered on_neighbor_update retcode=",retcode
    if haggle.HAGGLE_NO_ERROR != retcode:
        print "error registering ondataobject\n"
        sys.exit(-1)