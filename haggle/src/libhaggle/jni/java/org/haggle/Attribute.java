package org.haggle;

public class Attribute {
        private long nativeAttribute = 0;
        private boolean disposed = false;
        private boolean isPartOfList = false;
        private native boolean nativeNew(String name, String value, long weight);
        private native void nativeFree();

        public native String getName();
        public native String getValue();
        public native long getWeight();

        public Attribute(String name, String value, long weight)
        {
                nativeNew(name, value, weight);
        }
        public Attribute(String name, String value)
        {
                nativeNew(name, value, 1);
        }
        public Attribute(Attribute a)
        {
                nativeNew(a.getName(), a.getValue(), a.getWeight());
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
        public String toString()
        {
                return new String(getName() + "=" + getValue() + ":" + getWeight());
        }
        
        public class AttributeException extends Exception {
                AttributeException(String msg)
                {
                        super(msg);
                }
        }
        static {		
                System.loadLibrary("haggle_jni");
        }
}
