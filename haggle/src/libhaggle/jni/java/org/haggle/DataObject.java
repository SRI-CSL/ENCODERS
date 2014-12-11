package org.haggle;

import org.haggle.Attribute;
import java.util.Date;

public class DataObject {
        public long nativeDataObject = 0; // Pointer to native data object struct
        private boolean disposed = false;
        private native boolean newEmpty();
        private native boolean newFromFile(String filepath);
        private native boolean newFromBuffer(byte[] data);
        private native void nativeFree();
        
	public native boolean addAttribute(String name, String value, long weight);
	public boolean addAttribute(String name, String value) {
		return addAttribute(name, value, 1);
	}
	public boolean addAttribute(Attribute attr) {
		return addAttribute(attr.getName(), attr.getValue(), attr.getWeight());
	}
        public native Attribute getAttribute(String name, int n);
        public native Attribute getAttribute(String name, String value);
        public native long getNumAttributes();
        public native Attribute[] getAttributes();
        public native String getFilePath();                      
        public native String getFileName();
        public native int addFileHash();
	public int setCreateTime(Date time) {
		long secs = time.getTime() / 1000;
		return setCreateTime(secs, time.getTime() - (secs * 1000)); 
	}
	public native int setCreateTime(long sec, long usec);
	public native int setCreateTime();
        public native int setThumbnail(byte[] data);
        public native long getThumbnailSize();
        public native long getThumbnail(byte[] data);
        public native byte[] getRaw();

	public String toString() {
		return new String(getRaw());
	}
        public DataObject() throws DataObjectException
        {
                if (!newEmpty())
                        throw new DataObjectException("Could not create empty data object");
        }
        public DataObject(String filepath) throws DataObjectException
        {
                if (!newFromFile(filepath))
                        throw new DataObjectException("Could not create data object from file " + filepath);

        }
        public DataObject(byte[] data) throws DataObjectException
        {
                if (!newFromBuffer(data))
                        throw new DataObjectException("Could not create data object from byte data");
        } 
        public synchronized void dispose()
        {
                if (disposed == false) {
                        disposed = true;
                        nativeFree();
                }
        }
        protected void finalize() throws Throwable
        {
                dispose();
                super.finalize();
        }
        public class DataObjectException extends Exception {
                DataObjectException(String msg)
                {
                        super(msg);
                }
        }
        static {		
                System.loadLibrary("haggle_jni");
        }
}
