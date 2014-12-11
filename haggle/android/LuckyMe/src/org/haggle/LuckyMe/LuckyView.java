package org.haggle.LuckyMe;

import org.haggle.Handle;
import org.haggle.Interface;
import org.haggle.Node;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.util.Log;
import android.view.ContextMenu;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.CompoundButton;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.ToggleButton;
import android.widget.AdapterView.AdapterContextMenuInfo;

public class LuckyView extends Activity {
	private NodeAdapter nodeAdpt = null;
	private LuckyService mLuckyService = null;
	private ListView mNeighborList = null;
	private ToggleButton mLuckyServiceToggle = null;
	private TextView mHaggleStatus = null;	
	public TextView mNumDataObjectsTX = null;
	public TextView mNumDataObjectsRX = null;
	public TextView mAverageLuck = null;
	private boolean mIsBound = false;
	private Handler mLuckyEventHandler = null;
	public final String LUCKY_VIEW_TAG = "LuckyMe";
	private Messenger mMessenger = null;
	private Thread mHaggleStatusThread = null;
	private HaggleStatusChecker mHaggleStatusChecker = null;
	private ServiceConnection mConnection = null;
	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		
		Log.d(LUCKY_VIEW_TAG, "onCreate() called");

		mNumDataObjectsRX = (TextView) findViewById(R.id.num_dataobjects_rx);
		mNumDataObjectsTX = (TextView) findViewById(R.id.num_dataobjects_tx);
		mAverageLuck = (TextView) findViewById(R.id.average_luck);
		mHaggleStatus = (TextView) findViewById(R.id.haggle_status);
		mLuckyServiceToggle = (ToggleButton) findViewById(R.id.lucky_service_toggle);
		
