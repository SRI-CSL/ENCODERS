from ctypes import byref, c_int, CFUNCTYPE, POINTER, c_voidp, c_void_p
import sys
import time
import shutil
import logging

from haggleappmethods import *

sys.path.append("/usr/local/sbin")
import haggle

logger = logging.getLogger('haggleapp')
logger.setLevel(logging.DEBUG)

fh = logging.FileHandler('imagepublish.log')
fh.setLevel(logging.DEBUG)

ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)

formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
logger.addHandler(fh)
logger.addHandler(ch)
        
def publishImage(hagglehandle, imagepath,fileowner,count):
    logger.info("creating and publishing dataobject")
    logger.info("creating dataobject")
    dataobject = haggle.haggle_dataobject_new_from_file(imagepath)
    #logger.info("adding attributes. created dataobject=",dataobject)
    haggle.haggle_dataobject_add_attribute(dataobject, "FileOwner", fileowner)
    haggle.haggle_dataobject_add_attribute(dataobject, "Count", str(count))
    logger.info("publilshing")
    haggle.haggle_ipc_publish_dataobject(hagglehandle, dataobject)        


        
def main():
    if not isHaggleRunning():
        logger.error("haggle is not running!")
        sys.exit(-1)
    
    print sys.argv[1], sys.argv[2], sys.argv[3]
    fileowner = sys.argv[1]
    imagepath = sys.argv[2]
    forever = sys.argv[3]
    
    hagglehandle_ptr = haggle.haggle_handle_t()
    retcode = haggle.haggle_handle_get("imagepublisher",byref(hagglehandle_ptr))
    if retcode < 0:
        print "unable to get haggle handle retcode=",retcode
        sys.exit(-1)

    hagglesessionid = haggle.haggle_handle_get_session_id(hagglehandle_ptr);
    print "haggle sessionid=",hagglesessionid     
        
        
    print on_dataobject_func
    registerEvents(hagglehandle_ptr)
   
    cnt = 0
    while True:
        cnt = cnt + 1
        #dataobject api does not set stored. so copy the file
        destinationFile = imagepath+".copy"
        shutil.copyfile(imagepath,destinationFile)
        publishImage(hagglehandle_ptr,imagepath,fileowner,cnt)
        time.sleep(10)
        if "true" != forever.lower():
            break 

    while(True):
        time.sleep(5)

if __name__ == "__main__":
    main()        