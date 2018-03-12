package com.github.rdv0011.stouch;

import android.content.Context;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.IntBuffer;

/**
 * TODO: document your custom view class.
 */
public class KinectPreview extends SurfaceView implements SurfaceHolder.Callback {

    private final String LOG_TAG = "KinectPreview";
    private final int FREENECT_FRAME_W = 640;
    private final int FREENECT_FRAME_H = 480;
    private final int VideoBufferSizeARGB = FREENECT_FRAME_W * FREENECT_FRAME_H * 4;
    private static final int STOPPED = 0;
    private static final int STARTED = 1;
    private SurfaceHolder surfaceHolder;
    private final Object syncObject = new Object();
    private Thread thread;
    private boolean stopThread = false;
    private int state = STOPPED;
    private boolean frameReady = false;
    private boolean surfaceExist;
    private byte[] frameBuffer = new byte[VideoBufferSizeARGB];
    private int[] frameBufferIntArray = new int[FREENECT_FRAME_W * FREENECT_FRAME_H];
    private BitmapFactory.Options options;
    Bitmap bufferBMP = Bitmap.createBitmap(FREENECT_FRAME_W, FREENECT_FRAME_H,
            Bitmap.Config.ARGB_8888);

    public KinectPreview(Context context) {
        super(context);
        initializer();
    }

    public KinectPreview(Context context, AttributeSet attrs) {
        super(context, attrs);
        initializer();
    }

    public KinectPreview(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initializer();
    }

    private void initializer() {
        this.surfaceHolder = this.getHolder();
        this.surfaceHolder.addCallback(this);
        this.options = new BitmapFactory.Options();
        this.options.inMutable = true;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        /* Do nothing. Wait until surfaceChanged delivered */
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        if (this.surfaceHolder.getSurface() == null) {
            Log.e(LOG_TAG, "surfaceChanged(): surfaceHolder is null, nothing to do.");
            return;
        }

        synchronized (syncObject) {
            if (!surfaceExist) {
                surfaceExist = true;
                checkCurrentState();
            } else {
                /** Surface changed. We need to stop camera and restart with new parameters */
                /* Pretend that old surface has been destroyed */
                surfaceExist = false;
                checkCurrentState();
                /* Now use new surface. Say we have it now */
                surfaceExist = true;
                checkCurrentState();
            }
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(LOG_TAG, "surfaceDestroyed");
        synchronized (syncObject) {
            surfaceExist = false;
            checkCurrentState();
        }
    }

    private void checkCurrentState() {
        Log.d(LOG_TAG, "CheckCurrentState called");
        int targetState = STOPPED;

        if (surfaceExist && getVisibility() == VISIBLE) {
            targetState = STARTED;
        }

        if (targetState != state) {
            /* The state change detected. Need to exit the current state and enter target state */
            if (state == STARTED)
                stopWorker();
            state = targetState;
            if (state == STARTED)
                startWorker();
        }
    }

    private void startWorker() {
        Log.d(LOG_TAG, "Starting processing thread");
        stopThread = false;
        thread = new Thread(new KinectFrameWorker());
        thread.start();
    }

    private void stopWorker() {
        Log.d(LOG_TAG, "Stopping processing thread");
        try {
            stopThread = true;
            Log.d(LOG_TAG, "Notify thread");
            synchronized (this) {
                this.notify();
            }
            Log.d(LOG_TAG, "Wating for thread");
            if (thread != null)
                thread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            thread = null;
        }
        frameReady = false;
    }

    private class KinectFrameWorker implements Runnable {
        private BitmapFactory.Options options;

        public void run() {
            do {
                boolean hasFrame = false;
                synchronized (KinectPreview.this) {
                    try {
                        while (!frameReady && !stopThread) {
                            KinectPreview.this.wait();
                        }
                    } catch (InterruptedException e) {
                        Log.e(LOG_TAG, "KinectFrameWorker interrupted", e);
                    }
                    if (frameReady) {
                        frameReady = false;
                        hasFrame = true;
                    }
                }
                if (!stopThread && hasFrame) {
                    drawFrame(frameBuffer);
                }
            } while (!stopThread);
            Log.d(LOG_TAG, "KinectFrameWorker processing thread has been finished");
        }
    }

    private void drawFrame(byte[] raw) {
        Canvas canvas = getHolder().lockCanvas();
        if (canvas != null) {
            IntBuffer intBuf = ByteBuffer.wrap(raw).asIntBuffer();
            intBuf.get(frameBufferIntArray);
            bufferBMP.setPixels(frameBufferIntArray, 0, FREENECT_FRAME_W, 0, 0,
                    FREENECT_FRAME_W, FREENECT_FRAME_H);
            int width = canvas.getWidth();
            int height = bufferBMP.getHeight() * canvas.getWidth() / bufferBMP.getWidth();
            canvas.drawColor(0, android.graphics.PorterDuff.Mode.CLEAR);
            canvas.drawBitmap(bufferBMP,
                    new Rect(0, 0, bufferBMP.getWidth(), bufferBMP.getHeight()),
                    new Rect(0,
                            (canvas.getHeight() - height) / 2,
                            width,
                            (canvas.getHeight() - height) / 2 + height), null);
            getHolder().unlockCanvasAndPost(canvas);
        }
    }

    public void videoDataHandler(byte[] data) {
        synchronized (this) {
            assert(frameBuffer.length == data.length);
            // Copy one frame to the buffer
            System.arraycopy(data, 0, frameBuffer, 0, data.length);
            // Let the frame worker thread know that the frame is ready to be displayed
            frameReady = true;
            this.notify();
        }
    }
}
