package org.haggle;
import org.haggle.LaunchCallback;

public class Handle {
        private long nativeHandle = 0; // pointer to Haggle handle C struct
        private boolean disposed = false;
        private String name;
        	
        private native int getHandle(String name);
        private native void nativeFree(); // Must be called when handle is not used any more
	
	private static native void setDataPath(String path);
        public static native void unregister(String name);
	public void unregister() { unregister(name); }

        public native int getSessionId();
        public native int shutdown();
	public native int registerEventInterest(int type, EventHandler handler);
	public native int publishDataObject(DataObject dObj);
        public native int registerInterest(Attribute attr);
        public native int registerInterests(Attribute[] attrs);
        public native int unregisterInterest(Attribute attr);
        public native int unregisterInterests(Attribute[] attrs);
        public native int getApplicationInterestsAsync();
        public native int getDataObjectsAsync();
        public native int deleteDataObjectById(char[] id, boolean keepInBloomfilter);
        public native int deleteDataObject(DataObject dObj, boolean keepInBloomfilter);
        public native int deleteDataObjectById(char[] id);
	public native int deleteDataObject(DataObject dObj);
	public native int sendNodeDescription();

        // Should probably make the eventLoop functions throw some exceptions
	public native boolean eventLoopRunAsync(EventHandler handler); 
	public native boolean eventLoopRunAsync(); 
	public native boolean eventLoopRun(EventHandler handler);
	public native boolean eventLoopRun();
	public native boolean eventLoopStop();
        public native boolean eventLoopIsRunning();

        public static final int HAGGLE_ERROR = -100;
        public static final int HAGGLE_BUSY_ERROR = -96;
        public static final int HAGGLE_NO_ERROR = 0;

        // Useful for launcing Haggle from an application
        public static final int HAGGLE_DAEMON_ERROR = HAGGLE_ERROR;
        public static final int HAGGLE_DAEMON_NOT_RUNNING = HAGGLE_NO_ERROR;
        public static final int HAGGLE_DAEMON_RUNNING = 1;
        public static final int HAGGLE_DAEMON_CRASHED = 2;
        
        public static native long getDaemonPid();
        public static native int getDaemonStatus();
        public static native boolean spawnDaemon();
        public static native boolean spawnDaemon(String path);
        public static native boolean spawnDaemon(LaunchCallback c);
        public static native boolean spawnDaemon(String path, LaunchCallback c);
        
	// Non-native methods follow here
	public Handle(String name) throws RegistrationFailedException, AlreadyRegisteredException
        {
		int ret = getHandle(name);
		
		switch (ret) {
			case HAGGLE_NO_ERROR:
				break;
			case HAGGLE_BUSY_ERROR:
				throw new AlreadyRegisteredException("Already registered", ret);
			case HAGGLE_ERROR:
			default:
				throw new RegistrationFailedException("Registration failed with value " + ret, ret);
		}
                this.name = name;
        }
        public class RegistrationFailedException extends Exception {
		private int err;
		
                RegistrationFailedException(String msg, int err)
                {
                        super(msg);
			this.err = err;
                }
		public int getError() 
		{
			return err;
		}
        }
	public class AlreadyRegisteredException extends RegistrationFailedException {
                AlreadyRegisteredException(String msg, int err)
                {
                        super(msg, err);
                }
        }
        public synchronized void dispose()
        {
                if (!disposed) {
                        disposed = true;
                        nativeFree();
                }
        }
        protected void finalize() throws Throwable
        {
                dispose();
                super.finalize();
        }
        static {
                System.loadLibrary("haggle_jni");
        }
}