		mLuckyServiceToggle
				.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
					@Override
					public void onCheckedChanged(CompoundButton buttonView,
							boolean isChecked) {
						Log.d(LUCKY_VIEW_TAG, "Toggle LuckyService on="
								+ isChecked);
						if (isChecked) {
							if (isLuckyServiceRunning()) {
								// Nothing to do, service was running
							} else {
								startLuckyService();
							}
						} else {
							stopLuckyService();
						}
					}
				});
		nodeAdpt = new NodeAdapter(this);
		mNeighborList = (ListView) findViewById(R.id.neighbor_list);
		mNeighborList.setAdapter(nodeAdpt);

		// We also want to show context menu for longpressed items in the
		// neighbor list
		registerForContextMenu(mNeighborList);

		mLuckyEventHandler = new LuckyEventHandler();
		mMessenger = new Messenger(mLuckyEventHandler);
		mHaggleStatusChecker = new HaggleStatusChecker(this);
		
		mConnection = new ServiceConnection() {
			public void onServiceConnected(ComponentName className, IBinder service) {
				// This is called when the connection with the service has been
				// established, giving us the service object we can use to
				// interact with the service. Because we have bound to a explicit
				// service that we know is running in our own process, we can
				// cast its IBinder to a concrete class and directly access it.
				mLuckyService = ((LuckyService.LuckyBinder) service).getService();

				mLuckyService.setMessenger(mMessenger);
				Log.i(LUCKY_VIEW_TAG, "Connected to LuckyService.");
				// Tell the user about this for our demo.
				// Toast.makeText(Binding.this, R.string.local_service_connected,
				// Toast.LENGTH_SHORT).show();
				if (isLuckyServiceRunning()) {
					mLuckyServiceToggle.setChecked(true);
				}
				mLuckyService.requestAllUpdateMessages();
			}

			public void onServiceDisconnected(ComponentName className) {
				// This is called when the connection with the service has been
				// unexpectedly disconnected -- that is, its process crashed.
				// Because it is running in our same process, we should never
				// see this happen.
				mLuckyService = null;
				// Toast.makeText(Binding.this, R.string.local_service_disconnected,
				// Toast.LENGTH_SHORT).show();
				Log.i(LUCKY_VIEW_TAG, "Disconnected from LuckyService.");
				mLuckyServiceToggle.setChecked(false);
				mIsBound = false;
			}
		};
	}

	class LuckyEventHandler extends Handler {
		public void handleMessage(Message msg) {
			//Log.d(LUCKY_VIEW_TAG, "Got a message type " + msg.what);
			
			if (mLuckyService == null) {
				Log.d(LUCKY_VIEW_TAG, "handleMessage: mLuckyService is null");
				return;
			}
			/*
			 * Handle the message coming from LuckyService.
			 */
			switch (msg.what) {
			case LuckyService.MSG_NEIGHBOR_UPDATE:
				nodeAdpt.updateNeighbors(mLuckyService.getNeighbors());
				break;
			case LuckyService.MSG_NUM_DATAOBJECTS_TX:
				mNumDataObjectsTX.setText(Integer.toString(msg.arg1));
				break;
			case LuckyService.MSG_NUM_DATAOBJECTS_RX:
				mNumDataObjectsRX.setText(Integer.toString(msg.arg1));
				mAverageLuck.setText(Integer.toString(msg.arg2));
				break;
			case LuckyService.MSG_LUCKY_SERVICE_START:
				break;
			case LuckyService.MSG_LUCKY_SERVICE_STOP:
				break;
			default:
				break;
			}
		}
	}
	// Class to update a TextView on the Activity's UI thread.
	class TextViewUpdater implements Runnable {
		private TextView tv;
		private String text;

		TextViewUpdater(TextView tv, String text) {
			this.tv = tv;
			this.text = text;
		}

		@Override
		public void run() {
			tv.setText(text);
		}
	}

	class HaggleStatusChecker implements Runnable {
		private boolean shouldExit = false;
		private int prevStatus = Handle.HAGGLE_DAEMON_NOT_RUNNING;
		private LuckyView lv = null;
		
		HaggleStatusChecker(LuckyView lv) {
			this.lv = lv;
		}
		class ServiceStopper implements Runnable {
			@Override
			public void run() {
				stopLuckyService();
			}
		}
		@Override
		public void run() {
			while (!shouldExit) {
				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					continue;
				}
				
				int status = Handle.getDaemonStatus();

				if (status != prevStatus) {
					switch (status) {
					case Handle.HAGGLE_DAEMON_CRASHED:
						lv.runOnUiThread(new TextViewUpdater(mHaggleStatus, "Crashed"));
						lv.runOnUiThread(new ServiceStopper());
						break;
					case Handle.HAGGLE_DAEMON_NOT_RUNNING:
						lv.runOnUiThread(new TextViewUpdater(mHaggleStatus, "Not running"));
						break;
					case Handle.HAGGLE_DAEMON_ERROR:
						lv.runOnUiThread(new TextViewUpdater(mHaggleStatus, "Error!"));
						break;
					case Handle.HAGGLE_DAEMON_RUNNING:
						lv.runOnUiThread(new TextViewUpdater(mHaggleStatus, "Running"));
						break;
					}
				}
				prevStatus = status;
			}
			// Reset in case we are restarted
			shouldExit = false;
		}

		public void stop() {
			shouldExit = true;
		}
	}
	private void startHaggleStatusChecker() 
	{
		if (mHaggleStatusThread == null) {
			mHaggleStatusThread = new Thread(mHaggleStatusChecker);
			try {
				mHaggleStatusThread.start();
				Log.d(LUCKY_VIEW_TAG, "Started Haggle status checker " 
						+ mHaggleStatusThread.hashCode());
			} catch (IllegalThreadStateException e) {
				Log.d(LUCKY_VIEW_TAG, "Illegal thread state " +
						mHaggleStatusThread.getState().toString());
			}
		}
	}
	private void stopHaggleStatusChecker() {
		Log.d(LUCKY_VIEW_TAG, "Stopping HaggleStatusThread");
		mHaggleStatusChecker.stop();
		mHaggleStatusThread.interrupt();
		try {
			mHaggleStatusThread.join();
			Log.d(LUCKY_VIEW_TAG, "Joined with HaggleStatusThread");
			mHaggleStatusThread = null;
		} catch (InterruptedException e) {
		}

		Log.d(LUCKY_VIEW_TAG, "Stopped HaggleStatusThread");
	}

	public Handler getHandler() {
		return mLuckyEventHandler;
	}

	@Override
	public void onCreateContextMenu(ContextMenu menu, View v,
			ContextMenuInfo menuInfo) {
		menu.setHeaderTitle("Node Information");
		menu.add("Interfaces");
		menu.add("Cancel");
	}

	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterContextMenuInfo info = (AdapterContextMenuInfo) item
				.getMenuInfo();
		if (item.getTitle() == "Interfaces") {
			AlertDialog.Builder builder;
			AlertDialog alertDialog;

			Context mContext = getApplicationContext();
			LayoutInflater inflater = (LayoutInflater) mContext
					.getSystemService(LAYOUT_INFLATER_SERVICE);
			View layout = inflater.inflate(
					R.layout.neighbor_list_context_dialog,
					(ViewGroup) findViewById(R.id.layout_root));

			TextView text = (TextView) layout.findViewById(R.id.text);
			String t = "";
			Node node = nodeAdpt.getNode(info.position);

			if (node != null) {
				Interface[] ifaces = node.getInterfaces();

				for (int i = 0; i < ifaces.length; i++) {
					t += ifaces[i].getTypeString() + " "
							+ ifaces[i].getIdentifierString() + " "
							+ ifaces[i].getStatusString() + "\n";
				}
			}

			text.setText(t);
			builder = new AlertDialog.Builder(this);
			builder.setView(layout);
			alertDialog = builder.create();
			alertDialog.setTitle("Node Interfaces");
			alertDialog.show();
		}
		return true;
	}

	void doBindService(boolean autocreate) {
		int flag = 0;

		if (autocreate)
			flag = Context.BIND_AUTO_CREATE;
		// Establish a connection with the service. We use an explicit
		// class name because we want a specific service implementation that
		// we know will be running in our own process (and thus won't be
		// supporting component replacement by other applications).
		Intent i = new Intent(this, LuckyService.class);
		mIsBound = bindService(i, mConnection, flag);
		if (!mIsBound) {
			Log.d(LUCKY_VIEW_TAG, "Could not bind to Lucky service");
		}
	}

	void doUnbindService() {
		if (mIsBound) {
			// Detach our existing connection.
			Log.d(LUCKY_VIEW_TAG, "Unbinding from Lucky service");
			unbindService(mConnection);
			mIsBound = false;
		}
	}

	private boolean isLuckyServiceRunning() {
		if (mLuckyService != null) {
			return mLuckyService.isRunning();
		}
		return false;
	}

	private void startLuckyService() {
		Log.d(LUCKY_VIEW_TAG, "Starting Lucky service");
		startService(new Intent(this, LuckyService.class));
		doBindService(true);
	}

	private void stopLuckyService() {
		doUnbindService();

		if (mLuckyService != null) {
			Log.d(LUCKY_VIEW_TAG, "Stopping Lucky service");
			stopService(new Intent(this, LuckyService.class));
			mLuckyService = null;
			Log.d(LUCKY_VIEW_TAG, "Lucky service stopped");
		}
		nodeAdpt.clear();
		Log.d(LUCKY_VIEW_TAG, "Node adapter cleared");
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		Log.d(LUCKY_VIEW_TAG, "onDestroy");
	}

	@Override
	protected void onStart() {
		super.onStart();
		Log.d(LUCKY_VIEW_TAG, "onStart: binding to service");

		startHaggleStatusChecker();
		
		if (!mIsBound) {
			doBindService(false);

/*
			// Force Bluetooth adaptor on
			BluetoothAdapter bt = BluetoothAdapter.getDefaultAdapter();

			if (bt != null)
				bt.enable();

			// Disable WiFi
			WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);

			if (wifi != null)
				wifi.setWifiEnabled(false);
*/
		}
	}

	@Override
	protected void onStop() {
		super.onStop();
		Log.d(LUCKY_VIEW_TAG, "onStop: unbinding from service");
		doUnbindService();
		stopHaggleStatusChecker();
		mLuckyEventHandler = null;
	}
}
