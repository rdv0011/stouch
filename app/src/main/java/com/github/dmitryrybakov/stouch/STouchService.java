package com.github.dmitryrybakov.stouch;
 
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

public class STouchService extends Service
{
	private final IBinder mBinder = (IBinder)new LocalBinder();
	private boolean mFreenectInitialized = false; 

	public class LocalBinder extends Binder {
		STouchService getService() {
            // Return this instance of LocalService so clients can call public methods
            return STouchService.this;
        }
    }
	
	@Override
	public void onCreate() {
	    super.onCreate();
	    mFreenectInitialized = init();
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		cleanup();
	}
	
	@Override
	public IBinder onBind(Intent intent) {
		return mBinder;
	}
	
	boolean getFreenectInitialized() {
		return mFreenectInitialized;
	}
 
    public native boolean init();
    public native void cleanup();
 
    static
    {
        System.loadLibrary("stouch");
    }
}