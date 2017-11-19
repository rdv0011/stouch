package com.github.rdv0011.stouch;
 
import com.github.rdv0011.stouch.STouchService.LocalBinder;

import android.widget.TextView;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.os.Bundle;
import android.util.DisplayMetrics;

public class STouchActivity extends AppCompatActivity {
    STouchService mService;
    boolean mBound = false;
    TextView textView1 = null;
    STouchEventInjector mTouchInjector;

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
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);
        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Snackbar.make(view, "Replace with your own action", Snackbar.LENGTH_LONG)
                        .setAction("Action", null).show();
            }
        });

        // Example of a call to a native method
        textView1 = (TextView) findViewById(R.id.sample_text);
        bindImpl();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unbindImpl();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
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

    public void virtualMetricsUpdated(int xVirtualOffset, int yVirtualOffset,
                                    int virtualWidth, int virtualHeight) {
        DisplayMetrics metrics = getApplicationContext().getResources().getDisplayMetrics();
        mTouchInjector = new STouchEventInjector(xVirtualOffset, yVirtualOffset,
                virtualWidth, virtualWidth, metrics.widthPixels, metrics.heightPixels);
    }

    public void sendEvent(int x, int y) {
        if (mTouchInjector != null)
            mTouchInjector.sendEvent(x, y);
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

    static {
        System.loadLibrary("stouch");
    }
}
