package com.github.rdv0011.stouch;
 
import com.github.rdv0011.stouch.STouchService.LocalBinder;

import android.os.Handler;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.support.v7.app.AppCompatActivity;
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
import android.widget.ToggleButton;

public class STouchActivity extends AppCompatActivity {
    STouchService mService;
    boolean mBound = false;
    TextView serviceStatusTextView = null;
    TextView fpsTextView = null;
    TextView roiTextView = null;
    TextView touchStatusTextView = null;
    STouchEventInjector mTouchInjector;
    private boolean mFreenectInitialized = false;

    private Handler fpsUpdateHandler = new Handler();
    private Runnable fpsUpdateTask = new Runnable() {
        @Override
        public void run() {

            int fpsCount = fps();
            int calibrationState = calibrationState();
            int calibrationFrameCount = calibrationFrameCount();

            String fpsText = String.format(getString(R.string.fps_format),
                    fpsCount, calibrationState, calibrationFrameCount);
            fpsTextView.setText(fpsText);

            fpsUpdateHandler.postDelayed(fpsUpdateTask, 300);
        }
    };

    // Defines callbacks for service binding, passed to bindService()
    private ServiceConnection mConnection = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
            LocalBinder binder = (LocalBinder)service;
            mService = binder.getService();
            mBound = true;
            serviceStatusTextView.setText(R.string.freenect_stopped);
            if (initDetector()) {
                serviceStatusTextView.setText(R.string.freenect_started);
            }
		}

		@Override
		public void onServiceDisconnected(ComponentName name) {
			mBound = false;
            cleanupDetector();
            serviceStatusTextView.setText(R.string.freenect_stopped);
		}
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Switch the app to the full screen mode and hide status bar
        super.onCreate(savedInstanceState);
        // Set main layout
        setContentView(R.layout.activity_main);
        serviceStatusTextView = (TextView) findViewById(R.id.service_status_text);
        fpsTextView = (TextView) findViewById(R.id.fps_text);
        roiTextView = (TextView) findViewById(R.id.roi_text);
        touchStatusTextView = (TextView) findViewById(R.id.touch_status_text);
        bindImpl();
        fpsUpdateHandler.post(fpsUpdateTask);
        ToggleButton calibrationToggle = (ToggleButton) findViewById(R.id.startCalibrationButton);
        calibrationToggle.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                startCalibration(isChecked);
            }
        });
        ToggleButton touchToggle = (ToggleButton) findViewById(R.id.startTouchButton);
        touchToggle.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                startTouch(isChecked);
                int touchStatus = isChecked ? R.string.touch_started: R.string.touch_stopped;
                touchStatusTextView.setText(touchStatus);
            }
        });
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
            serviceStatusTextView.setText(R.string.freenect_stopped);
        }
    }

    public void virtualROIUpdated(int xVirtualOffset, int yVirtualOffset,
                                    int virtualWidth, int virtualHeight) {
        if (getApplicationContext() != null) {
            DisplayMetrics metrics = getApplicationContext().getResources().getDisplayMetrics();
            mTouchInjector = new STouchEventInjector(xVirtualOffset, yVirtualOffset,
                    virtualWidth, virtualHeight, metrics.widthPixels, metrics.heightPixels);
            roiTextView.setText(String.format(getString(R.string.roi_format),
                    xVirtualOffset, yVirtualOffset, virtualWidth, virtualHeight));
        }
    }

    public void sendEvent(int x, int y) {
        if (mTouchInjector != null)
            mTouchInjector.sendEvent(x, y);
    }

    public void videoDataHandler(byte[] data) {
        KinectPreview preview = findViewById(R.id.kinectPreview);
        preview.videoDataHandler(data);
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

    public native int fps();
    public native boolean init(Object activityObj);
    public native void cleanup();
    public native void startCalibration(boolean start);
    public native int calibrationState();
    public native int calibrationFrameCount();
    public native void startTouch(boolean start);

    private boolean initDetector() {
        return init(this);
    }

    private void cleanupDetector() {
        cleanup();
    }

    static {
        System.loadLibrary("stouch");
    }
}
