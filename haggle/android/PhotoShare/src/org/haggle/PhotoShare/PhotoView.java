/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Minyoung Kim (MK)
 *   Ashish Gehani (AG)
 */

package org.haggle.PhotoShare;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.DataInputStream;
import java.util.ArrayList;
import java.util.HashMap;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.DialogInterface.OnClickListener;
import android.content.res.TypedArray;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Gallery;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.AdapterView.OnItemClickListener;

import org.haggle.*;
import org.haggle.DataObject.DataObjectException;

public class PhotoView extends Activity implements OnClickListener {
	public static final int MENU_TAKE_PICTURE  = 1;
	public static final int MENU_INTERESTS = 2;
	public static final int MENU_SHUTDOWN_HAGGLE = 3;
	
	public static final int REGISTRATION_FAILED_DIALOG = 1;
	public static final int SPAWN_DAEMON_FAILED_DIALOG = 2;
	public static final int PICTURE_ATTRIBUTES_DIALOG = 3;
	
	public static final String KEY_PICTURE_PATHS = "KeyPicturePath";
	public static final String KEY_HAGGLE_HANDLE = "KeyHaggleHandle";
	public static final String KEY_PICTURE_POS = "KeyPicturePos";
	public static final String KEY_PHOTOSHARE = "KeyPhotoShare";
	public static final String DATAOBJECT_ATTRIBUTE_ABE_POLICY = "Access";

	private ImageAdapter imgAdpt = null;
	private NodeAdapter nodeAdpt = null;
	private PhotoShare ps = null;
	private Gallery gallery = null;
	private TextView neighlistHeader = null;
	private boolean shouldRegisterWithHaggle = true;
	private File takenPicture = null;
	
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

    	Log.d(PhotoShare.LOG_TAG, "PhotoView:onCreate()");	

