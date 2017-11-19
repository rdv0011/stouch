package com.github.rdv0011.stouch;

import android.app.Instrumentation;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.util.Log;

/**
 * Created by dmitry on 2017-11-10.
 */

public class STouchEventInjector {
    Instrumentation m_Instrumentation = new Instrumentation();
    float xScale;
    float yScale;
    int xOffset;
    int yOffset;

    STouchEventInjector(int xVirtualOffset, int yVirtualOffset,
                        int virtualWidth, int virtualHeight,
                        int displayWidth, int displayHeight) {
        xOffset = -xVirtualOffset;
        yOffset = -yVirtualOffset;
        xScale = displayWidth / (float)virtualWidth;
        yScale = displayHeight / (float)virtualHeight;
        Log.d(this.getClass().getName(),
                String.format("Touch device successfully opened size [%d, %d] scale [%f,%f]",
                virtualWidth, virtualHeight, xScale, yScale));
    }

    public void sendEvent(int x, int y) {
        float displayX = (x + xOffset) * xScale;
        float displayY = (y + yOffset) * yScale;
        m_Instrumentation.sendPointerSync(MotionEvent.obtain(SystemClock.uptimeMillis(),
                SystemClock.uptimeMillis(), MotionEvent.ACTION_DOWN, displayX, displayY, 0));
        m_Instrumentation.sendPointerSync(MotionEvent.obtain(SystemClock.uptimeMillis(),
                SystemClock.uptimeMillis(), MotionEvent.ACTION_UP, displayX, displayY, 0));
    }
}
