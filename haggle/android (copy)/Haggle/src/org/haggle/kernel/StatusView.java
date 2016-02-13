package org.haggle.kernel;

import org.haggle.kernel.R;
import android.app.Activity;
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
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

public class StatusView extends Activity {
	Button mStopButton = null;
	Haggle mHaggleService = null;
	ServiceConnection mConnection = null;
	Messenger mMessenger = null;
	HaggleEventHandler mHaggleEventHandler = null;
	boolean mIsBound = false;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		
		mStopButton = (Button) findViewById(R.id.stop_button);
		mStopButton.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				if (mHaggleService != null)
					mHaggleService.shutdown();
				
				mStopButton.setEnabled(false);
				Toast.makeText(getBaseContext(), "Haggle stopped...", Toast.LENGTH_SHORT).show();
			}
		});
		
		Log.d("Haggle::StatusView", "onCreate()");
		
		mHaggleEventHandler = new HaggleEventHandler();
		mMessenger = new Messenger(mHaggleEventHandler);
		
		mConnection = new ServiceConnection() {
			public void onServiceConnected(ComponentName className, IBinder service) {
				// This is called when the connection with the service has been
				// established, giving us the service object we can use to
				// interact with the service. Because we have bound to a explicit
				// service that we know is running in our own process, we can
				// cast its IBinder to a concrete class and directly access it.
				mHaggleService = ((Haggle.HaggleBinder) service).getHaggle();
			}

			public void onServiceDisconnected(ComponentName className) {
				// This is called when the connection with the service has been
				// unexpectedly disconnected -- that is, its process crashed.
				// Because it is running in our same process, we should never
				// see this happen.
				mHaggleService = null;
				Log.i("Haggle::StatusView", "Disconnected from Haggle service.");
			}
		};
	}
	
	void doBindService() {
	    bindService(new Intent(this, Haggle.class), 
	    		mConnection, Context.BIND_AUTO_CREATE);
	    mIsBound = true;
	}

	void doUnbindService() {
	    if (mIsBound) {
	        unbindService(mConnection);
	        mIsBound = false;
	    }
	}

	class HaggleEventHandler extends Handler {
		public void handleMessage(Message msg) {
			//Log.d(LUCKY_VIEW_TAG, "Got a message type " + msg.what);
			
			if (mHaggleService == null) {
				Log.d("Haggle", "handleMessage: mHaggleService is null");
				return;
			}
			/*
			 * Handle the message coming from LuckyService.
			 */
			switch (msg.what) {
			default:
				break;
			}
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	protected void onRestart() {
		super.onRestart();
	}

	@Override
	protected void onResume() {
		super.onResume();
	}

	@Override
	protected void onStart() {
		super.onStart();
		Log.d("Haggle::StatusView", "onStart()");
		doBindService();
	}

	@Override
	protected void onStop() {
		super.onStop();
		doUnbindService();
	}
}
