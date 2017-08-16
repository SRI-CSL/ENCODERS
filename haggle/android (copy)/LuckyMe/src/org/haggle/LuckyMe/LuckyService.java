package org.haggle.LuckyMe;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Random;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Binder;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.PowerManager;
import android.os.RemoteException;
import android.provider.Settings.Secure;
import android.telephony.TelephonyManager;
import android.util.Log;

import org.haggle.Attribute;
import org.haggle.DataObject;
import org.haggle.EventHandler;
import org.haggle.Handle;
import org.haggle.LaunchCallback;
import org.haggle.Node;
import org.haggle.DataObject.DataObjectException;
import org.haggle.Handle.AlreadyRegisteredException;
import org.haggle.Handle.RegistrationFailedException;

public class LuckyService extends Service implements EventHandler {
	private Thread mDataThread = null;
	//private DataObjectGenerator mDataGenerator = null;
	//private ExperimentSetupReader mExpReader = null;
	private org.haggle.Handle hh = null;
	private boolean haggleIsRunning = false;
	private int num_dataobjects_rx = 0;
	private int num_dataobjects_tx = 0;
	public final String LUCKY_SERVICE_TAG = "LuckyService";
	public final String LUCKYME_APPNAME = "LuckyMe";
	public boolean isRunning = false;
	private int attributePoolSize = 100;
	private int numDataObjectAttributes = 4;
	private int numInterestAttributes = 10;
	private Random prng = new Random();
	private Zipf zipf = new Zipf(attributePoolSize, 0.8);
	private boolean ignoreShutdown = false;
	private Messenger mClientMessenger = null;
	public static final int MSG_NEIGHBOR_UPDATE = 0;
	public static final int MSG_NUM_DATAOBJECTS_TX = 1;
	public static final int MSG_NUM_DATAOBJECTS_RX = 2;
	public static final int MSG_LUCKY_SERVICE_START = 3;
	public static final int MSG_LUCKY_SERVICE_STOP = 4;
	private Node[] neighbors = null;
	private String androidId;
	private long nodeId = 0;
	private Attribute[] interests = null;
	private HashMap<Long, Long> myLuck = new HashMap<Long, Long>();
	private long cumulativeLuck = 0;
	private String deviceId;
	private WakeLockHandler mWakeLockHandler;

