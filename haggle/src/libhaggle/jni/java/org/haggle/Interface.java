package org.haggle;

public class Interface {
        private long nativeInterface = 0;
        private boolean disposed = false;

        private native void nativeFree();

        // Types - should be synced with the ones in the libhaggle
        // native code (interface.h).
        public final static int IFTYPE_UNDEF = 0;
	public final static int IFTYPE_APPLICATION_PORT = 1;
	public final static int IFTYPE_APPLICATION_LOCAL = 2;
	public final static int IFTYPE_ETHERNET = 3;
	public final static int IFTYPE_WIFI = 4;
	public final static int IFTYPE_BLUETOOTH = 5;
	public final static int IFTYPE_MEDIA = 6;
	public final static int _IFTYPE_MAX = 7;

	public final static int IFSTATUS_UNDEF = 0;
	public final static int IFSTATUS_UP = 1;
	public final static int IFSTATUS_DOWN = 2;
	public final static int _IFSTATUS_MAX = 3;
	
        public native int getType();
	public native int getStatus();
        public native String getName();
        public native String getIdentifierString();

        private Interface()
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
        public class InterfaceException extends Exception {
                InterfaceException(String msg)
                {
                        super(msg);
                }
        }
	public String getTypeString() {
		switch (getType()) {
			case IFTYPE_UNDEF:
				return "Undef";
			case IFTYPE_APPLICATION_PORT:
				return "Application:port";
			case IFTYPE_APPLICATION_LOCAL:
				return "Application:local";
			case IFTYPE_BLUETOOTH:
				return "Bluetooth";
			case IFTYPE_ETHERNET:
				return "Ethernet";
			case IFTYPE_WIFI:
				return "WiFi";
			case IFTYPE_MEDIA:
				return "Media";
			default:
				break;
		}
		return "Unknown";
	}
	public String getStatusString() {
		switch (getStatus()) {
			case IFSTATUS_UP:
				return "Up";
			case IFSTATUS_DOWN:
				return "Down";
			default:
				break;
		}
		return "Unknown";
	}
        static {		
                System.loadLibrary("haggle_jni");
        }
}
