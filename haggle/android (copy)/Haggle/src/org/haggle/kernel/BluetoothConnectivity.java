/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 */

package org.haggle.kernel;

import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

public class BluetoothConnectivity extends BroadcastReceiver {
	Context mContext;
	String mBluetoothAdapterName, mBluetoothAdapterAddr;
	
	private native void onBluetoothTurnedOn(String addr, String name);
	private native void onBluetoothTurnedOff(String addr, String name);
	
	public BluetoothConnectivity(Context c) {
		super();
		mContext = c;
	}
	
	public int initialize() {
		BluetoothAdapter mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
		
		if (mBluetoothAdapter != null) {
			Log.d("Haggle", "Registering Bluetooth receiver");
			
			// Register for Bluetooth adapter state events
			IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
			filter.addAction(BluetoothAdapter.ACTION_SCAN_MODE_CHANGED);
			mContext.registerReceiver(this, filter);
			
			if (mBluetoothAdapter.isEnabled()) {
				mBluetoothAdapterAddr = mBluetoothAdapter.getAddress();
				mBluetoothAdapterName = mBluetoothAdapter.getName();
			}
			Log.d("Haggle", "Adapter info: " + mBluetoothAdapterAddr + " " + mBluetoothAdapterName);
		}
			
		return 0;
	}
	
	public void finalize() {
		BluetoothAdapter mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
		
		if (mBluetoothAdapter != null) {
		    mContext.unregisterReceiver(this);
		} 
	}
	
	@Override
	public void onReceive(Context context, Intent intent) {
		String action = intent.getAction();
		
		if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action) ||
				BluetoothAdapter.ACTION_SCAN_MODE_CHANGED.equals(action)) {
			BluetoothAdapter mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
			
			int mode = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, 
					BluetoothAdapter.ERROR);
			
			switch (mode){
			case BluetoothAdapter.SCAN_MODE_CONNECTABLE:
				Log.d("Haggle", "Bluetooth scan mode connectable");
				break;
			case BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE:
				Log.d("Haggle", "Bluetooth scan mode connectable discoverable");
				break;
			case BluetoothAdapter.SCAN_MODE_NONE:
				Log.d("Haggle", "Bluetooth scan mode none");
				break;
			case BluetoothAdapter.STATE_OFF:
				Log.d("Haggle", "Bluetooth off");
				break;
			case BluetoothAdapter.STATE_ON:
				mBluetoothAdapterAddr = mBluetoothAdapter.getAddress();
				mBluetoothAdapterName = mBluetoothAdapter.getName();
				Log.d("Haggle", "Bluetooth on " + 
						mBluetoothAdapterAddr + " " + mBluetoothAdapterName);
				onBluetoothTurnedOn(mBluetoothAdapterAddr, mBluetoothAdapterName);
				break;
			case BluetoothAdapter.STATE_TURNING_OFF:
				Log.d("Haggle", "Bluetooth turning off " + 
						mBluetoothAdapterAddr + " " + mBluetoothAdapterName);
				onBluetoothTurnedOff(mBluetoothAdapterAddr, mBluetoothAdapterName);
				break;
			case BluetoothAdapter.STATE_TURNING_ON:
				Log.d("Haggle", "Bluetooth turning on");
				break;
			}
		}
	}
}