	@Override
	public void onCreate() {
		super.onCreate();

		Log.i(LUCKY_SERVICE_TAG, "LuckyService created");
		//mDataGenerator = new DataObjectGenerator();
		//mDataThread = new Thread(mDataGenerator);
		mWakeLockHandler = new WakeLockHandler();
		mDataThread = new Thread(mWakeLockHandler);
		//mExpReader = new ExperimentSetupReader();
		//mDataThread = new Thread(mExpReader);
		androidId = Secure.getString(getContentResolver(), Secure.ANDROID_ID);
                
		Log.i(LUCKY_SERVICE_TAG, "Android id is " + androidId);

		try {
			nodeId = Long.decode("0x" + androidId);
			prng.setSeed(nodeId);
			//zipf.setSeed(nodeId);
			Log.i(LUCKY_SERVICE_TAG, "Node id is " + nodeId);
		} catch (NumberFormatException e) {
			Log.d(LUCKY_SERVICE_TAG, "Could not decode androidId into long integer");
		}

		// Set deviceId (IMEI, used to read trace file)
		TelephonyManager tm = (TelephonyManager)getSystemService(TELEPHONY_SERVICE); 
		deviceId = tm.getDeviceId();
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Log.d(LUCKY_SERVICE_TAG, "Received start id " + startId + ": " + intent);
		// We want this service to continue running until it is explicitly
		// stopped, so return sticky.
		
		if (hh == null) {
			switch (Handle.getDaemonStatus()) {
			case Handle.HAGGLE_DAEMON_CRASHED:
				Log.d(LUCKY_SERVICE_TAG, "Haggle crashed, starting again");
			case Handle.HAGGLE_DAEMON_NOT_RUNNING:
				if (!backupLogs()) {
					Log.d(LUCKY_SERVICE_TAG, "Could not backup log files");
				}
				if (Handle.spawnDaemon(new LaunchCallback() {

					public int callback(long milliseconds) {

						Log.d(LUCKY_SERVICE_TAG, "Spawning milliseconds..."
								+ milliseconds);

						if (milliseconds == 0) {
							// Daemon launched
						}
						return 0;
					}
				})) {
					Log.d(LUCKY_SERVICE_TAG, "Haggle daemon started");
					haggleIsRunning = true;
				} else {
					Log.d(LUCKY_SERVICE_TAG, "Haggle daemon start failed");
				}
				break;
			case Handle.HAGGLE_DAEMON_ERROR:
				Log.d(LUCKY_SERVICE_TAG, "Haggle daemon error");
				break;
			case Handle.HAGGLE_DAEMON_RUNNING:
				Log.d(LUCKY_SERVICE_TAG, "Haggle daemon already running");
				haggleIsRunning = true;
				break;
			}

			if (haggleIsRunning) {
				int i = 0;

				while (true) {
					try {
						hh = new Handle(LUCKYME_APPNAME);
					} catch (AlreadyRegisteredException e) {
						Log.i(LUCKY_SERVICE_TAG, "Already registered.");
						Handle.unregister(LUCKYME_APPNAME);
						if (i++ > 0)
							return START_STICKY;
						continue;
					} catch (RegistrationFailedException e) {
						Log.i(LUCKY_SERVICE_TAG, "Registration failed.");
						e.printStackTrace();
						return START_STICKY;
					}
					break;
				}
				hh.registerEventInterest(EVENT_HAGGLE_SHUTDOWN, this);
				hh.registerEventInterest(EVENT_INTEREST_LIST_UPDATE, this);
				hh.registerEventInterest(EVENT_NEW_DATAOBJECT, this);
				hh.registerEventInterest(EVENT_NEIGHBOR_UPDATE, this);

				hh.eventLoopRunAsync(this);
				isRunning = true;
			}
		}
		// START_NOT_STICKY: Do not automatically restart the service if it is 
		// killed due to low memory. 
		// TODO: Implement START_STICKY, i.e., handle restart, and pickup state
		// from where we left off.
		return START_NOT_STICKY;
	}

	@Override
	public void onDestroy() {
		super.onDestroy();

		stopDataGenerator();

		if (hh != null) {
			// Tell Haggle to stop, but first make sure we ignore the shutdown
			// event since we dispose of the handle here
			ignoreShutdown = true;
			hh.shutdown();
			hh.dispose();
			hh = null;
		}

		Log.i(LUCKY_SERVICE_TAG, "Service destroyed.");
	}

	@Override
	public IBinder onBind(Intent intent) {
		Log.i(LUCKY_SERVICE_TAG, "onBind: Client bound to service : " + intent);
		return mBinder;
	}

	@Override
	public void onRebind(Intent intent) {
		Log.i(LUCKY_SERVICE_TAG, "onRebind: Client bound to service : "
				+ intent);
		super.onRebind(intent);
	}

	@Override
	public boolean onUnbind(Intent intent) {
		super.onUnbind(intent);
		Log.i(LUCKY_SERVICE_TAG, "onUnbind: LuckyMe disconnected from service.");
		mClientMessenger = null;
		return true;
	}

	/**
	 * Class for clients to access. Because we know this service always runs in
	 * the same process as its clients, we don't need to deal with IPC.
	 */
	public class LuckyBinder extends Binder {
		LuckyService getService() {
			return LuckyService.this;
		}
	}

	class WakeLockHandler implements Runnable {
		private boolean shouldExit = false;
		private PowerManager.WakeLock wakeLock;
		private boolean hasLock = false;
		
		WakeLockHandler() {
			PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
			wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, LUCKY_SERVICE_TAG);
		}
		
