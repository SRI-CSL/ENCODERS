package org.haggle.PhotoShare;

import java.util.ArrayList;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.text.Editable;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.AdapterView.AdapterContextMenuInfo;

public class AddPictureAttributeView extends Activity {
	private ArrayList<String> attributeList = new ArrayList<String>();
	private ArrayAdapter<String> attributeAdpt = null;
	private EditText entry;
	private String filepath;
	private ListView attributeListView;
	
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

    	setResult(Activity.RESULT_CANCELED);
    	
		filepath = getIntent().getExtras().getString("filepath");
    	
		if (filepath == null) {
			finish();
		}
        setContentView(R.layout.add_picture_attribute_view);
        
        entry = (EditText) findViewById(R.id.entry);
        
        entry.setOnKeyListener(new View.OnKeyListener() {
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				switch (keyCode) {
				case KeyEvent.KEYCODE_ENTER:
					if (entry.hasFocus()) {
						addEnteredAttribute();
					}
					return true;
				}
				return false;
			}
        });
        
        ImageView iv = (ImageView) findViewById(R.id.taken_picture);

        final BitmapFactory.Options picOptions = new BitmapFactory.Options();
        picOptions.inJustDecodeBounds = true;
        
		Bitmap pic = BitmapFactory.decodeFile(filepath, picOptions);

        int width = getWindowManager().getDefaultDisplay().getWidth();
        
		float ratio = picOptions.outWidth / width;
        picOptions.inSampleSize = (int) (ratio * 4);
        picOptions.inJustDecodeBounds = false;
        
		pic = BitmapFactory.decodeFile(filepath, picOptions);
        iv.setImageBitmap(pic);

        attributeListView = (ListView) findViewById(R.id.attribute_list);
        attributeAdpt = new ArrayAdapter<String>(this, R.layout.list_text_item);
        attributeListView.setAdapter(attributeAdpt);
        
        registerForContextMenu(attributeListView);
	}   
	
	protected void onStop() {
		super.onStop();
	} 
	
	protected void onDestroy() {
		super.onDestroy();
	}

	private void addAttribute(String attr) {
		if (attributeAdpt.getPosition(attr) >= 0)
			return;

		attributeAdpt.add(attr);
		attributeList.add(attr);
	}
	private String removeAttribute(int pos) {
		String str = attributeAdpt.getItem(pos);
		if (str != null) {
			attributeAdpt.remove(str);
			attributeList.remove(str);
		}
		return str;
	}
	
	private void addEnteredAttribute() {
		Editable e = entry.getText();
		
    	String attrStr = e.toString();
    
    	if (attrStr.length() == 0)
    		return;
    
    	String interestArray[] = attrStr.split(" ");
    	
    	for (int i = 0; i < interestArray.length; i++) {
        	addAttribute(interestArray[i].trim());
    	}
    	
    	Log.d(PhotoShare.LOG_TAG, "Entry." + attrStr);
    
    	e.clear();
	}
	
	
	public boolean onContextItemSelected(MenuItem item) {
		AdapterContextMenuInfo info = (AdapterContextMenuInfo) item.getMenuInfo();

		removeAttribute(info.position);
		return super.onContextItemSelected(item);     
	}
	
	public void onCreateContextMenu(ContextMenu menu, View v,
			ContextMenuInfo menuInfo) {
		//super.onCreateContextMenu(menu, v, menuInfo);
		
		menu.setHeaderTitle("Attribute");
		menu.add("Delete");
        menu.add("Cancel");
	}

	
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		switch (keyCode) {
		
		case KeyEvent.KEYCODE_BACK:
			if (attributeList.size() == 0) {
				setResult(RESULT_CANCELED);
				break;
			}

			Intent i = new Intent();
        	i.putExtra("filepath", filepath);
        	i.putExtra("attributes", attributeList.toArray(new String[attributeList.size()]));
        	
        	setResult(RESULT_OK, i);
        	break;
		case KeyEvent.KEYCODE_ENTER:
			if (entry.hasFocus()) {
				addEnteredAttribute();
			}
			break;
		}
		return super.onKeyDown(keyCode, event);
	}
}
