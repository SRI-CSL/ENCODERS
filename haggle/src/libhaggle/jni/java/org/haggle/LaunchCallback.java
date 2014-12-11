package org.haggle;

public abstract class LaunchCallback {
        // This method is called continuously during the launching
        // of the Haggle daemon to give progress feedback to the 
        // application. Milliseconds is the the number of milliseconds
        // that has passed since the launching started. If milliseconds
        // is 0, the launch has completed.
        // If the application returns 1 from this function, the
        // feedback is abandonded.
        public abstract int callback(long milliseconds);
}