		void lockAcquire() {
			Log.d(LUCKY_SERVICE_TAG, "Acquire wake lock");
			wakeLock.acquire();
			hasLock = true;
		}
		void lockRelease() {
			Log.d(LUCKY_SERVICE_TAG, "Release wake lock");
			wakeLock.release();
			hasLock = false;
		}
		
		@Override
		public void run() {
				Log.d(LUCKY_SERVICE_TAG, "WakeLockHandler running");
				
				while (!shouldExit) {
					try {
						if (!hasLock) {
							// Hold lock for 30 mins
							lockAcquire();
							Thread.sleep(30 * 60 * 1000);
						} else {
							// Release lock for 5 mins
							lockRelease();
							Thread.sleep(5 * 60 * 1000);
						}
					} catch (InterruptedException e) {
						Log.d(LUCKY_SERVICE_TAG, "WakeLockHandler interrupted");
					}
				}
		}
		
		public void stop() {
			shouldExit = true;
		}
	}
	
	class DataObjectGenerator implements Runnable {
		private boolean shouldExit = false;

		@Override
		public void run() {
			try {
				Thread.sleep(10000);
			} catch (InterruptedException e) {
			}

			while (!shouldExit) {

				try {
					Thread.sleep(10000);
				} catch (InterruptedException e) {
					Log.d(LUCKY_SERVICE_TAG, "DataObjectGenerator interrupted");
					continue;
				}

				DataObject dObj = createRandomDataObject("/sdcard/luckyme.jpg");

				if (dObj != null) {
					hh.publishDataObject(dObj);
					num_dataobjects_tx++;
					Log.d(LUCKY_SERVICE_TAG, "Generated data object "
							+ num_dataobjects_tx + " luck=" + calculateLuck(dObj));
					sendClientMessage(MSG_NUM_DATAOBJECTS_TX);
				}
			}
			shouldExit = false;
		}

		public void stop() {
			shouldExit = true;
		}
	}

	private void startDataGenerator() {
		Log.i(LUCKY_SERVICE_TAG, "Starting data generator");
		mDataThread.start();
	}

	private void stopDataGenerator() {
		if (mDataThread != null) {
			//mDataGenerator.stop();
			//mExpReader.stop();
			mWakeLockHandler.stop();
			mDataThread.interrupt();
			try {
				mDataThread.join();
				Log.i(LUCKY_SERVICE_TAG, "Joined with data thread.");
			} catch (InterruptedException e) {
				Log.i(LUCKY_SERVICE_TAG, "Could not join with data thread.");
			}
		}
	}

	public boolean isRunning() {
		return isRunning;
	}

	private class FileBackupFilter implements FilenameFilter {
		private String suffix;

		FileBackupFilter(String suffix) {
			this.suffix = suffix;
		}

		@Override
		public boolean accept(File dir, String filename) {
			if (filename.endsWith(suffix))
				return true;
			return false;
		}
	}

	private boolean backupFile(String filename) {
		final String backupDir = "/sdcard/" + LUCKYME_APPNAME;

		File dir = new File(backupDir);

		if (!dir.exists() && !dir.mkdirs()) {
			Log.d(LUCKY_SERVICE_TAG, "saveFile: Could not create " + backupDir);
			return false;
		}

		File f = new File(filename);

		if (!f.exists()) {
			Log.d(LUCKY_SERVICE_TAG, "saveFile: No file=" + filename);
			return false;
		}

		if (!f.isFile()) {
			Log.d(LUCKY_SERVICE_TAG, "saveFile: file=" + filename
					+ " is not a file");
			return false;
		}

		File[] fileList = dir.listFiles(new FileBackupFilter(f.getName()));

		int backupNum = 0;

		for (int i = 0; i < fileList.length; i++) {
			int index = fileList[i].getName().indexOf('-');
			String num = fileList[i].getName().substring(0, index);
			// Log.d(LUCKY_SERVICE_TAG, "Found backup num " + num + " of file "
			// + f.getName());
			Integer n;

			try {
				n = Integer.decode(num);
			} catch (NumberFormatException e) {
				Log.d(LUCKY_SERVICE_TAG, "Could not parse integer: " + num);
				continue;
			}

			if (n.intValue() >= backupNum) {
				// Log.d(LUCKY_SERVICE_TAG, "backupNum=" + n);
				backupNum = n + 1;
			}
		}

		String backupFileName = dir.getAbsolutePath() + "/"
				+ Integer.toString(backupNum) + "-" + f.getName();

		File backupFile = new File(backupFileName);

		Log.d(LUCKY_SERVICE_TAG,
				"saveFile: file=" + backupFile.getAbsolutePath());

		// We must read the file and write it to the new location.
		// A simple move operation doesn't work across file systems,
		// so this solution is needed to move the file to the sdcard.
		FileInputStream fIn;

		try {
			fIn = new FileInputStream(f);
		} catch (FileNotFoundException e) {
			Log.d(LUCKY_SERVICE_TAG,
					"saveFile: Error opening input file=" + f.getAbsolutePath()
							+ " : " + e.getMessage());
			return false;
		}

		FileOutputStream fOut;

		try {
			fOut = new FileOutputStream(backupFile);
		} catch (FileNotFoundException e) {
			Log.d(LUCKY_SERVICE_TAG, "saveFile:Error opening output file="
					+ backupFile.getAbsolutePath() + " : " + e.getMessage());
			return false;
		}

		byte buf[] = new byte[1024];
		long totBytes = 0;
		boolean ret = false;

		while (true) {
			try {
				int len = fIn.read(buf);

				if (len == -1) {
					// EOF
					fIn.close();
					fOut.close();
					Log.d(LUCKY_SERVICE_TAG,
							"saveFile: successfully wrote file="
									+ backupFile.getAbsolutePath()
									+ " totBytes=" + totBytes);
					ret = true;
					f.delete();
					break;
				}
				fOut.write(buf, 0, len);

				totBytes += len;
			} catch (IOException e) {
				Log.d(LUCKY_SERVICE_TAG,
						"Error writing file=" + backupFile.getAbsolutePath()
								+ " : " + e.getMessage());
				break;
			}
		}

		return ret;
	}

	private final String backupFiles[] = { "/data/haggle/haggle.db",
			"/data/haggle/trace.log" };

	private boolean backupLogs() {
		boolean ret = true;

		for (int i = 0; i < backupFiles.length; i++) {
			boolean tmp_ret = backupFile(backupFiles[i]);

			if (!tmp_ret) {
				Log.d(LUCKY_SERVICE_TAG, "Could not backup file: "
						+ backupFiles[i]);
			}

			ret &= tmp_ret;
		}
		return ret;
	}

	class ExperimentSetupReader implements Runnable {
		private boolean shouldExit = false;
		ArrayList<Attribute> interestList = new ArrayList<Attribute>();
		private long nodeId = -1;
		private int cntExtraNode = -1;
		
		public ExperimentSetupReader() {
			
		}
		@Override
		public void run() {
			Log.i(LUCKY_SERVICE_TAG, "Reading experiment setup");
			
			try {
				InputStream in = getResources().openRawResource(R.raw.test);
				BufferedReader br = new BufferedReader(new InputStreamReader(in));

				while (!shouldExit) {
					String line = br.readLine();

					if (line == null) {
						Log.i(LUCKY_SERVICE_TAG, "Reached EOF of experiment configuration file");
						break;
					}

					if (line.length() == 0 || line.charAt(0) == '#')
						continue;
					
					//Log.i(LUCKY_SERVICE_TAG, "Line is: " + line);
					
					if (line.charAt(0) == 'n' && line.length() > 2) {
						String[] larr = line.split(" ");

						try {
							String id = larr[1];
							
							if (id.compareToIgnoreCase(deviceId) != 0)
									continue; 
							
							nodeId = Long.parseLong(larr[2]);
							 
							//Log.i(LUCKY_SERVICE_TAG, "parsing node deviceId=" + deviceId + " nodeId=" + nodeId);
							
							for (int i = 3; i < larr.length; i++) {
								String[] aarr = larr[i].split(":"); 
								
								if (aarr.length == 2) {
									try {
										long w = Long.parseLong(aarr[1]);
										Attribute a = new Attribute("LuckyMe", aarr[0], w);
										interestList.add(a);
										Log.d(LUCKY_SERVICE_TAG, "Read interest: " + a);
									} catch (NumberFormatException e) {
										Log.d(LUCKY_SERVICE_TAG, "bad weight format");
									}
								}
							}
						} catch (NumberFormatException e) {
							Log.d(LUCKY_SERVICE_TAG, "Bad node id " + larr[2]);
						}
					
					} else if (line.charAt(0) == 'x') {
						String[] larr = line.split(" ");

						//Log.i(LUCKY_SERVICE_TAG, "parsing extra node");
						
						DataObject dObj = null;
						try {
							dObj = new DataObject();
							dObj.addAttribute("NodeDescription", Integer.toString(cntExtraNode++));
						} catch (DataObjectException e1) {
							Log.d(LUCKY_SERVICE_TAG, "Could not create data object");
						}

						if (dObj != null) {
							for (int i = 1; i < larr.length; i++) {
								String[] aarr = larr[i].split(":"); 
								
								if (aarr.length == 2) {
									try {
										long w = Long.parseLong(aarr[1]);
										dObj.addAttribute("LuckyMe", aarr[0], w);
									} catch (NumberFormatException e) {
										Log.d(LUCKY_SERVICE_TAG, "bad number format");
									}
								}
							}
						}
						hh.publishDataObject(dObj);

					
					} else if (line.charAt(0) == 'd') {
						String[] larr = line.split(" ");
						//Log.i(LUCKY_SERVICE_TAG, "parsing data");
						
						if (larr.length < 2)
							continue;

						try {
							long id = Long.parseLong(larr[1]);
							
							if (id != nodeId) {
								//Log.d(LUCKY_SERVICE_TAG, "Ignoring data line [wrong node]: " + line);
								continue;
							}
							
						} catch (NumberFormatException e) {
							Log.d(LUCKY_SERVICE_TAG, "Bad data node id " + larr[1]);
							continue;
						}
						
						DataObject dObj = createEmptyDataObject("/sdcard/luckyme.jpg");

						if (dObj != null) {
							for (int i = 3; i < larr.length; i++) {
								try {
									dObj.addAttribute("LuckyMe", larr[i]);
								} catch (NumberFormatException e) {
									Log.d(LUCKY_SERVICE_TAG, "bad number format");
								}
							}
							//Log.d(LUCKY_SERVICE_TAG, "New data object: " + dObj);
							hh.publishDataObject(dObj);
							num_dataobjects_tx++;
							//Log.d(LUCKY_SERVICE_TAG, "Generated data object "
								//	+ num_dataobjects_tx + " luck=" + calculateLuck(dObj));
							sendClientMessage(MSG_NUM_DATAOBJECTS_TX);
							if (!shouldExit) {
								try {
									Thread.sleep(100);
								} catch (InterruptedException e) {
									Log.d(LUCKY_SERVICE_TAG,
											"ExperimentSetupReader interrupted");
									continue;
								}
							}
						}
					}
				}
				
				in.close();
			} catch (Resources.NotFoundException e) {
				
			} catch (IOException e) {
				
			}
			// Sleep some period of time before we publish our interests
			if (!shouldExit) {
				try {
					Thread.sleep(2000);
				} catch (InterruptedException e) {
					Log.d(LUCKY_SERVICE_TAG,
							"ExperimentSetupReader interrupted");
				}

				Log.i(LUCKY_SERVICE_TAG, "Done reading experiment setup");

				// Note, interests might require protection in the future.
				// However, at this point, this thread is the only one touching
				// the interest array before the interests are published.
				interests = new Attribute[interestList.size()];
				interests = interestList.toArray(interests);
				hh.registerInterests(interests);
			}

			Log.i(LUCKY_SERVICE_TAG, "experiment setup reader exits");
			// Reset in case thread is run again
			shouldExit = false;
		}
		public void stop() {
			shouldExit = true;
		}
	}
	
	private synchronized void setNeighbors(Node[] neighbors) {
		this.neighbors = neighbors;
	}

	public synchronized Node[] getNeighbors() {
		if (neighbors != null) {
			return neighbors.clone();
		}
		return null;
	}

	public void setMessenger(Messenger m) {
		mClientMessenger = m;
	}

	private void sendClientMessage(int msgType) {
		if (mClientMessenger == null) {
			return;
		}

		Message msg = Message.obtain(null, msgType);

		switch (msgType) {
		case LuckyService.MSG_NEIGHBOR_UPDATE:
			break;
		case LuckyService.MSG_NUM_DATAOBJECTS_TX:
			msg.arg1 = num_dataobjects_tx;
			break;
		case LuckyService.MSG_NUM_DATAOBJECTS_RX:
			msg.arg1 = num_dataobjects_rx;
			// Pass the average luck in arg2. Yes, casting here is slightly ugly.
			// However, overflowing the int is not a big issue here.
			if (num_dataobjects_rx > 0)
				msg.arg2 = (int)(cumulativeLuck / num_dataobjects_rx);
			else
				msg.arg2 = 0;
			break;
		case LuckyService.MSG_LUCKY_SERVICE_START:
			break;
		case LuckyService.MSG_LUCKY_SERVICE_STOP:
			break;
		default:
			return;
		}

		try {
			mClientMessenger.send(msg);
		} catch (RemoteException e) {
			Log.d(LUCKY_SERVICE_TAG, "Messange send failed");
		}
	}

	public void requestAllUpdateMessages() {
		sendClientMessage(MSG_NEIGHBOR_UPDATE);
		sendClientMessage(MSG_NUM_DATAOBJECTS_TX);
		sendClientMessage(MSG_NUM_DATAOBJECTS_RX);
	}

	// This is the object that receives interactions from clients.
	private final IBinder mBinder = new LuckyBinder();

	
	private DataObject createEmptyDataObject(String filename) {
		DataObject dObj = null;

		if (filename != null && filename.length() > 0) {
			try {
				dObj = new DataObject(filename);
				dObj.addFileHash();
			} catch (DataObjectException e) {
				Log.d(LUCKY_SERVICE_TAG, "File '" + filename
						+ "' does not exist");
				try {
					dObj = new DataObject();
				} catch (DataObjectException e1) {
					Log.d(LUCKY_SERVICE_TAG, "Could not create data object");
				}
			}
		} else {
			try {
				dObj = new DataObject();
			} catch (DataObjectException e1) {
				Log.d(LUCKY_SERVICE_TAG, "Could not create data object");
			}
		}

		dObj.setCreateTime();
		
		return dObj;
	}
	/*
	 * private DataObject createRandomDataObject() { return
	 * createRandomDataObject(null); }
	 */
	private DataObject createRandomDataObject(String filename) {
		DataObject dObj = createEmptyDataObject(filename);
		
		if (dObj == null) {
			return null;
		}

		HashSet<Integer> values = new HashSet<Integer>();

		for (int i = 0; i < numDataObjectAttributes;) {
			int value = prng.nextInt(attributePoolSize);

			if (values.contains(value)) {
				continue;
			}

			values.add(value);
			dObj.addAttribute(LUCKYME_APPNAME, "" + value);

			i++;
		}

		dObj.setCreateTime();
		
		return dObj;
	}

	private double fac(long n) {
		long i, t = 1;

		for (i = n; i > 1; i--)
			t *= i;

		return (double) t;
	}

	// Creates interests based on a center interest which is picked with
	// a zipf-distributed probability. Around this center value, select
	// interests with weights according to a binomial distribution centered
	// on the center interest.
	private Attribute[] createInterests() {
		long luck = 0;
		long n, u;
		Attribute[] interests = new Attribute[numInterestAttributes];
		boolean useNodeNrLuck = false;
		double p;

		if (useNodeNrLuck) {
			luck = nodeId;
		} else {
			luck = zipf.nextInt();
		}
		p = 0.5;
		n = numInterestAttributes - 1;

		u = (long) (n * p);

		for (int i = 0; i < numInterestAttributes; i++) {
			long interest, weight;
			interest = (luck + i - u + attributePoolSize) % attributePoolSize;
			weight = (long) (100 * fac(n) / (fac(n - i) * fac(i))
					* Math.pow(p, i) * Math.pow(1 - p, n - i));
			interests[i] = new Attribute(LUCKYME_APPNAME,
					Long.toString(interest), weight);
		}
		return interests;
	}

	private long calculateLuck(DataObject dObj) {
		long luck = 0;

		if (this.interests == null) {
			Log.i(LUCKY_SERVICE_TAG, "calculateLuck: interests array is null");
			return luck;
		}

		Attribute[] attrs = dObj.getAttributes();

		for (int i = 0; i < attrs.length; i++) {
			for (int j = 0; j < interests.length; j++) {
				if (attrs[i].getValue().compareTo(interests[j].getValue()) == 0) {
					luck += interests[j].getWeight();
				}
			}
		}
		return luck;
	}

	@Override
	public void onInterestListUpdate(Attribute[] interests) {
		
		if (interests == null) {
			Log.d(LUCKY_SERVICE_TAG, "Got null interest list!");
			return;
		}
		
		Log.i(LUCKY_SERVICE_TAG, "Got interest list of " + interests.length
				+ " items");

		if (interests.length > 0) {
			this.interests = interests;
		} else {
			/*
			 this.interests = createInterests();
			 hh.registerInterests(this.interests);
			*/
		}

		if (this.interests != null) {
			for (int i = 0; i < this.interests.length; i++) {
				Log.i(LUCKY_SERVICE_TAG, "Interest " + i + ":" + this.interests[i]);
			}
		}
		
		startDataGenerator();
	}

	@Override
	public void onNeighborUpdate(Node[] neighbors) {
		Log.i(LUCKY_SERVICE_TAG, "New neighbor update.");
		setNeighbors(neighbors);
		sendClientMessage(MSG_NEIGHBOR_UPDATE);
	}

	@Override
	public void onNewDataObject(DataObject dObj) {
		num_dataobjects_rx++;
		sendClientMessage(MSG_NUM_DATAOBJECTS_RX);
		Long luck = new Long(calculateLuck(dObj));

		cumulativeLuck += luck.longValue();
		
		Log.i(LUCKY_SERVICE_TAG, "Data object received. Luck=" + luck.longValue());
		
		// Count the number of data objects with this luck that we have received
		// by adding the count to a HashMap
		Long count = myLuck.get(luck);
		
		if (count != null) {
			count++;
			// Not sure if a put is necessary here, or if we directly 
			// changed the count in the map via the reference
		} else {
			count = new Long(1);
		}
		myLuck.put(luck, count);
	}

	@Override
	public void onShutdown(int reason) {
		Log.i(LUCKY_SERVICE_TAG, "Haggle was shut down.");
		if (!ignoreShutdown)
			stopSelf();
	}

	@Override
	public void onEventLoopStart() {
		Log.i(LUCKY_SERVICE_TAG, "Event loop started.");
		hh.getApplicationInterestsAsync();
	}

	@Override
	public void onEventLoopStop() {
		Log.i(LUCKY_SERVICE_TAG, "Event loop stopped.");
	}
}
