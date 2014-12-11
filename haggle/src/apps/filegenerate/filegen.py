import haggle
from ctypes import byref, c_int, CFUNCTYPE, POINTER, c_voidp
import sys

import time

def on_dataobject(event,nix):
    print "on_dataobject\n"
    return 0
ONDATAOBJECTFUNC = CFUNCTYPE(c_int, POINTER(haggle.struct_event), c_voidp)
on_dataobject_func = ONDATAOBJECTFUNC(on_dataobject)


def isHaggleRunning():
    pid = c_int()
    retcode = haggle.haggle_daemon_pid(byref(pid));
    return haggle.HAGGLE_ERROR != retcode

def registerEvents(hagglehandle):
    retcode = haggle.haggle_ipc_register_event_interest(hagglehandle,
            haggle.LIBHAGGLE_EVENT_NEW_DATAOBJECT, on_dataobject_func);
    if haggle.HAGGLE_NO_ERROR != retcode:
        print "error registering ondataobject"
        sys.exit(-1)

    
def publishDataObjectFromString(hagglehandle, content,fileowner,count):
    print "creating and publishing dataobject"
    print "creating dataobject"
    dataobject = haggle.haggle_dataobject_new_from_buffer(content,content.__len__())
    print "adding attributes. created dataobject=",dataobject
    haggle.haggle_dataobject_add_attribute(dataobject, "FileOwner", fileowner)
    haggle.haggle_dataobject_add_attribute(dataobject, "Count", str(count))
    haggle.haggle_dataobject_set_flags(dataobject, haggle.DATAOBJECT_FLAG_PERSISTENT)
    haggle.haggle_ipc_publish_dataobject(hagglehandle, dataobject)


def main():
    if not isHaggleRunning():
        print "haggle is not running!"
        sys.exit(-1)
    
    print sys.argv[1], sys.argv[2],sys.argv[3]
    fileowner = sys.argv[1]
    interest = sys.argv[2]
    loopcounter = int(sys.argv[3])
    
    hagglehandle_ptr = haggle.haggle_handle_t()
    retcode = haggle.haggle_handle_get("a17",byref(hagglehandle_ptr))
    if retcode < 0:
        print "unable to get haggle handle retcode=",retcode
        sys.exit(-1)
        
    print on_dataobject_func
    registerEvents(hagglehandle_ptr)
    
    
    retcode = haggle.haggle_ipc_add_application_interest(hagglehandle_ptr, "FileOwner", interest);
    
    for i in range(loopcounter):
        print i
        content = "testmessage|"+fileowner+"||"+str(i)+"|"
        publishDataObjectFromString(hagglehandle_ptr,content,fileowner,i)

    while(True):
        time.sleep(5)

if __name__ == "__main__":
    main()
