/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Minyoung Kim (MK)
 */

package org.haggle.PhotoShare;

import java.util.ArrayList;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.text.Editable;
import android.util.Log;
import android.view.ContextMenu;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.Toast;
import android.widget.AdapterView.AdapterContextMenuInfo;

import org.haggle.Attribute;

public class InterestView extends Activity {
	public static ArrayList<String> interests = new ArrayList<String>();
	private final ArrayList<String> deletedInterests = new ArrayList<String>();
	private final ArrayList<String> addedInterests = new ArrayList<String>();
	private ArrayAdapter<String> interestAdpt = null;
	private ListView interestListView;
	private EditText entry;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

        setContentView(R.layout.interest_view);
        
        entry = (EditText) findViewById(R.id.entry);

        entry.setOnKeyListener(new View.OnKeyListener() {
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				switch (keyCode) {
				case KeyEvent.KEYCODE_ENTER:
					if (entry.hasFocus()) {
						parseEntry();
					}
					return true;
				}
				return false;
			}
        });
        interestListView = (ListView) findViewById(R.id.interest_list);
       
        synchronized(interests) {
        		interestAdpt = new ArrayAdapter<String>(this, R.layout.list_text_item, interests);
        }
        interestListView.setAdapter(interestAdpt);
        
        registerForContextMenu(interestListView);

        final Button addButton = (Button) findViewById(R.id.add_button);
        addButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                // Perform action on click
            	parseEntry();
            }
        });
        final Button cancelButton = (Button) findViewById(R.id.cancel_button);
        cancelButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                // Perform action on click
            	Log.d(PhotoShare.LOG_TAG, "Cancel clicked");
        		Intent i = new Intent();
        		setResult(PhotoShare.CANCEL_INTEREST_REQUEST, i);
            	finish();
            }
        });
        
	}
	@Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		menu.setHeaderTitle("Interest");
		menu.add("Delete");
        menu.add("Cancel");
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        AdapterContextMenuInfo info = (AdapterContextMenuInfo) item.getMenuInfo();
     
        if (item.getTitle() == "Delete") {
        	deleteInterest(info.position);
        }
        
        return true;
    }

	@Override
	protected void onPause() {
		// TODO Auto-generated method stub
		super.onPause();
	}

	@Override
	protected void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
	}

	@Override
	protected void onStop() {
		// TODO Auto-generated method stub
		super.onStop();
	}
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
    	switch (keyCode) {
    	case KeyEvent.KEYCODE_BACK:
    		Log.d(PhotoShare.LOG_TAG,"Key back, creating added/deleted interests bundle");
    		
    		Intent i = new Intent();
    	
    		if (addedInterests.size() != 0)
    			i.putExtra("added", addedInterests.toArray(new String[addedInterests.size()]));
    		if (deletedInterests.size() != 0)
    			i.putExtra("deleted", deletedInterests.toArray(new String[deletedInterests.size()]));
    		setResult(RESULT_OK, i);
    		break;
    	}
    	
		return super.onKeyDown(keyCode, event);
	}
	public void parseEntry() {
		Editable ed = entry.getText();
    	String interest = ed.toString();
    	
    	if (interest == null || interest.length() == 0)
    		return;
    	
    	String[] split = interest.split(" ");
    	
    	for (int i = 0; i < split.length; i++) {
    		String[] split2 = split[i].split(":");
    		//long weight;
    		
    		if (split2.length == 2) {
    			try {
    				//weight = Long.parseLong(split2[1]);
    			} catch (Exception ex) {
    				// Just ignore and use default weight
    			}
    			interest = split2[0];
    		}

    		// Remove any trailing whitespaces and other non-wanted characters
    		interest = interest.trim();
    		
    		Log.d(PhotoShare.LOG_TAG, "Entry." + interest);

    		if (hasInterest(interest)) {
    			Toast toast = Toast.makeText(getApplicationContext(), "You already have the interest '" + interest + "'", Toast.LENGTH_SHORT);
    			toast.setGravity(Gravity.TOP|Gravity.CENTER, 0, 50);
    			toast.show();
    			continue;
    		}

    		addInterestToList(interest);
    	}
    	ed.clear();

	}
	public void addInterestToList(String interest) {
		if (!deletedInterests.remove(interest))
			addedInterests.add(interest);
		interestAdpt.add(interest);
		interestAdpt.notifyDataSetChanged();
	}
	
	public boolean hasInterest(String interest) {
		return interestAdpt.getPosition(interest) >= 0;
	}
	
	// Called from PhotoShare.java in a callback from Haggle (other thread)
	public static synchronized void setInterests(Attribute[] attrs) {
		interests.clear();
		
		for (int i = 0; i < attrs.length; i++) {
			interests.add(attrs[i].getValue());
		}
	}
	
	public void deleteInterest(String interest) {
		if (!addedInterests.remove(interest))
			deletedInterests.add(interest);
		interestAdpt.remove(interest);
		interestAdpt.notifyDataSetChanged();
	}
	public String deleteInterest(int pos) {
		String interest = interestAdpt.getItem(pos);
		
		if (interest != null) {
			if (!addedInterests.remove(interest))
				deletedInterests.add(interest);
			interestAdpt.remove(interest);
			interestAdpt.notifyDataSetChanged();
		}
		return interest;
	}
}
