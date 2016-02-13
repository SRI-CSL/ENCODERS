/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Hasnain Lakhani (HL)
 */

package org.haggle.kernel;

import android.app.Service;
import android.app.NotificationManager;
import android.app.Notification;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Binder;
import android.os.IBinder;
import android.os.Message;
import android.os.Handler;
import android.os.Messenger;
import android.os.RemoteException;
import android.util.Log;
import java.util.ArrayList;

// CBMEN, HL, Begin
import android.os.Environment;
import android.content.ContextWrapper;
import org.haggle.kernel.Utils;
import java.io.File;
import android.os.AsyncTask;
import java.lang.Void;
import java.nio.channels.FileChannel;
import java.io.FileInputStream;
import java.io.FileOutputStream;
// CBMEN, HL, End

/* 
   Haggle service can be started from command line with: 
   $ am startservice -a android.intent.action.MAIN -n org.haggle.kernel/org.haggle.kernel.Haggle
*/
public class Haggle extends Service {

// CBMEN, HL, Begin
	private class UnzipTask extends AsyncTask<Haggle, Void, Void> {
		protected Void doInBackground(Haggle... theHaggles) {
			// For internal storage
			File pythonDir = new File(new ContextWrapper(theHaggles[0]).getApplicationInfo().dataDir, "python");
			// For external storage
			// File pythonDir = new File(new ContextWrapper(theHaggles[0]).getApplicationInfo().getExternalFilesDir(null), "");

			// The following paths should match those in src/hagglekernel/SecurityManager.h
			File pythonUnzipComplete = new File("/data/data/org.haggle.kernel/files/python_unzip_complete");
			File pythonUnzipInProgress = new File("/data/data/org.haggle.kernel/files/python_unzip_in_progress");

			if (!pythonUnzipComplete.exists()) {
				// We need to copy out Python

				// Just in case
				File filesDir = new File("/data/data/org.haggle.kernel/files");
				if (!(filesDir.exists() && filesDir.isDirectory())) {
					filesDir.mkdirs();
				}

				try {
					if (pythonUnzipInProgress.exists()) {
						Log.i("Haggle", "Python unzip in progress file already exists, not creating");
					} else {
						if (pythonUnzipInProgress.createNewFile())
							Log.i("Haggle", "Created Python unzip in progress file");
						else
							Log.e("Haggle", "Couldn't create Python unzip in progress file");
							// This isn't fatal - we can still continue and try to unzip.
					}
				} catch (Exception e) {
					Log.e("Haggle", "Couldn't create Python unzip in progress file", e);
					// This isn't fatal - we can still continue and try to unzip.
				}

				try {
					Log.i("Haggle", "Unzipping Python inside UnzipTask");
					// NOTE: Adjust path here too if changing path at start of method
					if (Utils.unzip(getAssets().open("python_27.zip"), new ContextWrapper(theHaggles[0]).getApplicationInfo().dataDir + "/", false)) {
						Log.i("Haggle", "Successfully unzipped python_27.zip");
					} else {
						Log.e("Haggle", "Error while unzipping python_27.zip");
					}
				} catch (Exception e) {
					Log.e("Haggle", "IOException while trying to open python_27.zip", e);
					return null;
				}

				try {
					if (pythonUnzipInProgress.exists()) {
						if (pythonUnzipInProgress.delete())
							Log.i("Haggle", "Removed Python unzip in progress file");
						else
							Log.e("Haggle", "Couldn't remove Python unzip in progress file");
							// Again, not fatal, as long as the later file creation succeeds
					}
				} catch (Exception e) {
					Log.e("Haggle", "Couldn't remove Python unzip in progress file", e);
					// Again, not fatal, as long as the later file creation succeeds
				}

				try {
					if (pythonUnzipComplete.createNewFile())
						Log.i("Haggle", "Created Python unzip completed file");
					else
						Log.e("Haggle", "Couldn't create Python unzip completed file");
				} catch (Exception e) {
					Log.e("Haggle", "Couldn't remove Python unzip completed file", e);
					// This is fatal - but we just log it and hope user restarts haggle if they need crypto
				}
			}

			return null;
		}
	}

	static {
		System.load("/data/data/org.haggle.kernel/lib/libz.so");
		System.load("/data/data/org.haggle.kernel/lib/libsqlite3.so");
		System.load("/data/data/org.haggle.kernel/lib/libgmp.so");
		System.load("/data/data/org.haggle.kernel/lib/libpbc.so");
		System.load("/data/data/org.haggle.kernel/lib/libpython2.7.so");
// CBMEN, HL, End
		System.loadLibrary("hagglekernel_jni");
		nativeInit();
	}
	private native static int nativeInit();
	
	/** For showing and hiding our notification. */
	NotificationManager mNM;

	/** Keeps track of all current registered clients. */
	ArrayList<Messenger> mClients = new ArrayList<Messenger>();
	/** Holds last value set by a client. */
	int mValue = 0;
    
