package com.example.stouch2;
 
import com.example.stouch2.STouchService.LocalBinder;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.view.KeyEvent;
import android.view.View;
import android.widget.TextView;

public class STouchActivity extends Activity
{
    STouchService mService;
    boolean mBound = false;
    TextView textView1 = null;

    // Defines callbacks for service binding, passed to bindService()
    private ServiceConnection mConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
            LocalBinder binder = (LocalBinder) service;
            mService = binder.getService();
            mBound = true;
            textView1.setText(R.string.freenect_stopped);
            if (mService.getFreenectInitialized()) {
            	textView1.setText(R.string.freenect_started);
            }
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mBound = false;
            if (mService.getFreenectInitialized()) {
            	textView1.setText(R.string.freenect_stopped);
            }
		}
    };
    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        textView1 = (TextView)findViewById(R.id.textView1);        
        bindImpl();
    }
    
    private void bindImpl() {
        // Bind to LocalService
        Intent intent = new Intent(this, STouchService.class);
        bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
    }
    
    private void unbindImpl() {
        // Unbind from the service
        if (mBound) {
            unbindService(mConnection);
            mBound = false;
          	textView1.setText(R.string.freenect_stopped);
        }
    }

	@Override
	protected void onDestroy() {
		super.onDestroy();
	    unbindImpl();
	}
    
    public void onClickBind(View v) {
    	bindImpl();
    }
    
    public void onClickUnBind(View v) {
    	unbindImpl();
    }
    
    // Prevents onDestroy being called when back button is pressed
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
    	if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
    		this.moveTaskToBack(true);
    		return true;
    	}
    	return super.onKeyDown(keyCode, event);
    }
}