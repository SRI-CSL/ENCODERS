	package org.haggle.PhotoShare;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.ViewGroup.LayoutParams;
import android.view.animation.AnimationUtils;
import android.widget.ImageSwitcher;
import android.widget.ImageView;
import android.widget.ViewSwitcher;


public class FullscreenImageViewer extends Activity implements ViewSwitcher.ViewFactory {
    private ImageSwitcher mSwitcher;
    private String[] mPicturePaths = null;
	float origTouchX = 0, origTouchY = 0;
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        setContentView(R.layout.image_switcher);

        mSwitcher = (ImageSwitcher) findViewById(R.id.switcher);
        mSwitcher.setFactory(this);
        mSwitcher.setInAnimation(AnimationUtils.loadAnimation(this,
                android.R.anim.fade_in));
        mSwitcher.setOutAnimation(AnimationUtils.loadAnimation(this,
        		android.R.anim.fade_out));

        Bundle extras = getIntent().getExtras();

        if (extras != null) {
        	Log.d(PhotoShare.LOG_TAG, "FullscreenImageViewer: Setting extras");
        	mPicturePaths = extras.getStringArray(PhotoView.KEY_PICTURE_PATHS);
        	Log.d(PhotoShare.LOG_TAG, "FullscreenImageViewer: Setting pos");
        	int pos = extras.getInt(PhotoView.KEY_PICTURE_POS);

        	Log.d(PhotoShare.LOG_TAG, "FullscreenImageViewer: Pos is " + pos);
        	
        	BitmapFactory.Options opts = new BitmapFactory.Options();

            opts.inJustDecodeBounds = true;
            BitmapFactory.decodeFile(mPicturePaths[pos], opts); 
            
            double ratio = 1;
            
            if (opts.outHeight >= opts.outWidth) {
            	if (opts.outHeight > 480) {
            		ratio = opts.outHeight / 480;
            	} 
            } else {
            	if (opts.outWidth > 480) {
            		ratio = opts.outWidth / 480;
            	}  	
            }
            opts.inSampleSize = (int)ratio;
            opts.inJustDecodeBounds = false;
            
        	Bitmap bmp = BitmapFactory.decodeFile(mPicturePaths[pos], opts);

        	Log.d(PhotoShare.LOG_TAG, "FullscreenImageViewer: width " + bmp.getWidth() + " height=" + bmp.getHeight());
        	mSwitcher.setImageDrawable(new BitmapDrawable(bmp));
        }
    }
    
    @Override
	protected void onStop() {
		// TODO Auto-generated method stub
		super.onStop();
		Log.d(PhotoShare.LOG_TAG, "onStop() in FullscreenImageViewer");
		
	}


	@Override
	public boolean onTouchEvent(MotionEvent event) {
		
		switch (event.getAction()) {
		case MotionEvent.ACTION_CANCEL:
			break;
		case MotionEvent.ACTION_DOWN:
			origTouchX = event.getX();
			origTouchY = event.getY();
			break;
		case MotionEvent.ACTION_MOVE:
			break;
		case MotionEvent.ACTION_OUTSIDE:
			break;
		case MotionEvent.ACTION_UP:
			if (event.getX() - origTouchX > 0) {
				// Moved right
				Log.d(PhotoShare.LOG_TAG, "Swiped right");
				mSwitcher.showNext();
			} else {
				Log.d(PhotoShare.LOG_TAG, "Swiped left");
				mSwitcher.showPrevious();
			}
			//finish();
			break;
		}
		return true;
	}

	@Override
	public boolean onTrackballEvent(MotionEvent event) {
		Log.d(PhotoShare.LOG_TAG, "MotionEvent");
		return super.onTrackballEvent(event);
	}

	public View makeView() {
        ImageView i = new ImageView(getApplicationContext());
        i.setBackgroundColor(0xFF000000);
        i.setScaleType(ImageView.ScaleType.FIT_CENTER);
        i.setLayoutParams(new ImageSwitcher.LayoutParams(LayoutParams.FILL_PARENT,
                LayoutParams.FILL_PARENT));
        return i;
    }
}
