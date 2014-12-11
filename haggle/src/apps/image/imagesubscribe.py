from ctypes import byref, c_int, CFUNCTYPE, POINTER, c_voidp
import sys
import time

from haggleappmethods import *

sys.path.append("/usr/local/sbin")
import haggle

logger = logging.getLogger('haggleapp')
logger.setLevel(logging.DEBUG)

fh = logging.FileHandler('imagesubscribe.log')
fh.setLevel(logging.DEBUG)

ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)

formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
logger.addHandler(fh)
logger.addHandler(ch)
           
def main():
    if not isHaggleRunning():
        print "haggle is not running!"
        sys.exit(-1)
    
    print sys.argv[1]
    interest = sys.argv[1]    
       
    
    hagglehandle_ptr = haggle.haggle_handle_t()
    retcode = haggle.haggle_handle_get("imagesubscribe1",byref(hagglehandle_ptr))
    if retcode < 0:
        print "unable to get haggle handle retcode=",retcode
        sys.exit(-1)
        
    print on_dataobject_func
    registerEvents(hagglehandle_ptr)
   
    retcode = haggle.haggle_ipc_add_application_interest(hagglehandle_ptr, "FileOwner", interest);

    while(True):
        time.sleep(5)

if __name__ == "__main__":
    main()        