        // turn off the window's title bar
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.main_view);
        
        ps = (PhotoShare)getApplication();
        ps.setPhotoView(this);

        // Reference the neighlistHeader view
        neighlistHeader = (TextView) findViewById(R.id.list_header);
        try {
	        FileInputStream fis = new FileInputStream("/data/haggle/deviceColor");
	        DataInputStream dis = new DataInputStream(fis);
	        byte[] buf = new byte[9];
	        dis.read(buf, 0, 9);	// expecting #AARRGGBB
	        String color = new String(buf);
	        neighlistHeader.setBackgroundColor(Color.parseColor(color));
        } catch (Exception e) {
        	Log.d(PhotoShare.LOG_TAG, "set color failed: " + e);	
        }
        
        // Reference the Gallery view
        gallery = (Gallery) findViewById(R.id.gallery);
        // Set the adapter to our custom adapter (below)

        imgAdpt = new ImageAdapter(this, 250);
        
        gallery.setAdapter(imgAdpt);
        
        // Set a item click listener, and just Toast the clicked position
        gallery.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View v, int pos, long id) {
            	Log.d(PhotoShare.LOG_TAG, "Clicked image at pos " + pos);
            	final String[] filepaths = imgAdpt.getPictureFilePaths();
            	
            	if (filepaths.length == 0)
            		return;
            	
            	final Intent i = new Intent();
            	i.setClass(getApplicationContext(), FullscreenImageViewer.class);

            	i.putExtra(PhotoView.KEY_PICTURE_PATHS, filepaths);
            	i.putExtra(PhotoView.KEY_PICTURE_POS, pos);
            	parent.getContext().startActivity(i);
            }
        });

        // We also want to show context menu for longpressed items in the gallery
        registerForContextMenu(gallery);

        ListView neighlist = (ListView) findViewById(R.id.neighbor_list);
        
        nodeAdpt = new NodeAdapter(this);
        neighlist.setAdapter(nodeAdpt);

        // We also want to show context menu for longpressed items in the neighbor list
        registerForContextMenu(neighlist);
        
    	Log.d(PhotoShare.LOG_TAG, "PhotoView:onCreate() done");	
    }
    
	@Override
    public void onRestart() {
    	super.onRestart();
    	Log.d(PhotoShare.LOG_TAG, "PhotoView:onRestart()");
    }
	
    @Override
    public void onStart() {
    	super.onStart();
    	
    	Log.d(PhotoShare.LOG_TAG, "PhotoView:onStart() freemem=" +  Runtime.getRuntime().freeMemory());	
    	
    	if (shouldRegisterWithHaggle) {
    		
    		int ret = ps.initHaggle();

    		if (ret != PhotoShare.STATUS_OK) {
    			String errorMsg = "Unknown error.";
    			
    			if (ret == PhotoShare.STATUS_SPAWN_DAEMON_FAILED) {
    				errorMsg = "PhotoShare could not start Haggle daemon.";
    			} else if (ret == PhotoShare.STATUS_REGISTRATION_FAILED) {
    				errorMsg = "PhotoShare could not connect to Haggle.";
    			}
    			Log.d(PhotoShare.LOG_TAG, "Registration failed, showing alert dialog");
    			AlertDialog.Builder builder = new AlertDialog.Builder(this);
    			builder.setMessage(errorMsg)
    			.setCancelable(false)
    			.setNegativeButton("Quit", new DialogInterface.OnClickListener() {
    				public void onClick(DialogInterface dialog, int id) {
    					dialog.cancel();
    					finish();
    				}
    			});
    			AlertDialog alert = builder.create();
    			alert.show();

    		}  else {
    			Log.d(PhotoShare.LOG_TAG, "Registration with Haggle successful");
    	    	shouldRegisterWithHaggle = false;
    		}
    	}
    }
    
    @Override
    protected void onResume() {
    	super.onResume();
    	Log.d(PhotoShare.LOG_TAG, "PhotoView:onResume()");
	}
	@Override
	protected void onPause() {
    	super.onPause();
    	Log.d(PhotoShare.LOG_TAG, "PhotoView:onPause()");

 	}
    @Override
    protected void onStop() {
    	super.onStop();
    	Log.d(PhotoShare.LOG_TAG, "PhotoView:onStop()");
    }
    @Override
    protected void onDestroy() {
    	super.onDestroy();
    	Log.d(PhotoShare.LOG_TAG, "PhotoView:onDestroy()");
    }

    @Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
    	switch (keyCode) {
    	case KeyEvent.KEYCODE_BACK:
    	case KeyEvent.KEYCODE_HOME:
    		Log.d(PhotoShare.LOG_TAG,"Key back, exit application and deregister with Haggle");
    		ps.finiHaggle();
    		shouldRegisterWithHaggle = true;
    		this.finish();
    		break;
    	}
    	
		return super.onKeyDown(keyCode, event);
	}
    
    @Override
    protected void onPrepareDialog(int id, Dialog d) {
    	
    }
    
    public void onClick(DialogInterface dialog, int which) {
    	Log.d(PhotoShare.LOG_TAG,"onClick: call finish()");
        finish();
    }
    
    @Override
    protected Dialog onCreateDialog(int id) {
    	switch (id) {
    	case PhotoShare.STATUS_REGISTRATION_FAILED:
    		return new AlertDialog.Builder(this)
    		.setTitle(R.string.haggle_dialog_title)
    		.setIcon(android.R.drawable.ic_dialog_alert)
    		.setMessage(R.string.registration_failed)
    		.setPositiveButton(android.R.string.ok, this)
    		.setCancelable(false)
    		.create();
    	case PhotoShare.STATUS_SPAWN_DAEMON_FAILED:
    		return new AlertDialog.Builder(this)
    		.setTitle(R.string.haggle_dialog_title)
    		.setIcon(android.R.drawable.ic_dialog_alert)
    		.setMessage(R.string.spawn_daemon_failed)
    		.setPositiveButton(android.R.string.ok, this)
    		.setCancelable(false)
    		.create();
    	}

    	return null;
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
    	super.onCreateOptionsMenu(menu);

        menu.add(0, MENU_TAKE_PICTURE, 0, R.string.menu_take_picture).setIcon(android.R.drawable.ic_menu_camera);
        menu.add(0, MENU_INTERESTS, 0, R.string.menu_interests).setIcon(android.R.drawable.ic_menu_search);
        menu.add(0, MENU_SHUTDOWN_HAGGLE, 0, R.string.menu_shutdown_haggle).setIcon(android.R.drawable.ic_lock_power_off);

        return true;
	}
   

	@Override
    public boolean onOptionsItemSelected(MenuItem item) {
//    	super.onOptionsItemSelected(item);877/((
    	final Intent i = new Intent();
    	
    	switch (item.getItemId()) {
    	case MENU_TAKE_PICTURE:
			File dir = new File(Environment.getExternalStorageDirectory() + "/PhotoShare");
    		final String filename = "photoshare-" + System.currentTimeMillis() + ".jpg";
    		final String filepath = dir + "/" + filename;
			
    		if (!dir.mkdirs()) {
    			Log.d(PhotoShare.LOG_TAG, "Could not create directory " + dir);
    		}
    		
			Log.d(PhotoShare.LOG_TAG, "filepath is " + filepath);
			
			takenPicture = new File(filepath);
    		Intent intent = new Intent("android.media.action.IMAGE_CAPTURE");
    		intent.putExtra(MediaStore.EXTRA_OUTPUT, Uri.fromFile(takenPicture));
    		startActivityForResult(intent, PhotoShare.IMAGE_CAPTURE_REQUEST);
            /*
 				The ambition here would be to also insert the picture in the
 				gallery.
 				
 				ContentValues values = new ContentValues();
                values.put(Images.Media.TITLE, "title");
                values.put(Images.Media.BUCKET_ID, "test");
                values.put(Images.Media.DESCRIPTION, "test Image taken");
                Uri uri = getContentResolver().insert(Media.EXTERNAL_CONTENT_URI, values);
                Intent intent = new Intent("android.media.action.IMAGE_CAPTURE");
                intent.putExtra("output", uri.getPath());
        	*/
    		return true;
    	case MENU_INTERESTS:
        	i.setClass(getApplicationContext(), InterestView.class);
        	this.startActivityForResult(i, PhotoShare.ADD_INTEREST_REQUEST);
    		return true;
    	case MENU_SHUTDOWN_HAGGLE:
    		shouldRegisterWithHaggle = true;
    		ps.shutdownHaggle();
    		return true;
    	}
    	return false;
    }
	public Bitmap scaleImage(String filepath, int width) {
		BitmapFactory.Options opts = new BitmapFactory.Options();

    	opts.inJustDecodeBounds = true;
    	BitmapFactory.decodeFile(filepath, opts); 

    	double ratio = opts.outWidth / width;
    	
    	opts.inSampleSize = (int)ratio;
    	opts.inJustDecodeBounds = false;
    	
    	return BitmapFactory.decodeFile(filepath, opts);
	}

	private void onPictureAttributesResult(int resultCode, Intent data) {
		Log.d(PhotoShare.LOG_TAG,
				"Got result from add picture attributes request");
		if (resultCode != RESULT_OK)
			return;

		String takenPictureFilepath = data.getStringExtra("filepath");
		Log.d(PhotoShare.LOG_TAG, "File is " + takenPictureFilepath);

		if (takenPictureFilepath == null)
			return;

		String[] attrs = (String[]) data.getStringArrayExtra("attributes");

		if (attrs == null)
			return;

		try {
			DataObject dObj = new DataObject(takenPictureFilepath);

			for (int i = 0; i < attrs.length; i++) {

				// CBMEN, AG, Begin

				if (attrs[i].startsWith(DATAOBJECT_ATTRIBUTE_ABE_POLICY + "=")) {
					dObj.addAttribute(DATAOBJECT_ATTRIBUTE_ABE_POLICY, 
						attrs[i].substring(DATAOBJECT_ATTRIBUTE_ABE_POLICY.length()+1), 1);
					Log.d(PhotoShare.LOG_TAG, "Picture has access policy " +
						attrs[i].substring(DATAOBJECT_ATTRIBUTE_ABE_POLICY.length()+1));
				} else {

				// CBMEN, AG, End

				Log.d(PhotoShare.LOG_TAG, "Picture has attribute " + attrs[i]);
				dObj.addAttribute("Picture", attrs[i], 1);

				} // CBMEN, AG
			}

			// TODO move to separate thread...
			dObj.addFileHash();

			Bitmap bmp = scaleImage(dObj.getFilePath(), 50);
			ByteArrayOutputStream os = new ByteArrayOutputStream();
			bmp.compress(CompressFormat.JPEG, 75, os);

			dObj.setThumbnail(os.toByteArray());

			ps.getHaggleHandle().publishDataObject(dObj);
			
			ArrayList<Attribute> aa = new ArrayList<Attribute>();

			for (int i = 0; i < attrs.length; i++) {

				// Also add to our interest list in the interest view
				synchronized (InterestView.interests) {
					if (InterestView.interests.contains(attrs[i]) == false) {
						InterestView.interests.add(attrs[i]);
						aa.add(new Attribute("Picture", attrs[i], 1));
					}
				}
			}
			
			ps.getHaggleHandle().registerInterests(
					aa.toArray(new Attribute[aa.size()]));

			// Call dispose on data object and attribute to free native data
			// before GC in order to save memory
			dObj.dispose();
			
			for (int i = 0; i < aa.size(); i++) {
				aa.get(i).dispose();
			}
			Log.d(PhotoShare.LOG_TAG, "Disposed data object and " + aa.size() + " attributes");
		} catch (DataObjectException e) {
			// TODO Auto-generated catch block
			Log.d(PhotoShare.LOG_TAG, "Could not create data object for "
					+ takenPictureFilepath);
		}
	}

	private void onAddInterestResult(int resultCode, Intent data) {
		String[] deletedInterests = data.getStringArrayExtra("deleted");
		String[] addedInterests = data.getStringArrayExtra("added");
		
		if (addedInterests != null && addedInterests.length != 0) {
			Attribute[] aa = new Attribute[addedInterests.length];
			for (int i = 0; i < addedInterests.length; i++) {
                                if (!addedInterests[i].startsWith(DATAOBJECT_ATTRIBUTE_ABE_POLICY + "=")) { // CBMEN, AG
				aa[i] = new Attribute("Picture", addedInterests[i], 1);
				Log.d(PhotoShare.LOG_TAG, "Added interest " + addedInterests[i]);
				} // CBMEN, AG
			}
			ps.getHaggleHandle().registerInterests(aa);
			
			// Call dispose to free native data before GC
			for (int i = 0; i < aa.length; i++) {
				aa[i].dispose();
			}
		}

		if (deletedInterests != null && deletedInterests.length != 0) {
			Attribute[] aa = new Attribute[deletedInterests.length];
			for (int i = 0; i < deletedInterests.length; i++) {
                                if (!addedInterests[i].startsWith(DATAOBJECT_ATTRIBUTE_ABE_POLICY + "=")) { // CBMEN, AG
				aa[i] = new Attribute("Picture", deletedInterests[i], 1);
				Log.d(PhotoShare.LOG_TAG, "Deleted interest " + deletedInterests[i]);
				} // CBMEN, AG
			}
			ps.getHaggleHandle().unregisterInterests(aa);

			// Call dispose to free native data before GC
			for (int i = 0; i < aa.length; i++) {
				aa[i].dispose();
			}
		}
	}
	
	private void onImageCaptureResult(int resultCode, Intent data) {
		if (resultCode == Activity.RESULT_CANCELED) {
			Log.d(PhotoShare.LOG_TAG, "Image capture was canceled!");
			takenPicture = null;
			return;
		}
		if (resultCode != RESULT_OK) {
			takenPicture = null;
			Log.d(PhotoShare.LOG_TAG, "Image capture result was not OK!");
			return;
		}

		if (takenPicture == null) {
			Log.d(PhotoShare.LOG_TAG, "Taken picture path is null");
			return;
		}
		
		Log.d(PhotoShare.LOG_TAG, "Taken picture: " + takenPicture.getAbsolutePath());
		
		data = new Intent();
		
		data.putExtra("filepath", takenPicture.getAbsolutePath());
		data.setClass(getApplicationContext(), AddPictureAttributeView.class);

		this.startActivityForResult(data,
				PhotoShare.ADD_PICTURE_ATTRIBUTES_REQUEST);		
	}

	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
    	
		switch (requestCode) {
		case PhotoShare.ADD_PICTURE_ATTRIBUTES_REQUEST:
			onPictureAttributesResult(resultCode, data);
			break;
		case PhotoShare.ADD_INTEREST_REQUEST:
			if ( resultCode != PhotoShare.CANCEL_INTEREST_REQUEST )
			{
			   onAddInterestResult(resultCode, data);
			}
			else
			{
			   Log.d( PhotoShare.LOG_TAG, "canceled my interest activity" );
			}
			break;
		case PhotoShare.IMAGE_CAPTURE_REQUEST:
			onImageCaptureResult(resultCode, data);
			break;
		default:
			Log.d(PhotoShare.LOG_TAG, "Unknown activity result");
		}
    }
	@Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		Log.d(PhotoShare.LOG_TAG, "onCreateContextMenu");
		
		if (v == gallery) {
			menu.setHeaderTitle("Picture");
			menu.add("Delete");
			menu.add("Delete (keep in bloomfilter)");
			menu.add("View Attributes");
			menu.add("Cancel");
		} else { 
			// TODO We should check for the correct view like for the gallery
			/* ListView lv = (ListView) v;
			NodeAdapter na = (NodeAdapter) lv.getAdapter();
			*/
			menu.setHeaderTitle("Node Information");
			menu.add("Interfaces");
			menu.add("Cancel");
		}
	}

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        AdapterContextMenuInfo info = (AdapterContextMenuInfo) item.getMenuInfo();
        Log.d(PhotoShare.LOG_TAG, "onContextItemSelected " + item.getTitle() + "target view id=" + info.targetView.getId());
        if (gallery.getSelectedView() == info.targetView) {
        	if (imgAdpt.getDataObjects().length == 0)
    			return true;
    		
            Log.d(PhotoShare.LOG_TAG, "Target view is gallery");
        	if (item.getTitle() == "Delete") {
        		DataObject dObj = imgAdpt.deletePicture(info.position);
        		ps.getHaggleHandle().deleteDataObject(dObj);
        		// Call dispose() to free native data before garbage collection
        		dObj.dispose();
        		Log.d(PhotoShare.LOG_TAG, "Disposed of data object");
        		Toast.makeText(this, "Deleted data object...", Toast.LENGTH_SHORT).show();
        	} else if (item.getTitle() == "Delete (keep in bloomfilter)") {
        		DataObject dObj = imgAdpt.deletePicture(info.position);
        		ps.getHaggleHandle().deleteDataObject(dObj, true);
        		// Call dispose() to free native data before garbage collection
        		dObj.dispose();
        		Log.d(PhotoShare.LOG_TAG, "Disposed of data object");
        		Toast.makeText(this, "Deleted data object, but kept in bloomfilter...", Toast.LENGTH_SHORT).show();
        	} else if (item.getTitle() == "View Attributes") {
        	
        		AlertDialog.Builder builder;
        		AlertDialog alertDialog;

        		Context mContext = getApplicationContext();
        		LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(LAYOUT_INFLATER_SERVICE);
        		View layout = inflater.inflate(R.layout.picture_attributes_dialog,
        		                               (ViewGroup) findViewById(R.id.layout_root));

        		TextView text = (TextView) layout.findViewById(R.id.text);
        		
        		Attribute[] pa = imgAdpt.getDataObjects()[info.position].getAttributes();
        		String t = "";
        		for (int i = 0; i < pa.length; i++) {
        			t += pa[i].getValue() + "\n";
        		}
    			text.setText(t);
        		builder = new AlertDialog.Builder(this);
        		builder.setView(layout);
        		alertDialog = builder.create();
        		alertDialog.setTitle("Picture Attributes");
        		alertDialog.show();
        	}
        } else {
        	if (item.getTitle() == "Interfaces") {
        		AlertDialog.Builder builder;
        		AlertDialog alertDialog;

        		Context mContext = getApplicationContext();
        		LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(LAYOUT_INFLATER_SERVICE);
        		View layout = inflater.inflate(R.layout.picture_attributes_dialog,
        		                               (ViewGroup) findViewById(R.id.layout_root));

        		TextView text = (TextView) layout.findViewById(R.id.text);
        		String t = "";
        		Node node = nodeAdpt.getNode(info.position);
        		
        		if (node != null) {
        			Interface[] ifaces = node.getInterfaces();
        			
        			for (int i = 0; i < ifaces.length; i++) {
        				t += ifaces[i].getTypeString() + " " + ifaces[i].getIdentifierString() + " " + ifaces[i].getStatusString() +"\n";
        			}
        		}
        		
        		text.setText(t);
        		builder = new AlertDialog.Builder(this);
        		builder.setView(layout);
        		alertDialog = builder.create();
        		alertDialog.setTitle("Node Interfaces");
        		alertDialog.show();
        	}
        }
        return true;
    }
    public class DataUpdater implements Runnable {
    	int type;
    	DataObject dObj = null;
    	Node[] neighbors = null;
    	
    	public DataUpdater(DataObject dObj)
    	{
    		this.type = org.haggle.EventHandler.EVENT_NEW_DATAOBJECT;
    		this.dObj = dObj;
    	}
    	public DataUpdater(Node[] neighbors)
    	{
    		this.type = org.haggle.EventHandler.EVENT_NEIGHBOR_UPDATE;
    		if (this.neighbors != null) {
    			for (int i = 0; i < this.neighbors.length; i++) {
    				this.neighbors[i].dispose();
    			}
    		}
    		this.neighbors = neighbors;
    	}
        public void run() {
    		Log.d(PhotoShare.LOG_TAG, "Running data updater, thread id=" + Thread.currentThread().getId());
        	switch(type) {
        	case org.haggle.EventHandler.EVENT_NEIGHBOR_UPDATE:
        		Log.d(PhotoShare.LOG_TAG, "Event neighbor update");
        		nodeAdpt.updateNeighbors(neighbors);
        		break;
        	case org.haggle.EventHandler.EVENT_NEW_DATAOBJECT:
        		Log.d(PhotoShare.LOG_TAG, "Event new data object");
		        imgAdpt.updatePictures(dObj);
        		break;
        	}
    		Log.d(PhotoShare.LOG_TAG, "data updater done");
        }
    }
    class ImageAdapter extends BaseAdapter {
        int mGalleryItemBackground;
        public Context mContext;
        ImageProcessingThread ipt;
        
        // We use a combination of a HashMap and ArrayList to be able to get store our
        // ImageViews both by position and filepath
        private final HashMap<DataObject,ImageView> pictures = new HashMap<DataObject,ImageView>();
        private final ArrayList<DataObject> dataObjects = new ArrayList<DataObject>();
    	private int width, height;
    	
        public ImageAdapter(Context c, int width) {
            mContext = c;

            this.width = width;
            this.height = (int)((double)width * 0.8);

        	Log.d(PhotoShare.LOG_TAG, "ImageAdapter constructor");
            TypedArray a = obtainStyledAttributes(R.styleable.Gallery);
            mGalleryItemBackground = a.getResourceId(
                    R.styleable.Gallery_android_galleryItemBackground, 0);
            a.recycle();
            
            ipt = new ImageProcessingThread(this);
            new Thread(ipt).start();
        }
        
        public synchronized int getCount() {
        	
        	if (pictures.size() == 0)
        		return 1;
        	else
        		return pictures.size();
        }
        public void rescale(int width) {
        	
            this.width  = width / 2;
            this.height = (int)((double)width * 0.8);         
        }
        public synchronized Object getItem(int position) {
            return pictures.get(dataObjects.get(position));
        }

        public long getItemId(int position) {
            return position;
        }
        
        public synchronized DataObject deletePicture(int position) {
        	final DataObject dObj = dataObjects.get(position);
        	
        	if (dObj == null)
        		return null;
        	
        	dataObjects.remove(position);
        	
        	if (pictures.remove(dObj) == null)
        		return null;
        	
        	notifyDataSetChanged();
        	
        	return dObj;
        }
        public void refresh() {
        	notifyDataSetChanged();
        }

        public final Handler handler = new Handler() {
            public void handleMessage(Message msg) {
                //progressDialog.setProgress(total);

            	synchronized (this) {
            		gallery.setSelection(dataObjects.size() - 1, true);
            	}
            	notifyDataSetChanged();
            }
        };
        
        // This processes images in a separate thread
        final class ImageProcessingThread implements Runnable  {
			ImageAdapter imgAdpt;
			boolean shouldExit = false;
			private final ArrayList<DataObject> dataObjectsToProcess = new ArrayList<DataObject>();
			
			public ImageProcessingThread(ImageAdapter imgAdpt) {
				this.imgAdpt = imgAdpt;
			}
			public void processDataObject(DataObject dObj)
			{
				Log.d(PhotoShare.LOG_TAG, "Adding image to processing queue");
				
				synchronized(dataObjectsToProcess) {
					dataObjectsToProcess.add(dObj);
				}
				synchronized(this) {
					notify();
				}
			}
			public void cancel() { shouldExit = true; notify(); }
			public void run() {
				
				Log.d(PhotoShare.LOG_TAG, "Running image processing thread");
				
				while (!shouldExit) {
					boolean queueIsEmpty = false;

					synchronized(dataObjectsToProcess) {
						if (dataObjectsToProcess.isEmpty())
							queueIsEmpty = true;
					}
					if (queueIsEmpty) {
						try {
							synchronized(this) {
								wait();
							}
						} catch (InterruptedException e) {
							// TODO Auto-generated catch block
							return;
						}
					}
					DataObject dObj = null;

		        	Log.d(PhotoShare.LOG_TAG, "Processing new image in processing thread");

					synchronized(dataObjectsToProcess) {
						if (dataObjectsToProcess.isEmpty())
							continue;
					
						dObj = dataObjectsToProcess.remove(0);
					}
					
					String filepath = dObj.getFilePath();

		    		if (filepath == null || filepath.length() == 0) {
		    			Log.d(PhotoShare.LOG_TAG, "Bad filepath");
		    			continue;
		    		}
		    		
		        	Bitmap bmp = scaleImage(filepath, width);

		        	Log.d(PhotoShare.LOG_TAG, "time=" + System.currentTimeMillis() + " bitmap height=" + bmp.getHeight() + " width=" + bmp.getWidth());

		        	ImageView iv = new ImageView(getApplicationContext());
		        	iv.setScaleType(ImageView.ScaleType.CENTER_INSIDE);    
		        	iv.setLayoutParams(new Gallery.LayoutParams(width, height));
		        	iv.setBackgroundResource(mGalleryItemBackground); 
		        	iv.setImageDrawable(new BitmapDrawable(bmp));
		        
		        	synchronized (imgAdpt) {
		        		pictures.put(dObj, iv);
		        		dataObjects.add(dObj);
		        	}
		        
					imgAdpt.handler.sendEmptyMessage(0);
				}
			}
        }
        public synchronized void updatePictures(DataObject dObj) {

        	Log.d(PhotoShare.LOG_TAG, "Updating gallery images memfree=" + Runtime.getRuntime().freeMemory());
        	
        	ipt.processDataObject(dObj);
        }
        public synchronized DataObject[] getDataObjects() {
        	return dataObjects.toArray(new DataObject[dataObjects.size()]);
        }
        public synchronized String getDataObjectFilePath(int pos) {
        	return dataObjects.get(pos).getFilePath();
        }
        public synchronized String[] getPictureFilePaths() {
        	final String[] filepaths = new String[dataObjects.size()];
        	
        	for (int i = 0; i < dataObjects.size(); i++) {
        		filepaths[i] = dataObjects.get(i).getFilePath();
        	}
        	return filepaths;
        }
        public synchronized View getView(int position, View convertView, ViewGroup parent) {  

        	if (pictures.size() == 0) {
            	ImageView i = new ImageView(getApplicationContext());
        		i.setImageResource(R.drawable.haggle_logo_400); 
                i.setScaleType(ImageView.ScaleType.CENTER_INSIDE);    
                i.setLayoutParams(new Gallery.LayoutParams(width, height));
                i.setBackgroundResource(mGalleryItemBackground); 
                return i;
        	}
        	
        	return pictures.get(dataObjects.get(position));
        }
    }
}
