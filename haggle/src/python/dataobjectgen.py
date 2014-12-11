import haggle
import sys

def on_shutdown():
    print "shutting down"

def register4events(hagglehandle):
    print "registering haggle event handlers"
    retcode = haggle.haggle_ipc_register_event_interest(hagglehandle,
            haggle.LIBHAGGLE_EVENT_SHUTDOWN, on_shutdown);

def main():
    hagglehandle = haggle.getHaggleHandle("test2")
    print hagglehandle
    if hagglehandle is None:
        print "error haggle is not running"
        sys.exit(-1)
    print "retrieved haggle handle"
    #dataobject = haggle.haggle_dataobject_new_from_file("build.sh")
    #print dataobject
    #print retcode
    register4events(hagglehandle)
        
if __name__ == "__main__":
    main()    




#
#dump(dataobject)
