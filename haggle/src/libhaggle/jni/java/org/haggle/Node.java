package org.haggle;

public class Node {
        private long nativeNode = 0;
        private boolean disposed = false;
        private native void nativeFree();
        public native String getName();
        public native long getNumInterfaces();
        // Return interface number N, or null. TODO: throw exception
        public native Interface getInterfaceN(int n);
	public native Interface[] getInterfaces();
	
        private Node()
        {
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
        public class NodeException extends Exception {
                NodeException(String msg)
                {
                        super(msg);
                }
        }
        static {		
                System.loadLibrary("haggle_jni");
        }
}