	Thread mHaggleThread = null;
	/**
	 * Command to the service to register a client, receiving callbacks
	 * from the service.  The Message's replyTo field must be a Messenger of
	 * the client where callbacks should be sent.
	 */
	static final int MSG_REGISTER_CLIENT = 1;

	/**
	 * Command to the service to unregister a client, ot stop receiving callbacks
	 * from the service.  The Message's replyTo field must be a Messenger of
	 * the client as previously given with MSG_REGISTER_CLIENT.
	 */
	static final int MSG_UNREGISTER_CLIENT = 2;

	/**
	 * Command to service to set a new value.  This can be sent to the
	 * service to supply a new value, and will be sent by the service to
	 * any registered clients with the new value.
	 */
	static final int MSG_SET_VALUE = 3;

	/**
	 * Handler of incoming messages from clients.
	 */
	class IncomingHandler extends Handler {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case MSG_REGISTER_CLIENT:
				mClients.add(msg.replyTo);
				break;
			case MSG_UNREGISTER_CLIENT:
				mClients.remove(msg.replyTo);
				break;
			case MSG_SET_VALUE:
				mValue = msg.arg1;
				for (int i = mClients.size()-1; i>=0; i--) {
					try {
						mClients.get(i).send(Message.obtain(null,
										    MSG_SET_VALUE, mValue, 0));
					} catch (RemoteException e) {
						// The client is dead.  Remove it from the list;
						// we are going through the list from back to front
						// so this is safe to do inside the loop.
						mClients.remove(i);
					}
				}
				break;
			default:
				super.handleMessage(msg);
			}
		}
	}

	/**
	 * Target we publish for clients to send messages to IncomingHandler.
	 */
	final Messenger mMessenger = new Messenger(new IncomingHandler());

	native int mainLoop(String fileDirPath);
	public native int shutdown();
	private boolean isRunning = false;
	private int startId = -1;
	// CBMEN, HL - Not using Bluetooth anymore
	// private final BluetoothConnectivity mBluetoothConnectivity = 
	// 		new BluetoothConnectivity(this);
	
	private class HaggleMainLoop implements Runnable {
		@Override
		public void run() {
			Log.i("Haggle", "Haggle running");
			isRunning = true;
			int ret = mainLoop(getFilesDir().getAbsolutePath());
			Log.i("Haggle", "Haggle exits with value " + ret);
			isRunning = false;
			stopSelfResult(startId);
		}
	}
	@Override
	public void onCreate() {
		mNM = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);
		
		// Display a notification about us starting.
		showNotification();

		// CBMEN, HL - Launch AsyncTask to unzip if needed
		new UnzipTask().execute(this);
				
		// CBMEN, HL - not using Bluetooth anymore
		// mBluetoothConnectivity.initialize();
		
		if (!isRunning) {
			Log.i("Haggle", "Starting Haggle");
			mHaggleThread = new Thread(new HaggleMainLoop());
			mHaggleThread.start();
		}
	}

	@Override
	public void onLowMemory() {
		Log.i("Haggle", "Low memory! Shutting down Haggle");
		shutdown();
	}

	@Override
	public int onStartCommand(Intent intent, int startFlags, int startId) {
		this.startId = startId;
		return START_STICKY;
	}
	
	@Override
	public void onDestroy() {
		// Cancel the persistent notification.
		mNM.cancel(R.string.remote_service_started);

		// CBMEN, HL - not using Bluetooth anymore
		// mBluetoothConnectivity.finalize();
		
		// Tell the user we stopped.
		//Toast.makeText(this, R.string.remote_service_stopped, Toast.LENGTH_SHORT).show();
		if (isRunning) {
			shutdown();
		}

		try {
			mHaggleThread.join();
		} catch (InterruptedException e) {
		    
		}
		Log.i("Haggle", "Joined with Haggle main thread");
	}
	
	public class HaggleBinder extends Binder {
		Haggle getHaggle() {
			return Haggle.this;
		}
	}
	private final HaggleBinder mBinder = new HaggleBinder();
	
	/**
	 * When binding to the service, we return an interface to our messenger
	 * for sending messages to the service.
	 */
	@Override
	public IBinder onBind(Intent intent) {
		//return mMessenger.getBinder();
		return mBinder;
	}

	/**
	 * Show a notification while this service is running.
	 */
	private void showNotification() {
		CharSequence text = getText(R.string.remote_service_started);
		
		// Set the icon, scrolling text and timestamp
		Notification notification = new Notification(R.drawable.ic_stat_notify_haggle, text,
							     System.currentTimeMillis());
		
		// The PendingIntent to launch our StatusView activity if the
		// user selects this notification
		Intent status = new Intent(this, StatusView.class);
		PendingIntent contentIntent = PendingIntent.getActivity(this, 0, status, 0);
		
		// Set the info for the views that show in the
		// notification panel.
		notification.setLatestEventInfo(this, getText(R.string.remote_service_label),
						text, contentIntent);
		
		mNM.notify(R.string.remote_service_started, notification);
	}
}
