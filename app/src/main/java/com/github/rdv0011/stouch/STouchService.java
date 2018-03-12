package com.github.rdv0011.stouch;
 
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

public class STouchService extends Service
{
	private final IBinder mBinder = (IBinder)new LocalBinder();

	public class LocalBinder extends Binder {
		STouchService getService() {
            // Return this instance of LocalService so clients can call public methods
            return STouchService.this;
        }
    }
	
	@Override
	public void onCreate() {
	    super.onCreate();
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
	}
	
	@Override
	public IBinder onBind(Intent intent) {
		return mBinder;
	}
}
